#if 1
#define NO_UEFI

#include "contract_testing.h"

#include <random>
#include <map>


static const id QEARN_CONTRACT_ID(QEARN_CONTRACT_INDEX, 0, 0, 0);

static std::mt19937_64 rand64;

static std::vector<uint64> fullyUnlockedAmount;
static std::vector<id> fullyUnlockedUser;


class QearnChecker : public QEARN
{
public:
    void checkLockerArray()
    {
        // check that locker array is in consistent state
        std::map<int, unsigned long long> epochTotalLocked;
        for (uint64 idx = 0; idx < Locker.capacity(); ++idx)
        {
            const QEARN::LockInfo& lock = Locker.get(idx);
            if (lock._Locked_Amount == 0)
            {
                EXPECT_TRUE(isZero(lock.ID));
                EXPECT_EQ(lock._Locked_Epoch, 0);
            }
            else
            {
                EXPECT_GT(lock._Locked_Amount, QEARN_MINIMUM_LOCKING_AMOUNT);
                EXPECT_LE(lock._Locked_Amount, QEARN_MAX_LOCK_AMOUNT);
                EXPECT_FALSE(isZero(lock.ID));
                const QEARN::EpochIndexInfo& epochRange = EpochIndex.get(lock._Locked_Epoch);
                EXPECT_GE(idx, epochRange.startIndex);
                EXPECT_LT(idx, epochRange.endIndex);
                epochTotalLocked[lock._Locked_Epoch] += lock._Locked_Amount;
            }
        }

        for (const auto& epochAndTotal : epochTotalLocked)
        {
            const QEARN::RoundInfo& currentRoundInfo = _CurrentRoundInfo.get(epochAndTotal.first);
            EXPECT_EQ(currentRoundInfo._Total_Locked_Amount, epochAndTotal.second);
            std::cout << "Total locked amount in epoch " << epochAndTotal.first << " = " << epochAndTotal.second << ", total bonus " << currentRoundInfo._Epoch_Bonus_Amount << std::endl;
        }
    }

    void checkGetUnlockedInfo(uint32 epoch) 
    {
        fullyUnlockedAmount.clear();
        fullyUnlockedUser.clear();

        const QEARN::EpochIndexInfo& epochIndex = EpochIndex.get(epoch);
        for(uint64 idx = epochIndex.startIndex; idx < epochIndex.endIndex; ++idx)
        {
            if(Locker.get(idx)._Locked_Amount != 0)
            {
                fullyUnlockedAmount.push_back(Locker.get(idx)._Locked_Amount);
                fullyUnlockedUser.push_back(Locker.get(idx).ID);
            }
        }
    }

    void checkFullyUnlockedAmount()
    {
        for(uint32 idx = 0; idx < _FullyUnlocked_cnt; idx++)
        {
            const QEARN::HistoryInfo& FullyUnlockedInfo = FullyUnlocker.get(idx);

            EXPECT_EQ(fullyUnlockedAmount[idx], FullyUnlockedInfo._Unlocked_Amount);
            EXPECT_EQ(fullyUnlockedUser[idx], FullyUnlockedInfo._Unlocked_ID);
        }
    }
};

class ContractTestingQearn : protected ContractTesting
{
public:
    ContractTestingQearn()
    {
        INIT_CONTRACT(QEARN);
        initEmptySpectrum();
        rand64.seed(42);
    }

    QearnChecker* getState()
    {
        return (QearnChecker*)contractStates[QEARN_CONTRACT_INDEX];
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QEARN_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QEARN_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    QEARN::getLockInforPerEpoch_output getLockInforPerEpoch(uint16 epoch)
    {
        QEARN::getLockInforPerEpoch_input input{ epoch };
        QEARN::getLockInforPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    uint64 getUserLockedInfor(uint16 epoch, const id& user)
    {
        QEARN::getUserLockedInfor_input input;
        input.epoch = epoch;
        input.user = user;
        QEARN::getUserLockedInfor_output output;
        callFunction(QEARN_CONTRACT_INDEX, 2, input, output);
        return output.LockedAmount;
    }

    uint32 getStateOfRound(uint16 epoch)
    {
        QEARN::getStateOfRound_input input{ epoch };
        QEARN::getStateOfRound_output output;
        callFunction(QEARN_CONTRACT_INDEX, 3, input, output);
        return output.state;
    }

    uint64 getUserLockStatus(const id& user)
    {
        QEARN::getUserLockStatus_input input{ user };
        QEARN::getUserLockStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 4, input, output);
        return output.status;
    }

    QEARN::getEndedStatus_output getEndedStatus(const id& user)
    {
        QEARN::getEndedStatus_input input{ user };
        QEARN::getEndedStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 5, input, output);
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
        input.Amount = amount;
        input.Locked_Epoch = lockedEpoch;
        QEARN::unlock_output output;
        EXPECT_EQ(invokeUserProcedure(QEARN_CONTRACT_INDEX, 2, input, output, user, 0), expectSuccess);
        return output.returnCode;
    }

