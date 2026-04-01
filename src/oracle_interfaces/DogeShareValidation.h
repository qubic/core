using namespace QPI;

/**
* Oracle interface "DogeShareValidation" (see Price.h for general documentation about oracle interfaces).
*
* This is used for validating Doge shares.
*/
struct DogeShareValidation
{
    /// Oracle interface index
    static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

    /// Oracle query data / input to the oracle machine
    struct OracleQuery
    {
        Array<uint8, 4> taskPartialHeaderVersion;
        Array<uint8, 32> taskPartialHeaderPrevBlockHash;
        Array<uint8, 32> solutionMerkleRoot;
        Array<uint8, 4> solutionTime;
        Array<uint8, 4> taskPartialHeaderDifficultyNBits;
        Array<uint8, 4> solutionNonce;
        Array<uint8, 32> target;
    };

    /// Oracle reply data / output of the oracle machine
    struct OracleReply
    {
        bit isValid;
    };

    /// Return query fee, which may depend on the specific query (for example on the oracle).
    static sint64 getQueryFee(const OracleQuery& query)
    {
        return 1000;
    }
};
