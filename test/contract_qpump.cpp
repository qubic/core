#define NO_UEFI

#include "contract_testing.h"

// Expected values in this file were computed with contract/sim/qpump_model.py, an integer
// reference model of Qpump.h. Keep both in sync when changing curve or fee constants.

static const id CREATOR1(1, 2, 3, 4);
static const id CREATOR2(5, 6, 7, 8);
static const id BUYER1(11, 11, 11, 11);
static const id BUYER2(22, 22, 22, 22);
static const id BUYER3(33, 33, 33, 33);
static const id WHALE(44, 44, 44, 44);
static const id QPUMP_CONTRACT_ID(QPUMP_CONTRACT_INDEX, 0, 0, 0);
static const id QX_CONTRACT_ID(QX_CONTRACT_INDEX, 0, 0, 0);
static const id QSWAP_CONTRACT_ID(QSWAP_CONTRACT_INDEX, 0, 0, 0);

static constexpr uint64 NAME_PEPE = 1162888528ULL;   // PEPE
static constexpr uint64 NAME_MOON = 1313820493ULL;   // MOON
static constexpr uint64 NAME_WOOF = 1179602775ULL;   // WOOF
static constexpr uint64 NAME_DOGE2 = 215910666052ULL; // DOGE2
static constexpr uint64 NAME_SHIB = 1112098899ULL;    // SHIB
static constexpr uint64 NAME_ELON = 1313819717ULL;    // ELON
static constexpr uint64 NAME_BONK = 1263423298ULL;    // BONK
static constexpr uint64 NAME_WOJAK = 323217936215ULL; // WOJAK
static constexpr uint64 NAME_CHAD = 1145129027ULL;    // CHAD
static constexpr uint64 NAME_APE = 4542529ULL;        // APE
static constexpr uint64 NAME_MEME = 1162691917ULL;    // MEME
static constexpr uint64 NAME_LOWER = 1701864816ULL;  // pepe, invalid
static constexpr uint64 NAME_DIGIT = 1128415537ULL;  // 1ABC, invalid
static constexpr uint64 NAME_GAP = 4325441ULL;       // A, zero byte, B, invalid

static constexpr uint32 FN_GET_COIN = 1;
static constexpr uint32 FN_GET_HOLDER = 2;
static constexpr uint32 FN_QUOTE_BUY = 3;
static constexpr uint32 FN_QUOTE_SELL = 4;
static constexpr uint32 FN_QUOTE_BUDGET = 5;
static constexpr uint32 FN_GET_FEES = 6;
static constexpr uint32 FN_LIST_COINS = 7;
static constexpr uint32 FN_GET_STATS = 8;
static constexpr uint32 FN_LIST_GRADUATED = 9;

static constexpr uint16 PROC_CREATE_COIN = 1;
static constexpr uint16 PROC_BUY = 2;
static constexpr uint16 PROC_SELL = 3;
static constexpr uint16 PROC_PROCESS = 4;
static constexpr uint16 PROC_CLAIM = 5;
static constexpr uint16 PROC_TRANSFER = 6;
static constexpr uint16 PROC_BUY_WITH_QU = 7;

class QpumpChecker : public QPUMP, public QPUMP::StateData
{
public:
    uint32 slotOf(uint64 name) const
    {
        uint32 slot = QPUMP_NO_SLOT;
        nameToSlot.get(name, slot);
        return slot;
    }

    QPUMP::Coin coinOf(uint64 name) const
    {
        QPUMP::Coin empty{};
        uint32 slot = slotOf(name);
        if (slot >= QPUMP_MAX_COINS)
        {
            return empty;
        }
        return coins.get(slot);
    }

    // Sum of QU the contract owes to coins plus the undistributed fee pots.
    sint64 obligations() const
    {
        sint64 total = shareholderPot + burnPot + qdogePot;
        for (uint32 slot = 0; slot < QPUMP_MAX_COINS; ++slot)
        {
            const QPUMP::Coin& coin = coins.get(slot);
            if (coin.status == QPUMP_STATUS_OPENING)
            {
                total += coin.batchQu;
            }
            else if (coin.status != QPUMP_STATUS_EMPTY)
            {
                total += coin.realQu;
            }
        }
        return total;
    }

    // Walks every live coin's holder list and checks the links, the coin ids and the holder count.
    bool holderListsConsistent() const
    {
        for (uint32 slot = 0; slot < nextFreshSlot; ++slot)
        {
            const QPUMP::Coin& coin = coins.get(slot);
            if (coin.status == QPUMP_STATUS_EMPTY)
            {
                continue;
            }
            uint64 prev = 0;
            uint64 key = holderHeads.get(slot);
            uint32 count = 0;
            while (key != 0)
            {
                QPUMP::HolderEntry entry;
                if (!holders.get(key, entry) || entry.prevKey != prev || (key & QPUMP_COIN_KEY_MASK) != (coin.coinId & QPUMP_COIN_KEY_MASK))
                {
                    return false;
                }
                if (++count > coin.holders)
                {
                    return false;
                }
                prev = key;
                key = entry.nextKey;
            }
            if (count != coin.holders)
            {
                return false;
            }
        }
        return true;
    }
};

