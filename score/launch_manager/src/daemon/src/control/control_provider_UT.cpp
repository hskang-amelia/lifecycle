/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "score/mw/launch_manager/control/control_provider.hpp"

#include <cstddef>
#include <utility>

#include <gtest/gtest.h>

#include "score/mw/com/runtime.h"
#include "score/mw/lifecycle/run_target_activation_source.hpp"
#include "score/string_manipulation/arguments/arguments.h"

namespace score::mw::lifecycle::internal
{
namespace
{

class FakeRunTargetControl final : public IRunTargetControl
{
  public:
    [[nodiscard]] score::Result<IdentifierHash> getActiveRunTarget() const noexcept override
    {
        return MakeUnexpected(ExecErrc::kActivationInProgress);
    }

    [[nodiscard]] score::Result<void> setRequestedRunTarget(IdentifierHash) noexcept override { return {}; }

    void registerActiveRunTargetCallback(ActivationCallbackT callback) noexcept override
    {
        ++register_active_run_target_callback_count_;
        callback_ = std::move(callback);
    }

    std::size_t register_active_run_target_callback_count_{0U};
    ActivationCallbackT callback_{};
};

TEST(ControlProviderUT, FailedCreateDoesNotRegisterActivationCallback)
{
    RecordProperty("Description",
                   "ControlProvider::Create must not leave an activation callback registered when setup fails.");

    FakeRunTargetControl graph{};

    const auto result = ControlProvider::Create(&graph);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(graph.register_active_run_target_callback_count_, 0U);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    score::mw::com::runtime::InitializeRuntime(
        score::string_manipulation::GetArguments(argc, const_cast<const char**>(argv)));
    return RUN_ALL_TESTS();
}
