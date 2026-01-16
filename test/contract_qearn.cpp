#define NO_UEFI

#include <random>
#include <map>

#include "contract_testing.h"

#define PRINT_TEST_INFO 0

// test config:
// - 0 is fastest
// - 1 to enable more tests with random lock/unlock
// - 2 to enable even more tests with random lock/unlock
// - 3 to also check values more often (expensive functions)
// - 4 to also test out-of-user error
#define LARGE_SCALE_TEST 0


static const id QEARN_CONTRACT_ID(QEARN_CONTRACT_INDEX, 0, 0, 0);

static std::mt19937_64 rand64;

static id getUser(unsigned long long i);
static unsigned long long random(unsigned long long maxValue);

static std::vector<uint64> fullyUnlockedAmount;
static std::vector<id> fullyUnlockedUser;


class QearnChecker : public QEARN
{
public:
    void checkLockerArray(bool beforeEndEpoch, bool printInfo = false)
    {
        // check that locker array is in consistent state
        std::map<int, unsigned long long> epochTotalLocked;
        uint32 minEpoch = 0xffff;
        uint32 maxEpoch = 0;
        for (uint64 idx = 0; idx < locker.capacity(); ++idx)
        {
            const QEARN::LockInfo& lock = locker.get(idx);
            if (lock._lockedAmount == 0)
            {
                EXPECT_TRUE(isZero(lock.ID));
                EXPECT_EQ(lock._lockedEpoch, 0);
            }
            else
            {
                EXPECT_GT(lock._lockedAmount, QEARN_MINIMUM_LOCKING_AMOUNT);
                EXPECT_LE(lock._lockedAmount, QEARN_MAX_LOCK_AMOUNT);
                EXPECT_FALSE(isZero(lock.ID));
                const QEARN::EpochIndexInfo& epochRange = _epochIndex.get(lock._lockedEpoch);
                EXPECT_GE(idx, epochRange.startIndex);
                EXPECT_LT(idx, epochRange.endIndex);
                epochTotalLocked[lock._lockedEpoch] += lock._lockedAmount;

                minEpoch = std::min(minEpoch, lock._lockedEpoch);
                maxEpoch = std::max(minEpoch, lock._lockedEpoch);
            }
        }

        const uint32 beginEpoch = std::max((int)contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch, system.epoch - 52);
        EXPECT_LE(beginEpoch, minEpoch);
        EXPECT_LE(maxEpoch, uint32(system.epoch));

        if (PRINT_TEST_INFO)
        {
            const char * beforeAfterStr = (beforeEndEpoch) ? "Before" : "After";
            std::cout << "--- " << beforeAfterStr << " END_EPOCH in epoch " << system.epoch << std::endl;
        }

        for (uint32 epoch = beginEpoch; epoch <= system.epoch; ++epoch)
        {
            const QEARN::RoundInfo& currentRoundInfo = _currentRoundInfo.get(epoch);
            //if (!currentRoundInfo._Epoch_Bonus_Amount && !currentRoundInfo._Total_Locked_Amount)
            //    continue;
            unsigned long long totalLocked = epochTotalLocked[epoch];
            if (printInfo)
            {
                std::cout << "Total locked amount in epoch " << epoch << " = " << totalLocked << ", total bonus " << currentRoundInfo._epochBonusAmount << std::endl;
            }
            if (beforeEndEpoch || epoch != system.epoch - 52)
                EXPECT_EQ(currentRoundInfo._totalLockedAmount, totalLocked);
        }

        // check that old epoch indices have been reset
        for (uint32 epoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch; epoch < beginEpoch; ++epoch)
        {
            EXPECT_EQ(this->_epochIndex.get(epoch).startIndex, this->_epochIndex.get(epoch).endIndex);
        }
    }

    void checkGetUnlockedInfo(uint32 epoch) 
    {
        fullyUnlockedAmount.clear();
        fullyUnlockedUser.clear();

        const QEARN::EpochIndexInfo& epochIndex = _epochIndex.get(epoch);
        for(uint64 idx = epochIndex.startIndex; idx < epochIndex.endIndex; ++idx)
        {
            if(locker.get(idx)._lockedAmount != 0)
            {
                fullyUnlockedAmount.push_back(locker.get(idx)._lockedAmount);
                fullyUnlockedUser.push_back(locker.get(idx).ID);
            }
        }
    }

    void checkFullyUnlockedAmount()
    {
        for(uint32 idx = 0; idx < _fullyUnlockedCnt; idx++)
        {
            const QEARN::HistoryInfo& FullyUnlockedInfo = fullyUnlocker.get(idx);

            EXPECT_EQ(fullyUnlockedAmount[idx], FullyUnlockedInfo._unlockedAmount);
            EXPECT_EQ(fullyUnlockedUser[idx], FullyUnlockedInfo._unlockedID);
        }
    }

    void checkStatsPerEpoch(getBurnedAndBoostedStatsPerEpoch_output result, uint16 epoch)
    {
        EXPECT_EQ(result.boostedAmount, statsInfo.get(epoch).boostedAmount);
        EXPECT_EQ(result.burnedAmount, statsInfo.get(epoch).burnedAmount);
        EXPECT_EQ(result.rewardedAmount, statsInfo.get(epoch).rewardedAmount);
        EXPECT_EQ(result.boostedPercent, div(result.boostedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount));
        EXPECT_EQ(result.burnedPercent, div(result.burnedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount));
        EXPECT_EQ(result.rewardedPercent, div(result.rewardedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount));
    }