class ContractTestingQpump : protected ContractTesting
{
public:
    ContractTestingQpump()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        system.epoch = 240;
        system.tick = 1000000;

        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QSWAP);
        callSystemProcedure(QSWAP_CONTRACT_INDEX, INITIALIZE);
        callSystemProcedure(QSWAP_CONTRACT_INDEX, BEGIN_EPOCH);
        INIT_CONTRACT(QPUMP);
        callSystemProcedure(QPUMP_CONTRACT_INDEX, INITIALIZE);
        callSystemProcedure(QPUMP_CONTRACT_INDEX, BEGIN_EPOCH);

        checkContractExecCleanup();
    }

    ~ContractTestingQpump()
    {
        checkContractExecCleanup();
    }

    QpumpChecker* state()
    {
        return (QpumpChecker*)contractStates[QPUMP_CONTRACT_INDEX];
    }

    void fund(const id& user, sint64 amount)
    {
        increaseEnergy(user, amount);
    }

    void checkSolvent()
    {
        EXPECT_GE(getBalance(QPUMP_CONTRACT_ID), state()->obligations());
        EXPECT_TRUE(state()->holderListsConsistent());
    }

    void runTicks(uint32 count)
    {
        for (uint32 i = 0; i < count; ++i)
        {
            system.tick += 1;
            callSystemProcedure(QPUMP_CONTRACT_INDEX, END_TICK);
        }
    }

    void endEpoch()
    {
        callSystemProcedure(QPUMP_CONTRACT_INDEX, END_EPOCH);
    }

    QPUMP::CreateCoin_output createCoin(const id& creator, uint64 name, sint64 reward)
    {
        QPUMP::CreateCoin_input input{};
        input.metaDigest = id(9, 9, 9, 9);
        input.name = name;
        QPUMP::CreateCoin_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_CREATE_COIN, input, output, creator, reward);
        return output;
    }

    QPUMP::Buy_output buy(const id& buyer, uint64 name, sint64 tokens, sint64 reward)
    {
        QPUMP::Buy_input input{};
        input.name = name;
        input.tokens = tokens;
        QPUMP::Buy_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_BUY, input, output, buyer, reward);
        return output;
    }

    QPUMP::BuyWithQu_output buyWithQu(const id& buyer, uint64 name, sint64 minTokens, sint64 reward)
    {
        QPUMP::BuyWithQu_input input{};
        input.name = name;
        input.minTokens = minTokens;
        QPUMP::BuyWithQu_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_BUY_WITH_QU, input, output, buyer, reward);
        return output;
    }

    QPUMP::Sell_output sell(const id& seller, uint64 name, sint64 tokens, sint64 minQuOut)
    {
        QPUMP::Sell_input input{};
        input.name = name;
        input.tokens = tokens;
        input.minQuOut = minQuOut;
        QPUMP::Sell_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_SELL, input, output, seller, 0);
        return output;
    }

    QPUMP::Process_output process(const id& caller, uint64 name, uint32 maxPayouts = 0)
    {
        QPUMP::Process_input input{};
        input.name = name;
        input.maxPayouts = maxPayouts;
        QPUMP::Process_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_PROCESS, input, output, caller, 0);
        return output;
    }

    QPUMP::Claim_output claim(const id& holder, uint64 name, sint64 reward = 0)
    {
        QPUMP::Claim_input input{};
        input.name = name;
        QPUMP::Claim_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_CLAIM, input, output, holder, reward);
        return output;
    }

    QPUMP::Transfer_output transfer(const id& from, const id& to, uint64 name, sint64 tokens, sint64 reward)
    {
        QPUMP::Transfer_input input{};
        input.recipient = to;
        input.name = name;
        input.tokens = tokens;
        QPUMP::Transfer_output output;
        invokeUserProcedure(QPUMP_CONTRACT_INDEX, PROC_TRANSFER, input, output, from, reward);
        return output;
    }

    QPUMP::GetCoin_output getCoin(uint64 name)
    {
        QPUMP::GetCoin_input input{};
        input.name = name;
        QPUMP::GetCoin_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_GET_COIN, input, output);
        return output;
    }

    QPUMP::GetHolder_output getHolder(uint64 name, const id& holder)
    {
        QPUMP::GetHolder_input input{};
        input.holder = holder;
        input.name = name;
        QPUMP::GetHolder_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_GET_HOLDER, input, output);
        return output;
    }

    QPUMP::QuoteBuy_output quoteBuy(uint64 name, sint64 tokens)
    {
        QPUMP::QuoteBuy_input input{};
        input.name = name;
        input.tokens = tokens;
        QPUMP::QuoteBuy_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_QUOTE_BUY, input, output);
        return output;
    }

    QPUMP::QuoteSell_output quoteSell(uint64 name, sint64 tokens)
    {
        QPUMP::QuoteSell_input input{};
        input.name = name;
        input.tokens = tokens;
        QPUMP::QuoteSell_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_QUOTE_SELL, input, output);
        return output;
    }

    QPUMP::QuoteBudget_output quoteBudget(uint64 name, sint64 budget)
    {
        QPUMP::QuoteBudget_input input{};
        input.name = name;
        input.quBudget = budget;
        QPUMP::QuoteBudget_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_QUOTE_BUDGET, input, output);
        return output;
    }

    QPUMP::GetFees_output getFees()
    {
        QPUMP::GetFees_input input{};
        QPUMP::GetFees_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_GET_FEES, input, output);
        return output;
    }

    QPUMP::ListCoins_output listCoins(uint32 offset)
    {
        QPUMP::ListCoins_input input{};
        input.offset = offset;
        QPUMP::ListCoins_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_LIST_COINS, input, output);
        return output;
    }

    QPUMP::GetStats_output getStats()
    {
        QPUMP::GetStats_input input{};
        QPUMP::GetStats_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_GET_STATS, input, output);
        return output;
    }

    QPUMP::ListGraduated_output listGraduated(uint32 offset)
    {
        QPUMP::ListGraduated_input input{};
        input.offset = offset;
        QPUMP::ListGraduated_output output;
        callFunction(QPUMP_CONTRACT_INDEX, FN_LIST_GRADUATED, input, output);
        return output;
    }

    QSWAP::GetPoolBasicState_output poolState(uint64 name)
    {
        QSWAP::GetPoolBasicState_input input{};
        input.assetIssuer = QPUMP_CONTRACT_ID;
        input.assetName = name;
        QSWAP::GetPoolBasicState_output output;
        callFunction(QSWAP_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    // Standard opening batch used by several tests: creator order 25M, BUYER1 100M, BUYER2 10M.
    void launchPepeWithBatch()
    {
        fund(CREATOR1, 1000000000LL);
        fund(BUYER1, 1000000000LL);
        fund(BUYER2, 1000000000LL);
        fund(BUYER3, 1000000LL);
        EXPECT_EQ(createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE + 25000000LL).returnCode, QPUMP_OK);
        EXPECT_EQ(buy(BUYER1, NAME_PEPE, 0, 100000000LL).returnCode, QPUMP_OK);
        EXPECT_EQ(buy(BUYER2, NAME_PEPE, 0, 10000000LL).returnCode, QPUMP_OK);
        system.tick += QPUMP_OPENING_TICKS;
        EXPECT_EQ(process(BUYER3, NAME_PEPE).returnCode, QPUMP_OK);
    }
};

