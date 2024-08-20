using namespace QPI;

struct SWATCH2
{
};

struct SWATCH : public ContractBase
{
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
    _

    struct BEGIN_EPOCH_locals
    {
        Entity ownEntity;
        sint64 ownBalance;
    };

    BEGIN_EPOCH_WITH_LOCALS
        // Burn all coins of this contract. According to agreement of the quorum, a part of the
        // computor revenue is donated to this contract for burning.
        if (qpi.getEntity(SELF, locals.ownEntity))
        {
            locals.ownBalance = locals.ownEntity.incomingAmount - locals.ownEntity.outgoingAmount;
            if (locals.ownBalance > 0)
                qpi.burn(locals.ownBalance);
        }
    _
};