    void checkStatsForAll(getBurnedAndBoostedStats_output result)
    {
        uint64 totalBurnedAmountInSC = 0;
        uint64 totalBoostedAmountInSC = 0;
        uint64 totalRewardedAmountInSC = 0;
        uint64 sumBurnedPercent = 0;
        uint64 sumBoostedPercent = 0;
        uint64 sumRewardedPercent = 0;

        for(uint32 epoch = 138 ; epoch < system.epoch; epoch++)
        {
            totalBurnedAmountInSC += statsInfo.get(epoch).burnedAmount;
            totalBoostedAmountInSC += statsInfo.get(epoch).boostedAmount;
            totalRewardedAmountInSC += statsInfo.get(epoch).rewardedAmount;

            sumBurnedPercent += div(statsInfo.get(epoch).burnedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount);
            sumBoostedPercent += div(statsInfo.get(epoch).boostedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount);
            sumRewardedPercent += div(statsInfo.get(epoch).rewardedAmount * 10000000, _initialRoundInfo.get(epoch)._epochBonusAmount);
        }

        EXPECT_EQ(result.boostedAmount, totalBoostedAmountInSC);
        EXPECT_EQ(result.burnedAmount, totalBurnedAmountInSC);
        EXPECT_EQ(result.rewardedAmount, totalRewardedAmountInSC);
        EXPECT_EQ(result.averageBoostedPercent, div(sumBoostedPercent, system.epoch - 138ULL));
        EXPECT_EQ(result.averageBurnedPercent, div(sumBurnedPercent, system.epoch - 138ULL));
        EXPECT_EQ(result.averageRewardedPercent, div(sumRewardedPercent, system.epoch - 138ULL));
    }

    QEARN::EpochIndexInfo getEpochIndex(uint32 epoch) const
    {
        return _epochIndex.get(epoch);
    }
};

