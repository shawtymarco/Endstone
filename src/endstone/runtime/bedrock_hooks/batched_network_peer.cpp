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

#include "bedrock/network/batched_network_peer.h"

#include "bedrock/network/packet.h"
#include "bedrock/network/packet/clientbound_map_item_data_packet.h"
#include "bedrock/network/packet/resource_pack_stack_packet.h"
#include "bedrock/network/packet/resource_packs_info_packet.h"
#include "bedrock/network/packet/start_game_packet.h"
#include "bedrock/network/raknet_connector.h"
#include "bedrock/network/server_network_system.h"
#include "bedrock/server/server_instance.h"
#include "bedrock/world/actor/player/player.h"
#include "bedrock/world/level/level.h"
#include "endstone/core/level/level.h"
#include "endstone/core/map/map_view.h"
#include "endstone/core/player.h"
#include "endstone/core/server.h"
#include "endstone/core/util/socket_address.h"
#include "endstone/event/server/packet_receive_event.h"
#include "endstone/event/server/packet_send_event.h"
#include "endstone/runtime/hook.h"

namespace {
constexpr float RotationByteScale = 360.0F / 256.0F;

bool readByte(std::string_view data, std::size_t &offset, std::uint8_t &value)
{
    if (offset >= data.size()) {
        return false;
    }
    value = static_cast<std::uint8_t>(data[offset++]);
    return true;
}

bool readUnsignedVarInt64(std::string_view data, std::size_t &offset, std::uint64_t &value)
{
    value = 0;
    for (std::uint32_t shift = 0; shift < 64; shift += 7) {
        std::uint8_t byte{};
        if (!readByte(data, offset, byte)) {
            return false;
        }
        value |= static_cast<std::uint64_t>(byte & 0x7FU) << shift;
        if ((byte & 0x80U) == 0) {
            return true;
        }
    }
    return false;
}

bool skipOptionalField(std::string_view data, std::size_t &offset, std::size_t value_size)
{
    std::uint8_t present{};
    if (!readByte(data, offset, present) || present > 1) {
        return false;
    }
    if (present != 0) {
        if (value_size > data.size() - offset) {
            return false;
        }
        offset += value_size;
    }
    return true;
}

struct DeltaMovementHeader {
    std::uint64_t runtime_id{};
    bool on_ground{};
    bool force_move{};
    bool force_move_local_entity{};
    bool force_completion{};
};

std::optional<DeltaMovementHeader> readDeltaMovementHeader(std::string_view payload)
{
    DeltaMovementHeader result;
    std::size_t offset{};
    if (!readUnsignedVarInt64(payload, offset, result.runtime_id)) {
        return std::nullopt;
    }
    for (int index = 0; index < 3; ++index) {
        if (!skipOptionalField(payload, offset, sizeof(float))) {
            return std::nullopt;
        }
    }
    for (int index = 0; index < 3; ++index) {
        if (!skipOptionalField(payload, offset, sizeof(std::uint8_t))) {
            return std::nullopt;
        }
    }
    std::uint8_t on_ground{};
    std::uint8_t force_move{};
    std::uint8_t force_move_local_entity{};
    std::uint8_t force_completion{};
    if (!readByte(payload, offset, on_ground) || !readByte(payload, offset, force_move) ||
        !readByte(payload, offset, force_move_local_entity) || !readByte(payload, offset, force_completion) ||
        offset != payload.size() || on_ground > 1 || force_move > 1 || force_move_local_entity > 1 ||
        force_completion > 1) {
        return std::nullopt;
    }
    result.on_ground = on_ground != 0;
    result.force_move = force_move != 0;
    result.force_move_local_entity = force_move_local_entity != 0;
    result.force_completion = force_completion != 0;
    return result;
}

void writeRotationByte(BinaryStream &stream, float rotation)
{
    const auto truncated = static_cast<std::int32_t>(rotation / RotationByteScale);
    stream.writeByte(static_cast<std::uint8_t>(static_cast<std::uint32_t>(truncated) & 0xFFU), "Rotation", nullptr);
}

std::optional<std::string> makeAbsolutePlayerMovement(std::string_view payload, endstone::Player *observer)
{
    if (observer == nullptr) {
        return std::nullopt;
    }
    const auto movement = readDeltaMovementHeader(payload);
    if (!movement.has_value() || movement->runtime_id == observer->getRuntimeId()) {
        return std::nullopt;
    }

    auto &server = endstone::core::EndstoneServer::getInstance();
    auto *level = server.getEndstoneLevel();
    if (level == nullptr) {
        return std::nullopt;
    }
    auto *moving_player = level->getHandle().getRuntimePlayer(ActorRuntimeID{movement->runtime_id});
    if (moving_player == nullptr) {
        return std::nullopt;
    }

    std::uint8_t flags{};
    flags |= movement->on_ground ? 0x01U : 0;
    flags |= movement->force_move ? 0x02U : 0;
    flags |= movement->force_move_local_entity ? 0x04U : 0;
    flags |= movement->force_completion ? 0x08U : 0;
    const auto &[x, y, z] = moving_player->getPosition();
    const auto &[pitch, yaw] = moving_player->getRotation();

    BinaryStream out;
    out.writeUnsignedVarInt64(movement->runtime_id, "Actor Runtime ID", nullptr);
    out.writeByte(flags, "Flags", nullptr);
    out.writeFloat(x, "Position X", nullptr);
    out.writeFloat(y, "Position Y", nullptr);
    out.writeFloat(z, "Position Z", nullptr);
    writeRotationByte(out, pitch);
    writeRotationByte(out, yaw);
    writeRotationByte(out, yaw);
    return out.getBuffer();
}

void patchPacket(const StartGamePacket &packet)
{
    const auto &server = endstone::core::EndstoneServer::getInstance();
    if (const auto *level = server.getEndstoneLevel(); level && !level->getHandle().isClientSideGenerationEnabled()) {
        auto &pk = const_cast<StartGamePacket &>(packet);
        pk.settings.setRandomSeed(0);
    }
}

void patchPacket(const ResourcePacksInfoPacket &packet)
{
    const auto &server = endstone::core::EndstoneServer::getInstance();
    auto &pk = const_cast<ResourcePacksInfoPacket &>(packet);
    for (auto &pack_info : pk.data.resource_packs) {
        if (const auto *key = server.getContentKey(pack_info.m_pack_id_version)) {
            pack_info.content_key = *key;
        }
    }
}

void patchPacket(const ResourcePackStackPacket &packet)
{
    if (packet.texture_pack_required) {
        const auto &server = endstone::core::EndstoneServer::getInstance();
        if (server.getAllowClientPacks()) {
            auto &pk = const_cast<ResourcePackStackPacket &>(packet);
            // false, otherwise the client will remove its own non-server-supplied resource packs.
            pk.texture_pack_required = false;
        }
    }
}

void patchPacket(const ClientboundMapItemDataPacket &packet, endstone::core::EndstonePlayer &player)
{
    const auto &server = endstone::core::EndstoneServer::getInstance();
    auto *map = static_cast<endstone::core::EndstoneMapView *>(server.getMap(packet.getMapId().raw_id));
    if (!map) {
        return;
    }

    auto &pk = const_cast<ClientboundMapItemDataPacket &>(packet);
    if (pk.payload.map_pixels.empty() && pk.payload.decorations.empty()) {
        return;  // Map creation, no data to be patched
    }

    const auto &render = map->render(player);

    // Patch pixels only when this packet carries a texture update
    if (!pk.payload.map_pixels.empty()) {
        if (pk.payload.start_x < 0 || pk.payload.start_y < 0 || pk.payload.width <= 0 || pk.payload.height <= 0 ||
            pk.payload.start_x + pk.payload.width > MapConstants::MAP_SIZE ||
            pk.payload.start_y + pk.payload.height > MapConstants::MAP_SIZE) {
            return;  // Out of bounds
        }
        for (auto x = 0; x < pk.payload.width; ++x) {
            for (auto y = 0; y < pk.payload.height; ++y) {
                pk.payload.map_pixels[x + (y * pk.payload.width)] =
                    render.buffer[(pk.payload.start_x + x) + ((pk.payload.start_y + y) * MapConstants::MAP_SIZE)];
            }
        }
    }

    // Tracked actor ids and decorations go on the wire as parallel arrays
    pk.payload.unique_ids.clear();
    pk.payload.decorations.clear();
    for (const auto &cursor : render.cursors) {
        if (cursor.isVisible()) {
            pk.payload.unique_ids.emplace_back(ActorUniqueID::INVALID_ID);
            pk.payload.decorations.emplace_back(
                std::make_shared<MapDecoration>(static_cast<MapDecoration::Type>(cursor.getType()), cursor.getX(),
                                                cursor.getY(), cursor.getDirection(), cursor.getCaption(),
                                                mce::Color::WHITE  // TODO(map): support different colors
                                                ));
        }
    }
}

void patchPacket(Packet &packet, endstone::Player *player)
{
    switch (packet.getId()) {
    case MinecraftPacketIds::StartGame:
        patchPacket(static_cast<const StartGamePacket &>(packet));
        break;
    case MinecraftPacketIds::ResourcePacksInfo:
        patchPacket(static_cast<const ResourcePacksInfoPacket &>(packet));
        break;
    case MinecraftPacketIds::ResourcePackStack:
        patchPacket(static_cast<const ResourcePackStackPacket &>(packet));
        break;
    case MinecraftPacketIds::MapData:
        if (player) {
            patchPacket(static_cast<const ClientboundMapItemDataPacket &>(packet),
                        static_cast<endstone::core::EndstonePlayer &>(*player));
        }
        break;
    default:
        break;
    }
}
}  // namespace

