/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

public interface ApprovedTxObserver {
    default public void OnTxApprovedRejected(boolean approved, String accountName, String txId){};
    default public void OnTxPending(String accountName, String txId){};
}