class ContractTestingQearn : protected ContractTesting
{
    struct UnlockTableEntry
    {
        unsigned long long rewardPercent;
        unsigned long long burnPercent;
    };
    std::vector<UnlockTableEntry> epochChangesToUnlockParams;

public:
    ContractTestingQearn()
    {
        INIT_CONTRACT(QEARN);
        initEmptySpectrum();
        rand64.seed(42);

        for (unsigned int epChanges = 0; epChanges <= 52; ++epChanges)
        {
            if (epChanges <= 4)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 0, 0 });
            else if (epChanges <= 12)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 5, 45 });
            else if (epChanges <= 16)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 10, 45 });
            else if (epChanges <= 20)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 15, 40 });
            else if (epChanges <= 24)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 20, 40 });
            else if (epChanges <= 28)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 25, 35 });
            else if (epChanges <= 32)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 30, 35 });
            else if (epChanges <= 36)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 35, 35 });
            else if (epChanges <= 40)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 40, 30 });
            else if (epChanges <= 44)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 45, 30 });
            else if (epChanges <= 48)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 50, 30 });
            else if (epChanges <= 52)
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 55, 25 });
            else
                epochChangesToUnlockParams.push_back(UnlockTableEntry{ 100, 0 });
        }
    }

    QearnChecker* getState()
    {
        return (QearnChecker*)contractStates[QEARN_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QEARN_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);

        // If there is no entry for this epoch in allEpochData, create one with default init (all 0)
        allEpochData[system.epoch];
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QEARN_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    QEARN::getLockInfoPerEpoch_output getLockInfoPerEpoch(uint16 epoch) const
    {
        QEARN::getLockInfoPerEpoch_input input{ epoch };
        QEARN::getLockInfoPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    uint64 getUserLockedInfo(uint16 epoch, const id& user) const
    {
        QEARN::getUserLockedInfo_input input;
        input.epoch = epoch;
        input.user = user;
        QEARN::getUserLockedInfo_output output;
        callFunction(QEARN_CONTRACT_INDEX, 2, input, output);
        return output.lockedAmount;
    }

    uint32 getStateOfRound(uint16 epoch) const
    {
        QEARN::getStateOfRound_input input{ epoch };
        QEARN::getStateOfRound_output output;
        callFunction(QEARN_CONTRACT_INDEX, 3, input, output);
        return output.state;
    }

    uint64 getUserLockStatus(const id& user) const
    {
        QEARN::getUserLockStatus_input input{ user };
        QEARN::getUserLockStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 4, input, output);
        return output.status;
    }

    QEARN::getEndedStatus_output getEndedStatus(const id& user) const
    {
        QEARN::getEndedStatus_input input{ user };
        QEARN::getEndedStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QEARN::getStatsPerEpoch_output getStatsPerEpoch(uint16 epoch) const
    {
        QEARN::getStatsPerEpoch_input input{ epoch };
        QEARN::getStatsPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QEARN::getBurnedAndBoostedStats_output getBurnedAndBoostedStats() const
    {
        QEARN::getBurnedAndBoostedStats_input input;
        QEARN::getBurnedAndBoostedStats_output output;
        callFunction(QEARN_CONTRACT_INDEX, 7, input, output);
        return output;
    }

    QEARN::getBurnedAndBoostedStatsPerEpoch_output getBurnedAndBoostedStatsPerEpoch(uint16 epoch) const
    {
        QEARN::getBurnedAndBoostedStatsPerEpoch_input input{ epoch };
        QEARN::getBurnedAndBoostedStatsPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 8, input, output);
        return output;
    }

    sint32 lock(const id& user, long long amount, bool expectSuccess = true)
    {
        QEARN::lock_input input;
        QEARN::lock_output output;
        EXPECT_EQ(invokeUserProcedure(QEARN_CONTRACT_INDEX, 1, input, output, user, amount), expectSuccess);
        return output.returnCode;
    }

    sint32 unlock(const id& user, long long amount, uint16 lockedEpoch, bool expectSuccess = true)
    {
        QEARN::unlock_input input;
        input.amount = amount;
        input.lockedEpoch = lockedEpoch;
        QEARN::unlock_output output;
        EXPECT_EQ(invokeUserProcedure(QEARN_CONTRACT_INDEX, 2, input, output, user, 0), expectSuccess);
        return output.returnCode;
    }

    struct UserData
    {
        std::map<uint16, sint64> locked;
    };

    std::map<id, UserData> allUserData;

    struct EpochData
    {
        unsigned long long initialBonusAmount;
        unsigned long long initialTotalLockedAmount;
        unsigned long long bonusAmount;
        unsigned long long amountCurrentlyLocked;
    };

    std::map<uint16, EpochData> allEpochData;

    std::map<id, unsigned long long> amountUnlockPerUser;

    void simulateDonation(const unsigned long long donationAmount)
    {
        increaseEnergy(QEARN_CONTRACT_ID, donationAmount);

        unsigned long long& totalBonusAmount = allEpochData[system.epoch + 1].bonusAmount;
        totalBonusAmount += donationAmount;
        if (totalBonusAmount > QEARN_MAX_BONUS_AMOUNT)
            totalBonusAmount = QEARN_MAX_BONUS_AMOUNT;
    }

    bool lockAndCheck(const id& user, uint64 amountLock, bool expectSuccess = true)
    {
        // check consistency of epoch info expected vs returned by contract
        checkEpochInfo(system.epoch);

        // get amount and balances before action
#if LARGE_SCALE_TEST >= 3
        uint64 amountBefore = getUserLockedInfo(system.epoch, user);
        EXPECT_EQ(allUserData[user].locked[system.epoch], amountBefore);
#else
        uint64 amountBefore = allUserData[user].locked[system.epoch];
#endif
        sint64 userBalanceBefore = getBalance(user);
        sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);

        // call lock prcoedure
        uint32 retCode = lock(user, amountLock, expectSuccess);

        // check new amount and balances
        uint64 amountAfter = getUserLockedInfo(system.epoch, user);
        sint64 userBalanceAfter = getBalance(user);
        sint64 contractBalanceAfter = getBalance(QEARN_CONTRACT_ID);
        if (retCode == QEARN_LOCK_SUCCESS && expectSuccess)
        {
            EXPECT_EQ(amountAfter, amountBefore + amountLock);
            EXPECT_EQ(userBalanceAfter, userBalanceBefore - amountLock);
            EXPECT_EQ(contractBalanceAfter, contractBalanceBefore + amountLock);

            allUserData[user].locked[system.epoch] += amountLock;
            allEpochData[system.epoch].amountCurrentlyLocked += amountLock;
            allEpochData[system.epoch].initialTotalLockedAmount += amountLock;
        }
        else
        {
            EXPECT_EQ(amountAfter, amountBefore);
            EXPECT_EQ(userBalanceAfter, userBalanceBefore);
            EXPECT_EQ(contractBalanceAfter, contractBalanceBefore);
        }

        if (!expectSuccess)
            return false;

        // check return code
        if (retCode != QEARN_OVERFLOW_USER)
        {
            if (amountLock < QEARN_MINIMUM_LOCKING_AMOUNT || system.epoch < QEARN_INITIAL_EPOCH)
            {
                EXPECT_EQ(retCode, QEARN_INVALID_INPUT_AMOUNT);
            }
            else if (amountBefore + amountLock > QEARN_MAX_LOCK_AMOUNT)
            {
                EXPECT_EQ(retCode, QEARN_LIMIT_LOCK);
            }
        }

        return retCode == QEARN_LOCK_SUCCESS;
    }

    unsigned long long getAndCheckRewardFactorTenmillionth(uint16 epoch) const
    {
        auto edIt = allEpochData.find(epoch);
        EXPECT_NE(edIt, allEpochData.end());
        const EpochData& ed = edIt->second;
        const unsigned long long rewardFactorTenmillionth = QPI::div(ed.bonusAmount * 10000000ULL, ed.amountCurrentlyLocked);
        if (rewardFactorTenmillionth)
        {
            // detect overflow in computation of rewardFactorTenmillionth
            const double rewardFactorTenmillionthDouble = ed.bonusAmount * 10000000.0 / ed.amountCurrentlyLocked;
            double arthmeticError = double(rewardFactorTenmillionth) - rewardFactorTenmillionthDouble;
            EXPECT_LT(fabs(arthmeticError), 1e5);
        }

        return rewardFactorTenmillionth;
    }

    void checkEpochInfo(uint16 epoch)
    {
        const auto scEpochInfo = getLockInfoPerEpoch(epoch);
        EXPECT_LE(scEpochInfo.currentBonusAmount, QEARN_MAX_BONUS_AMOUNT);
        if (epoch < QEARN_INITIAL_EPOCH)
            return;
        auto edIt = allEpochData.find(epoch);
        EXPECT_NE(edIt, allEpochData.end());
        const EpochData& ed = edIt->second;
        EXPECT_EQ(getAndCheckRewardFactorTenmillionth(epoch), scEpochInfo.yield);
        EXPECT_EQ(ed.bonusAmount, scEpochInfo.currentBonusAmount);
        EXPECT_EQ(ed.amountCurrentlyLocked, scEpochInfo.currentLockedAmount);

        const auto scStatsInfo = getStatsPerEpoch(epoch);
        
        EXPECT_EQ(scStatsInfo.earlyUnlockedAmount, ed.initialTotalLockedAmount - ed.amountCurrentlyLocked);
        EXPECT_EQ(scStatsInfo.earlyUnlockedPercent, QPI::div((ed.initialTotalLockedAmount - ed.amountCurrentlyLocked) * 10000, ed.initialTotalLockedAmount));

        const auto scBurnedAndBoostedStatsPerEpoch = getBurnedAndBoostedStatsPerEpoch(epoch);
        const auto scBurnedAndBoostedStatsForAllEpoch = getBurnedAndBoostedStats();

        getState()->checkStatsPerEpoch(scBurnedAndBoostedStatsPerEpoch, epoch);
        getState()->checkStatsForAll(scBurnedAndBoostedStatsForAllEpoch);

        uint64 averageAPY = 0;
        uint32 cnt = 0;
        for(uint16 t = system.epoch - 1; t >= system.epoch - 52; t--)
        {
            auto preEdIt = allEpochData.find(t);
            const EpochData& preED = preEdIt->second;
            if (t < QEARN_INITIAL_EPOCH)
            {
                break;
            }
            if(preED.amountCurrentlyLocked == 0)
            {
                continue;
            }

            cnt++;
            EXPECT_EQ(getLockInfoPerEpoch(t).currentLockedAmount, preED.amountCurrentlyLocked);
            averageAPY += QPI::div(preED.bonusAmount * 10000000ULL, preED.amountCurrentlyLocked);
        }
        EXPECT_EQ(scStatsInfo.totalLockedAmount, getBalance(QEARN_CONTRACT_ID));
        EXPECT_EQ(scStatsInfo.averageAPY, QPI::div(averageAPY, cnt * 1ULL));
    }

    bool unlockAndCheck(const id& user, uint16 lockingEpoch, uint64 amountUnlock, bool expectSuccess = true)
    {
        // make sure that user exists in spectrum
        increaseEnergy(user, 1);

        // get old locked amount
#if LARGE_SCALE_TEST >= 3
        uint64 amountBefore = getUserLockedInfo(lockingEpoch, user);
        EXPECT_EQ(allUserData[user].locked[lockingEpoch], amountBefore);
#else
        uint64 amountBefore = allUserData[user].locked[lockingEpoch];
#endif
        sint64 userBalanceBefore = getBalance(user);
        sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);

        // call unlock prcoedure
        uint32 retCode = unlock(user, amountUnlock, lockingEpoch);

        // check new locked amount and balances
        uint64 amountAfter = getUserLockedInfo(lockingEpoch, user);
        sint64 userBalanceAfter = getBalance(user);
        sint64 contractBalanceAfter = getBalance(QEARN_CONTRACT_ID);
        if (retCode == QEARN_UNLOCK_SUCCESS && expectSuccess)
        {
            EXPECT_GE(amountBefore, amountUnlock);
            uint64 expectedAmountAfter = amountBefore - amountUnlock;
            if (expectedAmountAfter < QEARN_MINIMUM_LOCKING_AMOUNT)
            {
                expectedAmountAfter = 0;
            }
            EXPECT_EQ(amountAfter, expectedAmountAfter);
            uint64 amountUnlocked = amountBefore - amountAfter;

            uint16 epochTransitions = system.epoch - lockingEpoch;
            unsigned long long rewardFactorTenmillionth = getAndCheckRewardFactorTenmillionth(lockingEpoch);
            unsigned long long commonFactor = QPI::div(rewardFactorTenmillionth * amountUnlocked, 100ULL);
            unsigned long long amountReward = QPI::div(commonFactor * epochChangesToUnlockParams[epochTransitions].rewardPercent, 10000000ULL);
            unsigned long long amountBurn = QPI::div(commonFactor * epochChangesToUnlockParams[epochTransitions].burnPercent, 10000000ULL);
            {
                // Check for overflows
                double commonFactorError = fabs((double(rewardFactorTenmillionth) * double(amountUnlocked) / 100.0) - commonFactor);
                EXPECT_LT(commonFactorError, 1e3);
                double amountRewardError = fabs((double(commonFactor) * double(epochChangesToUnlockParams[epochTransitions].rewardPercent) / 10000000.0) - amountReward);
                EXPECT_LE(amountRewardError, 1);
                double amountBurnError = fabs((double(commonFactor) * double(epochChangesToUnlockParams[epochTransitions].burnPercent) / 10000000.0) - amountBurn);
                EXPECT_LE(amountBurnError, 1);
            }

            allUserData[user].locked[lockingEpoch] -= amountUnlocked;
            if(system.epoch == lockingEpoch)
            {
                allEpochData[lockingEpoch].initialTotalLockedAmount -= amountUnlocked;
            }
            allEpochData[lockingEpoch].amountCurrentlyLocked -= amountUnlocked;
            allEpochData[lockingEpoch].bonusAmount -= amountReward + amountBurn;

            // Edge case of unlocking of all locked funds in previous epoch -> bonus added to next round
            if (lockingEpoch != system.epoch && !allEpochData[lockingEpoch].amountCurrentlyLocked)
            {
                amountBurn += allEpochData[lockingEpoch].bonusAmount;
                allEpochData[lockingEpoch].bonusAmount = 0;
            }

            EXPECT_EQ(userBalanceAfter, userBalanceBefore + amountUnlocked + amountReward);
            EXPECT_EQ(contractBalanceAfter, contractBalanceBefore - amountUnlocked - amountReward - amountBurn);

            // Check consistency of epoch info expected vs returned by contract
            checkEpochInfo(lockingEpoch);

            // getEndedStatus() only included Early_Unlocked_Amount if unlocked after locking epoch
            if (lockingEpoch != system.epoch)
            {
                amountUnlockPerUser[user] += amountUnlocked;
            }
        }
        else
        {
            EXPECT_EQ(amountAfter, amountBefore);
            EXPECT_EQ(userBalanceAfter, userBalanceBefore);
            EXPECT_EQ(contractBalanceAfter, contractBalanceBefore);
        }

        return retCode == QEARN_UNLOCK_SUCCESS;
    }

    void endEpochAndCheck()
    {
        // check getStateOfRound
        uint16 payoutEpoch = system.epoch - 52;
        EXPECT_EQ(getStateOfRound(QEARN_INITIAL_EPOCH - 1), 2);
        EXPECT_EQ(getStateOfRound(payoutEpoch - 1), 2);
        EXPECT_EQ(getStateOfRound(payoutEpoch), (payoutEpoch >= QEARN_INITIAL_EPOCH) ? 1 : 2);
        EXPECT_EQ(getStateOfRound(system.epoch - 1), (system.epoch - 1 >= QEARN_INITIAL_EPOCH) ? 1 : 2);
        EXPECT_EQ(getStateOfRound(system.epoch), (system.epoch >= QEARN_INITIAL_EPOCH) ? 1 : 2);
        EXPECT_EQ(getStateOfRound(system.epoch + 1), (system.epoch + 1 >= QEARN_INITIAL_EPOCH) ? 0 : 2);

        // test getUserLockStatus()
        {
            id user = getUser(random(10));
            uint64 lockStatus = getUserLockStatus(user);
            const auto userDataIt = allUserData.find(user);
            if (userDataIt == allUserData.end())
            {
                EXPECT_EQ(lockStatus, 0);
            }
            else
            {
                auto& userData = userDataIt->second;
                for (int i = 0; i <= 52; ++i)
                {
                    if (lockStatus & 1)
                    {
                        EXPECT_GT(userData.locked[system.epoch - i], 0ll);
                    }
                    else
                    {
                        EXPECT_EQ(userData.locked[system.epoch - i], 0ll);
                    }
                    lockStatus >>= 1;
                }
            }
        }

        // check unlocked amounts returned by getEndedStatus()
        for (const auto& userAmountPairs : amountUnlockPerUser)
        {
            QEARN::getEndedStatus_output endedStatus = getEndedStatus(userAmountPairs.first);
            EXPECT_EQ(userAmountPairs.second, endedStatus.earlyUnlockedAmount);
        }

        checkEpochInfo(system.epoch);

        bool beforeEndEpoch = true;
        getState()->checkLockerArray(beforeEndEpoch, PRINT_TEST_INFO);
        getState()->checkGetUnlockedInfo(payoutEpoch);

        // get entity balances to check payout in END_EPOCH
        std::map<id, long long> oldUserBalance;
        long long oldContractBalance = getBalance(QEARN_CONTRACT_ID);
        for (const auto& userIdDataPair : allUserData)
        {
            const id& user = userIdDataPair.first;
            oldUserBalance[user] = getBalance(user);
        }
        checkEpochInfo(payoutEpoch);

        amountUnlockPerUser.clear();
        endEpoch();

        // check payout after END_EPOCH
        bool expectPayout = (allEpochData.find(payoutEpoch) != allEpochData.end());
        checkEpochInfo(payoutEpoch);
        if (expectPayout)
        {
            // compute and check expected payouts
            unsigned long long rewardFactorTenmillionth = getAndCheckRewardFactorTenmillionth(payoutEpoch);
            unsigned long long totalRewardsPaid = 0;
            const EpochData& ed = allEpochData[payoutEpoch];

            for (const auto& userIdBalancePair : oldUserBalance)
            {
                const id& user = userIdBalancePair.first;
                const long long oldUserBalance = userIdBalancePair.second;
                const UserData& userData = allUserData[user];

                auto userLockedAmountIter = userData.locked.find(payoutEpoch);
                if (userLockedAmountIter == userData.locked.end() || userLockedAmountIter->second == 0)
                    continue;
                const long long userLockedAmount = userLockedAmountIter->second;
                const unsigned long long userReward = userLockedAmount * rewardFactorTenmillionth / 10000000ULL;
                if (rewardFactorTenmillionth)
                    EXPECT_EQ((userLockedAmount * rewardFactorTenmillionth) / rewardFactorTenmillionth, userLockedAmount);
                totalRewardsPaid += userReward;

                EXPECT_EQ(oldUserBalance + userLockedAmount + userReward, getBalance(user));
            }
            EXPECT_EQ(oldContractBalance - ed.bonusAmount - ed.amountCurrentlyLocked, getBalance(QEARN_CONTRACT_ID));


            // all the bonus that has not been paid is burned (remainder due to inaccurate arithmetic and full bonus if nothing is locked until the end)
            EXPECT_LE(totalRewardsPaid, ed.bonusAmount);
            if (ed.amountCurrentlyLocked && ed.bonusAmount)
                EXPECT_GE(QPI::div(totalRewardsPaid * 1000, ed.bonusAmount), 998); // only small part of bonus should be burned
            else
                EXPECT_EQ(totalRewardsPaid, 0ull);
        }
        else
        {
            // no payout expected
            for (const auto& userIdBalancePair : oldUserBalance)
            {
                const id& user = userIdBalancePair.first;
                const long long oldUserBalance = userIdBalancePair.second;
                const long long currentUserBalance = getBalance(user);
                EXPECT_EQ(oldUserBalance, currentUserBalance);
            }
            EXPECT_EQ(oldContractBalance, getBalance(QEARN_CONTRACT_ID));
        }

        beforeEndEpoch = false;
        getState()->checkLockerArray(beforeEndEpoch, PRINT_TEST_INFO);
        getState()->checkFullyUnlockedAmount();
    }
};