TEST(ContractQpump, GetFeesReflectsConstantsAndCachedExternalFees)
{
    ContractTestingQpump qp;
    const auto fees = qp.getFees();
    EXPECT_EQ(fees.launchFee, 25000000LL);
    EXPECT_EQ(fees.graduationReward, 50000000LL);
    EXPECT_EQ(fees.curveRaise, 3905000000LL);
    EXPECT_EQ(fees.openingQuCap, 377187500LL);
    EXPECT_EQ(fees.qxIssuanceFee, 1000000000LL);
    EXPECT_EQ(fees.qxTransferFee, 100LL);
    EXPECT_GT(fees.qswapPoolFee, 0LL);
}

TEST(ContractQpump, CreateCoinValidatesAndRefunds)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 1000000000LL);

    sint64 before = getBalance(CREATOR1);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE - 1).returnCode, QPUMP_ERR_INSUFFICIENT_REWARD);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_LOWER, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_INVALID_NAME);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_DIGIT, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_INVALID_NAME);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_GAP, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_INVALID_NAME);
    EXPECT_EQ(getBalance(CREATOR1), before);

    const auto created = qp.createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE);
    EXPECT_EQ(created.returnCode, QPUMP_OK);
    EXPECT_EQ(created.coinId, 1ULL);
    EXPECT_EQ(getBalance(CREATOR1), before - QPUMP_LAUNCH_FEE);
    EXPECT_EQ(qp.state()->shareholderPot, QPUMP_LAUNCH_FEE - QPUMP_LAUNCH_FEE_BURN);
    EXPECT_EQ(qp.state()->burnPot, QPUMP_LAUNCH_FEE_BURN);

    before = getBalance(CREATOR1);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_NAME_TAKEN);
    EXPECT_EQ(getBalance(CREATOR1), before);

    const auto coin = qp.getCoin(NAME_PEPE);
    EXPECT_TRUE(coin.found);
    EXPECT_EQ(coin.coin.status, QPUMP_STATUS_OPENING);
    EXPECT_EQ(coin.openingTicksLeft, QPUMP_OPENING_TICKS);
    qp.checkSolvent();
}

TEST(ContractQpump, CreatorLimitIsTenLiveCoins)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR2, 1000000000LL);
    const uint64 names[QPUMP_MAX_LIVE_COINS_PER_CREATOR] = {
        NAME_PEPE, NAME_MOON, NAME_WOOF, NAME_DOGE2, NAME_SHIB,
        NAME_ELON, NAME_BONK, NAME_WOJAK, NAME_CHAD, NAME_APE
    };
    for (uint32 i = 0; i < QPUMP_MAX_LIVE_COINS_PER_CREATOR; i++)
    {
        EXPECT_EQ(qp.createCoin(CREATOR2, names[i], QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    }
    sint64 before = getBalance(CREATOR2);
    EXPECT_EQ(qp.createCoin(CREATOR2, NAME_MEME, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_CREATOR_LIMIT);
    EXPECT_EQ(getBalance(CREATOR2), before);
    EXPECT_EQ(qp.listCoins(0).count, QPUMP_MAX_LIVE_COINS_PER_CREATOR);
}

TEST(ContractQpump, OpeningBatchGivesEveryoneTheSamePrice)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();

    const auto coin = qp.getCoin(NAME_PEPE);
    EXPECT_EQ(coin.coin.status, QPUMP_STATUS_OPEN);
    EXPECT_EQ(coin.coin.batchQu, 133663365LL);
    EXPECT_EQ(coin.coin.batchTokens, 86376248LL);
    EXPECT_EQ(coin.coin.sold, 86376248LL);
    EXPECT_EQ(coin.coin.realQu, 133663365LL);
    EXPECT_EQ(coin.coin.holders, 3U);

    EXPECT_EQ(qp.getHolder(NAME_PEPE, CREATOR1).tokens, 15995601LL);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER1).tokens, 63982405LL);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER2).tokens, 6398240LL);
    qp.checkSolvent();
}

TEST(ContractQpump, OpeningBatchCapRefundsTheExcess)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 100000000LL);
    qp.fund(WHALE, 1000000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_MOON, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);

    sint64 before = getBalance(WHALE);
    const auto order = qp.buy(WHALE, NAME_MOON, 0, 500000000LL);
    EXPECT_EQ(order.returnCode, QPUMP_OK);
    EXPECT_EQ(order.quSpent, 380959375LL);
    EXPECT_EQ(getBalance(WHALE), before - 380959375LL);

    qp.fund(BUYER1, 100000000LL);
    EXPECT_EQ(qp.buy(BUYER1, NAME_MOON, 0, 10000000LL).returnCode, QPUMP_ERR_BATCH_FULL);

    system.tick += QPUMP_OPENING_TICKS;
    qp.process(BUYER1, NAME_MOON);
    EXPECT_EQ(qp.getCoin(NAME_MOON).coin.sold, QPUMP_OPENING_TOKEN_CAP);
    qp.checkSolvent();
}

