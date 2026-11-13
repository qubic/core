#pragma once

#include "platform/m256.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "contract_core/pre_qpi_def.h"
#include "qpi/qpi.h"
#include "qpi/impl/qpi_hash_map_impl.h"

// Which solution transactions of one tick are processed: of all the solutions a sender has in the tick,
// only the one with the largest transaction index. The others are superseded.
template <unsigned long long transactionsPerTick>
class AntTickSolutions
{
public:
    bool init()
    {
        if (!allocPoolWithErrorLog(L"AntTickSolutions::_largestIndexOfSender",
            sizeof(SenderMap), (void**)&_largestIndexOfSender, __LINE__))
        {
            return false;
        }
        reset();
        return true;
    }

    void deinit()
    {
        if (_largestIndexOfSender)
        {
            freePool(_largestIndexOfSender);
            _largestIndexOfSender = nullptr;
        }
    }

    void reset()
    {
        _largestIndexOfSender->reset();
        setMem(_superseded, sizeof(_superseded), 0);
    }

    // Call once for every solution transaction of the tick, in any order.
    void add(const m256i& sender, unsigned int transactionIndex)
    {
        if (transactionIndex >= transactionsPerTick)
        {
            return;
        }
        unsigned int earlier = 0;
        const bool senderKnown = _largestIndexOfSender->get(sender, earlier);
        if (!senderKnown)
        {
            _largestIndexOfSender->set(sender, transactionIndex);
        }
        else if (transactionIndex > earlier)
        {
            _superseded[earlier] = 1;
            _largestIndexOfSender->set(sender, transactionIndex);
        }
        else if (transactionIndex < earlier)
        {
            _superseded[transactionIndex] = 1;
        }
    }

    bool isSuperseded(unsigned int transactionIndex) const
    {
        return transactionIndex < transactionsPerTick && _superseded[transactionIndex] != 0;
    }

private:
    // Twice the number of transactions, so the map is never more than half full.
    typedef QPI::HashMap<m256i, unsigned int, 2 * transactionsPerTick> SenderMap;

    SenderMap* _largestIndexOfSender;
    unsigned char _superseded[transactionsPerTick];
};
