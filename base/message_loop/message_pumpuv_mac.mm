// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#import "base/message_loop/message_pump_mac.h"
#import "base/message_loop/message_pumpuv_mac.h"

#include <dlfcn.h>
#import <Foundation/Foundation.h>

#include <stdio.h>

#include <limits>

#include "base/logging.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/message_loop/timer_slack.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "v8/include/v8.h"

#if !defined(OS_IOS)
#import <AppKit/AppKit.h>
#endif  // !defined(OS_IOS)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "third_party/node/src/node_webkit.h"

namespace base {

namespace {

// static
const CFStringRef kMessageLoopExclusiveRunLoopMode =
    CFSTR("kMessageLoopExclusiveRunLoopMode");

void CFRunLoopAddSourceToAllModes(CFRunLoopRef rl, CFRunLoopSourceRef source) {
  CFRunLoopAddSource(rl, source, kCFRunLoopCommonModes);
  CFRunLoopAddSource(rl, source, kMessageLoopExclusiveRunLoopMode);
}

void CFRunLoopRemoveSourceFromAllModes(CFRunLoopRef rl,
                                       CFRunLoopSourceRef source) {
  CFRunLoopRemoveSource(rl, source, kCFRunLoopCommonModes);
  CFRunLoopRemoveSource(rl, source, kMessageLoopExclusiveRunLoopMode);
}

void NoOp(void* info) {
}

void UvNoOp(uv_async_t* handle) {
}

}  // namespace

// A scoper for autorelease pools created from message pump run loops.
// Avoids dirtying up the ScopedNSAutoreleasePool interface for the rare
// case where an autorelease pool needs to be passed in.
class MessagePumpScopedAutoreleasePool {
 public:
  explicit MessagePumpScopedAutoreleasePool(MessagePumpCFRunLoopBase* pump) :
      pool_(pump->CreateAutoreleasePool()) {
  }
   ~MessagePumpScopedAutoreleasePool() {
    [pool_ drain];
  }

 private:
  NSAutoreleasePool* pool_;
  DISALLOW_COPY_AND_ASSIGN(MessagePumpScopedAutoreleasePool);
};

bool MessagePumpUVNSRunLoop::RunWork() {
  if (!delegate_) {
    // This point can be reached with a NULL delegate_ if Run is not on the
    // stack but foreign code is spinning the CFRunLoop.  Arrange to come back
    // here when a delegate is available.
    delegateless_work_ = true;
    return false;
  }

  // The NSApplication-based run loop only drains the autorelease pool at each
  // UI event (NSEvent).  The autorelease pool is not drained for each
  // CFRunLoopSource target that's run.  Use a local pool for any autoreleased
  // objects if the app is not currently handling a UI event to ensure they're
  // released promptly even in the absence of UI events.
  MessagePumpScopedAutoreleasePool autorelease_pool(this);

  // Call DoWork and DoDelayedWork once, and if something was done, arrange to
  // come back here again as long as the loop is still running.
  bool did_work = delegate_->DoWork();
  bool resignal_work_source = did_work;

  TimeTicks next_time;
  delegate_->DoDelayedWork(&next_time);
  if (!did_work) {
    // Determine whether there's more delayed work, and if so, if it needs to
    // be done at some point in the future or if it's already time to do it.
    // Only do these checks if did_work is false. If did_work is true, this
    // function, and therefore any additional delayed work, will get another
    // chance to run before the loop goes to sleep.
    bool more_delayed_work = !next_time.is_null();
    if (more_delayed_work) {
      TimeDelta delay = next_time - TimeTicks::Now();
      if (delay > TimeDelta()) {
        // There's more delayed work to be done in the future.
        ScheduleDelayedWork(next_time);
      } else {
        // There's more delayed work to be done, and its time is in the past.
        // Arrange to come back here directly as long as the loop is still
        // running.
        resignal_work_source = true;
      }
    }
  }

  if (resignal_work_source) {
    CFRunLoopSourceSignal(work_source_);
  }else if (did_work) {
    // callbacks in Blink can result in uv status change, so
    // a run through is needed. This should remove the need for
    // the 500ms failsafe poll

    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:NULL
                                       subtype:0
                                         data1:0
                                         data2:0]
           atStart:NO];
  }

  return resignal_work_source;
}

bool MessagePumpUVNSRunLoop::RunIdleWork() {
  if (!delegate_) {
    // This point can be reached with a NULL delegate_ if Run is not on the
    // stack but foreign code is spinning the CFRunLoop.  Arrange to come back
    // here when a delegate is available.
    delegateless_idle_work_ = true;
    return false;
  }

  // The NSApplication-based run loop only drains the autorelease pool at each
  // UI event (NSEvent).  The autorelease pool is not drained for each
  // CFRunLoopSource target that's run.  Use a local pool for any autoreleased
  // objects if the app is not currently handling a UI event to ensure they're
  // released promptly even in the absence of UI events.
  MessagePumpScopedAutoreleasePool autorelease_pool(this);

  // Call DoIdleWork once, and if something was done, arrange to come back here
  // again as long as the loop is still running.
  bool did_work = delegate_->DoIdleWork();
  if (did_work) {
    CFRunLoopSourceSignal(idle_work_source_);
    // callbacks in Blink can result in uv status change, so
    // a run through is needed
    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:NULL
                                       subtype:0
                                         data1:0
                                         data2:0]
           atStart:NO];
  }

  return did_work;
}

