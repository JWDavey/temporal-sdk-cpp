#include <gtest/gtest.h>

#include <coroutine>
#include <stdexcept>
#include <string>
#include <vector>

#include "temporalio/async_/coroutine_scheduler.h"
#include "temporalio/async_/task.h"

using namespace temporalio::async_;

// ===========================================================================
// Basic scheduler tests
// ===========================================================================

TEST(CoroutineSchedulerTest, InitiallyEmpty) {
    CoroutineScheduler scheduler;
    EXPECT_TRUE(scheduler.empty());
    EXPECT_EQ(scheduler.size(), 0u);
}

TEST(CoroutineSchedulerTest, DrainEmptyReturnsFalse) {
    CoroutineScheduler scheduler;
    EXPECT_FALSE(scheduler.drain());
}

TEST(CoroutineSchedulerTest, ScheduleIncrementsSize) {
    CoroutineScheduler scheduler;

    auto task = []() -> Task<void> { co_return; }();
    scheduler.schedule(task.handle());

    EXPECT_FALSE(scheduler.empty());
    EXPECT_EQ(scheduler.size(), 1u);
}

TEST(CoroutineSchedulerTest, DrainRunsScheduledCoroutine) {
    CoroutineScheduler scheduler;

    bool executed = false;
    auto task = [&]() -> Task<void> {
        executed = true;
        co_return;
    }();

    // Task is lazy, suspended at initial_suspend
    EXPECT_FALSE(executed);

    scheduler.schedule(task.handle());
    bool ran = scheduler.drain();

    EXPECT_TRUE(ran);
    EXPECT_TRUE(executed);
    EXPECT_TRUE(scheduler.empty());
}

TEST(CoroutineSchedulerTest, DrainRunsMultipleCoroutines) {
    CoroutineScheduler scheduler;

    std::vector<int> order;

    auto task1 = [&]() -> Task<void> {
        order.push_back(1);
        co_return;
    }();

    auto task2 = [&]() -> Task<void> {
        order.push_back(2);
        co_return;
    }();

    auto task3 = [&]() -> Task<void> {
        order.push_back(3);
        co_return;
    }();

    scheduler.schedule(task1.handle());
    scheduler.schedule(task2.handle());
    scheduler.schedule(task3.handle());

    EXPECT_EQ(scheduler.size(), 3u);

    bool ran = scheduler.drain();
    EXPECT_TRUE(ran);
    EXPECT_TRUE(scheduler.empty());

    // Should execute in FIFO order (deque, front-popping)
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// ===========================================================================
// Coroutines that schedule more work during drain
// ===========================================================================

TEST(CoroutineSchedulerTest, CoroutineCanScheduleMoreWork) {
    CoroutineScheduler scheduler;

    std::vector<std::string> log;

    // This task, when resumed, will schedule another task.
    // We need a custom awaiter that enqueues to the scheduler.
    struct ScheduleAwaiter {
        CoroutineScheduler& sched;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) {
            // Re-enqueue ourselves
            sched.schedule(h);
        }
        void await_resume() noexcept {}
    };

    // A coroutine that yields once via the scheduler
    auto task = [&](CoroutineScheduler& sched) -> Task<void> {
        log.push_back("step1");
        co_await ScheduleAwaiter{sched};
        log.push_back("step2");
    }(scheduler);

    scheduler.schedule(task.handle());

    // First drain: executes step1, which suspends and re-enqueues,
    // then the scheduler picks up the re-enqueued handle and executes step2.
    bool ran = scheduler.drain();
    EXPECT_TRUE(ran);
    EXPECT_TRUE(scheduler.empty());

    ASSERT_EQ(log.size(), 2u);
    EXPECT_EQ(log[0], "step1");
    EXPECT_EQ(log[1], "step2");
}

// ===========================================================================
// Drain after drain
// ===========================================================================

TEST(CoroutineSchedulerTest, SecondDrainReturnsFalseWhenEmpty) {
    CoroutineScheduler scheduler;

    auto task = []() -> Task<void> { co_return; }();
    scheduler.schedule(task.handle());

    EXPECT_TRUE(scheduler.drain());
    EXPECT_FALSE(scheduler.drain());  // Nothing left
}

// ===========================================================================
// Size tracking
// ===========================================================================

TEST(CoroutineSchedulerTest, SizeDecreasesAfterDrain) {
    CoroutineScheduler scheduler;

    auto t1 = []() -> Task<void> { co_return; }();
    auto t2 = []() -> Task<void> { co_return; }();

    scheduler.schedule(t1.handle());
    scheduler.schedule(t2.handle());
    EXPECT_EQ(scheduler.size(), 2u);

    scheduler.drain();
    EXPECT_EQ(scheduler.size(), 0u);
}

// ===========================================================================
// Non-copyable, non-movable
// ===========================================================================

TEST(CoroutineSchedulerTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<CoroutineScheduler>);
    EXPECT_FALSE(std::is_copy_assignable_v<CoroutineScheduler>);
}

TEST(CoroutineSchedulerTest, IsNonMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<CoroutineScheduler>);
    EXPECT_FALSE(std::is_move_assignable_v<CoroutineScheduler>);
}

// ===========================================================================
// Ordering (FIFO)
// ===========================================================================

TEST(CoroutineSchedulerTest, FIFOOrdering) {
    CoroutineScheduler scheduler;

    constexpr int N = 10;
    std::vector<int> execution_order;
    std::vector<Task<void>> tasks;

    for (int i = 0; i < N; ++i) {
        tasks.push_back([&execution_order, i]() -> Task<void> {
            execution_order.push_back(i);
            co_return;
        }());
    }

    for (auto& t : tasks) {
        scheduler.schedule(t.handle());
    }

    scheduler.drain();

    ASSERT_EQ(execution_order.size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(execution_order[static_cast<size_t>(i)], i);
    }
}

// ===========================================================================
// Interleaving: multiple coroutines yielding back to scheduler
// ===========================================================================

TEST(CoroutineSchedulerTest, InterleavedExecution) {
    CoroutineScheduler scheduler;

    std::vector<std::string> log;

    struct YieldAwaiter {
        CoroutineScheduler& sched;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) {
            sched.schedule(h);
        }
        void await_resume() noexcept {}
    };

    auto task_a = [&](CoroutineScheduler& sched) -> Task<void> {
        log.push_back("A1");
        co_await YieldAwaiter{sched};
        log.push_back("A2");
        co_await YieldAwaiter{sched};
        log.push_back("A3");
    }(scheduler);

    auto task_b = [&](CoroutineScheduler& sched) -> Task<void> {
        log.push_back("B1");
        co_await YieldAwaiter{sched};
        log.push_back("B2");
    }(scheduler);

    scheduler.schedule(task_a.handle());
    scheduler.schedule(task_b.handle());

    scheduler.drain();

    // Expected FIFO interleaving:
    // Drain picks A (first scheduled): runs A1, A suspends -> re-enqueues A
    // Drain picks B (next in queue): runs B1, B suspends -> re-enqueues B
    // Drain picks A (re-enqueued): runs A2, A suspends -> re-enqueues A
    // Drain picks B (re-enqueued): runs B2, B completes
    // Drain picks A (re-enqueued): runs A3, A completes
    ASSERT_EQ(log.size(), 5u);
    EXPECT_EQ(log[0], "A1");
    EXPECT_EQ(log[1], "B1");
    EXPECT_EQ(log[2], "A2");
    EXPECT_EQ(log[3], "B2");
    EXPECT_EQ(log[4], "A3");
}
