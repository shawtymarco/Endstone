// Copyright (c) 2026, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#pragma once

#include <memory>
#include <utility>

#include "endstone/block/block.h"
#include "endstone/event/actor/actor_event.h"
#include "endstone/event/cancellable.h"
#include "endstone/level/location.h"

namespace endstone {

/**
 * @brief Called when a projectile hits an actor or block.
 *
 * Cancelling this event cancels the underlying projectile hit coordinator event.
 */
class ProjectileHitEvent : public Cancellable<ActorEvent<Actor>> {
public:
    ENDSTONE_EVENT(ProjectileHitEvent);

    explicit ProjectileHitEvent(Actor &projectile, Location hit_location, Actor *hit_actor = nullptr,
                                std::unique_ptr<Block> hit_block = nullptr)
        : Cancellable(projectile), hit_location_(hit_location), hit_actor_(hit_actor),
          hit_block_(std::move(hit_block))
    {
    }
    ~ProjectileHitEvent() override = default;

    [[nodiscard]] Actor &getProjectile() const { return getActor(); }
    [[nodiscard]] const Location &getHitLocation() const noexcept { return hit_location_; }
    [[nodiscard]] Actor *getHitActor() const noexcept { return hit_actor_; }
    [[nodiscard]] Block *getHitBlock() const noexcept { return hit_block_.get(); }

private:
    Location hit_location_;
    Actor *hit_actor_;
    std::unique_ptr<Block> hit_block_;
};

}  // namespace endstone
