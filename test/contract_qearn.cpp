#if 0
#define NO_UEFI

#include <random>
#include <map>

#include "contract_testing.h"


std::mt19937_64 rand64;

class QearnChecker : public QEARN
{
public:
    void checkLockerArray()
    {
        // check that locker array is in consistent state
#if 1
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
                EXPECT_LE(lock._Locked_Amount, 1000000000000ULL);
                EXPECT_FALSE(isZero(lock.ID));
                const QEARN::EpochIndexInfo& epochRange = EpochIndex.get(lock._Locked_Epoch);
                EXPECT_GE(idx, epochRange.startIndex);
                EXPECT_LT(idx, epochRange.endIndex);
                epochTotalLocked[lock._Locked_Epoch] += lock._Locked_Amount;
            }
        }

        for (const auto& epochAndTotal : epochTotalLocked)
        {
            const QEARN::RoundInfo& currentRoundInfo = _InitialRoundInfo.get(epochAndTotal.first);
            EXPECT_EQ(currentRoundInfo._Total_Locked_Amount, epochAndTotal.second);
            std::cout << "Total locked amount in epoch " << epochAndTotal.first << " = " << epochAndTotal.second << std::endl;
        }
#else
        for (uint64 epoch = 0; epoch < EpochIndex.capacity(); ++epoch)
        {
            uint64 sumLockedAmout = 0;
            const QEARN::EpochIndexInfo& epochIndex = EpochIndex.get(epoch);
            for (uint64 idx = epochIndex.startIndex; idx < epochIndex.endIndex; ++idx)
            {
                const QEARN::LockInfo& lock = Locker.get(idx);
                if (lock._Locked_Amount == 0)
                {
                    EXPECT_TRUE(isZero(lock.ID));
                    EXPECT_EQ(lock._Locked_Epoch, 0);
                }
                else
                {
                    EXPECT_LE(lock._Locked_Epoch, 1000000000000ULL);
                    EXPECT_FALSE(isZero(lock.ID));
                    EXPECT_EQ(lock._Locked_Epoch, epoch);
                    sumLockedAmout += lock._Locked_Amount;
                }
            }

            const QEARN::RoundInfo& currentRoundInfo = _InitialRoundInfo.get(epoch);
            EXPECT_EQ(currentRoundInfo._Total_Locked_Amount, sumLockedAmout);
        }
#endif
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

    QEARN::GetLockInforPerEpoch_output getLockInforPerEpoch(uint16 epoch)
    {
        QEARN::GetLockInforPerEpoch_input input{ epoch };
        QEARN::GetLockInforPerEpoch_output output;
        callFunction(QEARN_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    uint64 getUserLockedInfor(uint16 epoch, const id& user)
    {
        QEARN::GetUserLockedInfor_input input;
        input.epoch = epoch;
        input.user = user;
        QEARN::GetUserLockedInfor_output output;
        callFunction(QEARN_CONTRACT_INDEX, 2, input, output);
        return output.LockedAmount;
    }

    uint32 getStateOfRound(uint16 epoch)
    {
        QEARN::GetStateOfRound_input input{ epoch };
        QEARN::GetStateOfRound_output output;
        callFunction(QEARN_CONTRACT_INDEX, 3, input, output);
        return output.state;
    }

    uint64 getUserLockStatus(const id& user)
    {
        QEARN::GetUserLockStatus_input input{ user };
        QEARN::GetUserLockStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 4, input, output);
        return output.status;
    }

    QEARN::GetEndedStatus_output getEndedStatus(const id& user)
    {
        QEARN::GetEndedStatus_input input{ user };
        QEARN::GetEndedStatus_output output;
        callFunction(QEARN_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    sint32 lock(const id& user, long long amount)
    {
        QEARN::Lock_input input;
        QEARN::Lock_output output;
        invokeUserProcedure(QEARN_CONTRACT_INDEX, 1, input, output, user, amount);
        return output.returnCode;
    }

    sint32 unlock(const id& user, long long amount, uint16 lockedEpoch)
    {
        QEARN::Unlock_input input;
        input.Amount = amount;
        input.Locked_Epoch = lockedEpoch;
        QEARN::Unlock_output output;
        invokeUserProcedure(QEARN_CONTRACT_INDEX, 2, input, output, user, 0);
        return output.returnCode;
    }
};


static id getUser(unsigned long long i)
{
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

unsigned long long random(unsigned long long maxValue)
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
        unsigned long long userIdx = random(totalUsers);
        users.push_back(getUser(userIdx));
    }
    return users;
}

TEST(TestContractQearn, RandomTest)
{
    ContractTestingQearn qearn;

    uint16 firstEpoch = contractDescriptions[QEARN_CONTRACT_INDEX].constructionEpoch;
    uint16 lastEpoch = firstEpoch + 60;

    constexpr unsigned int totalUsers = 20;
    constexpr unsigned int maxUserLocking = 20;

    for (system.epoch = firstEpoch; system.epoch <= lastEpoch; ++system.epoch)
    {
        // send revenue donation to qearn contract
        unsigned long long revenueDonationAmount = random(1000000000);
        increaseEnergy(id(QEARN_CONTRACT_INDEX, 0, 0, 0), revenueDonationAmount);

        qearn.beginEpoch();

        // locking
        auto lockUsers = getRandomUsers(totalUsers, maxUserLocking);
        for (const auto& user : lockUsers)
        {
            // get old amount and random amount for locking
            uint64 amountBefore = qearn.getUserLockedInfor(system.epoch, user);
            uint64 amountLock = random(1000000000);

            // make user that user has enough qus
            increaseEnergy(user, amountLock);

            // call lock prcoedure
            uint32 retCode = qearn.lock(user, amountLock);

            // check amount
            uint64 amountAfter = qearn.getUserLockedInfor(system.epoch, user);
            if (retCode == QEARN_LOCK_SUCCESS)
            {
                EXPECT_EQ(amountAfter, amountBefore + amountLock);
            }
            else
            {
                EXPECT_EQ(amountAfter, amountBefore);
            }
        }

        // TODO: random unlocking in current and prior epochs

        qearn.getState()->checkLockerArray();

        // this messes up the locker array state at the moment, please fix the bugs
        qearn.endEpoch();

        qearn.getState()->checkLockerArray();
    }
    uint32 state = qearn.getStateOfRound(128);

}
#endif
