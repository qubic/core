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
        sint64 burn10, burn17;
    };

    BEGIN_EPOCH_WITH_LOCALS()
    {
        // Burn to balance fee reserves of QVAULT (10), QBond (17)
        if (qpi.getEntity(SELF, locals.ownEntity))
        {
            locals.ownBalance = locals.ownEntity.incomingAmount - locals.ownEntity.outgoingAmount;
            locals.burn10 = 38300000000LL;   // 38.3 Billion QUBIC
            locals.burn17 = 6250000000LL;    // 6.25 Billion QUBIC
            if (locals.ownBalance >= locals.burn10 + locals.burn17)
            {
                qpi.burn(locals.burn10, 10);
                qpi.burn(locals.burn17, 17);
            }
        }
    }
};
