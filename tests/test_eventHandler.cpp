// tests/eventHandler.test.cpp
#include <gtest/gtest.h>

#include "unleash/EventHandler/eventHandler.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace {

struct Waiter {
    std::mutex m;
    std::condition_variable cv;
    bool done = false;

    void signal() {
        {
            std::lock_guard<std::mutex> lk(m);
            done = true;
        }
        cv.notify_all();
    }

    bool waitFor(std::chrono::milliseconds timeout = 500ms) {
        std::unique_lock<std::mutex> lk(m);
        return cv.wait_for(lk, timeout, [&]{ return done; });
    }
};

} // namespace

TEST(EventHandler, DoesNothingWhenNotStarted)
{
    unleash::EventHandler eh;

    std::atomic<int> called{0};
    eh.onSent([&] { called.fetch_add(1, std::memory_order_relaxed); });

    // Not started -> emit should do nothing
    eh.emitSent();

    // Give a little time just in case (should still not call)
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(called.load(std::memory_order_relaxed), 0);
}

TEST(EventHandler, EmitsSentCallbackWhenStarted)
{
    unleash::EventHandler eh;
    eh.start();

    Waiter w;
    std::atomic<int> called{0};

    eh.onSent([&] {
        called.fetch_add(1, std::memory_order_relaxed);
        w.signal();
    });

    eh.emitSent();

    ASSERT_TRUE(w.waitFor(500ms)) << "Sent callback was not invoked in time";
    EXPECT_EQ(called.load(std::memory_order_relaxed), 1);

    eh.stop();
}

TEST(EventHandler, EmitsErrorWithPayload)
{
    unleash::EventHandler eh;
    eh.start();

    Waiter w;
    std::string msg;
    std::string details;

    eh.onError([&](const unleash::EventHandler::ClientError& err) {
        msg = err.message;
        details = err.details;
        w.signal();
    });

    unleash::EventHandler::ClientError err{"network error", "timeout"};
    eh.emitError(err);

    ASSERT_TRUE(w.waitFor(500ms)) << "Error callback was not invoked in time";
    EXPECT_EQ(msg, "network error");
    EXPECT_EQ(details, "timeout");

    eh.stop();
}

TEST(EventHandler, EmitsImpressionWithPayload)
{
    unleash::EventHandler eh;
    eh.start();

    Waiter w;
    std::string flag;
    bool enabled = false;

    eh.onImpression([&](const std::string& flagName, bool isEnabled) {
        flag = flagName;
        enabled = isEnabled;
        w.signal();
    });

    eh.emitImpression("myFlag", true);

    ASSERT_TRUE(w.waitFor(500ms)) << "Impression callback was not invoked in time";
    EXPECT_EQ(flag, "myFlag");
    EXPECT_TRUE(enabled);

    eh.stop();
}

TEST(EventHandler, ClearAllDisablesCallbacks)
{
    unleash::EventHandler eh;
    eh.start();

    std::atomic<int> called{0};
    eh.onReady([&] { called.fetch_add(1, std::memory_order_relaxed); });

    // First time should work
    {
        Waiter w;
        eh.onReady([&] { called.fetch_add(1, std::memory_order_relaxed); w.signal(); });
        eh.emitReady();
        ASSERT_TRUE(w.waitFor(500ms)) << "Ready callback was not invoked in time";
    }

    // Clear and try again: must not call
    eh.clearAll();
    eh.emitReady();

    std::this_thread::sleep_for(100ms);
    // called == 1 from the first emission (exactly once)
    EXPECT_EQ(called.load(std::memory_order_relaxed), 1);

    eh.stop();
}

TEST(EventHandler, StopIsCleanAndIdempotent)
{
    unleash::EventHandler eh;

    // stop without start should be safe
    eh.stop();

    eh.start();
    eh.stop();

    // stop again should be safe
    eh.stop();

    SUCCEED();
}

TEST(EventHandler, MultipleEmitsAreProcessed)
{
    unleash::EventHandler eh;
    eh.start();

    std::mutex m;
    std::condition_variable cv;
    int count = 0;

    eh.onUpdate([&] {
        std::lock_guard<std::mutex> lk(m);
        ++count;
        cv.notify_all();
    });

    // Emit several updates
    for (int i = 0; i < 5; ++i) {
        eh.emitUpdate();
    }

    // Wait until we see all of them (or timeout)
    {
        std::unique_lock<std::mutex> lk(m);
        ASSERT_TRUE(cv.wait_for(lk, 1s, [&]{ return count == 5; }))
            << "Expected 5 update callbacks, got " << count;
    }

    eh.stop();
}

TEST(EventHandler, CallbackExceptionDoesNotKillThread)
{
    unleash::EventHandler eh;
    eh.start();

    // Set a callback that throws
    eh.onInit([&] {
        throw std::runtime_error("boom");
    });

    // And a second callback we can observe afterwards
    Waiter w;
    eh.onSent([&] { w.signal(); });

    // Emit init (will throw inside worker thread, should be caught)
    eh.emitInit();

    // Then emit sent; if worker thread survived, this must arrive
    eh.emitSent();

    ASSERT_TRUE(w.waitFor(1s)) << "Worker thread likely died after exception";

    eh.stop();
}
