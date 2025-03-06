#pragma once

#include "../platform/m256.h"
#include "platform/memory_util.h"

struct ContractAction
{
    enum Type {
        none = 0,
        quTransferType,
    };

    unsigned char type;

    union
    {
        struct QuTransfer
        {
            m256i sourcePublicKey;
            m256i destinationPublicKey;
            long long amount;
        } quTransfer;
    };
};


// Class for tracking changes in spectrum and universe during a contract procedure invocation
template <unsigned int maxActions>
class ContractActionTracker
{
public:
    bool allocBuffer()
    {
        actions = nullptr;
        return allocPoolWithErrorLog(L"ContractActionTracker", maxActions * sizeof(ContractAction), (void**)&actions, __LINE__);
    }

    void freeBuffer()
    {
        if (actions)
            freePool(actions);
    }

    // Called before every use, allocBuffer() needs to be called before.
    void init()
    {
        ASSERT(actions != nullptr);
        numActions = 0;
    }

    bool addQuTransfer(const m256i& sourcePublicKey, const m256i& destinationPublicKey, long long amount)
    {
        ASSERT(actions != nullptr);
        ASSERT(numActions <= maxActions);

        if (numActions == maxActions)
            return false;

        ContractAction& qa = actions[numActions++];
        qa.type = ContractAction::quTransferType;
        qa.quTransfer.sourcePublicKey = sourcePublicKey;
        qa.quTransfer.destinationPublicKey = destinationPublicKey;
        qa.quTransfer.amount = amount;

        return true;
    }

    long long getOverallQuTransferBalance(const m256i& publicKey)
    {
        long long amount = 0;
        for (unsigned int i = 0; i < numActions; ++i)
        {
            ContractAction& qa = actions[i];
            if (qa.type == ContractAction::quTransferType)
            {
                if (qa.quTransfer.sourcePublicKey == publicKey)
                    amount = amount - qa.quTransfer.amount;
                if (qa.quTransfer.destinationPublicKey == publicKey)
                    amount = amount + qa.quTransfer.amount;
            }
        }
        return amount;
    }

private:
    ContractAction* actions;
    unsigned int numActions;
};
