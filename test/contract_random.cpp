#define NO_UEFI

#include <cstring>
#include "contract_testing.h"

class ContractTestingRandom : public ContractTesting
{
public:
	ContractTestingRandom()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(RANDOM);
		callSystemProcedure(0, INITIALIZE);
	}

	// Commit+reveal convenience
	void commit(const id& miner, const bit_4096& commitBits, uint64_t deposit)
	{
		increaseEnergy(miner, deposit *2); // ensure enough QU
		RANDOM::RevealAndCommit_input inp{};
		inp.committedDigest = k12Digest(commitBits); // Use real K12 digest for test
		RANDOM::RevealAndCommit_output out{};
		invokeUserProcedure(0,1, inp, out, miner, deposit);
	}

	void revealAndCommit(const id& miner, const bit_4096& revealBits, const bit_4096& newCommitBits, uint64_t deposit)
	{
		RANDOM::RevealAndCommit_input inp{};
		inp.revealedBits = revealBits;
		inp.committedDigest = k12Digest(newCommitBits);
		RANDOM::RevealAndCommit_output out{};
		invokeUserProcedure(0,1, inp, out, miner, deposit);
	}

	void stopMining(const id& miner, const bit_4096& revealBits)
	{
		RANDOM::RevealAndCommit_input inp{};
		inp.revealedBits = revealBits;
		inp.committedDigest = id::zero();
		RANDOM::RevealAndCommit_output out{};
		invokeUserProcedure(0,1, inp, out, miner,0);
	}

	bool buyEntropy(const id& buyer, uint32_t numBytes, uint64_t minMinerDeposit, uint64_t suggestedFee, bool expectSuccess)
	{
		increaseEnergy(buyer, suggestedFee +10000);
		RANDOM::BuyEntropy_input inp{};
		inp.numberOfBytes = numBytes;
		inp.minMinerDeposit = minMinerDeposit;
		RANDOM::BuyEntropy_output out{};
		invokeUserProcedure(0,2, inp, out, buyer, suggestedFee);
		if (expectSuccess)
			EXPECT_TRUE(out.success);
		else
			EXPECT_FALSE(out.success);
		return out.success;
	}

	// Direct call to get price
	uint64_t queryPrice(uint32_t numBytes, uint64_t minMinerDeposit)
	{
		RANDOM::QueryPrice_input q{};
		q.numberOfBytes = numBytes;
		q.minMinerDeposit = minMinerDeposit;
		RANDOM::QueryPrice_output o{};
		callFunction(0,3, q, o);
		return o.price;
	}

	// Helper entropy/id for test readability
	static bit_4096 testBits(uint64_t v) {
		bit_4096 b{};
		uint64_t* ptr = reinterpret_cast<uint64_t*>(&b);
		for (int i =0; i <64; ++i) ptr[i] = v ^ (0xDEADBEEF12340000ULL | i);
		return b;
	}
	static id testId(uint64_t base) {
		id d = id::zero();
		for (int i =0; i <32; ++i) d.m256i_u8[i] = uint8_t((base >> (i %8)) + i);
		return d;
	}
	static id k12Digest(const bit_4096& b) {
		id digest = id::zero();
		KangarooTwelve(&b, sizeof(b), &digest, sizeof(digest));
		return digest;
	}
};

//------------------------------
// TEST CASES
//------------------------------

// Helper macros for tick/time simulation
#define SET_TICK(val) (system.tick = (val))
#define GET_TICK() (system.tick)
// To simulate an empty tick, set numberTickTransactions to -1
#define SET_TICK_IS_EMPTY(val) (numberTickTransactions = ((val) ? -1 :0))

TEST(ContractRandom, BasicCommitRevealStop)
{
	ContractTestingRandom random;
	id miner = ContractTestingRandom::testId(10);
	bit_4096 E1 = ContractTestingRandom::testBits(101);
	bit_4096 E2 = ContractTestingRandom::testBits(202);

	random.commit(miner, E1, 1000);
	random.revealAndCommit(miner, E1, E2, 1000);
	random.stopMining(miner, E2);

	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
}

TEST(ContractRandom, TimeoutsAndRefunds)
{
	ContractTestingRandom random;
	id miner = ContractTestingRandom::testId(11);
	bit_4096 bits = ContractTestingRandom::testBits(303);

	random.commit(miner, bits, 2000);

	// Timeout: Advance tick past deadline
	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	int timeoutTick = GET_TICK() + co0.revealTimeoutTicks + 1;
	SET_TICK(timeoutTick);

	// Trigger timeout (choose any call, including another commit or dummy reveal)
	RANDOM::RevealAndCommit_input dummy = {};
	RANDOM::RevealAndCommit_output out{};
	random.invokeUserProcedure(0, 1, dummy, out, miner, 0);

	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
	EXPECT_EQ(co.lostDepositsRevenue, 2000);
}