static id getUser(unsigned long long i)
{
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

static unsigned long long random(unsigned long long maxValue)
{
    return rand64() % (maxValue + 1);
}

static std::vector<id> getRandomUsers(unsigned int totalUsers, unsigned int maxNum)
{
    unsigned long long userCount = random(maxNum);
    std::vector<id> users;
    users.reserve(userCount);
    for (unsigned int i = 0; i < userCount; ++i)
    {
        unsigned long long userIdx = random(totalUsers - 1);
        users.push_back(getUser(userIdx));
    }
    return users;
}


TEST(TestContractQearn, ErrorChecking)
{
    ContractTestingQearn qearn;
    id user(1, 2, 3, 4);

    system.epoch = QEARN_INITIAL_EPOCH - 1;

    qearn.beginEpoch();

    // special test case: trying to lock/unlock before QEARN_INITIAL_EPOCH must fail
    {
        id user2(98765, 43, 2, 1);
        increaseEnergy(user2, QEARN_MAX_LOCK_AMOUNT);
        EXPECT_FALSE(qearn.lockAndCheck(user2, QEARN_MAX_LOCK_AMOUNT));
        EXPECT_EQ(qearn.unlock(user2, QEARN_MAX_LOCK_AMOUNT, system.epoch), QEARN_INVALID_INPUT_LOCKED_EPOCH);
    }

    qearn.endEpoch();

    system.epoch = QEARN_INITIAL_EPOCH;

    qearn.beginEpoch();

    // test cases, for which procedures is not executed:
    {
        // 1. non-existing entities = invalid ID)
        EXPECT_FALSE(qearn.lockAndCheck(id::zero(), QEARN_MAX_LOCK_AMOUNT, false));
        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MAX_LOCK_AMOUNT, false));

        // 2. valid ID but negative amount / insufficient balance
        increaseEnergy(user, 1);
        EXPECT_FALSE(qearn.lockAndCheck(user, -10, false));
        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MINIMUM_LOCKING_AMOUNT, false));
    }

    // test cases, for which procedure is executed (valid ID, enough balance)
    increaseEnergy(user, QEARN_MAX_LOCK_AMOUNT * 1000);
    {
        EXPECT_FALSE(qearn.lockAndCheck(user, 0));
        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MINIMUM_LOCKING_AMOUNT / 2));
        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MINIMUM_LOCKING_AMOUNT - 1));

        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MAX_LOCK_AMOUNT + 1));
        EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MAX_LOCK_AMOUNT * 2));
    }

    // in order trigger out-of-lock-slots error, lock with many users
