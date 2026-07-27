#define NO_UEFI

#include <algorithm>
#include <cstddef>
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
static const id CCF_CONTRACT_ID(CCF_CONTRACT_INDEX, 0, 0, 0);

static std::mt19937_64 rand64;

static id getUser(unsigned long long i);
static unsigned long long random(unsigned long long maxValue);

static std::vector<uint64> fullyUnlockedAmount;
static std::vector<id> fullyUnlockedUser;

struct QearnV1StateLayout
{
    QPI::Array<QEARN::RoundInfo, QEARN_MAX_EPOCHS> initialRoundInfo;
    QPI::Array<QEARN::RoundInfo, QEARN_MAX_EPOCHS> currentRoundInfo;
    QPI::Array<QEARN::EpochIndexInfo, QEARN_MAX_EPOCHS> epochIndex;
    QPI::Array<QEARN::LockInfo, QEARN_MAX_LOCKS> locker;
    QPI::Array<QEARN::HistoryInfo, QEARN_MAX_USERS> earlyUnlocker;
    QPI::Array<QEARN::HistoryInfo, QEARN_MAX_USERS> fullyUnlocker;
    uint32 earlyUnlockedCnt;
    uint32 fullyUnlockedCnt;
    QPI::Array<QEARN::StatsInfo, QEARN_MAX_EPOCHS> statsInfo;
};

static_assert(
    sizeof(QearnV1StateLayout) == offsetof(QEARN::StateData, lockerLockPeriods),
    "QEarn V2 PADDING must preserve the exact V1 state prefix");
static_assert(
    sizeof(QearnV1StateLayout) == 214171656ULL,
    "QEarn V1 deployed state size changed; PADDING is no longer safe");
static_assert(
    sizeof(QEARN::StateData) == 244645904ULL,
    "QEarn V2 state layout changed; review the epoch-227 deployment plan");

static_assert(sizeof(QEARN::lockV2_input) == 4);
static_assert(sizeof(QEARN::lockV2_output) == 4);
static_assert(sizeof(QEARN::unlockV2_input) == 8);
static_assert(sizeof(QEARN::unlockV2_output) == 4);
static_assert(sizeof(QEARN::getV2LockInfoPerEpoch_input) == 8);
static_assert(sizeof(QEARN::getV2LockInfoPerEpoch_output) == 128);
static_assert(offsetof(QEARN::getV2LockInfoPerEpoch_output, finalized) == 112);
static_assert(offsetof(QEARN::getV2LockInfoPerEpoch_output, state) == 116);
static_assert(offsetof(QEARN::getV2LockInfoPerEpoch_output, returnCode) == 120);
static_assert(sizeof(QEARN::getV2UserLockedInfo_input) == 40);
static_assert(offsetof(QEARN::getV2UserLockedInfo_input, epoch) == 32);
static_assert(offsetof(QEARN::getV2UserLockedInfo_input, lockPeriod) == 36);
static_assert(sizeof(QEARN::getV2UserLockedInfo_output) == 24);
static_assert(offsetof(QEARN::getV2UserLockedInfo_output, maturityEpoch) == 8);
static_assert(offsetof(QEARN::getV2UserLockedInfo_output, returnCode) == 16);


class QearnChecker : public QEARN, public QEARN::StateData
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

static constexpr uint16 QEARN_RECOVERED_EPOCH = 217;
static constexpr unsigned long long QEARN_RECOVERED_EPOCH_BONUS = 50227542196ULL;

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

    QEARN::getV2LockInfoPerEpoch_output getV2LockInfoPerEpoch(uint32 epoch, uint32 lockPeriod) const
    {
        QEARN::getV2LockInfoPerEpoch_input input;
        input.epoch = epoch;
        input.lockPeriod = lockPeriod;
        QEARN::getV2LockInfoPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 9, input, output);
        return output;
    }

    QEARN::getV2UserLockedInfo_output getV2UserLockedInfo(uint32 epoch, const id& user, uint32 lockPeriod) const
    {
        QEARN::getV2UserLockedInfo_input input;
        input.user = user;
        input.epoch = epoch;
        input.lockPeriod = lockPeriod;
        QEARN::getV2UserLockedInfo_output output;
        callFunction(QEARN_CONTRACT_INDEX, 10, input, output);
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

    sint32 lockV2(const id& user, long long amount, uint32 lockPeriod, bool expectSuccess = true)
    {
        QEARN::lockV2_input input;
        input.lockPeriod = lockPeriod;
        QEARN::lockV2_output output;
        EXPECT_EQ(invokeUserProcedure(QEARN_CONTRACT_INDEX, 3, input, output, user, amount), expectSuccess);
        return output.returnCode;
    }

    sint32 unlockV2(const id& user, uint32 lockedEpoch, uint32 lockPeriod, bool expectSuccess = true)
    {
        QEARN::unlockV2_input input;
        input.lockedEpoch = lockedEpoch;
        input.lockPeriod = lockPeriod;
        QEARN::unlockV2_output output;
        EXPECT_EQ(invokeUserProcedure(QEARN_CONTRACT_INDEX, 4, input, output, user, 0), expectSuccess);
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
        unsigned long long& totalBonusAmount = allEpochData[system.epoch + 1].bonusAmount;

        unsigned long long amount = donationAmount;
        // Epoch 217's bonus is overwritten by Qearn.h's, fund it with
        // exactly that value (clamp the accumulated donation)
        if (system.epoch + 1 == QEARN_RECOVERED_EPOCH)
        {
            amount = (totalBonusAmount >= QEARN_RECOVERED_EPOCH_BONUS) ? 0ULL : (QEARN_RECOVERED_EPOCH_BONUS - totalBonusAmount);
        }

        increaseEnergy(QEARN_CONTRACT_ID, amount);

        totalBonusAmount += amount;
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

TEST(TestContractQearn, V2ActivationAndLegacyWrappersRemainCompatible)
{
    ContractTestingQearn qearn;
    const uint32 legacyEpoch = QEARN_V2_ACTIVATION_EPOCH - 1;
    const uint32 v2Epoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 principal = 100000000ULL;
    const id legacyUser(1001, 1, 1, 1);
    const id inactiveV2User(1002, 2, 2, 2);
    const id wrapperUser(1003, 3, 3, 3);
    const id shortTermUser(1004, 4, 4, 4);

    system.epoch = legacyEpoch;
    qearn.beginEpoch();

    increaseEnergy(legacyUser, principal);
    EXPECT_EQ(qearn.lock(legacyUser, principal), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.getUserLockedInfo(legacyEpoch, legacyUser), principal);

    increaseEnergy(inactiveV2User, principal);
    const sint64 inactiveBalance = getBalance(inactiveV2User);
    EXPECT_EQ(qearn.lockV2(inactiveV2User, principal, QEARN_V2_LOCK_PERIOD_13), QEARN_V2_NOT_ACTIVE);
    EXPECT_EQ(getBalance(inactiveV2User), inactiveBalance);
    EXPECT_EQ(qearn.getV2LockInfoPerEpoch(legacyEpoch, QEARN_V2_LOCK_PERIOD_13).returnCode,
        QEARN_INVALID_INPUT_LOCKED_EPOCH);
    EXPECT_EQ(qearn.getV2UserLockedInfo(legacyEpoch, inactiveV2User, QEARN_V2_LOCK_PERIOD_13).returnCode,
        QEARN_INVALID_INPUT_LOCKED_EPOCH);
    qearn.endEpoch();

    system.epoch = v2Epoch;
    qearn.beginEpoch();

    increaseEnergy(wrapperUser, principal);
    increaseEnergy(shortTermUser, principal);
    EXPECT_EQ(qearn.lock(wrapperUser, principal), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(shortTermUser, principal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);

    // The V1 lock procedure remains a compatibility wrapper for a V2 52-epoch lock.
    EXPECT_EQ(qearn.getUserLockedInfo(v2Epoch, wrapperUser), principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, wrapperUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount, principal);
    EXPECT_EQ(qearn.getUserLockedInfo(v2Epoch, shortTermUser), 0ULL);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, shortTermUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount, principal);

    // Neither the new procedure nor the legacy 52-epoch wrapper may unlock in the lock epoch.
    EXPECT_EQ(qearn.unlockV2(shortTermUser, v2Epoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_INVALID_INPUT_LOCKED_EPOCH);
    EXPECT_EQ(qearn.unlock(wrapperUser, QEARN_MINIMUM_LOCKING_AMOUNT, v2Epoch),
        QEARN_INVALID_INPUT_LOCKED_EPOCH);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, shortTermUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount, principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, wrapperUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount, principal);

    // A pre-activation position continues to use the original V1 unlock path after activation.
    const sint64 legacyBalance = getBalance(legacyUser);
    EXPECT_EQ(qearn.unlock(legacyUser, principal, legacyEpoch), QEARN_UNLOCK_SUCCESS);
    EXPECT_EQ(getBalance(legacyUser), legacyBalance + principal);
    EXPECT_EQ(qearn.getUserLockedInfo(legacyEpoch, legacyUser), 0ULL);
    qearn.endEpoch();

    system.epoch = v2Epoch + 1;
    qearn.beginEpoch();

    // The legacy wrapper requires an exact full-position amount, preventing an
    // old partial-unlock UI from silently closing the complete V2 position.
    const sint64 wrapperBalance = getBalance(wrapperUser);
    EXPECT_EQ(qearn.unlock(wrapperUser, QEARN_MINIMUM_LOCKING_AMOUNT, v2Epoch),
        QEARN_INVALID_INPUT_UNLOCK_AMOUNT);
    EXPECT_EQ(getBalance(wrapperUser), wrapperBalance);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        v2Epoch, wrapperUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount, principal);
    EXPECT_EQ(qearn.unlock(wrapperUser, principal, v2Epoch), QEARN_UNLOCK_SUCCESS);
    EXPECT_EQ(getBalance(wrapperUser), wrapperBalance + principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, wrapperUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount, 0ULL);

    const sint64 shortTermBalance = getBalance(shortTermUser);
    EXPECT_EQ(qearn.unlockV2(shortTermUser, v2Epoch, QEARN_V2_LOCK_PERIOD_13), QEARN_UNLOCK_SUCCESS);
    EXPECT_EQ(getBalance(shortTermUser), shortTermBalance + principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(v2Epoch, shortTermUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount, 0ULL);
    EXPECT_EQ(qearn.getEndedStatus(wrapperUser).earlyRewardedAmount, 0ULL);
    EXPECT_EQ(qearn.getEndedStatus(shortTermUser).earlyRewardedAmount, 0ULL);
}

TEST(TestContractQearn, V2FailedPrincipalTransferDoesNotClosePosition)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 principal = 100000000ULL;
    const id contractUser(QVAULT_CONTRACT_INDEX, 0, 0, 0);

    system.epoch = lockedEpoch;
    qearn.beginEpoch();
    increaseEnergy(contractUser, principal);
    EXPECT_EQ(qearn.lockV2(contractUser, principal, QEARN_V2_LOCK_PERIOD_13),
        QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    const sint64 userBalanceBefore = getBalance(contractUser);
    const sint64 qearnBalanceBefore = getBalance(QEARN_CONTRACT_ID);

    // QPI rejects transfers to contracts while an incoming-transfer callback
    // is active. A failed repayment must leave the position fully intact.
    contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
    EXPECT_EQ(qearn.unlockV2(contractUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_TRANSFER_FAILED);
    contractCallbacksRunning = NoContractCallback;

    EXPECT_EQ(getBalance(contractUser), userBalanceBefore);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), qearnBalanceBefore);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, contractUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount, principal);
    auto term13 =
        qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    EXPECT_EQ(term13.currentLockedAmount, principal);
    EXPECT_EQ(term13.termEarlyUnlockedAmount, 0ULL);
    EXPECT_EQ(term13.termForfeitedClaimAmount, 0ULL);

    EXPECT_EQ(qearn.unlockV2(contractUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_UNLOCK_SUCCESS);
    EXPECT_EQ(getBalance(contractUser), userBalanceBefore + static_cast<sint64>(principal));
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, contractUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount, 0ULL);
}

