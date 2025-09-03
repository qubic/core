#pragma once

// Include this first, to ensure "logging/logging.h" isn't included before the custom LOG_BUFFER_SIZE has been defined
#include "logging_test.h"

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// make test example contracts available in all compile units
#define INCLUDE_CONTRACT_TEST_EXAMPLES

#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include "contract_core/qpi_spectrum_impl.h"
#include "contract_core/qpi_asset_impl.h"
#include "contract_core/qpi_system_impl.h"
#include "contract_core/qpi_ticking_impl.h"
#include "contract_core/qpi_ipo_impl.h"
#include "contract_core/qpi_mining_impl.h"

#include "test_util.h"


class ContractTesting : public LoggingTest
{
public:
    ContractTesting()
    {

#ifdef __AVX512F__
        initAVX512FourQConstants();
#endif
        initCommonBuffers();
        initContractExec();
        initSpecialEntities();

        contractStates[0] = (unsigned char*)malloc(contractDescriptions[0].stateSize);
        setMem(contractStates[0], contractDescriptions[0].stateSize, 0);
    }

    ~ContractTesting()
    {
        deinitSpecialEntities();
        deinitAssets();
        deinitSpectrum();
        deinitCommonBuffers();
        deinitContractExec();
        for (unsigned int i = 0; i < contractCount; ++i)
        {
            if (contractStates[i])
            {
                free(contractStates[i]);
                contractStates[i] = nullptr;
            }
        }
    }

    void initEmptySpectrum()
    {
        initSpectrum();
        memset(spectrum, 0, spectrumSizeInBytes);
        updateSpectrumInfo();
    }

    void initEmptyUniverse()
    {
        initAssets();
        memset(assets, 0, universeSizeInBytes);
        as.indexLists.reset();
    }

    template <typename InputType, typename OutputType>
    unsigned int callFunction(unsigned int contractIndex, unsigned short functionInputType, const InputType& input, OutputType& output, bool checkInputSize = true, bool expectSuccess = true) const
    {
        EXPECT_LT(contractIndex, contractCount);
        EXPECT_NE(contractStates[contractIndex], nullptr);
        QpiContextUserFunctionCall qpiContext(contractIndex);
        if (checkInputSize)
        {
            unsigned short expectedInputSize = contractUserFunctionInputSizes[contractIndex][functionInputType];
            EXPECT_EQ((int)expectedInputSize, sizeof(input));
        }
        unsigned int errorCode = qpiContext.call(functionInputType, &input, sizeof(input));
        EXPECT_EQ((int)qpiContext.outputSize, sizeof(output));
        if (expectSuccess)
        {
            EXPECT_EQ(errorCode, 0);
        }
        copyMem(&output, qpiContext.outputBuffer, sizeof(output));
        qpiContext.freeBuffer();
        return errorCode;
    }

    template <typename InputType, typename OutputType>
    bool invokeUserProcedure(
        unsigned int contractIndex, unsigned short procedureInputType, const InputType& input, OutputType& output,
        const id& user, sint64 amount,
        bool checkInputSize = true, bool expectSuccess = true)
    {
        // check inputs and init output
        EXPECT_LT(contractIndex, contractCount);
        EXPECT_NE(contractStates[contractIndex], nullptr);
        if (checkInputSize)
        {
            unsigned short expectedInputSize = contractUserProcedureInputSizes[contractIndex][procedureInputType];
            EXPECT_EQ((int)expectedInputSize, sizeof(input));
        }
        setMemory(output, 0);

        // transfer amount (fee / invocation reward)
        int userSpectrumIndex = spectrumIndex(user);
        if (userSpectrumIndex < 0 || !decreaseEnergy(userSpectrumIndex, amount))
            return false;
        increaseEnergy(id(contractIndex, 0, 0, 0), amount);

        // run callback for incoming transfer of amount / fee / invocation reward
        if (amount > 0 && contractSystemProcedures[contractIndex][POST_INCOMING_TRANSFER])
        {
            QpiContextSystemProcedureCall qpiContext(contractIndex, POST_INCOMING_TRANSFER);
            QPI::PostIncomingTransfer_input input{ user, amount, QPI::TransferType::procedureTransaction };
            qpiContext.call(input);
        }

        // run user procedure
        QpiContextUserProcedureCall qpiContext(contractIndex, user, amount);
        qpiContext.call(procedureInputType, &input, sizeof(input));

        // check results, copy output and cleanup
        EXPECT_EQ((int)qpiContext.outputSize, sizeof(output));
        if (expectSuccess)
        {
            EXPECT_EQ(contractError[contractIndex], 0);
        }
        copyMem(&output, qpiContext.outputBuffer, sizeof(output));
        qpiContext.freeBuffer();
        return true;
    }

