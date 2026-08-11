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

#pragma once

#include <cstdint>

#include "bedrock/world/level/block_source_listener.h"

namespace endstone::core {

class EndstoneBlockSourceListener final : public BlockSourceListener {
public:
    explicit EndstoneBlockSourceListener(::BlockSource &source);
    ~EndstoneBlockSourceListener() override;

    EndstoneBlockSourceListener(const EndstoneBlockSourceListener &) = delete;
    EndstoneBlockSourceListener &operator=(const EndstoneBlockSourceListener &) = delete;

    void onSourceCreated(::BlockSource &) override;
    void onSourceDestroyed(::BlockSource &) override;
    void onAreaChanged(::BlockSource &, const BlockPos &, const BlockPos &) override;
    void onBlockChanged(::BlockSource &, const BlockPos &, std::uint32_t, const ::Block &, const ::Block &, int,
                        const ActorBlockSyncMessage *, BlockChangedEventTarget, ::Actor *) override;
    void onBrightnessChanged(::BlockSource &, const BlockPos &) override;
    void onBlockEntityChanged(::BlockSource &, ::BlockActor &) override;
    void onEntityChanged(::BlockSource &, ::Actor &) override;
    void onBlockEvent(::BlockSource &, int, int, int, int, int) override;

private:
    ::BlockSource *source_;
};

}  // namespace endstone::core