#if LARGE_SCALE_TEST >= 4
    // notes: - disabled by default because it takes long
    //        - seems like the last locker slot is never used in QEARN (FIXME)
    for (uint64 i = 0; i < QEARN_MAX_LOCKS - 1; ++i)
    {
        id otherUser(i, 42, 1234, 642);
        long long amount = QEARN_MINIMUM_LOCKING_AMOUNT + (7 * i) % (QEARN_MAX_LOCK_AMOUNT - QEARN_MINIMUM_LOCKING_AMOUNT);
        increaseEnergy(otherUser, amount);
        EXPECT_TRUE(qearn.lockAndCheck(otherUser, amount));
    }
    EXPECT_FALSE(qearn.lockAndCheck(user, QEARN_MINIMUM_LOCKING_AMOUNT));
#endif

    // note: lock implements no checking of system.epoch

    // for unlock, successfully lock some funds
    id otherUser(1, 42, 1234, 642);
    long long amount = QEARN_MINIMUM_LOCKING_AMOUNT;
    increaseEnergy(otherUser, amount);
    EXPECT_TRUE(qearn.lockAndCheck(otherUser, amount));

    // unlock with too high amount
    EXPECT_EQ(qearn.unlock(otherUser, QEARN_MAX_LOCK_AMOUNT + 1, system.epoch), QEARN_INVALID_INPUT_UNLOCK_AMOUNT);

    // unlock with too low amount
    EXPECT_EQ(qearn.unlock(otherUser, QEARN_MINIMUM_LOCKING_AMOUNT - 1, system.epoch), QEARN_INVALID_INPUT_AMOUNT);

    // unlock with wrong user
    EXPECT_EQ(qearn.unlock(user, QEARN_MINIMUM_LOCKING_AMOUNT, system.epoch), QEARN_EMPTY_LOCKED);

    // unlock with wrong epoch
    EXPECT_EQ(qearn.unlock(otherUser, QEARN_MINIMUM_LOCKING_AMOUNT, 1), QEARN_INVALID_INPUT_LOCKED_EPOCH);
    EXPECT_EQ(qearn.unlock(otherUser, QEARN_MINIMUM_LOCKING_AMOUNT, QEARN_MAX_EPOCHS + 1), QEARN_INVALID_INPUT_LOCKED_EPOCH);

    // finally, test success case
    EXPECT_EQ(qearn.unlock(otherUser, QEARN_MINIMUM_LOCKING_AMOUNT, system.epoch), QEARN_UNLOCK_SUCCESS);
}