void BatchedNetworkPeer::sendPacket(const std::string &data, Reliability reliability, Compressibility compressible)
{
    ReadOnlyBinaryStream stream(data, false);
    auto result = stream.getUnsignedVarInt().discardError();
    if (!result) {
        const auto &server = endstone::core::EndstoneServer::getInstance();
        server.getLogger().critical("BatchedNetworkPeer::sendPacket: Failed to parse raw packet header!");
        return;
    }

    // Parse packet header
    auto header = PacketHeader::fromRaw(result.value());
    const auto &id = getId();

    // Get player object - if exists
    const auto &server = endstone::core::EndstoneServer::getInstance();
    const auto *server_player =
        server.getServer().getMinecraft()->getServerNetworkHandler()->getServerPlayer(id, header.getSenderSubId());
    endstone::Player *player = nullptr;
    if (server_player) {
        player = &server_player->getEndstoneActor<endstone::core::EndstonePlayer>();
    }

    // Create packet send event
    auto payload = stream.getView().substr(stream.getReadPointer());
    std::optional<std::string> replacement_payload;
    if (header.getPacketId() == MinecraftPacketIds::MoveDeltaActor &&
        server.isPlayerMovementBroadcastAbsoluteEnabled()) {
        replacement_payload = makeAbsolutePlayerMovement(payload, player);
        if (replacement_payload.has_value()) {
            header = PacketHeader{header.getRecipientSubId(), MinecraftPacketIds::MoveAbsoluteActor,
                                  header.getSenderSubId()};
            payload = *replacement_payload;
        }
    }
    endstone::PacketSendEvent e{player, static_cast<int>(header.getPacketId()), payload,
                                endstone::core::EndstoneSocketAddress::fromNetworkIdentifier(id),
                                static_cast<int>(header.getSenderSubId())};

    // Patch specific outbound packets (deserialize -> modify -> re-serialize)
    switch (header.getPacketId()) {
    case MinecraftPacketIds::StartGame:
    case MinecraftPacketIds::ResourcePacksInfo:
    case MinecraftPacketIds::ResourcePackStack:
    case MinecraftPacketIds::MapData: {
        auto packet = MinecraftPackets::createPacket(header.getPacketId());
        if (!packet) {
            server.getLogger().critical("BatchedNetworkPeer::sendPacket: Unknown packet id: {}",
                                        static_cast<int>(header.getPacketId()));
            return;
        }

        auto &network = server.getServer().getNetwork();
        if (!packet->readNoHeader(stream, network.getPacketReflectionCtx(), header.getSenderSubId()).ignoreError()) {
            server.getLogger().critical("BatchedNetworkPeer::sendPacket: Failed to parse packet with id: {}",
                                        static_cast<int>(packet->getId()));
            return;
        }

        patchPacket(*packet, player);

        BinaryStream out;
        packet->writeWithSerializationMode(out, network.getPacketReflectionCtx(),
                                           network.getPacketOverrides().getOverrideModeForPacket(packet->getId()));
        e.setPayload(out.getBuffer());
        break;
    }
    default:
        break;
    }

    server.getPluginManager().callEvent(e);
    if (e.isCancelled()) {
        return;
    }

    if (replacement_payload.has_value() || e.getPayload().data() != payload.data()) {
        BinaryStream out;
        header.write(out);
        out.writeRawBytes(e.getPayload());
        ENDSTONE_HOOK_CALL_ORIGINAL(&BatchedNetworkPeer::sendPacket, this, out.getBuffer(), reliability, compressible);
    }
    else {
        ENDSTONE_HOOK_CALL_ORIGINAL(&BatchedNetworkPeer::sendPacket, this, data, reliability, compressible);
    }
}

