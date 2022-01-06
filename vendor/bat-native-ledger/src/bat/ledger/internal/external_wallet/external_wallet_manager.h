/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_MANAGER_H_

#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/future.h"
#include "bat/ledger/internal/external_wallet/external_wallet_data.h"
#include "bat/ledger/internal/external_wallet/external_wallet_handler.h"

namespace ledger {

// TODO(zenparsing): Consider renaming this whole thing to "LinkedWallet".
class ExternalWalletManager : public BATLedgerContext::Object {
 public:
  static const char kContextKey[];

  std::string GetAuthorizationURL(ExternalWalletProvider provider);

  Future<absl::optional<ExternalWallet>> HandleAuthorizationResponse(
      ExternalWalletProvider provider,
      const base::flat_map<std::string, std::string>& auth_params);

  Future<absl::optional<double>> GetBalance();

  Future<absl::optional<ExternalWalletTransferResult>> TransferBAT(
      const std::string& destination,
      double amount,
      const std::string& description);

  Future<absl::optional<ExternalWalletTransferResult>> TransferBAT(
      const std::string& destination,
      double amount);

  absl::optional<ExternalWallet> GetExternalWallet();

  bool HasExternalWallet();

  absl::optional<std::string> GetContributionFeeAddress();

  absl::optional<std::string> GetContributionTokenOrderAddress();

  Future<bool> Unlink();
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_MANAGER_H_