TEST(TestContractQearn, V2ValidatesTermsAmountsAndLockEpochRange)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const id user(1051, 1, 1, 1);

    system.epoch = lockedEpoch;
    qearn.beginEpoch();

    increaseEnergy(user, QEARN_MINIMUM_LOCKING_AMOUNT);
    const sint64 balanceBeforeInvalidPeriod = getBalance(user);
    EXPECT_EQ(qearn.lockV2(user, QEARN_MINIMUM_LOCKING_AMOUNT, 12),
        QEARN_INVALID_LOCK_PERIOD);
    EXPECT_EQ(getBalance(user), balanceBeforeInvalidPeriod);
    EXPECT_EQ(qearn.getV2LockInfoPerEpoch(lockedEpoch, 12).returnCode,
        QEARN_INVALID_LOCK_PERIOD);
    EXPECT_EQ(qearn.getV2UserLockedInfo(lockedEpoch, user, 12).returnCode,
        QEARN_INVALID_LOCK_PERIOD);

    increaseEnergy(user, QEARN_MINIMUM_LOCKING_AMOUNT - 1);
    const sint64 balanceBeforeSmallLock = getBalance(user);
    EXPECT_EQ(qearn.lockV2(
        user, QEARN_MINIMUM_LOCKING_AMOUNT - 1, QEARN_V2_LOCK_PERIOD_52),
        QEARN_INVALID_INPUT_AMOUNT);
    EXPECT_EQ(getBalance(user), balanceBeforeSmallLock);

    increaseEnergy(user, QEARN_MAX_LOCK_AMOUNT + QEARN_MINIMUM_LOCKING_AMOUNT);
    EXPECT_EQ(qearn.lockV2(user, QEARN_MAX_LOCK_AMOUNT, QEARN_V2_LOCK_PERIOD_52),
        QEARN_LOCK_SUCCESS);
    const sint64 balanceBeforeLimit = getBalance(user);
    EXPECT_EQ(qearn.lockV2(user, QEARN_MINIMUM_LOCKING_AMOUNT, QEARN_V2_LOCK_PERIOD_52),
        QEARN_LIMIT_LOCK);
    EXPECT_EQ(getBalance(user), balanceBeforeLimit);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, user, QEARN_V2_LOCK_PERIOD_52).lockedAmount,
        QEARN_MAX_LOCK_AMOUNT);
}

TEST(TestContractQearn, V2HonorsLastLockEpoch)
{
    ContractTestingQearn qearn;
    const id lastEpochUser(1052, 2, 2, 2);
    const id lateUser(1053, 3, 3, 3);

    system.epoch = QEARN_V2_LAST_LOCK_EPOCH;
    qearn.beginEpoch();
    increaseEnergy(lastEpochUser, QEARN_MINIMUM_LOCKING_AMOUNT);
    EXPECT_EQ(qearn.lockV2(
        lastEpochUser, QEARN_MINIMUM_LOCKING_AMOUNT, QEARN_V2_LOCK_PERIOD_13),
        QEARN_LOCK_SUCCESS);

    system.epoch = QEARN_V2_LAST_LOCK_EPOCH + 1;
    increaseEnergy(lateUser, QEARN_MINIMUM_LOCKING_AMOUNT);
    const sint64 lateBalance = getBalance(lateUser);
    EXPECT_EQ(qearn.lockV2(
        lateUser, QEARN_MINIMUM_LOCKING_AMOUNT, QEARN_V2_LOCK_PERIOD_13),
        QEARN_V2_NOT_ACTIVE);
    EXPECT_EQ(getBalance(lateUser), lateBalance);
}