void MessagePumpUVNSRunLoop::PreWaitObserverHook() {
  // call tick callback before sleep in mach port
  // in the same way node upstream handle this in MakeCallBack,
  // or the tick callback is blocked in some cases
  if (node::g_env) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    node::CallTickCallback(node::g_env, v8::Undefined(isolate));
  }
}

MessagePumpUVNSRunLoop::MessagePumpUVNSRunLoop()
    : keep_running_(true) {
  CFRunLoopSourceContext source_context = CFRunLoopSourceContext();
  source_context.perform = NoOp;
  quit_source_ = CFRunLoopSourceCreate(NULL,  // allocator
                                       0,     // priority
                                       &source_context);
  CFRunLoopAddSourceToAllModes(run_loop(), quit_source_);

  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(uv_default_loop(), &dummy_uv_handle_, UvNoOp);

  // Start worker that will interrupt main loop when having uv events.
  embed_closed_ = 0;
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);

  // Execute loop for once.
  uv_run(uv_default_loop(), UV_RUN_NOWAIT);
  node::g_nw_uv_run = uv_run;

}

MessagePumpUVNSRunLoop::~MessagePumpUVNSRunLoop() {
  CFRunLoopRemoveSourceFromAllModes(run_loop(), quit_source_);
  CFRelease(quit_source_);
  // Clear uv.
  embed_closed_ = 1;
  uv_thread_join(&embed_thread_);
}

void MessagePumpUVNSRunLoop::DoRun(Delegate* delegate) {
  v8::Isolate* isolate = NULL;

  // Pause uv in nested loop.
  if (nesting_level() > 0) {
    pause_uv_ = true;
  }

  while (keep_running_) {
    // NSRunLoop manages autorelease pools itself.
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate distantFuture]];
    if (node::g_env && nesting_level() == 0) {
        isolate = v8::Isolate::GetCurrent();
        v8::HandleScope scope(isolate);
        // Deal with uv events.
	if (!(*node::g_nw_uv_run)(uv_default_loop(), UV_RUN_NOWAIT)) {
	  VLOG(1) << "Quit from uv";
	  keep_running_ = false; // Quit from uv.
          break;
	}
        if(0 == uv_backend_timeout(uv_default_loop())) {
           [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                      location:NSZeroPoint
                                 modifierFlags:0
                                     timestamp:0
                                  windowNumber:0
                                       context:NULL
                                       subtype:0
                                         data1:0
                                         data2:0]
              atStart:NO];
        }
        // Tell the worker thread to continue polling.
        uv_sem_post(&embed_sem_);
    }
  }

  keep_running_ = true;
  // Resume uv.
  if (nesting_level() > 0) {
    pause_uv_ = false;
    uv_sem_post(&embed_sem_);
  }
}

void MessagePumpUVNSRunLoop::Quit() {
  keep_running_ = false;
  CFRunLoopSourceSignal(quit_source_);
  CFRunLoopWakeUp(run_loop());
}

void MessagePumpUVNSRunLoop::EmbedThreadRunner(void *arg) {
  base::MessagePumpUVNSRunLoop* message_pump =
      static_cast<base::MessagePumpUVNSRunLoop*>(arg);

  int r;
  struct kevent errors[1];

  while (!message_pump->embed_closed_) {
    uv_loop_t* loop = uv_default_loop();

    // We should at leat poll every 500ms.
    // theoratically it's not needed, but act as a fail-safe
    // for unknown corner cases

    int timeout = uv_backend_timeout(loop);
#if 1
    if (timeout > 500 || timeout < 0)
      timeout = 500;
#endif

    // Wait for new libuv events.
    int fd = uv_backend_fd(loop);

    do {
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      r = kevent(fd, NULL, 0, errors, 1, timeout < 0 ? NULL : &ts);
    } while (r == -1 && errno == EINTR);

    // Don't wake up main loop if in a nested loop, so we'll keep waiting for
    // the semaphore and uv loop will be paused.
    if (!message_pump->pause_uv_) {
      // Send a fake event to wake the loop up.
      NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
      [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                          location:NSMakePoint(0, 0)
                                     modifierFlags:0
                                         timestamp:0
                                      windowNumber:0
                                           context:NULL
                                           subtype:0
                                             data1:0
                                             data2:0]
               atStart:NO];
      [pool release];
    }

    // Wait for the main loop to deal with events.
    uv_sem_wait(&message_pump->embed_sem_);
  }
}

}  // namespace base