TEST(ContractRandom, EmptyTickRefund)
{
	ContractTestingRandom random;
	id miner = ContractTestingRandom::testId(12);
	bit_4096 bits = ContractTestingRandom::testBits(404);

	random.commit(miner, bits, 3000);

	// Use GetContractInfo to get revealTimeoutTicks
	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	int refundTick = system.tick + co0.revealTimeoutTicks;
	system.tick = refundTick;
	numberTickTransactions = -1;

	// All deadlines expire on an empty tick: refund
	RANDOM::RevealAndCommit_input dummy = {};
	RANDOM::RevealAndCommit_output out{};
	random.invokeUserProcedure(0, 1, dummy, out, miner, 0);
	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
}

TEST(ContractRandom, BuyEntropyEligibility)
{
	ContractTestingRandom random;
	id miner = ContractTestingRandom::testId(13);
	id buyer = ContractTestingRandom::testId(14);
	bit_4096 bits = ContractTestingRandom::testBits(321);

	// No miners: should fail
	EXPECT_FALSE(random.buyEntropy(buyer, 8, 1000, 8000, false));

	// Commit/reveal
	random.commit(miner, bits, 1000);
	random.revealAndCommit(miner, bits, ContractTestingRandom::testBits(333), 1000);

	// Should now succeed
	EXPECT_TRUE(random.buyEntropy(buyer, 16, 1000, 16000, true));

	// Advance past freshness window, should fail again
	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	system.tick = system.tick + co0.revealTimeoutTicks + 1;
	EXPECT_FALSE(random.buyEntropy(buyer, 16, 1000, 16000, false));
}

TEST(ContractRandom, QueryPriceLogic)
{
	ContractTestingRandom random;
	RANDOM::QueryPrice_input q{};
	q.numberOfBytes = 16;
	q.minMinerDeposit = 1000;
	RANDOM::QueryPrice_output o{};
	random.callFunction(0, 3, q, o);
	uint64_t price = random.queryPrice(16, 1000);
	EXPECT_EQ(price, o.price);
}

TEST(ContractRandom, CompactionBehavior)
{
	ContractTestingRandom random;
	for (int i = 0; i < 10; ++i) {
		id miner = ContractTestingRandom::testId(100 + i);
		bit_4096 bits = ContractTestingRandom::testBits(1001 + i);
		random.commit(miner, bits, 5000);
		random.revealAndCommit(miner, bits, ContractTestingRandom::testBits(2001 + i), 5000);
		random.stopMining(miner, ContractTestingRandom::testBits(2001 + i));
	}
}

TEST(ContractRandom, MultipleMinersAndBuyers)
{
	ContractTestingRandom random;
	id minerA = ContractTestingRandom::testId(1001);
	id minerB = ContractTestingRandom::testId(1002);
	id buyer1 = ContractTestingRandom::testId(1003);
	id buyer2 = ContractTestingRandom::testId(1004);
	bit_4096 entropyA = ContractTestingRandom::testBits(5678);
	bit_4096 entropyB = ContractTestingRandom::testBits(6789);

	// Both miners commit, same deposit
	random.commit(minerA, entropyA, 10000);
	random.commit(minerB, entropyB, 10000);
	random.revealAndCommit(minerA, entropyA, ContractTestingRandom::testBits(8888), 10000);
	random.revealAndCommit(minerB, entropyB, ContractTestingRandom::testBits(9999), 10000);

	// Buyer1 can purchase with either miner as eligible
	EXPECT_TRUE(random.buyEntropy(buyer1, 8, 10000, 20000, true));
	// Buyer2 requires more security than available
	EXPECT_FALSE(random.buyEntropy(buyer2, 16, 20000, 35000, false));
}

TEST(ContractRandom, MaxCommitmentsAndEviction)
{
	ContractTestingRandom random;
	// Fill the commitments array
	const int N = 32;
	std::vector<id> miners;
	for (int i = 0; i < N; ++i) {
		miners.push_back(ContractTestingRandom::testId(300 + i));
		random.commit(miners.back(), ContractTestingRandom::testBits(1234 + i), 5555);
	}

	// Reveal all out-of-order, ensure compaction
	for (int i = N - 1; i >= 0; --i) {
		random.revealAndCommit(miners[i], ContractTestingRandom::testBits(1234 + i), ContractTestingRandom::testBits(2000 + i), 5555);
		random.stopMining(miners[i], ContractTestingRandom::testBits(2000 + i));
	}
}

