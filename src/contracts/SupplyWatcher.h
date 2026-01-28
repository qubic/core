using namespace QPI;

struct SWATCH2
{
};

struct SWATCH : public ContractBase
{
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
    }

    struct BEGIN_EPOCH_locals
    {
        Entity ownEntity;
        sint64 ownBalance;
        sint64 reserve6, reserve7, reserve8;
        sint64 target, adjustedTotal;
        sint64 burn6, burn7, burn8;
        sint64 leftover;
        sint64 count;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        // Burn to balance fee reserves of GQMPROP (6), SWATCH (7), and CCF (8)
        if (qpi.getEntity(SELF, locals.ownEntity))
        {
            locals.ownBalance = locals.ownEntity.incomingAmount - locals.ownEntity.outgoingAmount;
            if (locals.ownBalance > 0)
            {
                locals.reserve6 = qpi.queryFeeReserve(6);
                locals.reserve7 = qpi.queryFeeReserve(7);
                locals.reserve8 = qpi.queryFeeReserve(8);

                locals.target = QPI::div(locals.reserve6 + locals.reserve7 + locals.reserve8 + locals.ownBalance, 3LL);

                // Exclude reserves already above target and recalculate
                locals.count = 3;
                locals.adjustedTotal = locals.ownBalance;
                if (locals.reserve6 < locals.target) locals.adjustedTotal += locals.reserve6; else locals.count--;
                if (locals.reserve7 < locals.target) locals.adjustedTotal += locals.reserve7; else locals.count--;
                if (locals.reserve8 < locals.target) locals.adjustedTotal += locals.reserve8; else locals.count--;

                if (locals.count > 0)
                    locals.target = QPI::div(locals.adjustedTotal, locals.count);

                locals.burn6 = (locals.target > locals.reserve6) ? (locals.target - locals.reserve6) : 0;
                locals.burn7 = (locals.target > locals.reserve7) ? (locals.target - locals.reserve7) : 0;
                locals.burn8 = (locals.target > locals.reserve8) ? (locals.target - locals.reserve8) : 0;

                locals.leftover = locals.ownBalance - locals.burn6 - locals.burn7 - locals.burn8;
                if (locals.leftover > 0)
                {
                    locals.burn6 += QPI::div(locals.leftover, 3LL);
                    locals.burn7 += QPI::div(locals.leftover, 3LL);
                    locals.burn8 += QPI::div(locals.leftover, 3LL);
                }

                if (locals.burn6 > 0) qpi.burn(locals.burn6, 6);
                if (locals.burn7 > 0) qpi.burn(locals.burn7, 7);
                if (locals.burn8 > 0) qpi.burn(locals.burn8, 8);
            }
        }
    }
};
