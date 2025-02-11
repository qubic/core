#pragma once

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

// reduced size of logging buffer (512 MB instead of 8 GB)
#define LOG_BUFFER_SIZE (2*268435456ULL)

// make test example contracts available in all compile units
#define INCLUDE_CONTRACT_TEST_EXAMPLES

#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include "contract_core/qpi_spectrum_impl.h"
#include "contract_core/qpi_asset_impl.h"
#include "contract_core/qpi_system_impl.h"
#include "contract_core/qpi_ticking_impl.h"

#include "logging_test.h"

#include "test_util.h"


class ContractTesting : public LoggingTest
{
public:
    ContractTesting()
    {
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
    void callFunction(unsigned int contractIndex, unsigned short functionInputType, const InputType& input, OutputType& output, bool checkInputSize = true, bool expectSuccess = true) const
    {
        EXPECT_LT(contractIndex, contractCount);
        EXPECT_NE(contractStates[contractIndex], nullptr);
        QpiContextUserFunctionCall qpiContext(contractIndex);
        if (checkInputSize)
        {
            unsigned short expectedInputSize = contractUserFunctionInputSizes[contractIndex][functionInputType];
            EXPECT_EQ((int)expectedInputSize, sizeof(input));
        }
        qpiContext.call(functionInputType, &input, sizeof(input));
        EXPECT_EQ((int)qpiContext.outputSize, sizeof(output));
        if (expectSuccess)
        {
            EXPECT_EQ(contractError[contractIndex], 0);
        }
        copyMem(&output, qpiContext.outputBuffer, sizeof(output));
        qpiContext.freeBuffer();
    }

    template <typename InputType, typename OutputType>
    bool invokeUserProcedure(
        unsigned int contractIndex, unsigned short procedureInputType, const InputType& input, OutputType& output,
        const id& user, sint64 amount,
        bool checkInputSize = true, bool expectSuccess = true)
    {
        EXPECT_LT(contractIndex, contractCount);
        EXPECT_NE(contractStates[contractIndex], nullptr);
        setMemory(output, 0);
        int userSpectrumIndex = spectrumIndex(user);
        if (userSpectrumIndex < 0 || !decreaseEnergy(userSpectrumIndex, amount))
            return false;
        increaseEnergy(id(contractIndex, 0, 0, 0), amount);
        QpiContextUserProcedureCall qpiContext(contractIndex, user, amount);
        if (checkInputSize)
        {
            unsigned short expectedInputSize = contractUserProcedureInputSizes[contractIndex][procedureInputType];
            EXPECT_EQ((int)expectedInputSize, sizeof(input));
        }
        qpiContext.call(procedureInputType, &input, sizeof(input));
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
