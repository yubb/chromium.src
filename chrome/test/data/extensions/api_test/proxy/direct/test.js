// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// proxy api test
// browser_tests.exe --gtest_filter=ProxySettingsApiTest.ProxyDirectSettings

function expect(expected, message) {
  return chrome.test.callbackPass(function(value) {
    chrome.test.assertEq(expected, value, message);
  });
}

var config = {
  mode: "direct",
};

chrome.test.runTests([
  // Verify that execution has started to make sure flaky timeouts are not
  // caused by us.
  function verifyTestsHaveStarted() {
    chrome.test.succeed();
  },
  function setAutoSettings() {
    chrome.experimental.proxy.settings.set(
        {'value': config},
        chrome.test.callbackPass());
  },
  function verifyRegular() {
    chrome.experimental.proxy.settings.get(
        {'incognito': false},
        expect({ 'value': config,
                 'levelOfControl': "ControlledByThisExtension" },
               "invalid proxy settings"));
  },
  function verifyIncognito() {
    chrome.experimental.proxy.settings.get(
        {'incognito': true},
        expect({ 'value': config,
                 'incognitoSpecific': false,
                 'levelOfControl': "ControlledByThisExtension" },
               "invalid proxy settings"));
  }
]);