// Test case for gap removal logic in overflow check (lines 635-656 in Qearn.h)
// This test verifies that when the locker array is near capacity and contains gaps,
// attempting to lock triggers gap removal, allowing the lock to succeed.
// Note: This test is disabled by default because it requires filling many slots (QEARN_MAX_LOCKS - 1)
// Enable with LARGE_SCALE_TEST >= 4 to run this comprehensive test

#if LARGE_SCALE_TEST >= 4
TEST(TestContractQearn, GapRemovalOnOverflow)
{
    std::cout << "gap removal test. If you want to test this case as soon, please set the QEARN_MAX_LOCKS to a smaller value on the contract." << std::endl;
    ContractTestingQearn qearn;
    
    system.epoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    qearn.beginEpoch();
    qearn.endEpoch();

    system.epoch = QEARN_INITIAL_EPOCH;

    qearn.beginEpoch();
    
    // Create a scenario where we fill up the locker array and create gaps
    // Strategy: Fill up to near capacity, unlock some to create gaps, 
    // then try to lock again which triggers gap removal
    
    const uint64 numGapsToCreate = 100;  // Create some gaps by unlocking
    // Fill up to QEARN_MAX_LOCKS - 1 so that after unlocking (which doesn't change endIndex),
    // the next lock attempt will trigger the overflow check (endIndex >= QEARN_MAX_LOCKS - 1)
    const uint64 targetEndIndex = QEARN_MAX_LOCKS - 1;
    
    std::vector<id> usersToUnlock;
    usersToUnlock.reserve(numGapsToCreate);
    
    // Step 1: Fill up the array to near capacity
    // We'll fill up to targetEndIndex, then unlock some to create gaps
    // The endIndex will stay high, so when we try to lock again, it will trigger overflow check
    for (uint64 i = 0; i < targetEndIndex; ++i)
    {
        id testUser(i, 100, 200, 300);
        uint64 amount = QEARN_MINIMUM_LOCKING_AMOUNT + 1;
        increaseEnergy(testUser, amount);
        EXPECT_TRUE(qearn.lockAndCheck(testUser, amount));
        
        // Store some users to unlock later (to create gaps)
        if (i < numGapsToCreate)
        {
            usersToUnlock.push_back(testUser);
        }
    }
    
    // Step 2: Verify we're near capacity
    QearnChecker* state = qearn.getState();
    uint32 endIndexBeforeUnlock = state->getEpochIndex(system.epoch).endIndex;
    EXPECT_GE(endIndexBeforeUnlock, targetEndIndex);
    
    // Step 3: Unlock some users to create gaps in the locker array
    // Note: endIndex doesn't decrease when unlocking, so gaps are created but endIndex stays high
    for (const auto& userToUnlock : usersToUnlock)
    {
        uint64 unlockAmount = QEARN_MINIMUM_LOCKING_AMOUNT + 1;
        EXPECT_EQ(qearn.unlock(userToUnlock, unlockAmount, system.epoch), QEARN_UNLOCK_SUCCESS);
    }
    
    // Step 4: Verify endIndex is still high (gaps created but not removed yet)
    uint32 endIndexAfterUnlock = state->getEpochIndex(system.epoch).endIndex;
    EXPECT_EQ(endIndexAfterUnlock, endIndexBeforeUnlock);  // endIndex doesn't change on unlock
    
    // Step 5: Try to lock one more user - this should trigger overflow check and gap removal
    // After gap removal, the lock should succeed because we created gaps earlier
    id finalUser(targetEndIndex + 1, 100, 200, 300);
    uint64 finalAmount = QEARN_MINIMUM_LOCKING_AMOUNT + 1;
    increaseEnergy(finalUser, finalAmount);
    
    // The lock should succeed after gap removal
    sint32 retCode = qearn.lock(finalUser, finalAmount);
    
    // Verify that gap removal happened and lock succeeded
    // After gap removal, endIndex should be less than QEARN_MAX_LOCKS - 1
    uint32 endIndexAfterGapRemoval = state->getEpochIndex(system.epoch).endIndex;
    
    // The lock should succeed because gaps were removed
    EXPECT_EQ(retCode, QEARN_LOCK_SUCCESS);
    EXPECT_EQ(endIndexAfterGapRemoval, QEARN_MAX_LOCKS - numGapsToCreate);
    EXPECT_LT(endIndexAfterGapRemoval, QEARN_MAX_LOCKS - 1);
    EXPECT_LT(endIndexAfterGapRemoval, endIndexAfterUnlock);  // endIndex should decrease after gap removal
    
    // Verify the locker array is consistent after gap removal
    qearn.getState()->checkLockerArray(true, false);
    
    // Verify the final user's lock was successful
    EXPECT_EQ(qearn.getUserLockedInfo(system.epoch, finalUser), finalAmount);
    
    qearn.endEpoch();
}
#endif

