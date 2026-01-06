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
        sint64 burnAmountPerContract;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        // Burn all coins of this contract. According to agreement of the quorum, a part of the
        // computor revenue is donated to this contract for burning.
        // Burns are split equally between GQMPROP (6), SWATCH (7), and CCF (8).
        if (qpi.getEntity(SELF, locals.ownEntity))
        {
            locals.ownBalance = locals.ownEntity.incomingAmount - locals.ownEntity.outgoingAmount;
            if (locals.ownBalance > 0)
            {
                locals.burnAmountPerContract = locals.ownBalance / 3;
                qpi.burn(locals.burnAmountPerContract, 6);  // GQMPROP
                qpi.burn(locals.burnAmountPerContract, 7);  // SWATCH
                qpi.burn(locals.burnAmountPerContract, 8);  // CCF
            }
        }
    }
};