TEST(TestContractQearn, V2KeepsThreeTermsSeparateForOneAddress)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 principal = 100000000ULL;
    const id user(1061, 13, 26, 52);

    system.epoch = lockedEpoch;
    qearn.beginEpoch();
    increaseEnergy(user, principal * 3);
    EXPECT_EQ(qearn.lockV2(user, principal, QEARN_V2_LOCK_PERIOD_13),
        QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user, principal, QEARN_V2_LOCK_PERIOD_26),
        QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user, principal, QEARN_V2_LOCK_PERIOD_52),
        QEARN_LOCK_SUCCESS);

    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, user, QEARN_V2_LOCK_PERIOD_13).lockedAmount, principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, user, QEARN_V2_LOCK_PERIOD_26).lockedAmount, principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, user, QEARN_V2_LOCK_PERIOD_52).lockedAmount, principal);
    EXPECT_EQ(qearn.getUserLockedInfo(lockedEpoch, user), principal);
    EXPECT_EQ(qearn.getUserLockStatus(user), 1ULL);
}

TEST(TestContractQearn, V1MaturityIsPreservedAlongsideLiveV2Cohorts)
{
    ContractTestingQearn qearn;
    const uint32 v1Epoch = QEARN_V2_ACTIVATION_EPOCH - 1;
    const uint32 v2Epoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 1000000000ULL;
    const id v1User(1101, 1, 1, 1);
    const id v2ShortUser(1102, 13, 2, 2);
    const id v2LongUser(1103, 52, 3, 3);

    system.epoch = v1Epoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(v1User, principal);
    EXPECT_EQ(qearn.lock(v1User, principal), QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    system.epoch = v2Epoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(v2ShortUser, principal);
    increaseEnergy(v2LongUser, principal);
    EXPECT_EQ(qearn.lockV2(v2ShortUser, principal, QEARN_V2_LOCK_PERIOD_13),
        QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(v2LongUser, principal, QEARN_V2_LOCK_PERIOD_52),
        QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    for (system.epoch = v2Epoch + 1;
        system.epoch <= v2Epoch + QEARN_V2_LOCK_PERIOD_52;
        ++system.epoch)
    {
        qearn.beginEpoch();
        qearn.endEpoch();

        if (system.epoch == v1Epoch + QEARN_V2_LOCK_PERIOD_52)
        {
            EXPECT_EQ(getBalance(v1User), principal + bonus);
            EXPECT_EQ(qearn.getUserLockedInfo(v1Epoch, v1User), 0ULL);
            EXPECT_EQ(getBalance(v2LongUser), 0);
        }
    }

    EXPECT_EQ(getBalance(v2ShortUser), principal + bonus * 10 / 100);
    EXPECT_EQ(getBalance(v2LongUser), principal + bonus * 90 / 100);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);

    const auto v1Round = qearn.getLockInfoPerEpoch(v1Epoch);
    EXPECT_EQ(v1Round.lockedAmount, principal);
    EXPECT_EQ(v1Round.bonusAmount, bonus);
    EXPECT_EQ(v1Round.yield, 1000000ULL);
    const auto v2LongRound =
        qearn.getV2LockInfoPerEpoch(v2Epoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(v2LongRound.termRewardedAmount, bonus * 90 / 100);
    EXPECT_EQ(v2LongRound.currentTermReturn, 900000ULL);
}

TEST(TestContractQearn, V2AllocatesBonusAcrossAllTerms)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 1000000000ULL;
    const id user13(2001, 13, 13, 13);
    const id user26(2002, 26, 26, 26);
    const id user52(2003, 52, 52, 52);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();

    increaseEnergy(user13, principal);
    increaseEnergy(user26, principal);
    increaseEnergy(user52, principal);
    EXPECT_EQ(qearn.lockV2(user13, principal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user26, principal, QEARN_V2_LOCK_PERIOD_26), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user52, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    const auto term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    const auto term26 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_26);
    const auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);

    EXPECT_EQ(term13.returnCode, QEARN_LOCK_SUCCESS);
    EXPECT_EQ(term26.returnCode, QEARN_LOCK_SUCCESS);
    EXPECT_EQ(term52.returnCode, QEARN_LOCK_SUCCESS);
    EXPECT_EQ(term13.initialLockedAmount, principal);
    EXPECT_EQ(term26.initialLockedAmount, principal);
    EXPECT_EQ(term52.initialLockedAmount, principal);
    EXPECT_EQ(term13.initialRewardPool, bonus * 10 / 100);
    EXPECT_EQ(term26.initialRewardPool, bonus * 20 / 100);
    EXPECT_EQ(term52.initialRewardPool, bonus * 70 / 100);
    EXPECT_EQ(term13.currentRewardPool, term13.initialRewardPool);
    EXPECT_EQ(term26.currentRewardPool, term26.initialRewardPool);
    EXPECT_EQ(term52.currentRewardPool, term52.initialRewardPool);
    EXPECT_EQ(term13.currentTermReturn, 100000ULL);
    EXPECT_EQ(term26.currentTermReturn, 200000ULL);
    EXPECT_EQ(term52.currentTermReturn, 700000ULL);
    EXPECT_EQ(term13.maturityEpoch, lockedEpoch + QEARN_V2_LOCK_PERIOD_13);
    EXPECT_EQ(term26.maturityEpoch, lockedEpoch + QEARN_V2_LOCK_PERIOD_26);
    EXPECT_EQ(term52.maturityEpoch, lockedEpoch + QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.epochBonusAmount, bonus);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);
    EXPECT_EQ(term52.finalized, 1U);
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term26.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);

    // The original read APIs expose the compatibility 52-epoch term only.
    const auto legacyRoundInfo = qearn.getLockInfoPerEpoch(lockedEpoch);
    EXPECT_EQ(legacyRoundInfo.lockedAmount, principal);
    EXPECT_EQ(legacyRoundInfo.bonusAmount, bonus * 70 / 100);
    EXPECT_EQ(legacyRoundInfo.currentLockedAmount, principal);
    EXPECT_EQ(legacyRoundInfo.currentBonusAmount, bonus * 70 / 100);
    EXPECT_EQ(legacyRoundInfo.yield, 700000ULL);
    EXPECT_EQ(qearn.getUserLockedInfo(lockedEpoch, user13), 0ULL);
    EXPECT_EQ(qearn.getUserLockedInfo(lockedEpoch, user26), 0ULL);
    EXPECT_EQ(qearn.getUserLockedInfo(lockedEpoch, user52), principal);
}

TEST(TestContractQearn, V2EarlyExitReturnsPrincipalAndRoutesForfeitureToCohort52)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 firstLock = 400000000ULL;
    const uint64 secondLock = 600000000ULL;
    const uint64 principal = firstLock + secondLock;
    const id shortTermUser(3001, 13, 1, 1);
    const id longTermUser(3002, 52, 2, 2);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();

    increaseEnergy(shortTermUser, principal);
    increaseEnergy(longTermUser, principal);
    EXPECT_EQ(qearn.lockV2(shortTermUser, firstLock, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(shortTermUser, secondLock, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(longTermUser, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);

    const sint64 sameEpochBalance = getBalance(shortTermUser);
    EXPECT_EQ(qearn.unlockV2(shortTermUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_INVALID_INPUT_LOCKED_EPOCH);
    EXPECT_EQ(getBalance(shortTermUser), sameEpochBalance);
    EXPECT_EQ(qearn.getV2UserLockedInfo(lockedEpoch, shortTermUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount,
        principal);
    qearn.endEpoch();

    auto term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.initialRewardPool, 10000000ULL);
    EXPECT_EQ(term52.initialRewardPool, 90000000ULL);

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    const sint64 userBalanceBefore = getBalance(shortTermUser);
    const sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);
    EXPECT_EQ(qearn.unlockV2(shortTermUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13), QEARN_UNLOCK_SUCCESS);

    // The two deposits formed one position: it is closed in full and pays no early reward.
    EXPECT_EQ(getBalance(shortTermUser), userBalanceBefore + principal);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), contractBalanceBefore - principal);
    EXPECT_EQ(qearn.getV2UserLockedInfo(lockedEpoch, shortTermUser, QEARN_V2_LOCK_PERIOD_13).lockedAmount,
        0ULL);
    const auto endedStatus = qearn.getEndedStatus(shortTermUser);
    EXPECT_EQ(endedStatus.earlyUnlockedAmount, principal);
    EXPECT_EQ(endedStatus.earlyRewardedAmount, 0ULL);

    term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.currentLockedAmount, 0ULL);
    EXPECT_EQ(term13.currentRewardPool, 0ULL);
    EXPECT_EQ(term13.epochForfeitedClaimAmount, 10000000ULL);
    EXPECT_EQ(term52.initialRewardPool, 90000000ULL);
    EXPECT_EQ(term52.currentRewardPool, 100000000ULL);
    EXPECT_EQ(term52.epochTransferredTo52Amount, 10000000ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(qearn.getV2UserLockedInfo(lockedEpoch, longTermUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount,
        principal);

    const auto stats = qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch);
    EXPECT_EQ(stats.boostedAmount, 10000000ULL);
    EXPECT_EQ(stats.burnedAmount, 0ULL);
}

