// Copyright (c) 2026, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#pragma once

#include <utility>

#include "bedrock/world/actor/actor.h"
#include "bedrock/world/actor/mob.h"
#include "endstone/event/actor/actor_spawn_event.h"

namespace endstone::core {

inline thread_local SpawnReason spawn_reason_override = SpawnReason::Unknown;

class ScopedSpawnReason final {
public:
    explicit ScopedSpawnReason(SpawnReason reason) noexcept
        : previous_(std::exchange(spawn_reason_override, reason))
    {
    }
    ~ScopedSpawnReason() { spawn_reason_override = previous_; }

    ScopedSpawnReason(const ScopedSpawnReason &) = delete;
    ScopedSpawnReason &operator=(const ScopedSpawnReason &) = delete;

private:
    SpawnReason previous_;
};

[[nodiscard]] inline SpawnReason resolveSpawnReason(const ::Actor &actor, const ::Mob *mob) noexcept
{
    if (spawn_reason_override != SpawnReason::Unknown) {
        return spawn_reason_override;
    }
    if (mob != nullptr) {
        if (mob->isNaturallySpawned()) {
            return SpawnReason::Natural;
        }
        switch (mob->getSpawnMethod()) {
        case MobSpawnMethod::SpawnEgg:
            return SpawnReason::SpawnEgg;
        case MobSpawnMethod::Command:
            return SpawnReason::Command;
        case MobSpawnMethod::Dispenser:
            return SpawnReason::Dispenser;
        case MobSpawnMethod::Spawner:
            return SpawnReason::Spawner;
        case MobSpawnMethod::Unknown:
        case MobSpawnMethod::SpawnMethod_Count:
            break;
        }
    }
    switch (actor.getInitializationMethod()) {
    case ActorInitializationMethod::Loaded:
        return SpawnReason::Loaded;
    case ActorInitializationMethod::Born:
        return SpawnReason::Breeding;
    case ActorInitializationMethod::Transformed:
        return SpawnReason::Transformation;
    case ActorInitializationMethod::Invalid:
    case ActorInitializationMethod::Spawned:
    case ActorInitializationMethod::Updated:
    case ActorInitializationMethod::Event:
    case ActorInitializationMethod::Legacy:
        return SpawnReason::Unknown;
    }
    return SpawnReason::Unknown;
}

}  // namespace endstone::core
