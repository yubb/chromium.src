// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/remoting/key_code_test_map.h"
#include "chrome/test/remoting/remote_desktop_browsertest.h"
#include "chrome/test/remoting/remote_test_helper.h"
#include "chrome/test/remoting/waiter.h"
#include "extensions/browser/app_window/app_window.h"

namespace remoting {

class Me2MeBrowserTest : public RemoteDesktopBrowserTest {
 protected:
  void TestKeypressInput(ui::KeyboardCode, const char*);

  void ConnectPinlessAndCleanupPairings(bool cleanup_all);
  bool IsPairingSpinnerHidden();
  void SetupForRemoteHostTest();

  void RestoreApp();
  void MinimizeApp();
};

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Connect_Local_Host) {
  SetUpTest();

  ConnectToLocalHost(false);

  // TODO(chaitali): Change the mouse input test to also work in the
  // HTTP server framework
  // TestMouseInput();

  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Connect_Remote_Host) {
  VerifyInternetAccess();
  Install();
  LaunchChromotingApp(false);

  // Authorize, Authenticate, and Approve.
  Auth();
  ExpandMe2Me();

  ConnectToRemoteHost(remote_host_name(), false);

  // TODO(weitaosu): Find a way to verify keyboard input injection.
  // We cannot use TestKeyboardInput because it assumes
  // that the client and the host are on the same machine.

  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Remote_Host_Keypress) {
  SetupForRemoteHostTest();

  // Test all key characters
  int length = sizeof(test_alpha_map)/sizeof(KeyCodeTestMap);
  for (int i = 0; i < length; i++) {
    KeyCodeTestMap key = test_alpha_map[i];
    TestKeypressInput(key.vkey_code, key.code);
  }
  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Remote_Host_Digitpress) {
  SetupForRemoteHostTest();

  // Test all digit characters
  int length = sizeof(test_digit_map)/sizeof(KeyCodeTestMap);
  for (int i = 0; i < length; i++) {
    KeyCodeTestMap key = test_digit_map[i];
    TestKeypressInput(key.vkey_code, key.code);
  }
  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Remote_Host_Specialpress) {
  SetupForRemoteHostTest();

  // Test all special characters
  int length = sizeof(test_special_map)/sizeof(KeyCodeTestMap);
  for (int i = 0; i < length; i++) {
    KeyCodeTestMap key = test_special_map[i];
    TestKeypressInput(key.vkey_code, key.code);
  }
  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Remote_Host_Numpadpress) {
  SetupForRemoteHostTest();

  // Test all numpad characters
  int length = sizeof(test_numpad_map)/sizeof(KeyCodeTestMap);
  for (int i = 0; i < length; i++) {
    KeyCodeTestMap key = test_numpad_map[i];
    TestKeypressInput(key.vkey_code, key.code);
  }
  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Connect_Pinless) {
  SetUpTest();

  ASSERT_FALSE(HtmlElementVisible("paired-client-manager-message"))
      << "The host must have no pairings before running the pinless test.";

  // Test that cleanup works with either the Delete or Delete all buttons.
  ConnectPinlessAndCleanupPairings(false);
  ConnectPinlessAndCleanupPairings(true);

  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_v2_Alive_OnLostFocus) {
  SetUpTest();

  // Connect to host.
  ConnectToLocalHost(false);

  // Minimize the window
  MinimizeApp();

  // Wait for a few seconds for app to process any notifications it
  // would have got from minimizing.
  ASSERT_TRUE(TimeoutWaiter(base::TimeDelta::FromSeconds(4)).Wait());

  // Validate that the session is still active.
  EXPECT_TRUE(RemoteDesktopBrowserTest::IsSessionConnected());

  // Maximize so we can disconnect and teardown.
  RestoreApp();

  // Cleanup
  DisconnectMe2Me();
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest, MANUAL_Me2Me_RetryOnHostOffline) {
  content::WebContents* content = SetUpTest();
  LoadScript(content, FILE_PATH_LITERAL("me2me_browser_test.js"));
  RunJavaScriptTest(content, "RetryOnHostOffline", "{"
    "pin: '" + me2me_pin() + "'"
  "}");

  Cleanup();
}