TEST(TestContractQearn, V2PaysEachTermAtItsMaturity)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 1000000000ULL;
    const uint64 reward13 = 10000000ULL;
    const uint64 reward26 = 20000000ULL;
    const uint64 reward52 = 70000000ULL;
    const id user13(4001, 13, 1, 1);
    const id user26(4002, 26, 2, 2);
    const id user52(4003, 52, 3, 3);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(user13, principal);
    increaseEnergy(user26, principal);
    increaseEnergy(user52, principal);
    EXPECT_EQ(qearn.lockV2(user13, principal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user26, principal, QEARN_V2_LOCK_PERIOD_26), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user52, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    for (system.epoch = lockedEpoch + 1;
        system.epoch <= lockedEpoch + QEARN_V2_LOCK_PERIOD_52;
        ++system.epoch)
    {
        qearn.beginEpoch();

        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_13)
        {
            EXPECT_EQ(qearn.unlockV2(user13, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
                QEARN_V2_POSITION_MATURED);
            EXPECT_EQ(getBalance(user13), 0);
        }
        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_26)
        {
            EXPECT_EQ(qearn.unlockV2(user26, lockedEpoch, QEARN_V2_LOCK_PERIOD_26),
                QEARN_V2_POSITION_MATURED);
            EXPECT_EQ(getBalance(user26), 0);
        }
        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_52)
        {
            EXPECT_EQ(qearn.unlockV2(user52, lockedEpoch, QEARN_V2_LOCK_PERIOD_52),
                QEARN_V2_POSITION_MATURED);
            EXPECT_EQ(getBalance(user52), 0);
        }

        qearn.endEpoch();

        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_13)
        {
            EXPECT_EQ(getBalance(user13), principal + reward13);
            EXPECT_EQ(qearn.getV2UserLockedInfo(
                lockedEpoch, user13, QEARN_V2_LOCK_PERIOD_13).lockedAmount, 0ULL);
            const auto endedStatus = qearn.getEndedStatus(user13);
            EXPECT_EQ(endedStatus.fullyUnlockedAmount, principal);
            EXPECT_EQ(endedStatus.fullyRewardedAmount, reward13);
        }
        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_26)
        {
            EXPECT_EQ(getBalance(user26), principal + reward26);
            EXPECT_EQ(qearn.getV2UserLockedInfo(
                lockedEpoch, user26, QEARN_V2_LOCK_PERIOD_26).lockedAmount, 0ULL);
            const auto endedStatus = qearn.getEndedStatus(user26);
            EXPECT_EQ(endedStatus.fullyUnlockedAmount, principal);
            EXPECT_EQ(endedStatus.fullyRewardedAmount, reward26);
        }
        if (system.epoch == lockedEpoch + QEARN_V2_LOCK_PERIOD_52)
        {
            EXPECT_EQ(getBalance(user52), principal + reward52);
            EXPECT_EQ(qearn.getV2UserLockedInfo(
                lockedEpoch, user52, QEARN_V2_LOCK_PERIOD_52).lockedAmount, 0ULL);
            const auto endedStatus = qearn.getEndedStatus(user52);
            EXPECT_EQ(endedStatus.fullyUnlockedAmount, principal);
            EXPECT_EQ(endedStatus.fullyRewardedAmount, reward52);
        }
    }

    const auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.epochRewardedAmount, bonus);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);
    EXPECT_EQ(term52.currentTermReturn, 700000ULL);
    EXPECT_EQ(bonus,
        term52.currentRewardPool + term52.epochRewardedAmount
        + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(qearn.getStatsPerEpoch(lockedEpoch).earlyUnlockedAmount, 0ULL);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);
}

TEST(TestContractQearn, V2CapsReturnsAndTransfersLongPoolSurplusToCCF)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 100000000ULL;
    const id user13(5001, 13, 1, 1);
    const id user26(5002, 26, 2, 2);
    const id user52(5003, 52, 3, 3);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(user13, principal);
    increaseEnergy(user26, principal);
    increaseEnergy(user52, principal);
    EXPECT_EQ(qearn.lockV2(user13, principal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user26, principal, QEARN_V2_LOCK_PERIOD_26), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(user52, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);

    const sint64 contractBalanceBeforeFinalization = getBalance(QEARN_CONTRACT_ID);
    const sint64 ccfBalanceBeforeFinalization = getBalance(CCF_CONTRACT_ID);
    qearn.endEpoch();

    auto term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    const auto term26 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_26);
    auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.initialRewardPool, 3000000ULL);
    EXPECT_EQ(term26.initialRewardPool, 6000000ULL);
    EXPECT_EQ(term52.initialRewardPool, 18000000ULL);
    EXPECT_EQ(term13.currentTermReturn, 300000ULL);
    EXPECT_EQ(term26.currentTermReturn, 600000ULL);
    EXPECT_EQ(term52.currentTermReturn, 1800000ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 73000000ULL);
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term26.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), contractBalanceBeforeFinalization - 73000000LL);
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID), ccfBalanceBeforeFinalization + 73000000LL);
    EXPECT_EQ(qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch).burnedAmount, 0ULL);

    // With the 52-epoch pool already at 18%, a forfeited short-term reward is surplus.
    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    const sint64 contractBalanceBeforeExit = getBalance(QEARN_CONTRACT_ID);
    const sint64 ccfBalanceBeforeExit = getBalance(CCF_CONTRACT_ID);
    EXPECT_EQ(qearn.unlockV2(user13, lockedEpoch, QEARN_V2_LOCK_PERIOD_13), QEARN_UNLOCK_SUCCESS);

    term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.epochForfeitedClaimAmount, 3000000ULL);
    EXPECT_EQ(term52.currentRewardPool, 18000000ULL);
    EXPECT_EQ(term52.epochTransferredTo52Amount, 0ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 76000000ULL);
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term26.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch).burnedAmount, 0ULL);
    EXPECT_EQ(getBalance(user13), principal);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID),
        contractBalanceBeforeExit - static_cast<sint64>(principal) - 3000000LL);
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID), ccfBalanceBeforeExit + 3000000LL);
}

TEST(TestContractQearn, V2ShortOnlyCohortTransfersUnusedAndForfeitedRewardToCCF)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 100000000ULL;
    const uint64 shortRewardCap = 3000000ULL;
    const id shortUser(5101, 13, 1, 1);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(shortUser, principal);
    EXPECT_EQ(qearn.lockV2(shortUser, principal, QEARN_V2_LOCK_PERIOD_13),
        QEARN_LOCK_SUCCESS);
    const sint64 ccfBalanceBefore = getBalance(CCF_CONTRACT_ID);
    qearn.endEpoch();

    auto term13 =
        qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    auto term52 =
        qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.currentRewardPool, shortRewardCap);
    EXPECT_EQ(term52.initialLockedAmount, 0ULL);
    EXPECT_EQ(term52.currentRewardPool, 0ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, bonus - shortRewardCap);
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBefore + static_cast<sint64>(bonus - shortRewardCap));
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    EXPECT_EQ(qearn.unlockV2(shortUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_UNLOCK_SUCCESS);

    term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.currentRewardPool, 0ULL);
    EXPECT_EQ(term13.termForfeitedClaimAmount, shortRewardCap);
    EXPECT_EQ(term52.currentRewardPool, 0ULL);
    EXPECT_EQ(term52.epochTransferredTo52Amount, 0ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, bonus);
    EXPECT_EQ(getBalance(shortUser), static_cast<sint64>(principal));
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBefore + static_cast<sint64>(bonus));
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);
}