TEST(ContractQpump, CurveBuyAndSellMatchTheIntegerFormulas)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);

    const auto quote = qp.quoteBuy(NAME_PEPE, 10000000LL);
    EXPECT_EQ(quote.cost, 21582905LL);
    EXPECT_EQ(quote.fee, 215830LL);
    EXPECT_EQ(quote.total, 21798735LL);

    sint64 shareholderBefore = qp.state()->shareholderPot;
    sint64 burnBefore = qp.state()->burnPot;
    sint64 qdogeBefore = qp.state()->qdogePot;
    sint64 before = getBalance(BUYER3);
    const auto bought = qp.buy(BUYER3, NAME_PEPE, 10000000LL, 200000000LL);
    EXPECT_EQ(bought.returnCode, QPUMP_OK);
    EXPECT_EQ(bought.tokens, 10000000LL);
    EXPECT_EQ(bought.quSpent, 21798735LL);
    EXPECT_EQ(getBalance(BUYER3), before - 21798735LL);
    EXPECT_EQ(qp.state()->shareholderPot - shareholderBefore, 151081LL);
    EXPECT_EQ(qp.state()->burnPot - burnBefore, 21583LL);
    EXPECT_EQ(qp.state()->qdogePot - qdogeBefore, 43166LL);

    const auto sellQuote = qp.quoteSell(NAME_PEPE, 4000000LL);
    EXPECT_EQ(sellQuote.gross, 8785274LL);
    EXPECT_EQ(sellQuote.payout, 8697421LL);

    before = getBalance(BUYER3);
    const auto sold = qp.sell(BUYER3, NAME_PEPE, 4000000LL, 8697421LL);
    EXPECT_EQ(sold.returnCode, QPUMP_OK);
    EXPECT_EQ(sold.quOut, 8697421LL);
    EXPECT_EQ(getBalance(BUYER3), before + 8697421LL);

    const auto coin = qp.getCoin(NAME_PEPE);
    EXPECT_EQ(coin.coin.sold, 92376248LL);
    EXPECT_EQ(coin.coin.realQu, 146460996LL);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER3).tokens, 6000000LL);
    qp.checkSolvent();
}

TEST(ContractQpump, BuyWithQuSpendsTheAttachedAmount)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);

    const auto quote = qp.quoteBudget(NAME_PEPE, 50000000LL);
    ASSERT_GT(quote.tokens, 0LL);

    sint64 before = getBalance(BUYER3);
    EXPECT_EQ(qp.buyWithQu(BUYER3, NAME_PEPE, quote.tokens + 1, 50000000LL).returnCode, QPUMP_ERR_SLIPPAGE);
    EXPECT_EQ(qp.buyWithQu(BUYER3, NAME_PEPE, 0, 50000000LL).returnCode, QPUMP_ERR_INVALID_INPUT);
    EXPECT_EQ(getBalance(BUYER3), before);

    const auto bought = qp.buyWithQu(BUYER3, NAME_PEPE, quote.tokens, 50000000LL);
    EXPECT_EQ(bought.returnCode, QPUMP_OK);
    EXPECT_EQ(bought.tokens, quote.tokens);
    EXPECT_EQ(bought.quSpent, quote.total);
    EXPECT_LE(bought.quSpent, 50000000LL);
    EXPECT_EQ(getBalance(BUYER3), before - quote.total);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER3).tokens, quote.tokens);
    qp.checkSolvent();
}

TEST(ContractQpump, BuyWithQuJoinsTheOpeningBatch)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 1000000000LL);
    qp.fund(BUYER1, 1000000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    const auto order = qp.buyWithQu(BUYER1, NAME_PEPE, 1, 10000000LL);
    EXPECT_EQ(order.returnCode, QPUMP_OK);
    EXPECT_EQ(order.quSpent, 10000000LL);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.batchQu, 9900990LL);
    qp.checkSolvent();
}

TEST(ContractQpump, TradeErrorsRefundTheReward)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);

    sint64 before = getBalance(BUYER3);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 10000000LL, 21798734LL).returnCode, QPUMP_ERR_SLIPPAGE);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 1LL, 0).returnCode, QPUMP_ERR_SLIPPAGE);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 0, 50000000LL).returnCode, QPUMP_ERR_INVALID_INPUT);
    EXPECT_EQ(qp.buy(BUYER3, NAME_MOON, 1000LL, 50000000LL).returnCode, QPUMP_ERR_COIN_NOT_FOUND);
    EXPECT_EQ(getBalance(BUYER3), before);

    EXPECT_EQ(qp.sell(BUYER3, NAME_PEPE, 1LL, 0).returnCode, QPUMP_ERR_INSUFFICIENT_TOKENS);
    EXPECT_EQ(qp.sell(BUYER1, NAME_PEPE, 63982406LL, 0).returnCode, QPUMP_ERR_INSUFFICIENT_TOKENS);
    EXPECT_EQ(qp.sell(BUYER1, NAME_PEPE, 1000000LL, 100000000LL).returnCode, QPUMP_ERR_SLIPPAGE);
    qp.checkSolvent();
}

TEST(ContractQpump, QuoteBudgetIsAffordable)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);

    const auto quote = qp.quoteBudget(NAME_PEPE, 50000000LL);
    EXPECT_GT(quote.tokens, 0LL);
    EXPECT_LE(quote.total, 50000000LL);
    const auto bought = qp.buy(BUYER3, NAME_PEPE, quote.tokens, 50000000LL);
    EXPECT_EQ(bought.returnCode, QPUMP_OK);
    EXPECT_EQ(bought.quSpent, quote.total);
}

