// Copyright (c) 2026, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include <memory>
#include <type_traits>

#include <gtest/gtest.h>

#include "endstone/damage/damage_source.h"
#include "endstone/event/actor/actor_knockback_event.h"
#include "endstone/event/actor/actor_spawn_event.h"
#include "endstone/event/actor/projectile_hit_event.h"
#include "endstone/event/block/block_state_changed_event.h"
#include "endstone/event/level/dimension_load_event.h"

namespace {

class LegacyCompatibleDamageSource final : public endstone::DamageSource {
public:
    [[nodiscard]] std::string_view getType() const override { return "test"; }
    [[nodiscard]] endstone::Actor *getActor() const override { return nullptr; }
    [[nodiscard]] endstone::Actor *getDamagingActor() const override { return nullptr; }
    [[nodiscard]] bool isIndirect() const override { return false; }
};

static_assert(!std::is_abstract_v<LegacyCompatibleDamageSource>);
static_assert(std::is_constructible_v<endstone::ActorSpawnEvent, endstone::Actor &>);
static_assert(std::is_constructible_v<endstone::ActorSpawnEvent, endstone::Actor &, endstone::SpawnReason>);
static_assert(std::is_constructible_v<endstone::ActorKnockbackEvent, endstone::Mob &, endstone::Actor *,
                                      endstone::Vector>);
static_assert(std::is_constructible_v<endstone::ProjectileHitEvent, endstone::Actor &, endstone::Location,
                                      endstone::Actor *, std::unique_ptr<endstone::Block>>);
static_assert(std::is_constructible_v<endstone::BlockStateChangedEvent, std::unique_ptr<endstone::Block>,
                                      std::unique_ptr<endstone::BlockData>, std::unique_ptr<endstone::BlockData>,
                                      endstone::Actor *>);
static_assert(!std::is_base_of_v<endstone::ICancellable, endstone::BlockStateChangedEvent>);
static_assert(std::is_constructible_v<endstone::DimensionLoadEvent, endstone::Dimension &>);

TEST(EventApiTest, LegacyDamageSourceDefaultsToUnknownLocation)
{
    const LegacyCompatibleDamageSource source;
    EXPECT_FALSE(source.getSourceLocation().has_value());
}

TEST(EventApiTest, SpawnReasonExposesNaturalClassification)
{
    EXPECT_NE(endstone::SpawnReason::Natural, endstone::SpawnReason::Unknown);
}

}  // namespace