TEST(TestContractQearn, V2LongEarlyExitShrinksCapAndPreservesSurvivorReturn)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 100000000ULL;
    const uint64 principal = 100000000ULL;
    const uint64 initialPool = 36000000ULL;
    const uint64 initialCCFTransfer = 64000000ULL;
    const uint64 exitingClaim = 18000000ULL;
    const uint64 survivorReward = 18000000ULL;
    const id exitingUser(6001, 52, 1, 1);
    const id survivingUser(6002, 52, 2, 2);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(exitingUser, principal);
    increaseEnergy(survivingUser, principal);
    EXPECT_EQ(qearn.lockV2(exitingUser, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(survivingUser, principal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    const sint64 ccfBalanceBeforeFinalization = getBalance(CCF_CONTRACT_ID);
    qearn.endEpoch();

    auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.initialLockedAmount, principal * 2);
    EXPECT_EQ(term52.currentLockedAmount, principal * 2);
    EXPECT_EQ(term52.initialRewardPool, initialPool);
    EXPECT_EQ(term52.currentRewardPool, initialPool);
    EXPECT_EQ(term52.currentTermReturn, 1800000ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, initialCCFTransfer);
    EXPECT_EQ(exitingClaim, initialPool * principal / (principal * 2));
    EXPECT_EQ(bonus, term52.currentRewardPool + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBeforeFinalization + static_cast<sint64>(initialCCFTransfer));

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    const sint64 exitingBalanceBefore = getBalance(exitingUser);
    const sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);
    const sint64 ccfBalanceBeforeExit = getBalance(CCF_CONTRACT_ID);
    EXPECT_EQ(qearn.unlockV2(exitingUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_52),
        QEARN_UNLOCK_SUCCESS);

    // The exit returns principal only. Its abandoned 18% claim stays in the
    // long pool until the smaller survivor-only cap sends that surplus to CCF.
    EXPECT_EQ(getBalance(exitingUser), exitingBalanceBefore + principal);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID),
        contractBalanceBefore - static_cast<sint64>(principal + exitingClaim));
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBeforeExit + static_cast<sint64>(exitingClaim));
    const auto exitingStatus = qearn.getEndedStatus(exitingUser);
    EXPECT_EQ(exitingStatus.earlyUnlockedAmount, principal);
    EXPECT_EQ(exitingStatus.earlyRewardedAmount, 0ULL);

    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.currentLockedAmount, principal);
    EXPECT_EQ(term52.termEarlyUnlockedAmount, principal);
    EXPECT_EQ(term52.initialRewardPool, initialPool);
    EXPECT_EQ(term52.currentRewardPool, survivorReward);
    EXPECT_EQ(term52.termForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term52.epochForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term52.epochTransferredTo52Amount, 0ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, initialCCFTransfer + exitingClaim);
    EXPECT_EQ(term52.currentTermReturn, 1800000ULL);
    EXPECT_EQ(qearn.getV2UserLockedInfo(
        lockedEpoch, survivingUser, QEARN_V2_LOCK_PERIOD_52).lockedAmount, principal);
    EXPECT_EQ(bonus,
        term52.currentRewardPool + term52.epochRewardedAmount
        + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch).burnedAmount, 0ULL);
    qearn.endEpoch();

    for (system.epoch = lockedEpoch + 2;
        system.epoch <= lockedEpoch + QEARN_V2_LOCK_PERIOD_52;
        ++system.epoch)
    {
        qearn.beginEpoch();
        qearn.endEpoch();
    }

    EXPECT_EQ(getBalance(exitingUser), principal);
    EXPECT_EQ(getBalance(survivingUser), principal + survivorReward);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);
    const auto survivingStatus = qearn.getEndedStatus(survivingUser);
    EXPECT_EQ(survivingStatus.fullyUnlockedAmount, principal);
    EXPECT_EQ(survivingStatus.fullyRewardedAmount, survivorReward);

    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.currentLockedAmount, 0ULL);
    EXPECT_EQ(term52.currentRewardPool, 0ULL);
    EXPECT_EQ(term52.termEarlyUnlockedAmount, principal);
    EXPECT_EQ(term52.termRewardedAmount, survivorReward);
    EXPECT_EQ(term52.termForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term52.epochRewardedAmount, survivorReward);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, initialCCFTransfer + exitingClaim);
    EXPECT_EQ(term52.currentTermReturn, 1800000ULL);
    EXPECT_EQ(term52.state, 2U);
    EXPECT_EQ(bonus,
        term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
}

TEST(TestContractQearn, V2FinalLongEarlyExitTransfersTheEntireRewardPoolToCCF)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 18000000ULL;
    const uint64 principal = 100000000ULL;
    const id longUser(6101, 52, 1, 1);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(longUser, principal);
    EXPECT_EQ(qearn.lockV2(longUser, principal, QEARN_V2_LOCK_PERIOD_52),
        QEARN_LOCK_SUCCESS);
    const sint64 ccfBalanceBefore = getBalance(CCF_CONTRACT_ID);
    qearn.endEpoch();

    auto term52 =
        qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.currentRewardPool, bonus);
    EXPECT_EQ(term52.currentTermReturn, 1800000ULL);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    EXPECT_EQ(qearn.unlockV2(longUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_52),
        QEARN_UNLOCK_SUCCESS);

    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.currentLockedAmount, 0ULL);
    EXPECT_EQ(term52.currentRewardPool, 0ULL);
    EXPECT_EQ(term52.termEarlyUnlockedAmount, principal);
    EXPECT_EQ(term52.termForfeitedClaimAmount, bonus);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, bonus);
    EXPECT_EQ(getBalance(longUser), static_cast<sint64>(principal));
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBefore + static_cast<sint64>(bonus));
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);
    EXPECT_EQ(bonus,
        term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
}