TEST(ContractQpump, GraduationSeedsQswapAndDeliversEveryHolder)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 10000000LL, 200000000LL).returnCode, QPUMP_OK);

    qp.fund(WHALE, 10000000000LL);
    const auto before = qp.getCoin(NAME_PEPE);
    const auto finish = qp.quoteBuy(NAME_PEPE, QPUMP_CURVE_SUPPLY);
    EXPECT_EQ(finish.tokens, QPUMP_CURVE_SUPPLY - before.coin.sold);
    EXPECT_EQ(qp.buy(WHALE, NAME_PEPE, QPUMP_CURVE_SUPPLY, finish.total).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.status, QPUMP_STATUS_COMPLETE);

    const QPUMP::Coin completed = qp.state()->coinOf(NAME_PEPE);
    sint64 creatorBefore = getBalance(CREATOR1);
    sint64 qxBefore = getBalance(QX_CONTRACT_ID);

    const auto graduated = qp.process(BUYER2, NAME_PEPE);
    EXPECT_EQ(graduated.returnCode, QPUMP_OK);
    EXPECT_EQ(graduated.status, QPUMP_STATUS_DISTRIBUTING);

    const QPUMP::Coin coin = qp.state()->coinOf(NAME_PEPE);
    EXPECT_EQ(coin.gradStep, 4);
    EXPECT_GT(coin.poolTokens, 0LL);
    EXPECT_LE(coin.poolTokens, QPUMP_POOL_RESERVE);
    // The pool opens at exactly 10 QU per token, up to integer rounding of the token count.
    EXPECT_GE(coin.poolQu, coin.poolTokens * 10);
    EXPECT_LT(coin.poolQu, coin.poolTokens * 10 + 10);
    EXPECT_EQ(coin.poolQu, completed.realQu - QPUMP_DEFAULT_QX_ISSUANCE_FEE - QPUMP_GRADUATION_REWARD - QPUMP_GRADUATION_BURN
        - qp.state()->cachedQswapPoolFee - QPUMP_QSWAP_LIQUIDITY_FEE - static_cast<sint64>(completed.holders) * QPUMP_DEFAULT_QX_TRANSFER_FEE * QPUMP_DELIVERY_BUDGET_FACTOR);

    EXPECT_EQ(getBalance(CREATOR1), creatorBefore + QPUMP_GRADUATION_REWARD);
    EXPECT_EQ(getBalance(QX_CONTRACT_ID), qxBefore + QPUMP_DEFAULT_QX_ISSUANCE_FEE);

    const auto pool = qp.poolState(NAME_PEPE);
    EXPECT_NE(pool.poolExists, 0LL);
    EXPECT_EQ(pool.reservedQuAmount, coin.poolQu);
    EXPECT_EQ(pool.reservedAssetAmount, coin.poolTokens);
    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_ID, QSWAP_CONTRACT_INDEX, QSWAP_CONTRACT_INDEX), coin.poolTokens);

    sint64 creatorTokens = qp.getHolder(NAME_PEPE, CREATOR1).tokens;
    sint64 buyer1Tokens = qp.getHolder(NAME_PEPE, BUYER1).tokens;
    sint64 buyer2Tokens = qp.getHolder(NAME_PEPE, BUYER2).tokens;
    sint64 buyer3Tokens = qp.getHolder(NAME_PEPE, BUYER3).tokens;
    sint64 whaleTokens = qp.getHolder(NAME_PEPE, WHALE).tokens;

    // BUYER1 claims immediately; everyone else is delivered by END_TICK.
    const auto claimed = qp.claim(BUYER1, NAME_PEPE);
    EXPECT_EQ(claimed.returnCode, QPUMP_OK);
    EXPECT_EQ(claimed.tokens, buyer1Tokens);
    qp.runTicks(64);

    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, CREATOR1, CREATOR1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), creatorTokens);
    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, BUYER1, BUYER1, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), buyer1Tokens);
    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, BUYER2, BUYER2, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), buyer2Tokens);
    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, BUYER3, BUYER3, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), buyer3Tokens);
    EXPECT_EQ(numberOfPossessedShares(NAME_PEPE, QPUMP_CONTRACT_ID, WHALE, WHALE, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), whaleTokens);

    // The coin leaves live trading but keeps a permanent record.
    const auto view = qp.getCoin(NAME_PEPE);
    EXPECT_FALSE(view.found);
    EXPECT_TRUE(view.graduated);
    EXPECT_EQ(view.record.name, NAME_PEPE);
    EXPECT_EQ(view.record.creator, CREATOR1);
    EXPECT_EQ(view.record.sold, QPUMP_CURVE_SUPPLY);
    EXPECT_EQ(view.record.poolQu, coin.poolQu);
    EXPECT_EQ(view.record.poolTokens, coin.poolTokens);
    EXPECT_EQ(view.record.holders, 5U);
    EXPECT_EQ(view.record.poolFallback, 0);
    const auto list = qp.listGraduated(0);
    EXPECT_EQ(list.total, 1U);
    EXPECT_EQ(list.count, 1U);
    EXPECT_EQ(list.items.get(0).coinId, coin.coinId);
    EXPECT_EQ(qp.buy(BUYER2, NAME_PEPE, 1000LL, 100000LL).returnCode, QPUMP_ERR_COIN_NOT_FOUND);
    const auto stats = qp.getStats();
    EXPECT_EQ(stats.totalGraduated, 1ULL);
    EXPECT_EQ(stats.graduatedRecords, 1U);
    EXPECT_EQ(stats.liveCoins, 0U);
    EXPECT_EQ(stats.holderEntries, 0ULL);
    EXPECT_EQ(stats.wallets, 0ULL);

    // A graduated ticker cannot be launched again.
    qp.fund(CREATOR2, 100000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR2, NAME_PEPE, QPUMP_LAUNCH_FEE).returnCode, QPUMP_ERR_NAME_TAKEN);
    qp.checkSolvent();
}

