#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>
#include <memory>

#include "unleash/Store/flagStore.hpp"
#include "unleash/Domain/toggleSet.hpp"

using namespace unleash;

TEST(FlagStore, StartsNotReady)
{
    FlagStore store;

    EXPECT_FALSE(store.isReady());
}

TEST(FlagStore, HasNonNullSnapshotOnConstruction)
{
    FlagStore store;

    auto snap = store.snapshot();
    ASSERT_NE(snap, nullptr);
}

TEST(FlagStore, ReplaceSetsReadyAndSwapsSnapshotAtomically)
{
    FlagStore store;

    auto before = store.snapshot();
    ASSERT_NE(before, nullptr);
    EXPECT_FALSE(store.isReady());

    auto newSnap = std::make_shared<const ToggleSet>(); 
    store.replace(newSnap);

    EXPECT_TRUE(store.isReady());

    auto after = store.snapshot();
    ASSERT_NE(after, nullptr);

    EXPECT_EQ(after.get(), newSnap.get());

    EXPECT_NE(after.get(), before.get());
}

TEST(FlagStore, ReplaceIgnoresNullSnapshotAndDoesNotChangeState)
{
    FlagStore store;

    auto before = store.snapshot();
    const bool readyBefore = store.isReady();

    store.replace(nullptr);

    auto after = store.snapshot();
    EXPECT_EQ(after.get(), before.get());
    EXPECT_EQ(store.isReady(), readyBefore);
}

TEST(FlagStore, ConcurrentReadersDoNotCrashDuringReplace)
{
    FlagStore store;

    std::atomic_bool stop{false};
    std::atomic_int reads{0};

    std::vector<std::thread> threads;
    constexpr int kReaders = 8;
    threads.reserve(kReaders);

    for (int i = 0; i < kReaders; ++i) {
        threads.emplace_back([&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                auto s = store.snapshot();
                if (s) {
                    reads.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (int i = 0; i < 2000; ++i) {
        store.replace(std::make_shared<const ToggleSet>());
    }

    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();

    EXPECT_TRUE(store.isReady());
    EXPECT_GT(reads.load(std::memory_order_relaxed), 0);
}