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

#include "endstone/core/level/block_source_listener.h"

#include <memory>

#include "bedrock/world/actor/actor.h"
#include "bedrock/world/level/block_source.h"
#include "endstone/core/actor/actor.h"
#include "endstone/core/block/block.h"
#include "endstone/core/block/block_data.h"
#include "endstone/core/server.h"
#include "endstone/event/block/block_state_changed_event.h"

BlockSourceListener::~BlockSourceListener() = default;

namespace endstone::core {

EndstoneBlockSourceListener::EndstoneBlockSourceListener(::BlockSource &source) : source_(&source)
{
    source.addListener(*this);
}

EndstoneBlockSourceListener::~EndstoneBlockSourceListener()
{
    if (source_ != nullptr) {
        source_->removeListener(*this);
    }
}

void EndstoneBlockSourceListener::onSourceCreated(::BlockSource &) {}

void EndstoneBlockSourceListener::onSourceDestroyed(::BlockSource &source)
{
    if (source_ == &source) {
        source_ = nullptr;
    }
}

void EndstoneBlockSourceListener::onAreaChanged(::BlockSource &, const BlockPos &, const BlockPos &) {}

void EndstoneBlockSourceListener::onBlockChanged(::BlockSource &source, const BlockPos &position, std::uint32_t,
                                                  const ::Block &old_block, const ::Block &new_block, int,
                                                  const ActorBlockSyncMessage *, BlockChangedEventTarget,
                                                  ::Actor *actor)
{
    if (old_block.getRuntimeId() == new_block.getRuntimeId()) {
        return;
    }

    auto &server = EndstoneServer::getInstance();
    if (!server.isPrimaryThread()) {
        return;
    }

    auto old_data = std::make_unique<EndstoneBlockData>(const_cast<::Block &>(old_block));
    auto new_data = std::make_unique<EndstoneBlockData>(const_cast<::Block &>(new_block));
    auto *endstone_actor = actor == nullptr ? nullptr : &actor->getEndstoneActor();
    BlockStateChangedEvent event{EndstoneBlock::at(source, position), std::move(old_data), std::move(new_data),
                                 endstone_actor};
    server.getPluginManager().callEvent(event);
}

void EndstoneBlockSourceListener::onBrightnessChanged(::BlockSource &, const BlockPos &) {}

void EndstoneBlockSourceListener::onBlockEntityChanged(::BlockSource &, ::BlockActor &) {}

void EndstoneBlockSourceListener::onEntityChanged(::BlockSource &, ::Actor &) {}

void EndstoneBlockSourceListener::onBlockEvent(::BlockSource &, int, int, int, int, int) {}

}  // namespace endstone::core
