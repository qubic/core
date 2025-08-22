using namespace QPI;

constexpr sint64 QDRAW_TICKET_PRICE = 1000000LL;
constexpr uint64 QDRAW_MAX_PARTICIPANTS = 1024;

struct QDRAW2
{
};

struct QDRAW : public ContractBase
{
public:
    struct buyTicket_input
    {
        uint64 ticketCount;
    };
    struct buyTicket_output
    {
    };

    struct getInfo_input
    {
    };
    struct getInfo_output 
    {
        sint64 pot;
        uint64 participantCount;
        id     lastWinner;
        sint64 lastWinAmount;
        uint8  lastDrawHour;
        uint8  currentHour;
        uint8  nextDrawHour;
};


    struct getParticipants_input
    {
    };
    struct getParticipants_output
    {
        uint64 participantCount;
        uint64 uniqueParticipantCount;
        Array<id, QDRAW_MAX_PARTICIPANTS> participants;
        Array<uint64, QDRAW_MAX_PARTICIPANTS> ticketCounts;
    };

protected:
    Array<id, QDRAW_MAX_PARTICIPANTS> _participants;
    uint64 _participantCount;
    sint64 _pot;
    uint8 _lastDrawHour;
    id _lastWinner;
    sint64 _lastWinAmount;
    id _owner;

    inline static bool isMonopoly(const Array<id, QDRAW_MAX_PARTICIPANTS>& arr, uint64 count) 
    {
        if (count != QDRAW_MAX_PARTICIPANTS) return false;
        id first = arr.get(0);
        for (uint64 i = 1; i < count; ++i) {
            if (arr.get(i) != first) return false;
        }
        return true;
    }

    PUBLIC_PROCEDURE(buyTicket)
    {
        uint64 available = QDRAW_MAX_PARTICIPANTS - state._participantCount;
        if (QDRAW_MAX_PARTICIPANTS == state._participantCount || input.ticketCount == 0 || input.ticketCount > available)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        sint64 totalCost = (sint64)input.ticketCount * (sint64)QDRAW_TICKET_PRICE;
        if (qpi.invocationReward() < totalCost)
        {
            if (qpi.invocationReward() > 0)
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        for (uint64 i = 0; i < input.ticketCount; ++i)
            state._participants.set(state._participantCount + i, qpi.invocator());

            state._participantCount += input.ticketCount;
            state._pot += totalCost;

        if (qpi.invocationReward() > totalCost)
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - totalCost);
    }

    PUBLIC_FUNCTION(getInfo)
    {
        uint8 qpiHour = qpi.hour();
        output.pot = state._pot;
        output.participantCount = state._participantCount;
        output.lastDrawHour = state._lastDrawHour;
        output.currentHour = qpiHour;
        output.nextDrawHour = (uint8)((qpiHour + 1) % 24);
        output.lastWinner = state._lastWinner;
        output.lastWinAmount = state._lastWinAmount;
    }

    PUBLIC_FUNCTION(getParticipants)
    {
        uint64 uniqueCount = 0;
        for (uint64 i = 0; i < state._participantCount; ++i)
        {
            id p = state._participants.get(i);
            bool found = false;
            for (uint64 j = 0; j < uniqueCount; ++j)
            {
                if (output.participants.get(j) == p)
                {
                    uint64 cnt = output.ticketCounts.get(j);
                    output.ticketCounts.set(j, cnt + 1);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                output.participants.set(uniqueCount, p);
                output.ticketCounts.set(uniqueCount, 1);
                ++uniqueCount;
            }
        }
        output.participantCount = state._participantCount;
        output.uniqueParticipantCount = uniqueCount;
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_PROCEDURE(buyTicket, 1);
        REGISTER_USER_FUNCTION(getInfo, 2);
        REGISTER_USER_FUNCTION(getParticipants, 3);
    }

    INITIALIZE()
    {
        state._participantCount = 0;
        state._pot = 0;
        state._lastWinAmount = 0;
        state._lastWinner = NULL_ID;
        state._lastDrawHour = qpi.hour();
        state._owner = ID(_Q, _D, _R, _A, _W, _U, _R, _A, _L, _C, _L, _P, _P, _E, _Q, _O, _G, _Q, _C, _U, _J, _N, _F, _B, _B, _B, _A, _A, _F, _X, _W, _Y, _Y, _M, _M, _C, _U, _C, _U, _K, _T, _C, _R, _Q, _B, _S, _M, _Z, _U, _D, _M, _V, _X, _P, _N,_F, _Y, _X, _U, _M);
    }

    BEGIN_TICK()
    {
        uint8 currentHour = qpi.hour();
        if (currentHour != state._lastDrawHour) {
            state._lastDrawHour = currentHour;    

            if (state._participantCount > 0)
            {
                if (isMonopoly(state._participants, state._participantCount)) {
                    id only = state._participants.get(0);
                    qpi.transfer(only, QDRAW_TICKET_PRICE);
                    qpi.transfer(state._owner, state._pot - QDRAW_TICKET_PRICE);
                    state._lastWinner = only;
                    state._lastWinAmount = QDRAW_TICKET_PRICE;
                } else {
                    m256i spectrumDigest = qpi.getPrevSpectrumDigest();
                    id rand = qpi.K12(spectrumDigest);
                    uint64 idx = rand.u64._0 % state._participantCount;
                    id winner = state._participants.get(idx);
                    qpi.transfer(winner, state._pot);
                    state._lastWinner = winner;
                    state._lastWinAmount = state._pot;
                }

                state._participantCount = 0;
                state._pot = 0;
                
            }
        }
    }
};