void testRandomLockWithoutUnlock(const uint16 numEpochs, const unsigned int totalUsers, const unsigned int maxUserLocking)
{
    std::cout << "random test without early unlock for " << numEpochs << " epochs with " << totalUsers << " total users and up to " << maxUserLocking << " lock calls per epoch" << std::endl;
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    const uint16 lastEpoch = firstEpoch + numEpochs;

    // first epoch is without donation/bonus
    for (system.epoch = firstEpoch; system.epoch <= lastEpoch; ++system.epoch)
    {
        // invoke BEGIN_EPOCH
        qearn.beginEpoch();

        // simulate a random additional donation during the epoch
        qearn.simulateDonation(random(ISSUANCE_RATE / 2));

        // locking
        auto lockUsers = getRandomUsers(totalUsers, maxUserLocking);
        for (const auto& user : lockUsers)
        {
            // get random amount for locking and make sure that user has enough qus (may be invalid amount for locking)
            uint64 amountLock = random(QEARN_MAX_LOCK_AMOUNT * 4 / 3);
            increaseEnergy(user, amountLock);

            qearn.lockAndCheck(user, amountLock);
        }

        // invoke END_EPOCH and check correct payouts
        qearn.endEpochAndCheck();

        // send revenue donation to qearn contract (happens after END_EPOCH but before system.epoch is incremented and before BEGIN_EPOCH
        qearn.simulateDonation(random(ISSUANCE_RATE));
    }
}