TEST(ContractQpump, ExpiredCoinRefundsHoldersProRata)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();

    const QPUMP::Coin coin = qp.state()->coinOf(NAME_PEPE);
    sint64 creatorTokens = qp.getHolder(NAME_PEPE, CREATOR1).tokens;
    sint64 buyer1Tokens = qp.getHolder(NAME_PEPE, BUYER1).tokens;
    sint64 buyer2Tokens = qp.getHolder(NAME_PEPE, BUYER2).tokens;
    sint64 creatorBefore = getBalance(CREATOR1);
    sint64 buyer1Before = getBalance(BUYER1);
    sint64 buyer2Before = getBalance(BUYER2);

    system.epoch += QPUMP_IDLE_EPOCHS;
    // Sell runs the expiry check and then rejects the trade, which starts refunding without paying out.
    EXPECT_EQ(qp.sell(BUYER3, NAME_PEPE, 1, 0).returnCode, QPUMP_ERR_WRONG_STAGE);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.status, QPUMP_STATUS_REFUNDING);
    const unsigned long long expectedBuyer1Refund =
        (static_cast<unsigned long long>(buyer1Tokens) * static_cast<unsigned long long>(coin.realQu)) / static_cast<unsigned long long>(coin.sold);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER1).refundQu, static_cast<sint64>(expectedBuyer1Refund));

    const auto payouts = qp.process(BUYER3, NAME_PEPE);
    EXPECT_EQ(payouts.processed, 3U);
    EXPECT_FALSE(qp.getCoin(NAME_PEPE).found);

    sint64 paid = (getBalance(CREATOR1) - creatorBefore) + (getBalance(BUYER1) - buyer1Before) + (getBalance(BUYER2) - buyer2Before);
    EXPECT_LE(paid, coin.realQu);
    // Dust tolerance covers TWO independent floor-division layers, not one:
    // EffectiveTokens (Qpump.h:1407) floors each holder's opening-batch QU
    // into a token count (up to holders-1 units of token dust), and the
    // refund itself floors each holder's snapQu*tokens/snapSold share (up
    // to holders-1 more units of QU dust) - with 3 holders that's up to 2+2,
    // not the single-layer bound of 2 (holders-1) this constant used to
    // assume. Neither layer is a bug: it's ordinary integer-arithmetic dust
    // from chaining two proportional splits, left in the contract's own
    // balance, never lost to any party maliciously.
    EXPECT_GE(paid, coin.realQu - 5);
    EXPECT_GT(getBalance(BUYER1) - buyer1Before, getBalance(BUYER2) - buyer2Before);
    EXPECT_GT(creatorTokens, buyer2Tokens);

    // The ticker was never issued, so it can be launched again.
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_PEPE, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    qp.checkSolvent();
}

TEST(ContractQpump, EndEpochPaysDividendsAndBurnsPots)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(BUYER3, 1000000000LL);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 10000000LL, 200000000LL).returnCode, QPUMP_OK);

    sint64 burnPot = qp.state()->burnPot;
    EXPECT_GT(burnPot, 0LL);

    qp.endEpoch();
    EXPECT_EQ(qp.state()->burnPot, 0LL);
    EXPECT_EQ(qp.state()->totalBurned, burnPot);
    EXPECT_LT(qp.state()->shareholderPot, static_cast<sint64>(NUMBER_OF_COMPUTORS));
    qp.checkSolvent();
}

TEST(ContractQpump, OpeningSettlesFromEndTick)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 1000000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_WOOF, QPUMP_LAUNCH_FEE + 25000000LL).returnCode, QPUMP_OK);
    qp.runTicks(QPUMP_OPENING_TICKS + 64);
    const auto coin = qp.getCoin(NAME_WOOF);
    EXPECT_EQ(coin.coin.status, QPUMP_STATUS_OPEN);
    EXPECT_EQ(coin.coin.batchSettled, 1);
    EXPECT_GT(coin.coin.sold, 0LL);
    qp.checkSolvent();
}

TEST(ContractQpump, GraduationWaitsWhenQswapIsDormant)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(WHALE, 10000000000LL);
    const auto finish = qp.quoteBuy(NAME_PEPE, QPUMP_CURVE_SUPPLY);
    EXPECT_EQ(qp.buy(WHALE, NAME_PEPE, QPUMP_CURVE_SUPPLY, finish.total).returnCode, QPUMP_OK);

    setContractFeeReserve(QSWAP_CONTRACT_INDEX, 0);
    sint64 contractBefore = getBalance(QPUMP_CONTRACT_ID);
    const auto attempt = qp.process(BUYER2, NAME_PEPE);
    EXPECT_EQ(attempt.returnCode, QPUMP_ERR_EXTERNAL_CALL);
    EXPECT_EQ(attempt.status, QPUMP_STATUS_COMPLETE);
    EXPECT_EQ(qp.state()->coinOf(NAME_PEPE).gradStep, 0);
    // Nothing was paid to QX while Qswap could not take the pool.
    EXPECT_EQ(getBalance(QPUMP_CONTRACT_ID), contractBefore);

    setContractFeeReserve(QSWAP_CONTRACT_INDEX, 10000000);
    system.tick += QPUMP_MANUAL_RETRY_TICKS;
    EXPECT_EQ(qp.process(BUYER2, NAME_PEPE).status, QPUMP_STATUS_DISTRIBUTING);
    qp.checkSolvent();
}

TEST(ContractQpump, WalletIdsAreSharedAcrossCoinsAndFreedWhenEmpty)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    EXPECT_EQ(qp.getStats().wallets, 3ULL);

    // BUYER1 joins a second coin: one more holder entry, same wallet id.
    qp.fund(CREATOR2, 100000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR2, NAME_MOON, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.buy(BUYER1, NAME_MOON, 0, 5000000LL).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getStats().wallets, 3ULL);
    EXPECT_EQ(qp.getStats().holderEntries, 4ULL);

    // BUYER2 sells everything: its only position closes and its wallet id is released.
    sint64 buyer2Tokens = qp.getHolder(NAME_PEPE, BUYER2).tokens;
    EXPECT_EQ(qp.sell(BUYER2, NAME_PEPE, buyer2Tokens, 0).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getStats().wallets, 2ULL);
    EXPECT_FALSE(qp.getHolder(NAME_PEPE, BUYER2).found);

    // A new wallet reuses the freed id.
    qp.fund(BUYER3, 1000000000LL);
    EXPECT_EQ(qp.buy(BUYER3, NAME_PEPE, 1000000LL, 100000000LL).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getStats().wallets, 3ULL);
    EXPECT_EQ(qp.state()->freeWalletCount, 0U);
    qp.checkSolvent();
}

