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
#include <gtest/gtest.h>

#include <ctime>
#include <memory>
#include <string>

#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_handle.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"

using namespace testing;

using score::mw::lifecycle::IdentifierHash;
using score::mw::lifecycle::SupervisionBufferType;
using score::mw::lifecycle::SupervisionEvent;
using score::mw::lifecycle::SupervisionEventType;
using score::mw::lifecycle::SupervisionHandle;

namespace
{

class SupervisionHandleTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        buffer_ = std::make_shared<SupervisionBufferType>();
        buffer_->initialize();
    }

    /// Dequeues the single queued event and checks the buffer is drained.
    SupervisionEvent takeSingleEvent()
    {
        SupervisionEvent event{};
        EXPECT_TRUE(buffer_->tryDequeue(event));
        EXPECT_TRUE(buffer_->empty());
        return event;
    }

    const IdentifierHash process_id_{std::string_view{"test_process"}};
    std::shared_ptr<SupervisionBufferType> buffer_;
};

TEST_F(SupervisionHandleTest, ActivateSupervision_SpaceInBuffer_QueuesActivationEvent)
{
    RecordProperty("Description", "This test verifies that an activation request is queued with id and timestamp.");
    SupervisionHandle handle{process_id_, buffer_};

    EXPECT_TRUE(handle.activateSupervision({12, 34}));

    const SupervisionEvent event{takeSingleEvent()};
    EXPECT_EQ(event.id, process_id_);
    EXPECT_EQ(event.eventType, SupervisionEventType::kActivation);
    EXPECT_EQ(event.systemClockTimestamp.tv_sec, 12);
    EXPECT_EQ(event.systemClockTimestamp.tv_nsec, 34);
}

TEST_F(SupervisionHandleTest, DeactivateSupervision_SpaceInBuffer_QueuesDeactivationEvent)
{
    RecordProperty("Description", "This test verifies that a deactivation request is queued with id and timestamp.");
    SupervisionHandle handle{process_id_, buffer_};

    EXPECT_TRUE(handle.deactivateSupervision({56, 78}));

    const SupervisionEvent event{takeSingleEvent()};
    EXPECT_EQ(event.id, process_id_);
    EXPECT_EQ(event.eventType, SupervisionEventType::kDeactivation);
    EXPECT_EQ(event.systemClockTimestamp.tv_sec, 56);
    EXPECT_EQ(event.systemClockTimestamp.tv_nsec, 78);
}

TEST_F(SupervisionHandleTest, ActivateSupervision_FullBuffer_ReturnsFalse)
{
    RecordProperty("Description", "This test verifies that requests fail once the supervision buffer is full.");
    SupervisionHandle handle{process_id_, buffer_};

    while (handle.activateSupervision({0, 0}))
    {
    }

    EXPECT_FALSE(handle.activateSupervision({0, 0}));
    EXPECT_FALSE(handle.deactivateSupervision({0, 0}));
}

TEST_F(SupervisionHandleTest, GetConnectionId_ReturnsAliveInterfacePath)
{
    RecordProperty("Description", "This test verifies that the connection id is the component's IPC path.");
    SupervisionHandle handle{process_id_, buffer_};

    EXPECT_EQ(handle.getConnectionId(), score::mw::lifecycle::internal::aliveInterfacePath(process_id_));
}

}  // namespace
