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

#include <memory>
#include <utility>

#include "endstone/actor/actor.h"
#include "endstone/block/block_data.h"
#include "endstone/event/block/block_event.h"

namespace endstone {

/**
 * @brief Called after a block's runtime state has changed.
 *
 * This event observes both type changes and property-only changes. It is not
 * cancellable because the world mutation has already completed.
 */
class BlockStateChangedEvent final : public BlockEvent {
public:
    ENDSTONE_EVENT(BlockStateChangedEvent);

    BlockStateChangedEvent(std::unique_ptr<Block> block, std::unique_ptr<BlockData> old_data,
                           std::unique_ptr<BlockData> new_data, Actor *actor = nullptr)
        : BlockEvent(std::move(block)), old_data_(std::move(old_data)), new_data_(std::move(new_data)), actor_(actor)
    {
    }

    /**
     * @brief Gets the immutable block data before the change.
     */
    [[nodiscard]] BlockData &getOldData() const { return *old_data_; }

    /**
     * @brief Gets the immutable block data after the change.
     */
    [[nodiscard]] BlockData &getNewData() const { return *new_data_; }

    /**
     * @brief Gets the actor associated with the change, when BDS supplies one.
     */
    [[nodiscard]] Actor *getActor() const noexcept { return actor_; }

private:
    std::unique_ptr<BlockData> old_data_;
    std::unique_ptr<BlockData> new_data_;
    Actor *actor_;
};

}  // namespace endstone
