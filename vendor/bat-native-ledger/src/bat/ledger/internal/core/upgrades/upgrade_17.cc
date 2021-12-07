/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/upgrades/upgrade_17.h"

#include "bat/ledger/internal/core/upgrades/migration_job.h"

namespace ledger {

namespace {

const char kSQL[] = R"sql(
  ALTER TABLE contribution_info ADD processor INTEGER NOT NULL DEFAULT 1;
)sql";

}  // namespace

constexpr int Upgrade17::kVersion;

void Upgrade17::Start() {
  CompleteWith(context().StartJob<MigrationJob>(kVersion, kSQL));
}

}  // namespace ledger
