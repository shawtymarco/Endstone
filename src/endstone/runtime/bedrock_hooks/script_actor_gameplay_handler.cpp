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

#include "bedrock/scripting/event_handlers/script_actor_gameplay_handler.h"

#include <optional>
#include <utility>

#include "bedrock/world/actor/actor.h"
#include "endstone/core/actor/mob.h"
#include "endstone/core/block/block.h"
#include "endstone/core/damage/damage_source.h"
#include "endstone/core/entity/components/flag_components.h"
#include "endstone/core/server.h"
#include "endstone/event/actor/actor_damage_event.h"
#include "endstone/event/actor/actor_death_event.h"
#include "endstone/event/actor/actor_remove_event.h"
#include "endstone/event/actor/projectile_hit_event.h"
#include "endstone/runtime/bedrock_hooks/explosion_context.h"
#include "endstone/runtime/vtable_hook.h"

namespace {
bool handleEvent(const ActorKilledEvent &event)
{
    if (const auto *mob = WeakEntityRef(event.actor_context).tryUnwrap<::Mob>(); mob && !mob->isPlayer()) {
        const auto &server = endstone::core::EndstoneServer::getInstance();
        endstone::ActorDeathEvent e{mob->getEndstoneActor<endstone::core::EndstoneMob>(),
                                    std::make_unique<endstone::core::EndstoneDamageSource>(*event.source)};
        server.getPluginManager().callEvent(e);
    }
    return true;
}

bool handleEvent(const ActorRemovedEvent &event)
{
    if (auto *actor = WeakEntityRef(event.entity).tryUnwrap<::Actor>(); actor) {
        if (actor->isPlayer()) {
            // Don't call for player
            return true;
        }

        if (actor->hasComponent<endstone::core::InternalRemoveFlagComponent>()) {
            // Don't call if the entity is removed before it is even spawned (when the spawn event is cancelled)
            actor->addOrRemoveComponent<endstone::core::InternalRemoveFlagComponent>(false);
            return true;
        }

        endstone::ActorRemoveEvent e{actor->getEndstoneActor()};
        endstone::core::EndstoneServer::getInstance().getPluginManager().callEvent(e);
    }
    return true;
}

bool handleEvent(::ActorBeforeHurtEvent &event)
{
    const auto &source = event.source;
    const auto &server = endstone::core::EndstoneServer::getInstance();
    auto &mob = event.entity.getEndstoneActor<endstone::core::EndstoneMob>();
    std::optional<endstone::Location> source_location;
    if (const auto &explosion = endstone::runtime::getLastExplosionPos(); explosion.has_value()) {
        source_location.emplace(mob.getDimension(), explosion->x, explosion->y, explosion->z);
    }
    endstone::ActorDamageEvent e{
        mob, std::make_unique<endstone::core::EndstoneDamageSource>(source, std::move(source_location)), event.damage};
    server.getPluginManager().callEvent(e);
    if (e.isCancelled()) {
        return false;
    }
    event.damage = e.getDamage();
    return true;
}

bool handleEvent(const ::ProjectileHitEvent &event)
{
    auto &projectile = event.projectile.getEndstoneActor();
    const auto &hit = event.hit_result;
    const auto &position = hit.getPosition();
    endstone::Location location{projectile.getDimension(), position.x, position.y, position.z};
    endstone::Actor *hit_actor = nullptr;
    std::unique_ptr<endstone::Block> hit_block;
    if (hit.getType() == HitResultType::ENTITY || hit.getType() == HitResultType::ENTITY_OUT_OF_RANGE) {
        if (auto *actor = WeakEntityRef(hit.getEntity()).tryUnwrap<::Actor>(); actor != nullptr) {
            hit_actor = &actor->getEndstoneActor();
        }
    }
    else if (hit.getType() == HitResultType::TILE) {
        hit_block = endstone::core::EndstoneBlock::at(event.projectile.getDimensionBlockSource(), hit.getBlock());
    }
    endstone::ProjectileHitEvent e{projectile, location, hit_actor, std::move(hit_block)};
    endstone::core::EndstoneServer::getInstance().getPluginManager().callEvent(e);
    return !e.isCancelled();
}
}  // namespace

HandlerResult ScriptActorGameplayHandler::handleEvent1(const ActorGameplayEvent<void> &event)
{
    auto visitor = [&](auto &&arg) -> HandlerResult {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Details::ValueOrRef<const ActorKilledEvent>> ||
                      std::is_same_v<T, Details::ValueOrRef<const ActorRemovedEvent>>) {
            if (!handleEvent(arg.value())) {
                return HandlerResult::BypassListeners;
            }
        }
        return ENDSTONE_VHOOK_CALL_ORIGINAL(&ScriptActorGameplayHandler::handleEvent1, this, event);
    };
    return event.visit(visitor);
}

GameplayHandlerResult<CoordinatorResult> ScriptActorGameplayHandler::handleEvent3(
    const ActorGameplayEvent<CoordinatorResult> &event)
{
    auto visitor = [&](auto &&arg) -> GameplayHandlerResult<CoordinatorResult> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Details::ValueOrRef<const ::ProjectileHitEvent>>) {
            if (!handleEvent(arg.value())) {
                return {HandlerResult::BypassListeners, CoordinatorResult::Cancel};
            }
        }
        return ENDSTONE_VHOOK_CALL_ORIGINAL(&ScriptActorGameplayHandler::handleEvent3, this, event);
    };
    return event.visit(visitor);
}

GameplayHandlerResult<CoordinatorResult> ScriptActorGameplayHandler::handleEvent4(
    MutableActorGameplayEvent<CoordinatorResult> &event)
{
    auto visitor = [&](auto &&arg) -> GameplayHandlerResult<CoordinatorResult> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Details::ValueOrRef<::ActorBeforeHurtEvent>>) {
            if (!handleEvent(arg.value())) {
                return {HandlerResult::BypassListeners, CoordinatorResult::Cancel};
            }
        }
        return ENDSTONE_VHOOK_CALL_ORIGINAL(&ScriptActorGameplayHandler::handleEvent4, this, event);
    };
    return event.visit(visitor);
}