TEST(TestContractQearn, V2ShortFixedEntitlementsRouteRoundingDustTo52)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 101ULL;
    const uint64 exitingPrincipal = 10000000ULL;
    const uint64 survivingPrincipal = 20000000ULL;
    const uint64 longPrincipal = 10000000ULL;
    const uint64 shortPool = 10ULL;
    const uint64 exitingClaim = 3ULL;
    const uint64 survivorReward = 6ULL;
    const uint64 roundingDust = 1ULL;
    const uint64 initialLongPool = 91ULL;
    const id exitingUser(7001, 13, 1, 1);
    const id survivingUser(7002, 13, 2, 2);
    const id longUser(7003, 52, 3, 3);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(exitingUser, exitingPrincipal);
    increaseEnergy(survivingUser, survivingPrincipal);
    increaseEnergy(longUser, longPrincipal);
    EXPECT_EQ(qearn.lockV2(
        exitingUser, exitingPrincipal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(
        survivingUser, survivingPrincipal, QEARN_V2_LOCK_PERIOD_13), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(
        longUser, longPrincipal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    qearn.endEpoch();

    auto term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    auto term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.initialLockedAmount, exitingPrincipal + survivingPrincipal);
    EXPECT_EQ(term13.initialRewardPool, shortPool);
    EXPECT_EQ(term13.currentRewardPool, shortPool);
    EXPECT_EQ(term52.initialRewardPool, initialLongPool);
    EXPECT_EQ(term52.currentRewardPool, initialLongPool);
    EXPECT_EQ(exitingClaim,
        shortPool * exitingPrincipal / (exitingPrincipal + survivingPrincipal));
    EXPECT_EQ(survivorReward,
        shortPool * survivingPrincipal / (exitingPrincipal + survivingPrincipal));
    EXPECT_EQ(roundingDust, shortPool - exitingClaim - survivorReward);
    EXPECT_EQ(bonus, term13.currentRewardPool + term52.currentRewardPool);

    system.epoch = lockedEpoch + 1;
    qearn.beginEpoch();
    const sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);
    EXPECT_EQ(qearn.unlockV2(exitingUser, lockedEpoch, QEARN_V2_LOCK_PERIOD_13),
        QEARN_UNLOCK_SUCCESS);
    EXPECT_EQ(getBalance(exitingUser), exitingPrincipal);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID),
        contractBalanceBefore - static_cast<sint64>(exitingPrincipal));

    term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.currentLockedAmount, survivingPrincipal);
    EXPECT_EQ(term13.termEarlyUnlockedAmount, exitingPrincipal);
    EXPECT_EQ(term13.currentRewardPool, shortPool - exitingClaim);
    EXPECT_EQ(term13.termForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term13.epochForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term52.currentRewardPool, initialLongPool + exitingClaim);
    EXPECT_EQ(term52.epochTransferredTo52Amount, exitingClaim);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);
    EXPECT_EQ(bonus, term13.currentRewardPool + term52.currentRewardPool);
    qearn.endEpoch();

    for (system.epoch = lockedEpoch + 2;
        system.epoch <= lockedEpoch + QEARN_V2_LOCK_PERIOD_13;
        ++system.epoch)
    {
        qearn.beginEpoch();
        qearn.endEpoch();
    }

    // Entitlements always use the original pool and original total principal:
    // floor(10 * 10M / 30M) = 3 and floor(10 * 20M / 30M) = 6.
    // The unclaimable final qu is routed to this cohort's 52-epoch pool.
    EXPECT_EQ(shortPool, exitingClaim + survivorReward + roundingDust);
    EXPECT_EQ(getBalance(exitingUser), exitingPrincipal);
    EXPECT_EQ(getBalance(survivingUser), survivingPrincipal + survivorReward);
    EXPECT_EQ(getBalance(longUser), 0);
    const auto survivingStatus = qearn.getEndedStatus(survivingUser);
    EXPECT_EQ(survivingStatus.fullyUnlockedAmount, survivingPrincipal);
    EXPECT_EQ(survivingStatus.fullyRewardedAmount, survivorReward);

    term13 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_13);
    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term13.currentLockedAmount, 0ULL);
    EXPECT_EQ(term13.currentRewardPool, 0ULL);
    EXPECT_EQ(term13.termEarlyUnlockedAmount, exitingPrincipal);
    EXPECT_EQ(term13.termRewardedAmount, survivorReward);
    EXPECT_EQ(term13.termForfeitedClaimAmount, exitingClaim);
    EXPECT_EQ(term13.epochRewardedAmount, survivorReward);
    EXPECT_EQ(term52.currentRewardPool, initialLongPool + exitingClaim + roundingDust);
    EXPECT_EQ(term52.epochTransferredTo52Amount, exitingClaim + roundingDust);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);
    EXPECT_EQ(qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch).boostedAmount,
        exitingClaim + roundingDust);
    EXPECT_EQ(bonus,
        term13.currentRewardPool + term52.currentRewardPool
        + term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID),
        static_cast<sint64>(longPrincipal + term52.currentRewardPool));
}

TEST(TestContractQearn, V2TransfersLongMaturityRoundingRemainderToCCF)
{
    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 101ULL;
    const uint64 firstPrincipal = 10000000ULL;
    const uint64 secondPrincipal = 20000000ULL;
    const uint64 firstReward = 33ULL;
    const uint64 secondReward = 67ULL;
    const uint64 roundingRemainder = 1ULL;
    const id firstUser(7101, 52, 1, 1);
    const id secondUser(7102, 52, 2, 2);

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    increaseEnergy(firstUser, firstPrincipal);
    increaseEnergy(secondUser, secondPrincipal);
    EXPECT_EQ(qearn.lockV2(
        firstUser, firstPrincipal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    EXPECT_EQ(qearn.lockV2(
        secondUser, secondPrincipal, QEARN_V2_LOCK_PERIOD_52), QEARN_LOCK_SUCCESS);
    const sint64 ccfBalanceBefore = getBalance(CCF_CONTRACT_ID);
    qearn.endEpoch();

    auto term52 =
        qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.initialRewardPool, bonus);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, 0ULL);

    for (system.epoch = lockedEpoch + 1;
        system.epoch <= lockedEpoch + QEARN_V2_LOCK_PERIOD_52;
        ++system.epoch)
    {
        qearn.beginEpoch();
        qearn.endEpoch();
    }

    EXPECT_EQ(getBalance(firstUser), firstPrincipal + firstReward);
    EXPECT_EQ(getBalance(secondUser), secondPrincipal + secondReward);
    EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
        ccfBalanceBefore + static_cast<sint64>(roundingRemainder));
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);

    term52 = qearn.getV2LockInfoPerEpoch(lockedEpoch, QEARN_V2_LOCK_PERIOD_52);
    EXPECT_EQ(term52.currentRewardPool, 0ULL);
    EXPECT_EQ(term52.termRewardedAmount, firstReward + secondReward);
    EXPECT_EQ(term52.epochRewardedAmount, firstReward + secondReward);
    EXPECT_EQ(term52.epochTransferredToCCFAmount, roundingRemainder);
    EXPECT_EQ(bonus,
        term52.epochRewardedAmount + term52.epochTransferredToCCFAmount);
    EXPECT_EQ(qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch).burnedAmount, 0ULL);
}

