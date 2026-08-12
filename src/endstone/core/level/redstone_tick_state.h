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

#include <algorithm>
#include <chrono>

#include "endstone/level/dimension.h"

namespace endstone::core {

class RedstoneTickState {
public:
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    void recordExecuted(std::chrono::nanoseconds duration) noexcept
    {
        ++metrics_.executed_ticks;
        metrics_.total_duration += duration;
        metrics_.last_duration = duration;
        metrics_.maximum_duration = std::max(metrics_.maximum_duration, duration);
    }

    void recordSkipped() noexcept
    {
        ++metrics_.skipped_ticks;
        metrics_.last_duration = std::chrono::nanoseconds::zero();
    }

    [[nodiscard]] Dimension::RedstoneTickMetrics snapshot() const noexcept { return metrics_; }
    void reset() noexcept { metrics_ = {}; }

private:
    bool enabled_{true};
    Dimension::RedstoneTickMetrics metrics_{};
};

}  // namespace endstone::core