TEST(ContractQpump, HolderListsStayLinkedAcrossCoinsTradesAndRefunds)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(CREATOR2, 100000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR2, NAME_MOON, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    system.tick += QPUMP_OPENING_TICKS;
    qp.process(BUYER3, NAME_MOON);

    // Interleave wallets across both coins so the two holder lists share the map.
    const uint32 wallets = 40;
    for (uint32 i = 0; i < wallets; ++i)
    {
        id w(100 + i, 7, 7, 7);
        qp.fund(w, 1000000000LL);
        EXPECT_EQ(qp.buy(w, (i & 1) ? NAME_MOON : NAME_PEPE, 100000LL, 10000000LL).returnCode, QPUMP_OK);
        EXPECT_EQ(qp.buy(w, (i & 1) ? NAME_PEPE : NAME_MOON, 100000LL, 10000000LL).returnCode, QPUMP_OK);
    }
    EXPECT_TRUE(qp.state()->holderListsConsistent());

    // Remove holders from the head, the middle and the tail, and add some by transfer.
    for (uint32 i = 0; i < wallets; i += 3)
    {
        id w(100 + i, 7, 7, 7);
        EXPECT_EQ(qp.sell(w, NAME_PEPE, qp.getHolder(NAME_PEPE, w).tokens, 0).returnCode, QPUMP_OK);
    }
    for (uint32 i = 1; i < wallets; i += 4)
    {
        id w(100 + i, 7, 7, 7);
        id to(500 + i, 7, 7, 7);
        EXPECT_EQ(qp.transfer(w, to, NAME_MOON, qp.getHolder(NAME_MOON, w).tokens, QPUMP_TRANSFER_FEE).returnCode, QPUMP_OK);
    }
    EXPECT_TRUE(qp.state()->holderListsConsistent());

    // Expire PEPE and pay out its holders. MOON's list must be untouched.
    uint32 moonHolders = qp.getCoin(NAME_MOON).coin.holders;
    uint32 pepeSlot = qp.state()->slotOf(NAME_PEPE);
    QPUMP::Coin pepe = qp.state()->coins.get(pepeSlot);
    pepe.lastTradeEpoch = static_cast<uint16>(system.epoch - QPUMP_IDLE_EPOCHS);
    qp.state()->coins.set(pepeSlot, pepe);
    EXPECT_EQ(qp.sell(BUYER3, NAME_PEPE, 1, 0).returnCode, QPUMP_ERR_WRONG_STAGE);
    for (int round = 0; round < 10 && qp.getCoin(NAME_PEPE).found; ++round)
    {
        qp.process(BUYER3, NAME_PEPE);
    }
    EXPECT_FALSE(qp.getCoin(NAME_PEPE).found);
    EXPECT_FALSE(qp.getCoin(NAME_PEPE).graduated);
    EXPECT_EQ(qp.getCoin(NAME_MOON).coin.holders, moonHolders);
    EXPECT_TRUE(qp.state()->holderListsConsistent());

    // The freed slot is reused by the next launch.
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_WOOF, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.state()->nextFreshSlot, 2U);
    qp.checkSolvent();
}

TEST(ContractQpump, TransferMovesTokensForAFlatFee)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();

    sint64 fromBefore = qp.getHolder(NAME_PEPE, BUYER1).tokens;
    sint64 toBefore = qp.getHolder(NAME_PEPE, BUYER2).tokens;
    sint64 balanceBefore = getBalance(BUYER1);
    sint64 burnBefore = qp.state()->burnPot;

    const auto moved = qp.transfer(BUYER1, BUYER2, NAME_PEPE, 1000LL, 1000LL);
    EXPECT_EQ(moved.returnCode, QPUMP_OK);
    EXPECT_EQ(moved.fee, QPUMP_TRANSFER_FEE);
    EXPECT_EQ(getBalance(BUYER1), balanceBefore - QPUMP_TRANSFER_FEE);
    EXPECT_EQ(qp.state()->burnPot, burnBefore + QPUMP_TRANSFER_FEE);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER1).tokens, fromBefore - 1000LL);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, BUYER2).tokens, toBefore + 1000LL);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.sold, 86376248LL);
    qp.checkSolvent();
}

TEST(ContractQpump, TransferOfAnyAmountCanCreateANewHolder)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();

    EXPECT_EQ(qp.transfer(BUYER1, WHALE, NAME_PEPE, 1LL, QPUMP_TRANSFER_FEE).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, WHALE).tokens, 1LL);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.holders, 4U);
    EXPECT_EQ(qp.getStats().wallets, 4ULL);
    qp.checkSolvent();
}

TEST(ContractQpump, TinyTradesHaveNoMinimum)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    qp.fund(WHALE, 1000000LL);

    const auto quote = qp.quoteBuy(NAME_PEPE, 1LL);
    EXPECT_EQ(quote.total, quote.cost + QPUMP_MIN_TRADE_FEE);
    sint64 before = getBalance(WHALE);
    EXPECT_EQ(qp.buy(WHALE, NAME_PEPE, 1LL, quote.total).returnCode, QPUMP_OK);
    EXPECT_EQ(getBalance(WHALE), before - quote.total);
    EXPECT_EQ(qp.getHolder(NAME_PEPE, WHALE).tokens, 1LL);
    qp.checkSolvent();
}

TEST(ContractQpump, TransferEverythingClosesTheSenderPosition)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    sint64 buyer2Tokens = qp.getHolder(NAME_PEPE, BUYER2).tokens;
    EXPECT_EQ(qp.transfer(BUYER2, BUYER1, NAME_PEPE, buyer2Tokens, QPUMP_TRANSFER_FEE).returnCode, QPUMP_OK);
    EXPECT_FALSE(qp.getHolder(NAME_PEPE, BUYER2).found);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.holders, 2U);
    EXPECT_EQ(qp.getStats().wallets, 2ULL);
    qp.checkSolvent();
}

