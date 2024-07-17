#pragma once
#include "platform/memory.h"
#define VOTE_COUNTER_DATA_SIZE_IN_BYTES 848
#define VOTE_COUNTER_NUM_BIT_PER_COMP 10
static_assert((1<< VOTE_COUNTER_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(VOTE_COUNTER_DATA_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * VOTE_COUNTER_NUM_BIT_PER_COMP, "Invalid data size");

class VoteCounter
{
private:
	unsigned int votes[NUMBER_OF_COMPUTORS*2][NUMBER_OF_COMPUTORS];
	unsigned int buffer[NUMBER_OF_COMPUTORS];
protected:
	unsigned int extract10Bit(unsigned char* data, unsigned int idx)
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

public:
	void init()
	{
		setMem(votes, sizeof(votes), 0);
	}
	void registerVote(unsigned int tick, int computorIdx)
	{
		unsigned int slotId = tick % (NUMBER_OF_COMPUTORS * 2);
		votes[slotId][computorIdx] = tick;
	}

	// get and compress number of votes of 676 computors to 676x10 bit numbers between [fromTick, toTick)
	void compressVotePacket(unsigned int fromTick, unsigned int toTick, unsigned char votePacket[VOTE_COUNTER_DATA_SIZE_IN_BYTES])
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
		for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
		{
			update10Bit(votePacket, i, buffer[i]);
		}
	}
};