/* Copyright (c) 2014 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "TestUtil.h"       //Has to be first, compiler complains
#include "CoordinatorClusterClock.h"
#include "MockExternalStorage.h"

namespace RAMCloud {

class CoordinatorClusterClockTest : public ::testing::Test {
  public:
    Context context;
    MockExternalStorage storage;
    Tub<CoordinatorClusterClock> clock;

    CoordinatorClusterClockTest()
        : context()
        , storage(true)
        , clock()
    {
        context.externalStorage = &storage;
        clock.construct(&context);
    }

    DISALLOW_COPY_AND_ASSIGN(CoordinatorClusterClockTest);
};

TEST_F(CoordinatorClusterClockTest, constructor) {
    // Time dependent test.
    EXPECT_TRUE(clock->updater.isRunning());
    for (int i = 0; i < 1000; i++) {
        if (clock->safeClusterTimeMs >= 3000) {
            break;
        }
        usleep(1000);
    }
    EXPECT_GE(clock->safeClusterTimeMs, 3000U);
}

TEST_F(CoordinatorClusterClockTest, getTime) {
    // Time dependent test;
    // Stop updater to avoid update time dependency.
    clock->updater.stop();

    // Set large safe time so that the test will not hit it.
    clock->safeClusterTimeMs = 10000;
    EXPECT_GT(clock->getTime(), 0U);
    EXPECT_LT(clock->getTime(), 10000U);
}

TEST_F(CoordinatorClusterClockTest, getTime_stale) {
    // Stop updater to avoid update time dependency.
    clock->updater.stop();
    usleep(1000);
    EXPECT_EQ(0U, clock->safeClusterTimeMs);
    EXPECT_EQ(0U, clock->getTime());
    usleep(1000);
    EXPECT_EQ(0U, clock->getTime());
}

TEST_F(CoordinatorClusterClockTest, handleTimerEvent) {
    // Covers both the handleTimerEvent and recoverClusterTime methods.
    clock->updater.stop();
    EXPECT_FALSE(clock->updater.isRunning());
    EXPECT_EQ(0U, clock->recoverClusterTime(context.externalStorage));
    EXPECT_EQ(0U, clock->safeClusterTimeMs);
    storage.log.clear();
    clock->updater.handleTimerEvent();
    EXPECT_EQ("set(UPDATE, coordinatorClusterClock)", storage.log);
    storage.getResults.push(storage.setData);
    uint64_t storedTime = clock->recoverClusterTime(context.externalStorage);
    EXPECT_GT(storedTime, 3000U);
    EXPECT_EQ(storedTime, clock->safeClusterTimeMs);
}

TEST_F(CoordinatorClusterClockTest, getInternal) {
    CoordinatorClusterClock::Lock lock(clock->mutex);
    uint64_t firstTime = clock->getInternal(lock);
    usleep(50);
    uint64_t secondTime = clock->getInternal(lock);
    EXPECT_GT(secondTime, firstTime);
    EXPECT_GT(firstTime, clock->startingClusterTimeMs);
}

TEST_F(CoordinatorClusterClockTest, recoverClusterTime_noStoredValue) {
    // Covered by handleTimerEvent test.
    clock->updater.stop();
    EXPECT_FALSE(clock->updater.isRunning());

    storage.log.clear();
    EXPECT_EQ(0U, clock->recoverClusterTime(context.externalStorage));
    EXPECT_EQ("get(coordinatorClusterClock)", storage.log);

}

TEST_F(CoordinatorClusterClockTest, recoverClusterTime_recoveredValue) {
    // Covered by handleTimerEvent test.
    clock->updater.stop();
    EXPECT_FALSE(clock->updater.isRunning());

    clock->updater.handleTimerEvent();
    storage.getResults.push(storage.setData);

    storage.log.clear();
    uint64_t storedTime = clock->recoverClusterTime(context.externalStorage);
    EXPECT_EQ("get(coordinatorClusterClock)", storage.log);

    EXPECT_GT(storedTime, 3000U);
    EXPECT_EQ(storedTime, clock->safeClusterTimeMs);
}

}  // namespace RAMCloud
