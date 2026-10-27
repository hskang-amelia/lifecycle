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

#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

#include "score/mw/launch_manager/alive_monitor/details/common/TimeSortingBuffer.hpp"

using namespace std::chrono_literals;

using score::mw::lifecycle::internal::saf::common::TimeSortingBuffer;

namespace
{

class TimeSortingBufferTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "boundary-values");
    }

    /// Drains the buffer and returns the elements in reported order.
    static std::vector<std::uint32_t> drain(TimeSortingBuffer<std::uint32_t>& buffer)
    {
        std::vector<std::uint32_t> result;
        for (std::uint32_t* element = buffer.getNextElement(); element != nullptr; element = buffer.getNextElement())
        {
            result.push_back(*element);
        }
        return result;
    }
};

// --------------------------------------------------------------------------
// push / sort
// --------------------------------------------------------------------------

TEST_F(TimeSortingBufferTest, Push_SingleElement_IsRetrievable)
{
    RecordProperty("Description", "This test verifies that a single pushed element is reported back.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U}));
}

TEST_F(TimeSortingBufferTest, Push_AscendingTimestamps_ReportsInInsertionOrder)
{
    RecordProperty("Description", "This test verifies that elements pushed oldest-first are reported oldest-first.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_TRUE(buffer.push(2U, 200ns));
    ASSERT_TRUE(buffer.push(3U, 300ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U, 2U, 3U}));
}

TEST_F(TimeSortingBufferTest, Push_DescendingTimestamps_ReportsChronologically)
{
    RecordProperty("Description", "This test verifies that elements pushed newest-first are reported oldest-first.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(3U, 300ns));
    ASSERT_TRUE(buffer.push(2U, 200ns));
    ASSERT_TRUE(buffer.push(1U, 100ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U, 2U, 3U}));
}

TEST_F(TimeSortingBufferTest, Push_MiddleTimestamp_IsSortedBetweenNeighbors)
{
    RecordProperty("Description", "This test verifies that an element is sorted between older and newer elements.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_TRUE(buffer.push(3U, 300ns));
    ASSERT_TRUE(buffer.push(2U, 200ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U, 2U, 3U}));
}

TEST_F(TimeSortingBufferTest, Push_EqualTimestamps_KeepsInsertionOrder)
{
    RecordProperty("Description", "This test verifies that elements with equal timestamps keep insertion order.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_TRUE(buffer.push(2U, 100ns));
    ASSERT_TRUE(buffer.push(3U, 100ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U, 2U, 3U}));
}

TEST_F(TimeSortingBufferTest, Push_FullBuffer_ReturnsFalse)
{
    RecordProperty("Description", "This test verifies that pushing to a full buffer fails and keeps the contents.");
    TimeSortingBuffer<std::uint32_t> buffer{2U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_TRUE(buffer.push(2U, 200ns));
    EXPECT_FALSE(buffer.push(3U, 300ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{1U, 2U}));
}

TEST_F(TimeSortingBufferTest, Push_ZeroCapacity_ReturnsFalse)
{
    RecordProperty("Description", "This test verifies that pushing to a zero-capacity buffer fails.");
    TimeSortingBuffer<std::uint32_t> buffer{0U};
    EXPECT_FALSE(buffer.push(1U, 100ns));
    EXPECT_EQ(buffer.getNextElement(), nullptr);
}

// --------------------------------------------------------------------------
// getNextElement
// --------------------------------------------------------------------------

TEST_F(TimeSortingBufferTest, GetNextElement_EmptyBuffer_ReturnsNull)
{
    RecordProperty("Description", "This test verifies that an empty buffer reports no element.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    EXPECT_EQ(buffer.getNextElement(), nullptr);
}

TEST_F(TimeSortingBufferTest, GetNextElement_PastLastElement_ReturnsNull)
{
    RecordProperty("Description", "This test verifies that iteration past the newest element reports no element.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_NE(buffer.getNextElement(), nullptr);
    EXPECT_EQ(buffer.getNextElement(), nullptr);
    EXPECT_EQ(buffer.getNextElement(), nullptr);
}

// --------------------------------------------------------------------------
// clear
// --------------------------------------------------------------------------

TEST_F(TimeSortingBufferTest, Clear_FilledBuffer_RestartsEmpty)
{
    RecordProperty("Description", "This test verifies that clear resets contents and iteration state.");
    TimeSortingBuffer<std::uint32_t> buffer{2U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_NE(buffer.getNextElement(), nullptr);

    buffer.clear();

    EXPECT_EQ(buffer.getNextElement(), nullptr);
    ASSERT_TRUE(buffer.push(2U, 200ns));
    EXPECT_EQ(drain(buffer), (std::vector<std::uint32_t>{2U}));
}

// --------------------------------------------------------------------------
// move construction / assignment
// --------------------------------------------------------------------------

TEST_F(TimeSortingBufferTest, MoveConstruct_FilledBuffer_KeepsElements)
{
    RecordProperty("Description", "This test verifies that a moved-to buffer reports the moved elements.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));
    ASSERT_TRUE(buffer.push(2U, 200ns));

    TimeSortingBuffer<std::uint32_t> movedTo{std::move(buffer)};
    EXPECT_EQ(drain(movedTo), (std::vector<std::uint32_t>{1U, 2U}));
}

TEST_F(TimeSortingBufferTest, MoveAssign_FilledBuffer_KeepsElements)
{
    RecordProperty("Description", "This test verifies that a move-assigned buffer reports the moved elements.");
    TimeSortingBuffer<std::uint32_t> buffer{4U};
    ASSERT_TRUE(buffer.push(1U, 100ns));

    TimeSortingBuffer<std::uint32_t> movedTo{1U};
    movedTo = std::move(buffer);
    EXPECT_EQ(drain(movedTo), (std::vector<std::uint32_t>{1U}));
}

}  // namespace