    struct UserData
    {
        std::map<uint16, sint64> locked;
        std::map<uint16, sint64> unlockedInEpoch;
    };

    std::map<id, UserData> allUserData;

    struct EpochData
    {
        unsigned long long bonusAmount;
        unsigned long long amountCurrentlyLocked;
    };

    std::map<uint16, EpochData> allEpochData;

    void simulateDonation(const unsigned long long donationAmount)
    {
        increaseEnergy(QEARN_CONTRACT_ID, donationAmount);

        unsigned long long& totalBonusAmount = allEpochData[system.epoch + 1].bonusAmount;
        totalBonusAmount += donationAmount;
        if (totalBonusAmount > QEARN_MAX_BONUS_AMOUNT)
            totalBonusAmount = QEARN_MAX_BONUS_AMOUNT;
    }

    bool lockAndCheck(const id& user, long long amountLock, bool expectSuccess = true)
    {
        // get amount and balances before action
        uint64 amountBefore = getUserLockedInfor(system.epoch, user);
        sint64 userBalanceBefore = getBalance(user);
        sint64 contractBalanceBefore = getBalance(QEARN_CONTRACT_ID);

        // call lock prcoedure
        uint32 retCode = lock(user, amountLock, expectSuccess);

        // check new amount and balances
        uint64 amountAfter = getUserLockedInfor(system.epoch, user);
        sint64 userBalanceAfter = getBalance(user);
        sint64 contractBalanceAfter = getBalance(QEARN_CONTRACT_ID);
        if (retCode == QEARN_LOCK_SUCCESS && expectSuccess)
        {
            EXPECT_EQ(amountAfter, amountBefore + amountLock);
            EXPECT_EQ(userBalanceAfter, userBalanceBefore - amountLock);
            EXPECT_EQ(contractBalanceAfter, contractBalanceBefore + amountLock);

            allUserData[user].locked[system.epoch] += amountLock;
            allEpochData[system.epoch].amountCurrentlyLocked += amountLock;
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
        if (amountLock < QEARN_MINIMUM_LOCKING_AMOUNT)
        {
            EXPECT_EQ(retCode, QEARN_INVALID_INPUT_AMOUNT);
        }
        else if (amountBefore + amountLock > QEARN_MAX_LOCK_AMOUNT)
        {
            EXPECT_EQ(retCode, QEARN_LIMIT_LOCK);
        }

        return retCode == QEARN_LOCK_SUCCESS;
    }

    void endEpochAndCheck()
    {
        uint16 payoutEpoch = system.epoch - 52;
        getState()->checkLockerArray();
        getState()->checkGetUnlockedInfo(payoutEpoch);

        bool checkPayout = (allEpochData.find(payoutEpoch) != allEpochData.end());
        std::map<id, long long> expectedUserBalance;
        if (checkPayout)
        {
            const auto scEpochInfo = getLockInforPerEpoch(payoutEpoch);
            const EpochData& ed = allEpochData[payoutEpoch];
            const unsigned long long rewardFactorTenmillionth = QPI::div(ed.bonusAmount * 10000000ULL, ed.amountCurrentlyLocked);
            if (rewardFactorTenmillionth)
            {
                // detect overflow in computation of rewardFactorTenmillionth
                const double rewardFactorTenmillionthDouble = ed.bonusAmount * 10000000.0 / ed.amountCurrentlyLocked;
                double arthmeticError = double(rewardFactorTenmillionth) - rewardFactorTenmillionthDouble;
                EXPECT_LT(fabs(arthmeticError), 1e5);
            }

            EXPECT_EQ(ed.bonusAmount, scEpochInfo.CurrentBonusAmount);
            EXPECT_EQ(ed.amountCurrentlyLocked, scEpochInfo.CurrentLockedAmount);
            EXPECT_EQ(rewardFactorTenmillionth, scEpochInfo.Yield);

            unsigned long long totalRewardsPaid = 0;
            for (const auto& userIdDataPair : allUserData)
            {
                const id& user = userIdDataPair.first;
                const UserData& userData = userIdDataPair.second;
                const long long userBalance = getBalance(user);
                auto userLockedAmountIter = userData.locked.find(payoutEpoch);
                const long long userLockedAmount = (userLockedAmountIter == userData.locked.end()) ? 0 : userLockedAmountIter->second;
                const unsigned long long userReward = userLockedAmount * rewardFactorTenmillionth / 10000000ULL;
                if (rewardFactorTenmillionth)
                    EXPECT_EQ((userLockedAmount * rewardFactorTenmillionth) / rewardFactorTenmillionth, userLockedAmount);
                totalRewardsPaid += userReward;
                expectedUserBalance[user] = userBalance + userLockedAmount + userReward;
            }

            // all the bonus that has not been paid is burned (remainder due to inaccurate arithmetic and full bonus if nothing is locked until the end)
            EXPECT_LE(totalRewardsPaid, ed.bonusAmount);
            if (ed.amountCurrentlyLocked)
                EXPECT_LE(ed.bonusAmount - totalRewardsPaid, 10000000ull);
            else
                EXPECT_EQ(totalRewardsPaid, 0ull);
        }

        endEpoch();

        if (checkPayout)
        {
            const EpochData& ed = allEpochData[payoutEpoch];
            for (const auto& userIdBalancePair : expectedUserBalance)
            {
                const id& user = userIdBalancePair.first;
                const long long expectedUserBalance = userIdBalancePair.second;
                const long long actualUserBalance = getBalance(user);
                EXPECT_EQ(expectedUserBalance, actualUserBalance);
            }
        }

        getState()->checkLockerArray();
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


TEST(TestContractQearn, CheckLockErrors)
{
    ContractTestingQearn qearn;
    id user(1, 2, 3, 4);

    system.epoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;

    qearn.beginEpoch();

    // test cases, for which procedures is not executed:
    {
        // 1. non-existing entities = invalid ID)
        EXPECT_FALSE(qearn.lockAndCheck(id::zero(), QEARN_MAX_LOCK_AMOUNT, false));
        EXPECT_FALSE(qearn.lockAndCheck(id(1, 2, 3, 4), QEARN_MAX_LOCK_AMOUNT, false));

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
#if 0
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
}


TEST(TestContractQearn, RandomLockWithoutUnlock)
{
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    const uint16 lastEpoch = firstEpoch + 1;

    constexpr unsigned int totalUsers = 40;
    constexpr unsigned int maxUserLocking = 40;

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

TEST(TestContractQearn, RandomLockAndUnlock)
{
    ContractTestingQearn qearn;

    const uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    const uint16 lastEpoch = firstEpoch + 60;

    constexpr unsigned int totalUsers = 40;
    constexpr unsigned int maxUserLocking = 40;

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
        std::map<id, unsigned long long> UnlockedAmount;
        for (const auto& user : lockUsers)
        {
            auto it = UnlockedAmount.find(user); 

            if (it == UnlockedAmount.end()) 
            {
                UnlockedAmount[user] = 0;
            }
            for (sint32 LockedEpoch = system.epoch; LockedEpoch > system.epoch - 52; LockedEpoch--)
            {
                // get old amount and random amount for unlocking
                uint64 amountBefore = qearn.getUserLockedInfor(LockedEpoch, user);
                uint64 amountUnLock = random(1000000000);

                // call Unlock prcoedure
                uint32 retCode = qearn.unlock(user, amountUnLock, LockedEpoch);

                // check amount
                uint64 amountAfter = qearn.getUserLockedInfor(LockedEpoch, user);
                if (retCode == QEARN_UNLOCK_SUCCESS)
                {
                    if (amountBefore - amountUnLock < QEARN_MINIMUM_LOCKING_AMOUNT)
                    {
                        EXPECT_EQ(amountAfter, 0);
                        if (LockedEpoch != system.epoch)
                        {
                            UnlockedAmount[user] += amountBefore;
                        }
                    }
                    else 
                    {
                        EXPECT_EQ(amountBefore, amountAfter + amountUnLock);
                        if (LockedEpoch != system.epoch)
                        {
                            UnlockedAmount[user] += amountUnLock;
                        }
                    }
                }
                else
                {
                    EXPECT_EQ(amountAfter, amountBefore);
                }
            }
        }

        // check unlocked amounts
        for (const auto& user : lockUsers)
        {
            QEARN::getEndedStatus_output Unlocked_result = qearn.getEndedStatus(user);
            EXPECT_EQ(UnlockedAmount[user], Unlocked_result.Early_Unlocked_Amount);
        }

        // invoke END_EPOCH and check correct payouts
        qearn.endEpochAndCheck();

        // send revenue donation to qearn contract (happens after END_EPOCH but before system.epoch is incremented and before BEGIN_EPOCH
        qearn.simulateDonation(random(ISSUANCE_RATE));
    }
    uint32 state = qearn.getStateOfRound(firstEpoch);
}

#endif
