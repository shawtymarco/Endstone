// Copyright (c) 2026, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#pragma once

#include <optional>

#include "bedrock/core/math/vec3.h"

namespace endstone::runtime {

[[nodiscard]] const std::optional<Vec3> &getLastExplosionPos();

}  // namespace endstone::runtime