TEST(ContractRandom, EndEpochDistribution)
{
	ContractTestingRandom random;
	id miner1 = ContractTestingRandom::testId(99);
	id miner2 = ContractTestingRandom::testId(98);
	bit_4096 e1 = ContractTestingRandom::testBits(501);
	bit_4096 e2 = ContractTestingRandom::testBits(502);

	random.commit(miner1, e1, 10000);
	random.revealAndCommit(miner1, e1, ContractTestingRandom::testBits(601), 10000);
	random.commit(miner2, e2, 10000);
	random.revealAndCommit(miner2, e2, ContractTestingRandom::testBits(602), 10000);

	id buyer = ContractTestingRandom::testId(90);
	uint64_t price = random.queryPrice(16, 10000);
	random.buyEntropy(buyer, 16, 10000, price, true);

	// Simulate EndEpoch
	random.callSystemProcedure(0, END_EPOCH);

	// After epoch, earnings pools should be zeroed and recentMinerCount cleared
	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.minerEarningsPool, 0);
	EXPECT_EQ(co.shareholderEarningsPool, 0);
	EXPECT_EQ(co.recentMinerCount, 0);
}

TEST(ContractRandom, RecentMinerEvictionPolicy)
{
	ContractTestingRandom random;
	const int maxMiners = RANDOM_MAX_RECENT_MINERS;
	std::vector<id> miners;
	auto baseDeposit = 1000;

	// Fill up to RANDOM_MAX_RECENT_MINERS, all with same deposit
	for (int i = 0; i < maxMiners; ++i) {
		auto miner = ContractTestingRandom::testId(5000 + i);
		miners.push_back(miner);
		random.commit(miner, random.testBits(7000 + i), baseDeposit);
		random.revealAndCommit(miner, random.testBits(7000 + i), random.testBits(8000 + i), baseDeposit);
	}
	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	EXPECT_EQ(co0.recentMinerCount, maxMiners);

	// Add new miner with higher deposit, should evict one of the previous (lowest deposit)
	id highMiner = ContractTestingRandom::testId(99999);
	random.commit(highMiner, random.testBits(55555), baseDeposit * 10);
	random.revealAndCommit(highMiner, random.testBits(55555), random.testBits(55566), baseDeposit * 10);

	RANDOM::GetContractInfo_input ci1{};
	RANDOM::GetContractInfo_output co1{};
	random.callFunction(0, 1, ci1, co1);
	EXPECT_EQ(co1.recentMinerCount, maxMiners);

	// All lower deposit miners except one likely evicted, highMiner should be present with max deposit
	int foundHigh = 0;
	RANDOM::GetUserCommitments_input inp{};
	RANDOM::GetUserCommitments_output out{};
	for (uint32_t i = 0; i < maxMiners; ++i) {
		inp.userId = highMiner;
		random.callFunction(0, 2, inp, out);
		if (out.commitmentCount > 0) foundHigh++;
	}
	EXPECT_EQ(foundHigh, 1);
}

TEST(ContractRandom, BuyerPickinessHighRequirement)
{
	ContractTestingRandom random;
	id miner = random.testId(721);
	id buyer = random.testId(722);
	uint64_t lowDeposit = 1000, highDeposit = 100000;

	random.commit(miner, random.testBits(100), lowDeposit);
	random.revealAndCommit(miner, random.testBits(100), random.testBits(101), lowDeposit);

	// As buyer, require higher min deposit than any available miner supplied
	EXPECT_FALSE(random.buyEntropy(buyer, 8, highDeposit, 10000, false));
}

TEST(ContractRandom, MixedDepositLevels)
{
	ContractTestingRandom random;
	id lowMiner = random.testId(1001);
	id highMiner = random.testId(1002);
	id buyer = random.testId(1003);

	random.commit(lowMiner, random.testBits(88), 1000);
	random.commit(highMiner, random.testBits(89), 100000);
	random.revealAndCommit(lowMiner, random.testBits(88), random.testBits(188), 1000);
	random.revealAndCommit(highMiner, random.testBits(89), random.testBits(189), 100000);

	EXPECT_TRUE(random.buyEntropy(buyer, 8, 1000, 10000, true));
	EXPECT_TRUE(random.buyEntropy(buyer, 8, 100000, 100000, true));
	EXPECT_FALSE(random.buyEntropy(buyer, 8, 100001, 100000, false));
}

TEST(ContractRandom, EmptyTickRefund_MultiMiners)
{
	ContractTestingRandom random;
	id m1 = random.testId(931);
	id m2 = random.testId(932);
	random.commit(m1, random.testBits(401), 5000);
	random.commit(m2, random.testBits(402), 7000);

	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	int tick = system.tick + co0.revealTimeoutTicks;
	system.tick = tick;
	numberTickTransactions = -1;

	RANDOM::RevealAndCommit_input dummy = {};
	RANDOM::RevealAndCommit_output out{};
	random.invokeUserProcedure(0, 1, dummy, out, m1, 0);
	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
	// test both miners' balances for refund if desired
}