TEST(TestContractQearn, V2RandomizedMixedTermsPreserveAccountingThroughExitsAndMaturities)
{
    struct PropertyPosition
    {
        id user;
        uint32 termIndex;
        uint32 lockPeriod;
        uint32 exitEpochOffset;
        uint64 principal;
        bool active;
    };

    struct PropertyTermModel
    {
        uint64 initialLockedAmount;
        uint64 currentLockedAmount;
        uint64 earlyUnlockedAmount;
        uint64 initialRewardPool;
        uint64 currentRewardPool;
        uint64 rewardedAmount;
        uint64 forfeitedClaimAmount;
    };

    struct PropertyEpochModel
    {
        PropertyTermModel terms[QEARN_V2_NUMBER_OF_TERMS];
        uint64 rewardedAmount;
        uint64 forfeitedClaimAmount;
        uint64 transferredTo52Amount;
        uint64 transferredToCCFAmount;
    };

    ContractTestingQearn qearn;
    const uint32 lockedEpoch = QEARN_V2_ACTIVATION_EPOCH;
    const uint64 bonus = 250000123ULL;
    const uint32 walletCount = 18;
    const uint32 lockPeriods[QEARN_V2_NUMBER_OF_TERMS] = {
        QEARN_V2_LOCK_PERIOD_13,
        QEARN_V2_LOCK_PERIOD_26,
        QEARN_V2_LOCK_PERIOD_52,
    };
    const uint32 exitOffsets[QEARN_V2_NUMBER_OF_TERMS][6] = {
        {1, 2, 4, 7, 10, 12},
        {1, 3, 8, 13, 19, 25},
        {1, 5, 12, 20, 33, 51},
    };
    const uint64 seed = 0x514541524E5632ULL;
    std::mt19937_64 rng(seed);
    std::vector<PropertyPosition> positions;
    std::map<id, uint64> expectedWalletBalances;
    PropertyEpochModel model = {};
    SCOPED_TRACE(::testing::Message() << "seed=" << seed);

    // Every wallet owns all three independent terms. The first three wallets
    // remain through maturity; the others create deterministic, seeded exits.
    // All long positions except those first three exit, which guarantees both
    // dynamic-cap CCF routing and live long-term maturity recipients.
    for (uint32 walletIndex = 0; walletIndex < walletCount; ++walletIndex)
    {
        const id user(
            8000 + walletIndex,
            0xA000 + walletIndex,
            0xB000 + walletIndex,
            0xC000 + walletIndex);
        for (uint32 termIndex = 0;
            termIndex < QEARN_V2_NUMBER_OF_TERMS;
            ++termIndex)
        {
            const uint64 principal = 80000001ULL + (rng() % 80000000ULL);
            uint32 exitEpochOffset = 0;
            if (walletIndex >= 3)
            {
                bool exitsEarly = false;
                if (termIndex == QEARN_V2_TERM_INDEX_13)
                {
                    exitsEarly = walletIndex == 3 || (rng() % 100ULL) < 45ULL;
                }
                else if (termIndex == QEARN_V2_TERM_INDEX_26)
                {
                    exitsEarly = walletIndex == 3 || (rng() % 100ULL) < 50ULL;
                }
                else
                {
                    exitsEarly = true;
                }

                if (exitsEarly)
                {
                    exitEpochOffset =
                        exitOffsets[termIndex][rng() % 6ULL];
                    if (walletIndex == 3)
                    {
                        // Force mixed-term exits in one epoch. Their order is
                        // shuffled below, exercising order-dependent counters.
                        exitEpochOffset = 1;
                    }
                }
            }

            positions.push_back(PropertyPosition{
                user,
                termIndex,
                lockPeriods[termIndex],
                exitEpochOffset,
                principal,
                true,
            });
            model.terms[termIndex].initialLockedAmount += principal;
            model.terms[termIndex].currentLockedAmount += principal;
            expectedWalletBalances[user] += principal;
        }
    }

    system.epoch = lockedEpoch;
    increaseEnergy(QEARN_CONTRACT_ID, bonus);
    qearn.beginEpoch();
    for (const auto& wallet : expectedWalletBalances)
    {
        increaseEnergy(wallet.first, wallet.second);
    }
    for (const auto& position : positions)
    {
        EXPECT_EQ(qearn.lockV2(
            position.user,
            position.principal,
            position.lockPeriod),
            QEARN_LOCK_SUCCESS);
    }
    for (auto& wallet : expectedWalletBalances)
    {
        EXPECT_EQ(getBalance(wallet.first), 0);
        wallet.second = 0;
    }

    const sint64 initialCCFBalance = getBalance(CCF_CONTRACT_ID);
    model.terms[QEARN_V2_TERM_INDEX_13].initialRewardPool =
        std::min(
            bonus * QEARN_V2_ALLOCATION_PERCENT_13 / 100ULL,
            model.terms[QEARN_V2_TERM_INDEX_13].initialLockedAmount
                * QEARN_V2_RETURN_PERCENT_13 / 100ULL);
    model.terms[QEARN_V2_TERM_INDEX_13].currentRewardPool =
        model.terms[QEARN_V2_TERM_INDEX_13].initialRewardPool;
    model.terms[QEARN_V2_TERM_INDEX_26].initialRewardPool =
        std::min(
            bonus * QEARN_V2_ALLOCATION_PERCENT_26 / 100ULL,
            model.terms[QEARN_V2_TERM_INDEX_26].initialLockedAmount
                * QEARN_V2_RETURN_PERCENT_26 / 100ULL);
    model.terms[QEARN_V2_TERM_INDEX_26].currentRewardPool =
        model.terms[QEARN_V2_TERM_INDEX_26].initialRewardPool;

    const uint64 availableLongReward =
        bonus
        - model.terms[QEARN_V2_TERM_INDEX_13].initialRewardPool
        - model.terms[QEARN_V2_TERM_INDEX_26].initialRewardPool;
    const uint64 initialLongCap =
        model.terms[QEARN_V2_TERM_INDEX_52].initialLockedAmount
        * QEARN_V2_RETURN_PERCENT_52 / 100ULL;
    model.terms[QEARN_V2_TERM_INDEX_52].initialRewardPool =
        std::min(availableLongReward, initialLongCap);
    model.terms[QEARN_V2_TERM_INDEX_52].currentRewardPool =
        model.terms[QEARN_V2_TERM_INDEX_52].initialRewardPool;
    model.transferredToCCFAmount =
        availableLongReward
        - model.terms[QEARN_V2_TERM_INDEX_52].initialRewardPool;

    auto fundLongPool = [&](uint64 amount)
    {
        PropertyTermModel& longTerm =
            model.terms[QEARN_V2_TERM_INDEX_52];
        const uint64 previousPool = longTerm.currentRewardPool;
        const uint64 availableAmount = previousPool + amount;
        const uint64 currentCap =
            longTerm.currentLockedAmount
            * QEARN_V2_RETURN_PERCENT_52 / 100ULL;
        longTerm.currentRewardPool =
            std::min(availableAmount, currentCap);
        if (longTerm.currentRewardPool > previousPool)
        {
            model.transferredTo52Amount +=
                longTerm.currentRewardPool - previousPool;
        }
        model.transferredToCCFAmount +=
            availableAmount - longTerm.currentRewardPool;
    };

    auto checkAccounting = [&]()
    {
        uint64 activePrincipal = 0;
        uint64 currentRewardPools = 0;
        for (const auto& position : positions)
        {
            if (position.active)
            {
                activePrincipal += position.principal;
            }
        }

        for (uint32 termIndex = 0;
            termIndex < QEARN_V2_NUMBER_OF_TERMS;
            ++termIndex)
        {
            const PropertyTermModel& expectedTerm = model.terms[termIndex];
            const auto actualTerm = qearn.getV2LockInfoPerEpoch(
                lockedEpoch,
                lockPeriods[termIndex]);
            EXPECT_EQ(actualTerm.returnCode, QEARN_LOCK_SUCCESS);
            EXPECT_EQ(actualTerm.epochBonusAmount, bonus);
            EXPECT_EQ(actualTerm.finalized, 1U);
            EXPECT_EQ(actualTerm.initialLockedAmount,
                expectedTerm.initialLockedAmount);
            EXPECT_EQ(actualTerm.currentLockedAmount,
                expectedTerm.currentLockedAmount);
            EXPECT_EQ(actualTerm.termEarlyUnlockedAmount,
                expectedTerm.earlyUnlockedAmount);
            EXPECT_EQ(actualTerm.initialRewardPool,
                expectedTerm.initialRewardPool);
            EXPECT_EQ(actualTerm.currentRewardPool,
                expectedTerm.currentRewardPool);
            EXPECT_EQ(actualTerm.termRewardedAmount,
                expectedTerm.rewardedAmount);
            EXPECT_EQ(actualTerm.termForfeitedClaimAmount,
                expectedTerm.forfeitedClaimAmount);
            EXPECT_EQ(actualTerm.epochRewardedAmount,
                model.rewardedAmount);
            EXPECT_EQ(actualTerm.epochForfeitedClaimAmount,
                model.forfeitedClaimAmount);
            EXPECT_EQ(actualTerm.epochTransferredTo52Amount,
                model.transferredTo52Amount);
            EXPECT_EQ(actualTerm.epochTransferredToCCFAmount,
                model.transferredToCCFAmount);
            currentRewardPools += expectedTerm.currentRewardPool;
        }

        EXPECT_EQ(bonus,
            currentRewardPools
            + model.rewardedAmount
            + model.transferredToCCFAmount);
        EXPECT_EQ(getBalance(QEARN_CONTRACT_ID),
            static_cast<sint64>(activePrincipal + currentRewardPools));
        EXPECT_EQ(getBalance(CCF_CONTRACT_ID),
            initialCCFBalance
            + static_cast<sint64>(model.transferredToCCFAmount));

        const auto stats =
            qearn.getBurnedAndBoostedStatsPerEpoch(lockedEpoch);
        EXPECT_EQ(stats.burnedAmount, 0ULL);
        EXPECT_EQ(stats.boostedAmount, model.transferredTo52Amount);
        EXPECT_EQ(stats.rewardedAmount, model.rewardedAmount);
        for (const auto& wallet : expectedWalletBalances)
        {
            EXPECT_EQ(getBalance(wallet.first),
                static_cast<sint64>(wallet.second));
        }
    };

    auto checkCompactedLockerRange = [&]()
    {
        uint32 expectedActivePositions = 0;
        for (const auto& position : positions)
        {
            const uint64 expectedLockedAmount =
                position.active ? position.principal : 0ULL;
            EXPECT_EQ(qearn.getV2UserLockedInfo(
                lockedEpoch,
                position.user,
                position.lockPeriod).lockedAmount,
                expectedLockedAmount);
            if (position.active)
            {
                expectedActivePositions++;
            }
        }

        const QearnChecker* contractState = qearn.getState();
        const QEARN::EpochIndexInfo range =
            contractState->_epochIndex.get(lockedEpoch);
        EXPECT_EQ(range.endIndex - range.startIndex,
            expectedActivePositions);
        for (uint32 lockerIndex = range.startIndex;
            lockerIndex < range.endIndex;
            ++lockerIndex)
        {
            const QEARN::LockInfo& actualLock =
                contractState->locker.get(lockerIndex);
            const uint32 actualLockPeriod =
                contractState->lockerLockPeriods.get(lockerIndex);
            EXPECT_GT(actualLock._lockedAmount, 0ULL);
            EXPECT_EQ(actualLock._lockedEpoch, lockedEpoch);

            uint32 matchingPositions = 0;
            for (const auto& position : positions)
            {
                if (position.active
                    && position.user == actualLock.ID
                    && position.lockPeriod == actualLockPeriod
                    && position.principal == actualLock._lockedAmount)
                {
                    matchingPositions++;
                }
            }
            EXPECT_EQ(matchingPositions, 1U);
        }
    };

    auto exitPosition = [&](size_t positionIndex)
    {
        PropertyPosition& position = positions[positionIndex];
        PropertyTermModel& term = model.terms[position.termIndex];
        SCOPED_TRACE(
            ::testing::Message()
            << "exit position=" << positionIndex
            << ", term=" << position.lockPeriod
            << ", epochOffset=" << position.exitEpochOffset);
        EXPECT_TRUE(position.active);

        uint64 forfeitedAmount = 0;
        uint64 amountToLongPool = 0;
        if (position.termIndex == QEARN_V2_TERM_INDEX_52)
        {
            if (term.currentLockedAmount)
            {
                forfeitedAmount =
                    term.currentRewardPool * position.principal
                    / term.currentLockedAmount;
            }
        }
        else if (term.initialLockedAmount)
        {
            forfeitedAmount =
                term.initialRewardPool * position.principal
                / term.initialLockedAmount;
            forfeitedAmount =
                std::min(forfeitedAmount, term.currentRewardPool);
            amountToLongPool = forfeitedAmount;
        }

        EXPECT_EQ(qearn.unlockV2(
            position.user,
            lockedEpoch,
            position.lockPeriod),
            QEARN_UNLOCK_SUCCESS);

        if (position.termIndex != QEARN_V2_TERM_INDEX_52)
        {
            term.currentRewardPool -= forfeitedAmount;
        }
        term.currentLockedAmount -= position.principal;
        term.earlyUnlockedAmount += position.principal;
        term.forfeitedClaimAmount += forfeitedAmount;
        model.forfeitedClaimAmount += forfeitedAmount;
        if (position.termIndex != QEARN_V2_TERM_INDEX_52
            && !term.currentLockedAmount)
        {
            amountToLongPool += term.currentRewardPool;
            term.currentRewardPool = 0;
        }
        position.active = false;
        expectedWalletBalances[position.user] += position.principal;
        fundLongPool(amountToLongPool);
        checkAccounting();
    };

    auto matureTerm = [&](uint32 termIndex)
    {
        PropertyTermModel& term = model.terms[termIndex];
        const uint64 numerator =
            termIndex == QEARN_V2_TERM_INDEX_52
                ? term.currentRewardPool
                : term.initialRewardPool;
        const uint64 denominator =
            termIndex == QEARN_V2_TERM_INDEX_52
                ? term.currentLockedAmount
                : term.initialLockedAmount;
        uint64 remainingRewardPool = term.currentRewardPool;
        uint64 totalRewardedAmount = 0;
        uint64 totalUnlockedAmount = 0;

        // Stable compaction preserves lock order, matching the contract's
        // deterministic floor-and-remainder settlement.
        for (auto& position : positions)
        {
            if (!position.active || position.termIndex != termIndex)
            {
                continue;
            }

            uint64 rewardAmount = 0;
            if (denominator)
            {
                rewardAmount =
                    numerator * position.principal / denominator;
                rewardAmount =
                    std::min(rewardAmount, remainingRewardPool);
            }
            remainingRewardPool -= rewardAmount;
            totalRewardedAmount += rewardAmount;
            totalUnlockedAmount += position.principal;
            expectedWalletBalances[position.user] +=
                position.principal + rewardAmount;
            position.active = false;
        }

        EXPECT_EQ(term.currentLockedAmount, totalUnlockedAmount);
        term.currentLockedAmount -= totalUnlockedAmount;
        term.currentRewardPool = 0;
        term.rewardedAmount += totalRewardedAmount;
        model.rewardedAmount += totalRewardedAmount;
        if (termIndex == QEARN_V2_TERM_INDEX_52)
        {
            model.transferredToCCFAmount += remainingRewardPool;
        }
        else
        {
            fundLongPool(remainingRewardPool);
        }
    };

    qearn.endEpoch();
    checkAccounting();
    checkCompactedLockerRange();

    for (uint32 epochOffset = 1;
        epochOffset <= QEARN_V2_LOCK_PERIOD_52;
        ++epochOffset)
    {
        system.epoch = lockedEpoch + epochOffset;
        SCOPED_TRACE(
            ::testing::Message()
            << "seed=" << seed
            << ", epochOffset=" << epochOffset);
        qearn.beginEpoch();

        std::vector<size_t> scheduledExits;
        for (size_t positionIndex = 0;
            positionIndex < positions.size();
            ++positionIndex)
        {
            if (positions[positionIndex].active
                && positions[positionIndex].exitEpochOffset == epochOffset)
            {
                scheduledExits.push_back(positionIndex);
            }
        }
        std::shuffle(scheduledExits.begin(), scheduledExits.end(), rng);
        for (const size_t positionIndex : scheduledExits)
        {
            exitPosition(positionIndex);
        }

        if (epochOffset == QEARN_V2_LOCK_PERIOD_13)
        {
            matureTerm(QEARN_V2_TERM_INDEX_13);
        }
        if (epochOffset == QEARN_V2_LOCK_PERIOD_26)
        {
            matureTerm(QEARN_V2_TERM_INDEX_26);
        }
        if (epochOffset == QEARN_V2_LOCK_PERIOD_52)
        {
            matureTerm(QEARN_V2_TERM_INDEX_52);
        }

        qearn.endEpoch();
        checkAccounting();
        checkCompactedLockerRange();
    }

    EXPECT_GT(model.transferredTo52Amount, 0ULL);
    EXPECT_GT(model.transferredToCCFAmount, 0ULL);
    EXPECT_GT(model.rewardedAmount, 0ULL);
    for (uint32 termIndex = 0;
        termIndex < QEARN_V2_NUMBER_OF_TERMS;
        ++termIndex)
    {
        EXPECT_GT(model.terms[termIndex].earlyUnlockedAmount, 0ULL);
    }
    EXPECT_EQ(getBalance(QEARN_CONTRACT_ID), 0);
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
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    // This model checks the original 52-epoch economics. V2 behavior is covered by
    // focused tests above, so do not carry the V1 model across the activation boundary.
    const uint16 v1NumEpochs = std::min<uint16>(
        numEpochs, QEARN_V2_ACTIVATION_EPOCH - firstEpoch - 1);
    const uint16 lastEpoch = firstEpoch + v1NumEpochs;
    std::cout << "random V1 test without early unlock for " << v1NumEpochs << " epochs with " << totalUsers << " total users and up to " << maxUserLocking << " lock calls per epoch" << std::endl;

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
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    // This model checks V1 partial unlock rewards and burns. Keep it entirely
    // before activation because V2 early unlocks intentionally close full positions.
    const uint16 v1NumEpochs = std::min<uint16>(
        numEpochs, QEARN_V2_ACTIVATION_EPOCH - firstEpoch - 1);
    const uint16 lastEpoch = firstEpoch + v1NumEpochs;
    std::cout << "random V1 test with early unlock for " << v1NumEpochs << " epochs with " << totalUsers << " total users, up to " << maxUserLocking << " lock calls (per epoch), and up to " << maxUserUnlocking << " unlock calls (per running round)" << std::endl;

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
