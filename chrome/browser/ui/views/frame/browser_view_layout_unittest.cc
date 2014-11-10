// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_view_layout.h"

#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "testing/gtest/include/gtest/gtest.h"

class MockBrowserViewLayoutDelegate : public BrowserViewLayoutDelegate {
 public:
  explicit MockBrowserViewLayoutDelegate(views::View* contents_web_view)
      : contents_web_view_(contents_web_view),
        tab_strip_visible_(true),
        toolbar_visible_(true),
        bookmark_bar_visible_(true),
        download_shelf_needs_layout_(false) {
  }
  ~MockBrowserViewLayoutDelegate() override {}

  void set_download_shelf_needs_layout(bool layout) {
    download_shelf_needs_layout_ = layout;
  }
  void set_tab_strip_visible(bool visible) {
    tab_strip_visible_ = visible;
  }
  void set_toolbar_visible(bool visible) {
    toolbar_visible_ = visible;
  }
  void set_bookmark_bar_visible(bool visible) {
    bookmark_bar_visible_ = visible;
  }

  // BrowserViewLayout::Delegate overrides:
  views::View* GetContentsWebView() const override {
    return contents_web_view_;
  }
  bool IsTabStripVisible() const override { return tab_strip_visible_; }
  gfx::Rect GetBoundsForTabStripInBrowserView() const override {
    return gfx::Rect();
  }
  int GetTopInsetInBrowserView() const override { return 0; }
  int GetThemeBackgroundXInset() const override { return 0; }
  bool IsToolbarVisible() const override { return toolbar_visible_; }
  bool IsBookmarkBarVisible() const override { return bookmark_bar_visible_; }
  bool DownloadShelfNeedsLayout() const override {
    return download_shelf_needs_layout_;
  }

  FullscreenExitBubbleViews* GetFullscreenExitBubble() const override {
    return nullptr;
  }

 private:
  views::View* contents_web_view_;
  bool tab_strip_visible_;
  bool toolbar_visible_;
  bool bookmark_bar_visible_;
  bool download_shelf_needs_layout_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowserViewLayoutDelegate);
};

///////////////////////////////////////////////////////////////////////////////

// A simple view that prefers an initial size.
class MockView : public views::View {
 public:
  explicit MockView(gfx::Size initial_size)
      : size_(initial_size) {
    SetBoundsRect(gfx::Rect(gfx::Point(), size_));
  }
  ~MockView() override {}

  // views::View overrides:
  gfx::Size GetPreferredSize() const override { return size_; }

 private:
  gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(MockView);
};

///////////////////////////////////////////////////////////////////////////////

class MockImmersiveModeController : public ImmersiveModeController {
 public:
  MockImmersiveModeController() {}
  ~MockImmersiveModeController() override {}

  // ImmersiveModeController overrides:
  void Init(BrowserView* browser_view) override {}
  void SetEnabled(bool enabled) override {}
  bool IsEnabled() const override { return false; }
  bool ShouldHideTabIndicators() const override { return false; }
  bool ShouldHideTopViews() const override { return false; }
  bool IsRevealed() const override { return false; }
  int GetTopContainerVerticalOffset(
      const gfx::Size& top_container_size) const override {
    return 0;
  }
  ImmersiveRevealedLock* GetRevealedLock(AnimateReveal animate_reveal) override
      WARN_UNUSED_RESULT {
    return nullptr;
  }
  void OnFindBarVisibleBoundsChanged(
      const gfx::Rect& new_visible_bounds) override {}
  void SetupForTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MockImmersiveModeController);
};

///////////////////////////////////////////////////////////////////////////////
// Tests of BrowserViewLayout. Runs tests without constructing a BrowserView.
class BrowserViewLayoutTest : public BrowserWithTestWindowTest {
 public:
  BrowserViewLayoutTest()
      : delegate_(nullptr),
        top_container_(nullptr),
        tab_strip_(nullptr),
        toolbar_(nullptr),
        infobar_container_(nullptr),
        contents_container_(nullptr),
        contents_web_view_(nullptr),
        devtools_web_view_(nullptr) {}
  ~BrowserViewLayoutTest() override {}

  BrowserViewLayout* layout() { return layout_.get(); }
  MockBrowserViewLayoutDelegate* delegate() { return delegate_; }
  MockView* root_view() { return root_view_.get(); }
  MockView* top_container() { return top_container_; }
  TabStrip* tab_strip() { return tab_strip_; }
  MockView* toolbar() { return toolbar_; }
  InfoBarContainerView* infobar_container() { return infobar_container_; }
  MockView* contents_container() { return contents_container_; }

  // BrowserWithTestWindowTest overrides:
  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    root_view_.reset(new MockView(gfx::Size(800, 600)));

    immersive_mode_controller_.reset(new MockImmersiveModeController);

