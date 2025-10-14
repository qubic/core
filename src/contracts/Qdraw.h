using namespace QPI;

constexpr sint64 QDRAW_TICKET_PRICE = 1000000LL;
constexpr uint64 QDRAW_MAX_PARTICIPANTS = 1024 * X_MULTIPLIER;

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

    struct buyTicket_locals
    {
        uint64 available;
        sint64 totalCost;
        uint64 i;
    };

    struct getParticipants_locals
    {
        uint64 uniqueCount;
        uint64 i;
        uint64 j;
        bool found;
        id p;
    };

    struct BEGIN_TICK_locals
    {
        uint8 currentHour;
        id only;
        id rand;
        id winner;
        uint64 loopIndex;
    };

    inline static bool isMonopoly(const Array<id, QDRAW_MAX_PARTICIPANTS>& arr, uint64 count, uint64 loopIndex) 
    {
        if (count != QDRAW_MAX_PARTICIPANTS) 
        {
            return false;
        }
        for (loopIndex = 1; loopIndex < count; ++loopIndex)
        {
            if (arr.get(loopIndex) != arr.get(0))
            {
                return false;
            }
        }
        return true;
    }

    PUBLIC_PROCEDURE_WITH_LOCALS(buyTicket)
    {
        locals.available = QDRAW_MAX_PARTICIPANTS - state._participantCount;
        if (QDRAW_MAX_PARTICIPANTS == state._participantCount || input.ticketCount == 0 || input.ticketCount > locals.available)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }
        locals.totalCost = (sint64)input.ticketCount * (sint64)QDRAW_TICKET_PRICE;
        if (qpi.invocationReward() < locals.totalCost)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }
        for (locals.i = 0; locals.i < input.ticketCount; ++locals.i)
        {
            state._participants.set(state._participantCount + locals.i, qpi.invocator());
        }
        state._participantCount += input.ticketCount;
        state._pot += locals.totalCost;
        if (qpi.invocationReward() > locals.totalCost)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.totalCost);
        }
    }

    PUBLIC_FUNCTION(getInfo)
    {
        output.pot = state._pot;
        output.participantCount = state._participantCount;
        output.lastDrawHour = state._lastDrawHour;
        output.currentHour = qpi.hour();
        output.nextDrawHour = (uint8)(mod(qpi.hour() + 1, 24));
        output.lastWinner = state._lastWinner;
        output.lastWinAmount = state._lastWinAmount;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(getParticipants)
    {
        locals.uniqueCount = 0;
        for (locals.i = 0; locals.i < state._participantCount; ++locals.i)
        {
            locals.p = state._participants.get(locals.i);
            locals.found = false;
            for (locals.j = 0; locals.j < locals.uniqueCount; ++locals.j)
            {
                if (output.participants.get(locals.j) == locals.p)
                {
                    output.ticketCounts.set(locals.j, output.ticketCounts.get(locals.j) + 1);
                    locals.found = true;
                    break;
                }
            }
            if (!locals.found)
            {
                output.participants.set(locals.uniqueCount, locals.p);
                output.ticketCounts.set(locals.uniqueCount, 1);
                ++locals.uniqueCount;
            }
        }
        output.participantCount = state._participantCount;
        output.uniqueParticipantCount = locals.uniqueCount;
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
        state._owner = ID(_Q, _D, _R, _A, _W, _U, _R, _A, _L, _C, _L, _P, _P, _E, _Q, _O, _G, _Q, _C, _U, _J, _N, _F, _B, _B, _B, _A, _A, _F, _X, _W, _Y, _Y, _M, _M, _C, _U, _C, _U, _K, _T, _C, _R, _Q, _B, _S, _M, _Z, _U, _D, _M, _V, _X, _P, _N, _F);
    }

    BEGIN_TICK_WITH_LOCALS()
    {
        locals.currentHour = qpi.hour();
        if (locals.currentHour != state._lastDrawHour)
        {
            state._lastDrawHour = locals.currentHour;    
            if (state._participantCount > 0)
            {
                if (isMonopoly(state._participants, state._participantCount, locals.loopIndex))
                {
                    locals.only = state._participants.get(0);
                    qpi.burn(QDRAW_TICKET_PRICE);
                    qpi.transfer(locals.only, QDRAW_TICKET_PRICE);
                    qpi.transfer(state._owner, state._pot - QDRAW_TICKET_PRICE - QDRAW_TICKET_PRICE);
                    state._lastWinner = locals.only;
                    state._lastWinAmount = QDRAW_TICKET_PRICE;
                } 
                else 
                {
                    locals.rand = qpi.K12(qpi.getPrevSpectrumDigest());
                    locals.winner = state._participants.get(mod(locals.rand.u64._0, state._participantCount));
                    qpi.transfer(locals.winner, state._pot);
                    state._lastWinner = locals.winner;
                    state._lastWinAmount = state._pot;
                }
                state._participantCount = 0;
                state._pot = 0;
            }
        }
    }
};


