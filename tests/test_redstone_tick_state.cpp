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

#include <chrono>

#include <gtest/gtest.h>

#include "endstone/core/level/redstone_tick_state.h"

using namespace std::chrono_literals;

TEST(RedstoneTickStateTest, DefaultsToEnabledAndAccumulatesPerDimensionMetrics)
{
    endstone::core::RedstoneTickState state;

    EXPECT_TRUE(state.isEnabled());
    state.recordExecuted(12ns);
    state.recordExecuted(7ns);
    state.recordSkipped();

    const auto metrics = state.snapshot();
    EXPECT_EQ(metrics.executed_ticks, 2);
    EXPECT_EQ(metrics.skipped_ticks, 1);
    EXPECT_EQ(metrics.total_duration, 19ns);
    EXPECT_EQ(metrics.last_duration, 0ns);
    EXPECT_EQ(metrics.maximum_duration, 12ns);
}

TEST(RedstoneTickStateTest, SupportsRuntimeDisableAndMetricReset)
{
    endstone::core::RedstoneTickState state;
    state.setEnabled(false);
    state.recordSkipped();

    EXPECT_FALSE(state.isEnabled());
    EXPECT_EQ(state.snapshot().skipped_ticks, 1);

    state.reset();
    const auto metrics = state.snapshot();
    EXPECT_EQ(metrics.executed_ticks, 0);
    EXPECT_EQ(metrics.skipped_ticks, 0);
    EXPECT_EQ(metrics.total_duration, 0ns);
    EXPECT_EQ(metrics.maximum_duration, 0ns);
    EXPECT_FALSE(state.isEnabled());
}
