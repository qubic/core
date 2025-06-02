#pragma once
#include "platform/memory.h"
#include "network_messages/transactions.h"
#define VOTE_COUNTER_INPUT_TYPE 1
#define VOTE_COUNTER_DATA_SIZE_IN_BYTES 848
#define VOTE_COUNTER_NUM_BIT_PER_COMP 10
static_assert((1<< VOTE_COUNTER_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(VOTE_COUNTER_DATA_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * VOTE_COUNTER_NUM_BIT_PER_COMP, "Invalid data size");

class VoteCounter
{
private:
	unsigned int votes[NUMBER_OF_COMPUTORS*2][NUMBER_OF_COMPUTORS];
	unsigned long long accumulatedVoteCount[NUMBER_OF_COMPUTORS];
	unsigned int buffer[NUMBER_OF_COMPUTORS];
protected:
	unsigned int extract10Bit(const unsigned char* data, unsigned int idx)
	{
		//TODO: simplify this
		unsigned int byte0 = data[idx + (idx >> 2)];
		unsigned int byte1 = data[idx + (idx >> 2) + 1];
		unsigned int last_bit0 = 8 - (idx & 3) * 2;
		unsigned int first_bit1 = 10 - last_bit0;
		unsigned int res = (byte0 & ((1 << last_bit0) - 1)) << first_bit1;
		res |= (byte1 >> (8 - first_bit1));
		return res;
	}
	void update10Bit(unsigned char* data, unsigned int idx, unsigned int value)
	{
		//TODO: simplify this
		unsigned char& byte0 = data[idx + (idx >> 2)];
		unsigned char& byte1 = data[idx + (idx >> 2) + 1];
		unsigned int last_bit0 = 8 - (idx & 3) * 2;
		unsigned int first_bit1 = 10 - last_bit0;
		unsigned char mask0 = ~((1 << last_bit0) - 1);
		unsigned char mask1 = ((1 << (8-first_bit1)) - 1);
		byte0 &= mask0;
		byte1 &= mask1;
		unsigned char ubyte0 = (value >> first_bit1);
		unsigned char ubyte1 = (value & ((1 << first_bit1) - 1)) << (8-first_bit1);
		byte0 |= ubyte0;
		byte1 |= ubyte1;
	}

	void accumulateVoteCount(unsigned int computorIdx, unsigned int value)
	{
		accumulatedVoteCount[computorIdx] += value;
	}

public:
	static constexpr unsigned int VoteCounterDataSize = sizeof(votes) + sizeof(accumulatedVoteCount);
	void init()
	{
		setMem(votes, sizeof(votes), 0);
		setMem(accumulatedVoteCount, sizeof(accumulatedVoteCount), 0);
	}
	
	void registerNewVote(unsigned int tick, unsigned int computorIdx)
	{
		unsigned int slotId = tick % (NUMBER_OF_COMPUTORS * 2);
		votes[slotId][computorIdx] = tick;
	}

	// get and compress number of votes of 676 computors to 676x10 bit numbers between [fromTick, toTick)
	void compressNewVotesPacket(unsigned int fromTick, unsigned int toTick, unsigned int computorIdx, unsigned char votePacket[VOTE_COUNTER_DATA_SIZE_IN_BYTES])
	{
		setMem(votePacket, sizeof(votePacket), 0);
		setMem(buffer, sizeof(buffer), 0);
		for (unsigned int i = fromTick; i < toTick; i++)
		{
			unsigned int slotId = i % (NUMBER_OF_COMPUTORS * 2);
			for (int j = 0; j < NUMBER_OF_COMPUTORS; j++)
			{
				if (votes[slotId][j] == i)
				{
					buffer[j]++;
				}
			}
		}
		buffer[computorIdx] = 0; // remove self-report
		for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
		{
			update10Bit(votePacket, i, buffer[i]);
		}
	}

	bool validateNewVotesPacket(const unsigned char* votePacket, unsigned int computorIdx)
	{
		unsigned long long sum = 0;
		for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
		{
			buffer[i] = extract10Bit(votePacket, i);
			if (buffer[i] > NUMBER_OF_COMPUTORS)
			{
				return false;
			}
			sum += buffer[i];
		}
		// check #0: sum of all vote must be >= 675*451 (vote of the tick leader is removed)
		if (sum < (NUMBER_OF_COMPUTORS - 1) * QUORUM)
		{
			return false;
		}
		// check #1: own number of vote must be zero
		if (buffer[computorIdx] != 0)
		{
			return false;
		}
		return true;
	}

	void addVotes(const unsigned char* newVotePacket, unsigned int computorIdx)
	{
		if (validateNewVotesPacket(newVotePacket, computorIdx))
		{
			for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
			{
				unsigned int vote_count = extract10Bit(newVotePacket, i);
				accumulateVoteCount(i, vote_count);
			}
		}
	}

	void processTransactionData(const Transaction* transaction, const m256i& dataLock)
	{
#ifndef NO_UEFI
		int computorIndex = transaction->tick % NUMBER_OF_COMPUTORS;
		if (transaction->sourcePublicKey == broadcastedComputors.computors.publicKeys[computorIndex]) // this tx was sent by the tick leader of this tick
		{
			if (!transaction->amount
				&& transaction->inputSize == VOTE_COUNTER_DATA_SIZE_IN_BYTES + sizeof(m256i))
			{
				m256i txDataLock = m256i(transaction->inputPtr() + VOTE_COUNTER_DATA_SIZE_IN_BYTES);
				if (txDataLock == dataLock)
				{
					addVotes(transaction->inputPtr(), computorIndex);
				}
#ifndef NDEBUG
				else
				{
					CHAR16 dbg[256];
					setText(dbg, L"TRACE: [Vote counter tx] Wrong datalock from comp ");
					appendNumber(dbg, computorIndex, false);
					addDebugMessage(dbg);
				}
#endif
			}
		}
#endif
	}

	unsigned long long getVoteCount(unsigned int computorIdx)
	{
		return accumulatedVoteCount[computorIdx];
	}

	void saveAllDataToArray(unsigned char* dst)
	{
		copyMem(dst, &votes[0][0], sizeof(votes));
		copyMem(dst + sizeof(votes), &accumulatedVoteCount[0], sizeof(accumulatedVoteCount));
	}

	void loadAllDataFromArray(const unsigned char* src)
	{
		copyMem(&votes[0][0], src, sizeof(votes));
		copyMem(&accumulatedVoteCount[0], src + sizeof(votes), sizeof(accumulatedVoteCount));
	}
};