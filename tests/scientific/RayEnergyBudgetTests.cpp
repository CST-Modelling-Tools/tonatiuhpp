#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "kernel/run/RayEnergyBudget.h"

TEST(RayEnergyBudget, AccountsEachCompletedOutcomeExactlyOnce)
{
    RayEnergyBudget budget;
    budget.launched = 4;
    budget.record(RayTerminalOutcome::Surface);
    budget.record(RayTerminalOutcome::Air);
    budget.record(RayTerminalOutcome::Escaped);
    budget.record(RayTerminalOutcome::Surface);
    EXPECT_EQ(budget.surfaceTerminated, 2U);
    EXPECT_EQ(budget.airAbsorbed, 1U);
    EXPECT_EQ(budget.escaped, 1U);
    EXPECT_TRUE(budget.matches(4));
}

TEST(RayEnergyBudget, RejectsMissingDuplicatedAndUnlaunchedOutcomes)
{
    RayEnergyBudget budget;
    budget.launched = 3;
    budget.surfaceTerminated = 2;
    EXPECT_FALSE(budget.matches(3));

    budget.escaped = 1;
    EXPECT_TRUE(budget.matches(3));

    budget.airAbsorbed = 1;
    EXPECT_FALSE(budget.matches(3));

    budget.airAbsorbed = 0;
    EXPECT_FALSE(budget.matches(4));
}

TEST(RayEnergyBudget, RejectsOverflowInsteadOfWrappingTerminalSum)
{
    RayEnergyBudget budget;
    budget.launched = std::numeric_limits<std::uint64_t>::max();
    budget.surfaceTerminated = budget.launched;
    budget.airAbsorbed = 1;
    EXPECT_FALSE(budget.matches(budget.launched));
}