    void callSystemProcedure(unsigned int contractIndex, SystemProcedureID sysProcId, bool expectSuccess = true)
    {
        EXPECT_LT(contractIndex, contractCount);
        EXPECT_NE(contractStates[contractIndex], nullptr);
        QpiContextSystemProcedureCall qpiContext(contractIndex, sysProcId);
        qpiContext.call();
        if (expectSuccess)
        {
            EXPECT_EQ(contractError[contractIndex], 0);
        }
    }
};

#define INIT_CONTRACT(contractName) { \
    constexpr unsigned int contractIndex = contractName##_CONTRACT_INDEX; \
    EXPECT_LT(contractIndex, contractCount); \
    const unsigned long long size = contractDescriptions[contractIndex].stateSize; \
    contractStates[contractIndex] = (unsigned char*)malloc(size); \
    setMem(contractStates[contractIndex], size, 0); \
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(contractName); \
}

static inline long long getBalance(const id& pubKey)
{
    int index = spectrumIndex(pubKey);
    if (index < 0)
        return 0;
    long long balance = energy(index);
    EXPECT_GE(balance, 0ll);
    return balance;
}

// Update time returned by QPI functions based on utcTime, which can be set to current time with updateTime().
static inline void updateQpiTime()
{
    etalonTick.millisecond = utcTime.Nanosecond / 1000000;
    etalonTick.second = utcTime.Second;
    etalonTick.minute = utcTime.Minute;
    etalonTick.hour = utcTime.Hour;
    etalonTick.day = utcTime.Day;
    etalonTick.month = utcTime.Month;
    etalonTick.year = utcTime.Year - 2000;
}

// Check that the contract execution system state is clean (before / after running contracts).
static inline void checkContractExecCleanup()
{
    for (unsigned int i = 0; i < contractCount; ++i)
    {
        EXPECT_EQ(contractStateLock[i].getCurrentReaderLockCount(), 0);
    }

    for (unsigned int i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS; ++i)
    {
        EXPECT_EQ(contractLocalsStack[i].size(), 0);
        EXPECT_EQ(contractLocalsStackLock[i], 0);
    }
    EXPECT_EQ(contractLocalsStackLockWaitingCount, 0);
    EXPECT_EQ(contractCallbacksRunning, NoContractCallback);
}

// Issue contract shares and transfer ownership/possession of all shares to one entity
static inline void issueContractShares(unsigned int contractIndex, std::vector<std::pair<m256i, unsigned int>>& initialOwnerShares)
{
    int issuanceIndex, ownershipIndex, possessionIndex, dstOwnershipIndex, dstPossessionIndex;
    EXPECT_EQ(issueAsset(m256i::zero(), (char*)contractDescriptions[contractIndex].assetName, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex), NUMBER_OF_COMPUTORS);

    int totalShareCount = 0;
    for (const auto& ownerShareCountPair : initialOwnerShares)
        totalShareCount += ownerShareCountPair.second;
    EXPECT_LE(totalShareCount, NUMBER_OF_COMPUTORS);
    if (totalShareCount < NUMBER_OF_COMPUTORS)
    {
        std::cout << "Warning: issueContractShares() called with " << NUMBER_OF_COMPUTORS - totalShareCount << " less then expected shares, adding remaining shares to first owner." << std::endl;
        initialOwnerShares[0].second += NUMBER_OF_COMPUTORS - totalShareCount;
    }

    for (const auto& ownerShareCountPair : initialOwnerShares)
    {
        EXPECT_TRUE(transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, ownerShareCountPair.first, ownerShareCountPair.second, &dstOwnershipIndex, &dstPossessionIndex, true));
    }
}