    top_container_ = new MockView(gfx::Size(800, 60));
    tab_strip_ = new TabStrip(nullptr);
    top_container_->AddChildView(tab_strip_);
    toolbar_ = new MockView(gfx::Size(800, 30));
    top_container_->AddChildView(toolbar_);
    root_view_->AddChildView(top_container_);

    infobar_container_ = new InfoBarContainerView(nullptr);
    root_view_->AddChildView(infobar_container_);

    contents_web_view_ = new MockView(gfx::Size(800, 600));
    devtools_web_view_ = new MockView(gfx::Size(800, 600));
    devtools_web_view_->SetVisible(false);

    contents_container_ = new MockView(gfx::Size(800, 600));
    contents_container_->AddChildView(devtools_web_view_);
    contents_container_->AddChildView(contents_web_view_);
    ContentsLayoutManager* contents_layout_manager =
        new ContentsLayoutManager(devtools_web_view_, contents_web_view_);
    contents_container_->SetLayoutManager(contents_layout_manager);

    root_view_->AddChildView(contents_container_);

    // TODO(jamescook): Attach |layout_| to |root_view_|?
    layout_.reset(new BrowserViewLayout);
    delegate_ = new MockBrowserViewLayoutDelegate(contents_web_view_);
    layout_->Init(delegate_,
                  browser(),
                  nullptr,  // BrowserView.
                  top_container_,
                  tab_strip_,
                  toolbar_,
                  infobar_container_,
                  contents_container_,
                  contents_layout_manager,
                  immersive_mode_controller_.get());
  }

 private:
  scoped_ptr<BrowserViewLayout> layout_;
  MockBrowserViewLayoutDelegate* delegate_;  // Owned by |layout_|.
  scoped_ptr<MockView> root_view_;

  // Views owned by |root_view_|.
  MockView* top_container_;
  TabStrip* tab_strip_;
  MockView* toolbar_;
  InfoBarContainerView* infobar_container_;
  MockView* contents_container_;
  MockView* contents_web_view_;
  MockView* devtools_web_view_;

  scoped_ptr<MockImmersiveModeController> immersive_mode_controller_;

  DISALLOW_COPY_AND_ASSIGN(BrowserViewLayoutTest);
};

// Test basic construction and initialization.
TEST_F(BrowserViewLayoutTest, BrowserViewLayout) {
  EXPECT_TRUE(layout()->browser());
  EXPECT_TRUE(layout()->GetWebContentsModalDialogHost());
  EXPECT_FALSE(layout()->InfobarVisible());
}

// Test the core layout functions.
TEST_F(BrowserViewLayoutTest, Layout) {
  // Simulate a window with no interesting UI.
  delegate()->set_tab_strip_visible(false);
  delegate()->set_toolbar_visible(false);
  delegate()->set_bookmark_bar_visible(false);
  layout()->Layout(root_view());

  // Top views are zero-height.
  EXPECT_EQ("0,0 0x0", tab_strip()->bounds().ToString());
  EXPECT_EQ("0,0 800x0", toolbar()->bounds().ToString());
  EXPECT_EQ("0,0 800x0", infobar_container()->bounds().ToString());
  // Contents split fills the window.
  EXPECT_EQ("0,0 800x600", contents_container()->bounds().ToString());

  // Turn on the toolbar, like in a pop-up window.
  delegate()->set_toolbar_visible(true);
  layout()->Layout(root_view());

  // Now the toolbar has bounds and other views shift down.
  EXPECT_EQ("0,0 0x0", tab_strip()->bounds().ToString());
  EXPECT_EQ("0,0 800x30", toolbar()->bounds().ToString());
  EXPECT_EQ("0,30 800x0", infobar_container()->bounds().ToString());
  EXPECT_EQ("0,30 800x570", contents_container()->bounds().ToString());

  // TODO(jamescook): Tab strip and bookmark bar.
}

TEST_F(BrowserViewLayoutTest, LayoutDownloadShelf) {
  scoped_ptr<MockView> download_shelf(new MockView(gfx::Size(800, 50)));
  layout()->set_download_shelf(download_shelf.get());

  // If download shelf doesn't need layout, it doesn't move the bottom edge.
  delegate()->set_download_shelf_needs_layout(false);
  const int kBottom = 500;
  EXPECT_EQ(kBottom, layout()->LayoutDownloadShelf(kBottom));

  // Download shelf layout moves up the bottom edge and sets visibility.
  delegate()->set_download_shelf_needs_layout(true);
  download_shelf->SetVisible(false);
  EXPECT_EQ(450, layout()->LayoutDownloadShelf(kBottom));
  EXPECT_TRUE(download_shelf->visible());
  EXPECT_EQ("0,450 0x50", download_shelf->bounds().ToString());
}
