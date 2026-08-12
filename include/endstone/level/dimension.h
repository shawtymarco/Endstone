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

#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "endstone/block/block.h"
#include "endstone/inventory/item_stack.h"
#include "endstone/level/chunk.h"
#include "endstone/util/result.h"

namespace endstone {

/**
 * @brief Represents a dimension within a Level.
 */
class Dimension {
public:
    /**
     * @brief Represents various dimension types.
     */
    enum class Type {
        Overworld = 0,
        Nether = 1,
        TheEnd = 2,
        Custom = 999
    };

    /**
     * @brief Runtime statistics for this dimension's redstone tick hook.
     *
     * Executed ticks are calls forwarded to BDS. Skipped ticks are calls suppressed
     * because redstone ticking was disabled through setRedstoneTickingEnabled().
     */
    struct RedstoneTickMetrics {
        std::uint64_t executed_ticks{};
        std::uint64_t skipped_ticks{};
        std::chrono::nanoseconds total_duration{};
        std::chrono::nanoseconds last_duration{};
        std::chrono::nanoseconds maximum_duration{};
    };

    virtual ~Dimension() = default;

    /**
     * @brief Gets the name of this dimension
     *
     * @return Name of this dimension
     */
    [[nodiscard]] virtual std::string getName() const = 0;

    /**
     * @brief Gets the type of this dimension
     *
     * @return Type of this dimension
     */
    [[nodiscard]] virtual Type getType() const = 0;

    /**
     * @brief Gets the level to which this dimension belongs
     *
     * @return Level containing this dimension.
     */
    [[nodiscard]] virtual Level &getLevel() const = 0;

    /**
     * @brief Gets the Block at the given coordinates
     *
     * @param x X-coordinate of the block
     * @param y Y-coordinate of the block
     * @param z Z-coordinate of the block
     * @return Block at the given coordinates
     */
    [[nodiscard]] virtual std::unique_ptr<Block> getBlockAt(int x, int y, int z) const = 0;

    /**
     * @brief Gets the Block at the given Location.
     *
     * @param location Location of the block
     * @return Block at the given coordinates
     */
    [[nodiscard]] virtual std::unique_ptr<Block> getBlockAt(Location location) const = 0;

    /**
     * @brief Gets the highest non-empty (impassable) coordinate at the given coordinates.
     *
     * @param x X-coordinate of the blocks
     * @param z Z-coordinate of the blocks
     * @return Y-coordinate of the highest non-empty block
     */
    [[nodiscard]] virtual int getHighestBlockYAt(int x, int z) const = 0;

    /**
     * @brief Gets the highest non-empty (impassable) block at the given coordinates.
     *
     * @param x X-coordinate of the block
     * @param z Z-coordinate of the block
     * @return Highest non-empty block
     */
    [[nodiscard]] virtual std::unique_ptr<Block> getHighestBlockAt(int x, int z) const = 0;

    /**
     * @brief Gets the highest non-empty (impassable) block at the given Location.
     *
     * @param location Coordinates to get the highest block
     * @return Highest non-empty block
     */
    [[nodiscard]] virtual std::unique_ptr<Block> getHighestBlockAt(Location location) const = 0;

    /**
     * @brief Gets a list of all loaded Chunks
     *
     * @return All loaded chunks
     */
    [[nodiscard]] virtual std::vector<std::unique_ptr<Chunk>> getLoadedChunks() = 0;

    /**
     * @brief Checks if the Chunk at the given coordinates is loaded.
     *
     * @param x X-coordinate of the chunk
     * @param z Z-coordinate of the chunk
     * @return true if the chunk is loaded, otherwise false
     */
    [[nodiscard]] virtual bool isChunkLoaded(int x, int z) const = 0;

    /**
     * @brief Checks whether the dimension's chunk source knows the Chunk.
     *
     * @param x X-coordinate of the chunk
     * @param z Z-coordinate of the chunk
     * @return true if the chunk source knows the chunk, otherwise false
     */
    [[nodiscard]] virtual bool isChunkKnown(int x, int z) const = 0;

    /**
     * @brief Checks whether the Chunk has persisted data in the dimension's chunk storage.
     *
     * @param x X-coordinate of the chunk
     * @param z Z-coordinate of the chunk
     * @return true if the chunk is saved, otherwise false
     */
    [[nodiscard]] virtual bool isChunkSaved(int x, int z) const = 0;

    /**
     * @brief Requests the Chunk at the given coordinates to be loaded and keeps it resident.
     *
     * The resident hold lasts until unloadChunk() is called or the server restarts.
     *
     * @param x X-coordinate of the chunk
     * @param z Z-coordinate of the chunk
     * Loading may complete on a later tick. Use isChunkLoaded() to verify readiness.
     *
     * @return true if the load request was accepted, otherwise false
     */
    virtual bool loadChunk(int x, int z) = 0;

    /**
     * @brief Releases the resident hold placed by loadChunk().
     *
     * @param x X-coordinate of the chunk
     * @param z Z-coordinate of the chunk
     * @return true once the hold has been released
     */
    virtual bool unloadChunk(int x, int z) = 0;

    /**
     * @brief Drops an item at the specified Location
     *
     * @param location Location to drop the item
     * @param item ItemStack to drop
     *
     * @return Item entity created as a result of this method
     */
    [[nodiscard]] virtual Item &dropItem(Location location, const ItemStack &item) = 0;

    /**
     * @brief Creates an actor at the given Location
     *
     * @param location The location to spawn the actor
     * @param type The actor to spawn
     * @return Resulting Actor of this method
     */
    [[nodiscard]] virtual Actor *spawnActor(Location location, std::string type) = 0;

    /**
     * @brief Get a list of all actors in this dimension
     *
     * @return A List of all actors currently residing in this dimension
     */
    [[nodiscard]] virtual std::vector<Actor *> getActors() const = 0;

    /**
     * @brief Checks whether BDS redstone ticks are enabled in this dimension.
     */
    [[nodiscard]] virtual bool isRedstoneTickingEnabled() const = 0;

    /**
     * @brief Enables or disables BDS redstone ticks in this dimension.
     *
     * Disabling this changes vanilla gameplay semantics: circuits stop updating until
     * ticking is enabled again. The setting is runtime-only and is not persisted.
     */
    virtual void setRedstoneTickingEnabled(bool enabled) = 0;

    /**
     * @brief Gets a snapshot of this dimension's redstone tick metrics.
     */
    [[nodiscard]] virtual RedstoneTickMetrics getRedstoneTickMetrics() const = 0;

    /**
     * @brief Resets this dimension's redstone tick metrics.
     */
    virtual void resetRedstoneTickMetrics() = 0;
};

inline std::unique_ptr<Block> Location::getBlock() const
{
    return getDimension().getBlockAt(*this);
}

inline float Location::distanceSquared(const Location &other) const
{
    Preconditions::checkArgument(dimension_ == other.dimension_, "Cannot measure distance between {} and {}.",
                                 dimension_->getName(), other.dimension_->getName());
    return ((x_ - other.x_) * (x_ - other.x_)) + ((y_ - other.y_) * (y_ - other.y_)) +
           ((z_ - other.z_) * (z_ - other.z_));
}
}  // namespace endstone

template <>
struct std::formatter<endstone::Dimension> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const endstone::Dimension &self, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "Dimension(name={})", self.getName());
    }
};