IN_PROC_BROWSER_TEST_F(Me2MeBrowserTest,
                       MANUAL_Me2Me_Disable_Remote_Connection) {
  SetUpTest();

  DisableRemoteConnection();
  EXPECT_FALSE(IsLocalHostReady());

  Cleanup();
}

void Me2MeBrowserTest::SetupForRemoteHostTest() {
  VerifyInternetAccess();
  OpenClientBrowserPage();
  Install();
  LaunchChromotingApp(false);

  // Authorize, Authenticate, and Approve.
  Auth();
  ExpandMe2Me();
  ConnectToRemoteHost(remote_host_name(), false);

  // Wake up the machine if it's sleeping.
  // This is only needed when testing manually as the host machine
  // may be sleeping.
  SimulateKeyPressWithCode(ui::VKEY_RETURN, "Enter");
}

void Me2MeBrowserTest::TestKeypressInput(
    ui::KeyboardCode keyCode,
    const char* code) {
  remote_test_helper()->ClearLastEvent();
  VLOG(1) << "Pressing " << code;
  SimulateKeyPressWithCode(keyCode, code);
  Event event;
  remote_test_helper()->GetLastEvent(&event);
  ASSERT_EQ(Action::Keydown, event.action);
  ASSERT_EQ(keyCode, event.value);
}

void Me2MeBrowserTest::ConnectPinlessAndCleanupPairings(bool cleanup_all) {
  // First connection: verify that a PIN is requested, and request pairing.
  ConnectToLocalHost(true);
  DisconnectMe2Me();

  // TODO(jamiewalch): This reload is only needed because there's a bug in the
  // web-app whereby it doesn't refresh its pairing state correctly.
  // http://crbug.com/311290
  LaunchChromotingApp(false);
  ASSERT_TRUE(HtmlElementVisible("paired-client-manager-message"));

  // Second connection: verify that no PIN is requested.
  ClickOnControl("this-host-connect");
  WaitForConnection();
  DisconnectMe2Me();

  // Clean up pairings.
  ClickOnControl("open-paired-client-manager-dialog");
  ASSERT_TRUE(HtmlElementVisible("paired-client-manager-dialog"));

  if (cleanup_all) {
    ClickOnControl("delete-all-paired-clients");
  } else {
    std::string host_id = ExecuteScriptAndExtractString(
        "remoting.pairedClientManager.getFirstClientIdForTesting_()");
    std::string node_id = "delete-client-" + host_id;
    ClickOnControl(node_id);
  }

  // Wait for the "working" spinner to disappear. The spinner is shown by both
  // methods of deleting a host and is removed when the operation completes.
  ConditionalTimeoutWaiter waiter(
      base::TimeDelta::FromSeconds(5),
      base::TimeDelta::FromMilliseconds(200),
      base::Bind(&Me2MeBrowserTest::IsPairingSpinnerHidden, this));
  EXPECT_TRUE(waiter.Wait());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      "document.getElementById('delete-all-paired-clients').disabled"));

  ClickOnControl("close-paired-client-manager-dialog");
  ASSERT_FALSE(HtmlElementVisible("paired-client-manager-dialog"));
  ASSERT_FALSE(HtmlElementVisible("paired-client-manager-message"));
}

bool Me2MeBrowserTest::IsPairingSpinnerHidden() {
  return !HtmlElementVisible("paired-client-manager-dialog-working");
}

void Me2MeBrowserTest::MinimizeApp() {
  extensions::AppWindow* appWindow = GetFirstAppWindow();
  if (appWindow) {
    appWindow->Minimize();
  } else {
    browser()->window()->Minimize();
  }
}

void Me2MeBrowserTest::RestoreApp() {
  extensions::AppWindow* appWindow = GetFirstAppWindow();
  if (appWindow) {
    appWindow->Restore();
  } else {
    browser()->window()->Restore();
  }
}

}  // namespace remoting