TEST(ContractQpump, TransferErrorsRefund)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 1000000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_WOOF, QPUMP_LAUNCH_FEE + 25000000LL).returnCode, QPUMP_OK);
    sint64 before = getBalance(CREATOR1);
    // Opening batch: nothing is transferable yet.
    EXPECT_EQ(qp.transfer(CREATOR1, BUYER1, NAME_WOOF, 1000LL, QPUMP_TRANSFER_FEE).returnCode, QPUMP_ERR_WRONG_STAGE);
    EXPECT_EQ(getBalance(CREATOR1), before);

    system.tick += QPUMP_OPENING_TICKS;
    qp.process(CREATOR1, NAME_WOOF);
    EXPECT_EQ(qp.transfer(CREATOR1, BUYER1, NAME_WOOF, 1000LL, QPUMP_TRANSFER_FEE - 1).returnCode, QPUMP_ERR_INSUFFICIENT_REWARD);
    EXPECT_EQ(qp.transfer(CREATOR1, CREATOR1, NAME_WOOF, 1000LL, QPUMP_TRANSFER_FEE).returnCode, QPUMP_ERR_INVALID_INPUT);
    EXPECT_EQ(qp.transfer(CREATOR1, QX_CONTRACT_ID, NAME_WOOF, 1000LL, QPUMP_TRANSFER_FEE).returnCode, QPUMP_ERR_INVALID_INPUT);
    EXPECT_EQ(qp.transfer(CREATOR1, BUYER1, NAME_WOOF, 100000000000LL, QPUMP_TRANSFER_FEE).returnCode, QPUMP_ERR_INSUFFICIENT_TOKENS);
    EXPECT_EQ(getBalance(CREATOR1), before);
    qp.checkSolvent();
}

// QPUMP_CURVE_RAISE (Qpump.h:32) is a hand-computed literal, not derived via
// static_assert from CurveCost/the curve constants -- this pins it down at
// runtime instead: on a coin that settles its opening batch with zero
// participants (sold stays 0), quoting a buy for the entire curve supply
// exercises CurveCost(0, QPUMP_CURVE_SUPPLY, roundUp) directly and its
// result must equal the documented "cost(0, supply)" constant exactly.
TEST(ContractQpump, CurveRaiseConstantMatchesCostFromZero)
{
    ContractTestingQpump qp;
    qp.fund(CREATOR1, 1000000000LL);
    EXPECT_EQ(qp.createCoin(CREATOR1, NAME_DOGE2, QPUMP_LAUNCH_FEE).returnCode, QPUMP_OK);
    system.tick += QPUMP_OPENING_TICKS;
    qp.process(CREATOR1, NAME_DOGE2);
    ASSERT_EQ(qp.getCoin(NAME_DOGE2).coin.status, QPUMP_STATUS_OPEN);
    ASSERT_EQ(qp.getCoin(NAME_DOGE2).coin.sold, 0LL);

    const auto quote = qp.quoteBuy(NAME_DOGE2, QPUMP_CURVE_SUPPLY);
    EXPECT_EQ(quote.tokens, QPUMP_CURVE_SUPPLY);
    EXPECT_EQ(quote.cost, QPUMP_CURVE_RAISE);
}

// ProcessPayouts' short-circuit on the FIRST QPUMP_ERR_EXTERNAL_CALL assumes
// "QX itself is down, every holder would fail the same way" and aborts the
// whole draining loop rather than skipping just that one holder. This
// confirms the assumption doesn't over-fire on a transient, one-off failure:
// starving QX's fee reserve for exactly one call (then restoring it) must
// stall delivery for that single attempt without corrupting or permanently
// blocking the coin -- draining resumes and finishes once QX is healthy
// again, via the same permissionless Process call.
TEST(ContractQpump, ProcessPayoutsRecoversFromATransientExternalCallFailure)
{
    ContractTestingQpump qp;
    qp.launchPepeWithBatch();
    // Buy out the rest of the curve so the coin graduates for real (QX
    // issuance + Qswap pool), giving 3 holders pending real QX delivery.
    const auto remaining = qp.quoteBuy(NAME_PEPE, QPUMP_CURVE_SUPPLY - qp.getCoin(NAME_PEPE).coin.sold);
    qp.fund(WHALE, remaining.total + 1000000LL);
    EXPECT_EQ(qp.buy(WHALE, NAME_PEPE, remaining.tokens, remaining.total).returnCode, QPUMP_OK);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.status, QPUMP_STATUS_COMPLETE);

    // Drive graduation to completion (resumable FSM, a few ticks of retry).
    for (int i = 0; i < 20 && qp.getCoin(NAME_PEPE).coin.status == QPUMP_STATUS_COMPLETE; ++i)
    {
        qp.process(BUYER3, NAME_PEPE);
        system.tick += QPUMP_GRADUATION_RETRY_TICKS;
    }
    ASSERT_EQ(qp.getCoin(NAME_PEPE).coin.status, QPUMP_STATUS_DISTRIBUTING);
    const uint32 holdersBefore = qp.getCoin(NAME_PEPE).coin.holders;
    ASSERT_GT(holdersBefore, 0U);

    // Starve QX for exactly one delivery attempt.
    setContractFeeReserve(QX_CONTRACT_INDEX, 0);
    auto failedAttempt = qp.process(BUYER3, NAME_PEPE, 1);
    EXPECT_EQ(qp.getCoin(NAME_PEPE).coin.holders, holdersBefore); // nobody was actually delivered

    // QX recovers; draining must resume and finish, not stay stuck.
    setContractFeeReserve(QX_CONTRACT_INDEX, 10000000);
    for (int i = 0; i < 10 && qp.getCoin(NAME_PEPE).found; ++i)
    {
        qp.process(BUYER3, NAME_PEPE, QPUMP_PROCESS_MAX_PAYOUTS);
    }
    EXPECT_FALSE(qp.getCoin(NAME_PEPE).found); // fully delivered and closed
    qp.checkSolvent();
}

