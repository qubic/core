#pragma once

#include "gtest/gtest.h"

// workaround for name clash with stdlib
#define system qubicSystemStruct

#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include "contract_core/qpi_spectrum_impl.h"
#include "contract_core/qpi_asset_impl.h"
#include "contract_core/qpi_system_impl.h"


class ContractTesting
{
public:
    ContractTesting()
    {
        initCommonBuffers();
        initContractExec();

        contractStates[0] = (unsigned char*)malloc(contractDescriptions[0].stateSize);
        setMem(contractStates[0], contractDescriptions[0].stateSize, 0);
    }

    ~ContractTesting()
    {
        deinitAssets();
        deinitSpectrum();
        deinitCommonBuffers();
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
    }

    long long getBalance(const id& pubKey) const
    {
        int index = spectrumIndex(pubKey);
        if (index < 0)
            return 0;
        long long balance = energy(index);
        EXPECT_GE(balance, 0ll);
        return balance;
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
        QpiContextSystemProcedureCall qpiContext(contractIndex);
        qpiContext.call(sysProcId);
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

static std::ostream& operator<<(std::ostream& s, const id& v)
{
    CHAR16 identityWchar[61];
    char identityChar[61];
    getIdentity(v.m256i_u8, identityWchar, false);
    size_t size;
    wcstombs_s(&size, identityChar, identityWchar, 61);
    s << identityChar;
    return s;
}