TEST(ContractRandom, Timeout_MultiMiners)
{
	ContractTestingRandom random;
	id m1 = random.testId(7777);
	id m2 = random.testId(8888);
	random.commit(m1, random.testBits(111), 2000);
	random.commit(m2, random.testBits(112), 4000);
	RANDOM::GetContractInfo_input ci0{};
	RANDOM::GetContractInfo_output co0{};
	random.callFunction(0, 1, ci0, co0);
	int afterTimeout = system.tick + co0.revealTimeoutTicks + 1;
	system.tick = afterTimeout;

	RANDOM::RevealAndCommit_input dummy = {};
	RANDOM::RevealAndCommit_output out{};
	random.invokeUserProcedure(0, 1, dummy, out, m2, 0);

	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
	EXPECT_EQ(co.lostDepositsRevenue, 6000);
}

TEST(ContractRandom, MultipleBuyersEpochReset)
{
	ContractTestingRandom random;
	id miner = random.testId(1201);
	id buyer1 = random.testId(1301);
	id buyer2 = random.testId(1401);

	random.commit(miner, random.testBits(900), 8000);
	random.revealAndCommit(miner, random.testBits(900), random.testBits(901), 8000);

	EXPECT_TRUE(random.buyEntropy(buyer1, 8, 8000, 20000, true));
	EXPECT_TRUE(random.buyEntropy(buyer2, 16, 8000, 50000, true));

	random.callSystemProcedure(0, END_EPOCH);

	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.minerEarningsPool, 0);
	EXPECT_EQ(co.shareholderEarningsPool, 0);
	EXPECT_EQ(co.recentMinerCount, 0);
}

TEST(ContractRandom, QueryUserCommitmentsInfo)
{
	ContractTestingRandom random;
	id miner = random.testId(2001);

	random.commit(miner, random.testBits(1234), 10000);

	// Call GetUserCommitments for miner
	RANDOM::GetUserCommitments_input inp{};
	inp.userId = miner;
	RANDOM::GetUserCommitments_output out{};
	random.callFunction(0, 2, inp, out);
	EXPECT_GE(out.commitmentCount, 1);

	// Call GetContractInfo for global stats
	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_GE(co.totalCommits, 1);
}

TEST(ContractRandom, RejectInvalidDeposits)
{
	ContractTestingRandom random;
	id miner = random.testId(2012);

	// Try to commit invalid deposit (not a power of ten)
	RANDOM::RevealAndCommit_input inp{};
	inp.committedDigest = ContractTestingRandom::k12Digest(ContractTestingRandom::testBits(66));
	RANDOM::RevealAndCommit_output out{};
	// Use7777 which is not a power of ten, should not register a commitment
	random.invokeUserProcedure(0, 1, inp, out, miner, 7777);

	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
}

TEST(ContractRandom, BuyEntropyEdgeNumBytes)
{
	ContractTestingRandom random;
	id miner = random.testId(3031);
	id buyer = random.testId(3032);

	random.commit(miner, random.testBits(8888), 8000);
	random.revealAndCommit(miner, random.testBits(8888), random.testBits(8899), 8000);

	//1 byte (minimum)
	EXPECT_TRUE(random.buyEntropy(buyer, 1, 8000, 10000, true));
	//32 bytes (maximum)
	EXPECT_TRUE(random.buyEntropy(buyer, 32, 8000, 40000, true));
	//33 bytes (over contract max, should clamp or fail)
	EXPECT_FALSE(random.buyEntropy(buyer, 33, 8000, 50000, false));
}

TEST(ContractRandom, OutOfOrderRevealAndCompaction)
{
	ContractTestingRandom random;
	std::vector<id> miners;
	for (int i = 0; i < 8; ++i) {
		miners.push_back(random.testId(5400 + i));
		random.commit(miners.back(), random.testBits(8500 + i), 6000);
	}
	// Reveal/stop in random order
	random.revealAndCommit(miners[3], random.testBits(8500 + 3), random.testBits(9500 + 3), 6000);
	random.revealAndCommit(miners[1], random.testBits(8500 + 1), random.testBits(9500 + 1), 6000);
	random.stopMining(miners[3], random.testBits(9500 + 3));
	random.stopMining(miners[1], random.testBits(9500 + 1));
	// Now reveal/stop remainder
	for (int i = 0; i < 8; ++i) {
		if (i == 1 || i == 3) continue;
		random.revealAndCommit(miners[i], random.testBits(8500 + i), random.testBits(9500 + i), 6000);
		random.stopMining(miners[i], random.testBits(9500 + i));
	}
	RANDOM::GetContractInfo_input ci{};
	RANDOM::GetContractInfo_output co{};
	random.callFunction(0, 1, ci, co);
	EXPECT_EQ(co.activeCommitments, 0);
}