NetworkPeer::DataStatus BatchedNetworkPeer::_receivePacket(std::string &out_data,
                                                           const PacketRecvTimepointPtr &timepoint_ptr)
{
    const auto &server = endstone::core::EndstoneServer::getInstance();
    auto network_handler = server.getServer().getMinecraft()->getServerNetworkHandler();
    while (true) {
        const auto status =
            ENDSTONE_HOOK_CALL_ORIGINAL(&BatchedNetworkPeer::_receivePacket, this, out_data, timepoint_ptr);
        if (status != DataStatus::HasData) {
            return status;
        }

        ReadOnlyBinaryStream stream(out_data, false);
        auto result = stream.getUnsignedVarInt().discardError();
        if (!result) {
            return DataStatus::BrokenData;
        }

        const auto header = PacketHeader::fromRaw(result.value());
        const auto &id = getId();
        endstone::core::EndstonePlayer *player = nullptr;
        if (const auto *p = network_handler->getServerPlayer(id, header.getRecipientSubId())) {
            player = &p->getEndstoneActor<endstone::core::EndstonePlayer>();
        }

        const auto payload = stream.getView().substr(stream.getReadPointer());
        endstone::PacketReceiveEvent e{player, static_cast<int>(header.getPacketId()), payload,
                                       endstone::core::EndstoneSocketAddress::fromNetworkIdentifier(id),
                                       static_cast<int>(header.getRecipientSubId())};
        server.getPluginManager().callEvent(e);
        if (e.isCancelled()) {
            continue;
        }

        if (e.getPayload().data() == payload.data()) {
            return status;  // Nothing to do, the packet is the same, return immediately
        }

        // Plugins have changed the payload, keep header and replace the rest
        out_data.resize(stream.getReadPointer());
        out_data.append(e.getPayload().data(), e.getPayload().size());
        return status;
    }
}

const NetworkIdentifier &BatchedNetworkPeer::getId() const
{
    auto peer = peer_;
    while (peer->peer_) {
        peer = peer->peer_;
    }
    return static_cast<RakNetConnector::RakNetNetworkPeer &>(*peer).getId();
}