TEST(TestContractQearn, RandomLockWithoutUnlock)
{
    // params: epochs, total number of users, max users locking in epoch
    testRandomLockWithoutUnlock(100, 40, 10);
    testRandomLockWithoutUnlock(100, 20, 20);
#if LARGE_SCALE_TEST >= 1
    testRandomLockWithoutUnlock(300, 1000, 1000);
#endif
#if LARGE_SCALE_TEST >= 2
    testRandomLockWithoutUnlock(100, 20000, 10000);
#endif
}

void testRandomLockWithUnlock(const uint16 numEpochs, const unsigned int totalUsers, const unsigned int maxUserLocking, const unsigned int maxUserUnlocking)
{
    std::cout << "random test with early unlock for " << numEpochs << " epochs with " << totalUsers << " total users, up to " << maxUserLocking << " lock calls (per epoch), and up to " << maxUserUnlocking << " unlock calls (per running round)" << std::endl;
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    const uint16 lastEpoch = firstEpoch + numEpochs;

    for (system.epoch = firstEpoch; system.epoch <= lastEpoch; ++system.epoch)
    {
        // invoke BEGIN_EPOCH
        qearn.beginEpoch();

        // simulate a random additional donation during the epoch
        qearn.simulateDonation(random(ISSUANCE_RATE / 2));

        // locking
        auto lockUsers = getRandomUsers(totalUsers, maxUserLocking);
        for (const auto& user : lockUsers)
        {
            // get random amount for locking and make sure that user has enough qus (may be invalid amount for locking)
            uint64 amountLock = random(QEARN_MAX_LOCK_AMOUNT * 4 / 3);
            increaseEnergy(user, amountLock);

            qearn.lockAndCheck(user, amountLock);
        }

        // unlocking
        auto unlockUsers = getRandomUsers(totalUsers, maxUserUnlocking);
        for (const auto& user : unlockUsers)
        {
            for (sint32 lockedEpoch = system.epoch; lockedEpoch >= system.epoch - 52; lockedEpoch--)
            {
                uint64 amountUnlock = random(qearn.allUserData[user].locked[lockedEpoch] * 11 / 10);
                qearn.unlockAndCheck(user, lockedEpoch, amountUnlock);
            }
        }

        // invoke END_EPOCH and check correct payouts
        qearn.endEpochAndCheck();

        // send revenue donation to qearn contract (happens after END_EPOCH but before system.epoch is incremented and before BEGIN_EPOCH
        qearn.simulateDonation(random(ISSUANCE_RATE));
    }
}

TEST(TestContractQearn, RandomLockAndUnlock)
{
    // params: epochs, total number of users, max users locking in epoch, maxUserUnlocking
    testRandomLockWithUnlock(100, 40, 10, 10);
    testRandomLockWithUnlock(100, 40, 10, 8);   // less early unlocking
    testRandomLockWithUnlock(100, 40, 20, 19);  // more user activity
#if LARGE_SCALE_TEST >= 1
    testRandomLockWithUnlock(300, 1000, 1000, 1000);
    testRandomLockWithUnlock(300, 1000, 1000, 800);
#endif
#if LARGE_SCALE_TEST >= 2
    testRandomLockWithUnlock(400, 2000, 1500, 1200);
    testRandomLockWithUnlock(100, 20000, 10000, 8000);
#endif
}
