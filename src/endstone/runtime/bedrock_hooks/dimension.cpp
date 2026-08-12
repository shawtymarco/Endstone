// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bedrock/world/level/dimension/dimension.h"

#include <chrono>

#include "endstone/core/level/dimension.h"
#include "endstone/core/level/level.h"
#include "endstone/core/server.h"
#include "endstone/runtime/hook.h"

void Dimension::tickRedstone()
{
    auto &server = endstone::core::EndstoneServer::getInstance();
    auto *level = server.getEndstoneLevel();
    auto *dimension = level == nullptr ? nullptr : level->getDimension(getDimensionId().value);
    if (dimension == nullptr) {
        ENDSTONE_HOOK_CALL_ORIGINAL(&Dimension::tickRedstone, this);
        return;
    }

    auto &state = static_cast<endstone::core::EndstoneDimension *>(dimension)->getRedstoneTickState();
    if (!state.isEnabled()) {
        state.recordSkipped();
        return;
    }

    const auto started = std::chrono::steady_clock::now();
    ENDSTONE_HOOK_CALL_ORIGINAL(&Dimension::tickRedstone, this);
    state.recordExecuted(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - started));
}
