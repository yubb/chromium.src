// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/render_thread_impl_browser_test_ipc_helper.h"

#include "ipc/mojo/ipc_channel_mojo_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class RenderThreadImplBrowserIPCTestHelper::DummyListener
    : public IPC::Listener {
 public:
  bool OnMessageReceived(const IPC::Message& message) override { return true; }
};

RenderThreadImplBrowserIPCTestHelper::RenderThreadImplBrowserIPCTestHelper() {
  message_loop_.reset(new base::MessageLoopForIO());

  channel_id_ = IPC::Channel::GenerateVerifiedChannelID("");
  dummy_listener_.reset(new DummyListener());

  SetupIpcThread();

  // TODO(crbug.com/451221): Mojo should be used here even for Android.
  // Currently SetupMojo() calls InitializeMojo() which can be called at most
  // once for each process. This implices that every test case using
  // RenderThreadImplBrowserIPCTestHelper has to run in a dedicated process, but
  // that is false on OS_ANDROID. There is no way to call InitializeMojo() only
  // if it isn't initialized yet.
#if defined(OS_ANDROID)
  bool use_mojo_channel = false;
#else
  bool use_mojo_channel = IPC::ChannelMojo::ShouldBeUsed();
#endif
  if (use_mojo_channel) {
    SetupMojo();
  } else {
    channel_ = IPC::ChannelProxy::Create(channel_id_, IPC::Channel::MODE_SERVER,
                                         dummy_listener_.get(),
                                         ipc_thread_->task_runner());
  }
}

RenderThreadImplBrowserIPCTestHelper::~RenderThreadImplBrowserIPCTestHelper() {
}

void RenderThreadImplBrowserIPCTestHelper::SetupIpcThread() {
  ipc_thread_.reset(new base::Thread("test_ipc_thread"));
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  ASSERT_TRUE(ipc_thread_->StartWithOptions(options));
}

void RenderThreadImplBrowserIPCTestHelper::SetupMojo() {
  InitializeMojo();

  mojo_host_.reset(new IPC::ChannelMojoHost(ipc_thread_->task_runner()));
  mojo_application_host_.reset(new MojoApplicationHost());

  channel_ = IPC::ChannelProxy::Create(
      IPC::ChannelMojo::CreateServerFactory(mojo_host_->channel_delegate(),
                                            channel_id_),
      dummy_listener_.get(), ipc_thread_->task_runner());

  mojo_application_host_->Init();
  mojo_application_host_->Activate(channel_.get(),
                                   base::GetCurrentProcessHandle());
  mojo_host_->OnClientLaunched(base::GetCurrentProcessHandle());
}

}  // namespace content
