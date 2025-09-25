using namespace QPI;

// QVAULT Contract Constants
// Asset identifiers for QCAP and QVAULT tokens
constexpr uint64 QVAULT_QCAP_ASSETNAME = 1346454353;        // QCAP token asset name
constexpr uint64 QVAULT_QVAULT_ASSETNAME = 92686824592977;  // QVAULT token asset name

// Financial and operational constants
constexpr uint64 QVAULT_PROPOSAL_FEE = 10000000;            // Fee required to submit proposals (in qubic)
constexpr uint64 QVAULT_IPO_PARTICIPATION_MIN_FUND = 1000000000; // Minimum fund required for IPO participation
constexpr uint32 QVAULT_QCAP_MAX_SUPPLY = 21000000;         // Maximum total supply of QCAP tokens
constexpr uint32 QVAULT_MAX_NUMBER_OF_PROPOSAL = 65536;     // Maximum number of proposals allowed
constexpr uint32 QVAULT_X_MULTIPLIER = 1048576; // X multiplier for QVAULT
constexpr uint32 QVAULT_MIN_QUORUM_REQ = 330; // Minimum quorum requirement (330)
constexpr uint32 QVAULT_MAX_QUORUM_REQ = 670; // Maximum quorum requirement (670)
constexpr uint32 QVAULT_MAX_URLS_COUNT = 256; // Maximum number of URLs allowed
constexpr uint32 QVAULT_MIN_VOTING_POWER = 10000; // Minimum voting power required
constexpr uint32 QVAULT_SUM_OF_ALLOCATION_PERCENTAGES = 970; // Sum of allocation percentages
constexpr uint32 QVAULT_MAX_USER_VOTES = 16; // Maximum number of votes per user

// Yearly QCAP sale limits
constexpr uint32 QVAULT_2025MAX_QCAP_SALE_AMOUNT = 10714286; // Maximum QCAP sales for 2025
constexpr uint32 QVAULT_2026MAX_QCAP_SALE_AMOUNT = 15571429; // Maximum QCAP sales for 2026
constexpr uint32 QVAULT_2027MAX_QCAP_SALE_AMOUNT = 18000000; // Maximum QCAP sales for 2027

// Return code constants
constexpr sint32 QVAULT_SUCCESS = 0;                        // Operation completed successfully
constexpr sint32 QVAULT_INSUFFICIENT_QCAP = 1;              // User doesn't have enough QCAP tokens
constexpr sint32 QVAULT_NOT_ENOUGH_STAKE = 2;               // User doesn't have enough staked tokens
constexpr sint32 QVAULT_NOT_STAKER = 3;                     // User is not a staker
constexpr sint32 QVAULT_INSUFFICIENT_FUND = 4;              // Insufficient funds for operation
constexpr sint32 QVAULT_NOT_TRANSFERRED_SHARE = 5;           // Share transfer failed
constexpr sint32 QVAULT_NOT_IN_TIME = 6;                     // Operation attempted outside allowed timeframe
constexpr sint32 QVAULT_NOT_FAIR = 7;                        // Allocation proposal percentages don't sum to 1000
constexpr sint32 QVAULT_OVERFLOW_SALE_AMOUNT = 8;            // QCAP sale amount exceeds yearly limit
constexpr sint32 QVAULT_OVERFLOW_PROPOSAL = 9;               // Proposal ID exceeds maximum allowed
constexpr sint32 QVAULT_OVERFLOW_VOTES = 10;                  // Maximum number of votes per user exceeded
constexpr sint32 QVAULT_SAME_DECISION = 11;                   // User is voting with same decision as before
constexpr sint32 QVAULT_ENDED_PROPOSAL = 12;                  // Proposal voting period has ended
constexpr sint32 QVAULT_NO_VOTING_POWER = 13;                 // User has no voting power
constexpr sint32 QVAULT_FAILED = 14;                          // Operation failed
constexpr sint32 QVAULT_INSUFFICIENT_SHARE = 15;              // Insufficient shares for operation
constexpr sint32 QVAULT_ERROR_TRANSFER_ASSET = 16;            // Asset transfer error occurred
constexpr sint32 QVAULT_INSUFFICIENT_VOTING_POWER = 17;        // User doesn't have minimum voting power (10000)
constexpr sint32 QVAULT_INPUT_ERROR = 18;                      // Invalid input parameters
constexpr sint32 QVAULT_OVERFLOW_STAKER = 19;                  // Maximum number of stakers exceeded
constexpr sint32 QVAULT_INSUFFICIENT_SHARE_OR_VOTING_POWER = 20; // User doesn't have minimum voting power (10000) or shares
constexpr sint32 QVAULT_NOT_FOUND_STAKER_ADDRESS = 21;          // User not found in staker list

// Return result of proposal constants
constexpr uint8 QVAULT_PROPOSAL_PASSED = 0;
constexpr uint8 QVAULT_PROPOSAL_REJECTED = 1;
constexpr uint8 QVAULT_PROPOSAL_INSUFFICIENT_QUORUM = 2;
constexpr uint8 QVAULT_PROPOSAL_INSUFFICIENT_VOTING_POWER = 3;
constexpr uint8 QVAULT_PROPOSAL_INSUFFICIENT_QCAP = 4;
constexpr uint8 QVAULT_PROPOSAL_NOT_STARTED = 5;

/**
 * QVAULT2 - Placeholder struct for future use
 */
struct QVAULT2
{
};

/**
 * QVAULT Contract
 * Main contract for managing QCAP token staking, voting, and governance
 * Inherits from ContractBase to provide basic contract functionality
 */
struct QVAULT : public ContractBase
{

public:

    /**
     * Input structure for getData function
     * No input parameters required - returns all contract state data
     */
    struct getData_input
    {
    };

    /**
     * Output structure for getData function
     * Contains comprehensive contract state information including:
     * - Administrative data (fees)
     * - Financial data (funds, revenue, market cap)
     * - Staking data (staked amounts, voting power)
     * - Proposal counts for each type
     * - Configuration parameters (percentages, limits)
     */
    struct getData_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        uint64 totalVotingPower;           // Total voting power across all stakers
        uint64 proposalCreateFund;          // Fund accumulated from proposal fees
        uint64 reinvestingFund;            // Fund available for reinvestment
        uint64 totalEpochRevenue;          // Total revenue for current epoch
        uint64 fundForBurn;                // Fund allocated for token burning
        uint64 totalStakedQcapAmount;      // Total amount of QCAP tokens staked
        uint64 qcapMarketCap;              // Current QCAP market capitalization
        uint64 raisedFundByQcap;           // Total funds raised from QCAP sales
        uint64 lastRoundPriceOfQcap;       // QCAP price from last fundraising round
        uint64 revenueByQearn;             // Revenue generated from QEarn operations
        uint32 qcapSoldAmount;             // Total QCAP tokens sold to date
        uint32 shareholderDividend;        // Dividend percentage for shareholders (per mille)
        uint32 QCAPHolderPermille;         // Revenue allocation for QCAP holders (per mille)
        uint32 reinvestingPermille;        // Revenue allocation for reinvestment (per mille)
        uint32 burnPermille;               // Revenue allocation for burning (per mille)
        uint32 qcapBurnPermille;           // Revenue allocation for QCAP burning (per mille)
        uint32 numberOfStaker;             // Number of active stakers
        uint32 numberOfVotingPower;        // Number of users with voting power
        uint32 numberOfGP;                 // Number of General Proposals
        uint32 numberOfQCP;                // Number of Quorum Change Proposals
        uint32 numberOfIPOP;               // Number of IPO Participation Proposals
        uint32 numberOfQEarnP;             // Number of QEarn Participation Proposals
        uint32 numberOfFundP;              // Number of Fundraising Proposals
        uint32 numberOfMKTP;               // Number of Marketplace Proposals
        uint32 numberOfAlloP;              // Number of Allocation Proposals
        uint32 transferRightsFee;          // Fee for transferring share management rights
        uint32 minQuorumRq;                // Minimum quorum requirement (330)
        uint32 maxQuorumRq;                // Maximum quorum requirement (670)
        uint32 totalQcapBurntAmount;       // Total QCAP tokens burned to date
        uint32 circulatingSupply;          // Current circulating supply of QCAP
        uint32 quorumPercent;              // Current quorum percentage for proposals
    };

    /**
     * Input structure for stake function
     * @param amount Number of QCAP tokens to stake
     */
    struct stake_input
    {
        uint32 amount;
    };

    /**
     * Output structure for stake function
     * @param returnCode Status code indicating success or failure
     */
    struct stake_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for unStake function
     * @param amount Number of QCAP tokens to unstake
     */
    struct unStake_input
    {
        uint32 amount;
    };

    /**
     * Output structure for unStake function
     * @param returnCode Status code indicating success or failure
     */
    struct unStake_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitGP (General Proposal) function
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitGP_input
    {
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitGP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitGP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitQCP (Quorum Change Proposal) function
     * @param newQuorumPercent New quorum percentage to set (330-670)
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitQCP_input
    {
        uint32 newQuorumPercent;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitQCP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitQCP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitIPOP (IPO Participation Proposal) function
     * @param ipoContractIndex Index of the IPO contract to participate in
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitIPOP_input
    {
        uint32 ipoContractIndex;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitIPOP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitIPOP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitQEarnP (QEarn Participation Proposal) function
     * @param amountPerEpoch Amount to invest per epoch
     * @param numberOfEpoch Number of epochs to participate (max 52)
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitQEarnP_input
    {
        uint64 amountPerEpoch;
        uint32 numberOfEpoch;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitQEarnP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitQEarnP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitFundP (Fundraising Proposal) function
     * @param priceOfOneQcap Price per QCAP token in qubic
     * @param amountOfQcap Amount of QCAP tokens to sell
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitFundP_input
    {
        uint64 priceOfOneQcap;
        uint32 amountOfQcap;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitFundP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitFundP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitMKTP (Marketplace Proposal) function
     * @param amountOfQubic Amount of qubic to spend
     * @param shareName Name/identifier of the share to purchase
     * @param amountOfQcap Amount of QCAP tokens to offer
     * @param indexOfShare Index of the share in the marketplace
     * @param amountOfShare Amount of shares to purchase
     * @param url URL containing proposal details (max 256 bytes)
     */
    struct submitMKTP_input
    {
        uint64 amountOfQubic;
        uint64 shareName;
        uint32 amountOfQcap;
        uint32 indexOfShare;
        uint32 amountOfShare;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitMKTP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitMKTP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for submitAlloP (Allocation Proposal) function
     * @param reinvested Percentage for reinvestment (per mille)
     * @param burn Percentage for burning (per mille)
     * @param distribute Percentage for distribution (per mille)
     * @param url URL containing proposal details (max 256 bytes)
     * Note: All percentages must sum to 970 (per mille)
     */
    struct submitAlloP_input
    {
        uint32 reinvested;
        uint32 burn;
        uint32 distribute;
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;
    };

    /**
     * Output structure for submitAlloP function
     * @param returnCode Status code indicating success or failure
     */
    struct submitAlloP_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for voteInProposal function
     * @param priceOfIPO IPO price for IPO participation proposals
     * @param proposalType Type of proposal (1=GP, 2=QCP, 3=IPOP, 4=QEarnP, 5=FundP, 6=MKTP, 7=AlloP)
     * @param proposalId ID of the proposal to vote on
     * @param yes Voting decision (1=yes, 0=no)
     */
    struct voteInProposal_input
    {
        uint64 priceOfIPO;
        uint32 proposalType;
        uint32 proposalId;
        bit yes;
    };

    /**
     * Output structure for voteInProposal function
     * @param returnCode Status code indicating success or failure
     */
    struct voteInProposal_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for buyQcap function
     * @param amount Number of QCAP tokens to purchase
     */
    struct buyQcap_input
    {
        uint32 amount;
    };

    /**
     * Output structure for buyQcap function
     * @param returnCode Status code indicating success or failure
     */
    struct buyQcap_output
    {
        sint32 returnCode;
    };

    /**
     * Input structure for TransferShareManagementRights function
     * @param asset Asset information (name and issuer)
     * @param numberOfShares Number of shares to transfer management rights for
     * @param newManagingContractIndex Index of the new managing contract
     */
    struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};

    /**
     * Output structure for TransferShareManagementRights function
     * @param transferredNumberOfShares Number of shares successfully transferred
     * @param returnCode Status code indicating success or failure
     */
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
		sint32 returnCode;
	};

protected:

    /**
     * Staking information structure
     * Tracks individual staker addresses and their staked amounts
     */
    struct stakingInfo
    {
        id stakerAddress;    // Address of the staker
        uint32 amount;       // Amount of QCAP tokens staked
    };

    // Storage arrays for staking and voting power data
    Array<stakingInfo, QVAULT_X_MULTIPLIER> staker;      // Array of all stakers (max 1M stakers)
    Array<stakingInfo, QVAULT_X_MULTIPLIER> votingPower; // Array of users with voting power

    /**
     * General Proposal (GP) information structure
     * Stores details about general governance proposals
     */
    struct GPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 currentQuorumPercent;    // Quorum percentage when proposal was created
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum
    };

    // Storage array for general proposals
    Array<GPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> GP;

    /**
     * Quorum Change Proposal (QCP) information structure
     * Stores details about proposals to change the voting quorum percentage
     */
    struct QCPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 currentQuorumPercent;    // Current quorum percentage
        uint32 newQuorumPercent;        // Proposed new quorum percentage
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum
    };

    // Storage array for quorum change proposals
    Array<QCPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> QCP;

    /**
     * IPO Participation Proposal (IPOP) information structure
     * Stores details about proposals to participate in IPO contracts
     */
    struct IPOPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint64 totalWeight;             // Total weighted voting power for IPO participation
        uint64 assignedFund;            // Amount of funds assigned for IPO participation
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 ipoContractIndex;        // Index of the IPO contract to participate in
        uint32 currentQuorumPercent;    // Current quorum percentage
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum, 3=insufficient funds
    };

    // Storage array for IPO participation proposals
    Array<IPOPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> IPOP;

    /**
     * QEarn Participation Proposal (QEarnP) information structure
     * Stores details about proposals to participate in QEarn contracts
     */
    struct QEarnPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint64 amountOfInvestPerEpoch; // Amount to invest per epoch
        uint64 assignedFundPerEpoch;    // Amount of funds assigned per epoch
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 currentQuorumPercent;    // Current quorum percentage
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 numberOfEpoch;            // Number of epochs to participate
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum, 3=insufficient funds
    };

    // Storage array for QEarn participation proposals
    Array<QEarnPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> QEarnP;

    /**
     * Fundraising Proposal (FundP) information structure
     * Stores details about proposals to sell QCAP tokens for fundraising
     */
    struct FundPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint64 pricePerOneQcap;         // Price per QCAP token in qubic
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 amountOfQcap;            // Total amount of QCAP tokens to sell
        uint32 restSaleAmount;          // Remaining amount of QCAP tokens available for sale
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 currentQuorumPercent;    // Current quorum percentage
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum
    };

    // Storage array for fundraising proposals
    Array<FundPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> FundP;

    /**
     * Marketplace Proposal (MKTP) information structure
     * Stores details about proposals to purchase shares from the marketplace
     */
    struct MKTPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint64 amountOfQubic;           // Amount of qubic to spend on shares
        uint64 shareName;               // Name/identifier of the share to purchase
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 amountOfQcap;            // Amount of QCAP tokens to offer
        uint32 currentQuorumPercent;    // Current quorum percentage
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 shareIndex;              // Index of the share in the marketplace
        uint32 amountOfShare;           // Amount of shares to purchase
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum, 3=insufficient funds, 4=insufficient QCAP
    };

    // Storage array for marketplace proposals
    Array<MKTPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> MKTP;

    /**
     * Allocation Proposal (AlloP) information structure
     * Stores details about proposals to change revenue allocation percentages
     * All percentages are in per mille (parts per thousand)
     */
    struct AlloPInfo
    {
        id proposer;                    // Address of the proposal creator
        uint32 currentTotalVotingPower; // Total voting power when proposal was created
        uint32 numberOfYes;             // Number of yes votes received
        uint32 numberOfNo;              // Number of no votes received
        uint32 proposedEpoch;           // Epoch when proposal was created
        uint32 currentQuorumPercent;    // Current quorum percentage
        uint32 reinvested;              // Percentage for reinvestment (per mille)
        uint32 distributed;             // Percentage for distribution (per mille)
        uint32 burnQcap;                // Percentage for QCAP burning (per mille)
        Array<uint8, QVAULT_MAX_URLS_COUNT> url;          // URL containing proposal details
        uint8 result;                   // Proposal result: 0=passed, 1=rejected, 2=insufficient quorum
    };

    // Storage array for allocation proposals
    Array<AlloPInfo, QVAULT_MAX_NUMBER_OF_PROPOSAL> AlloP;

    // Contract configuration and administration
    id QCAP_ISSUER;                    // Address that can issue QCAP tokens

    /**
     * Vote status information structure
     * Tracks individual user votes on proposals
     */
    struct voteStatusInfo
    {
        uint32 proposalId;              // ID of the proposal voted on
        uint8 proposalType;             // Type of proposal (1-7)
        bit decision;                   // Voting decision (1=yes, 0=no)
    };

    // Storage for user voting history and vote counts
    HashMap<id, Array<voteStatusInfo, QVAULT_MAX_USER_VOTES>, QVAULT_X_MULTIPLIER> vote;        // User voting history (max 16 votes per user)
    HashMap<id, uint8, QVAULT_X_MULTIPLIER> countOfVote;                      // Count of votes per user

    // Financial state variables
    uint64 proposalCreateFund;          // Fund accumulated from proposal fees
    uint64 reinvestingFund;             // Fund available for reinvestment
    uint64 totalEpochRevenue;           // Total revenue for current epoch
    uint64 fundForBurn;                 // Fund allocated for token burning
    uint64 totalHistoryRevenue;         // Total historical revenue
    uint64 rasiedFundByQcap;            // Total funds raised from QCAP sales
    uint64 lastRoundPriceOfQcap;        // QCAP price from last fundraising round
    uint64 revenueByQearn;              // Revenue generated from QEarn operations

    // Per-epoch revenue tracking arrays
    Array<uint64, 65536> revenueInQcapPerEpoch;        // Revenue in QCAP per epoch
    Array<uint64, 65536> revenueForOneQcapPerEpoch;    // Revenue per QCAP token per epoch
    Array<uint64, 65536> revenueForOneQvaultPerEpoch;  // Revenue per QVAULT share per epoch
    Array<uint64, 65536> revenueForReinvestPerEpoch;   // Revenue for reinvestment per epoch
    Array<uint64, 1024> revenuePerShare;               // Revenue per share per epoch
    Array<uint32, 65536> burntQcapAmPerEpoch;          // QCAP amount burned per epoch

    // Staking and voting state
    uint32 totalVotingPower;            // Total voting power across all stakers
    uint32 totalStakedQcapAmount;       // Total amount of QCAP tokens staked
    uint32 qcapSoldAmount;              // Total QCAP tokens sold to date

    // Revenue allocation percentages (per mille)
    uint32 shareholderDividend;         // Dividend percentage for shareholders
    uint32 QCAPHolderPermille;          // Revenue allocation for QCAP holders
    uint32 reinvestingPermille;         // Revenue allocation for reinvestment
    uint32 burnPermille;                // Revenue allocation for burning
    uint32 qcapBurnPermille;            // Revenue allocation for QCAP burning
    uint32 totalQcapBurntAmount;        // Total QCAP tokens burned to date

    // Counters for stakers and voting power
    uint32 numberOfStaker;              // Number of active stakers
    uint32 numberOfVotingPower;         // Number of users with voting power

    // Proposal counters for each type
    uint32 numberOfGP;                  // Number of General Proposals
    uint32 numberOfQCP;                 // Number of Quorum Change Proposals
    uint32 numberOfIPOP;                // Number of IPO Participation Proposals
    uint32 numberOfQEarnP;              // Number of QEarn Participation Proposals
    uint32 numberOfFundP;               // Number of Fundraising Proposals
    uint32 numberOfMKTP;                // Number of Marketplace Proposals
    uint32 numberOfAlloP;               // Number of Allocation Proposals

    // Configuration parameters
    uint32 transferRightsFee;           // Fee for transferring share management rights
    uint32 quorumPercent;               // Current quorum percentage for proposals

    /**
     * @return pack qvault datetime data from year, month, day, hour, minute, second to a uint32
     * year is counted from 24 (2024)
     */
     inline static void packQvaultDate(uint32 _year, uint32 _month, uint32 _day, uint32 _hour, uint32 _minute, uint32 _second, uint32& res)
     {
         res = ((_year - 24) << 26) | (_month << 22) | (_day << 17) | (_hour << 12) | (_minute << 6) | (_second);
     }
 
     inline static uint32 qvaultGetYear(uint32 data)
     {
         return ((data >> 26) + 24);
     }
     inline static uint32 qvaultGetMonth(uint32 data)
     {
         return ((data >> 22) & 0b1111);
     }
     inline static uint32 qvaultGetDay(uint32 data)
     {
         return ((data >> 17) & 0b11111);
     }
     inline static uint32 qvaultGetHour(uint32 data)
     {
         return ((data >> 12) & 0b11111);
     }
     inline static uint32 qvaultGetMinute(uint32 data)
     {
         return ((data >> 6) & 0b111111);
     }
     inline static uint32 qvaultGetSecond(uint32 data)
     {
         return (data & 0b111111);
     }
     /*
     * @return unpack qvault datetime from uin32 to year, month, day, hour, minute, secon
     */
     inline static void unpackQvaultDate(uint8& _year, uint8& _month, uint8& _day, uint8& _hour, uint8& _minute, uint8& _second, uint32 data)
     {
         _year = qvaultGetYear(data); // 6 bits
         _month = qvaultGetMonth(data); //4bits
         _day = qvaultGetDay(data); //5bits
         _hour = qvaultGetHour(data); //5bits
         _minute = qvaultGetMinute(data); //6bits
         _second = qvaultGetSecond(data); //6bits
     }
 
    /**
     * Local variables for getData function
     */
    struct getData_locals
    {
        Asset qcapAsset;                // QCAP asset information
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Retrieves comprehensive contract state data
     * Returns all important contract information including financial data,
     * staking statistics, proposal counts, and configuration parameters
     * 
     * @param input No input parameters required
     * @param output Comprehensive contract state data
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getData)
    {
        output.quorumPercent = state.quorumPercent;
        output.totalVotingPower = state.totalVotingPower;
        output.proposalCreateFund = state.proposalCreateFund;
        output.reinvestingFund = state.reinvestingFund;
        output.totalEpochRevenue = state.totalEpochRevenue;
        output.shareholderDividend = state.shareholderDividend;
        output.QCAPHolderPermille = state.QCAPHolderPermille;
        output.reinvestingPermille = state.reinvestingPermille;
        output.burnPermille = state.burnPermille;
        output.qcapBurnPermille = state.qcapBurnPermille;
        output.numberOfStaker = state.numberOfStaker;
        output.numberOfVotingPower = state.numberOfVotingPower;
        output.qcapSoldAmount = state.qcapSoldAmount;
        output.numberOfGP = state.numberOfGP;
        output.numberOfQCP = state.numberOfQCP;
        output.numberOfIPOP = state.numberOfIPOP;
        output.numberOfQEarnP = state.numberOfQEarnP;
        output.numberOfFundP = state.numberOfFundP;
        output.numberOfMKTP = state.numberOfMKTP;
        output.numberOfAlloP = state.numberOfAlloP;
        output.transferRightsFee = state.transferRightsFee;
        output.fundForBurn = state.fundForBurn;
        output.totalStakedQcapAmount = state.totalStakedQcapAmount;
        output.minQuorumRq = QVAULT_MIN_QUORUM_REQ;
        output.maxQuorumRq = QVAULT_MAX_QUORUM_REQ;
        output.totalQcapBurntAmount = state.totalQcapBurntAmount;
        output.raisedFundByQcap = state.rasiedFundByQcap;

        locals.qcapAsset.assetName = QVAULT_QCAP_ASSETNAME;
        locals.qcapAsset.issuer = state.QCAP_ISSUER;

        output.circulatingSupply = (uint32)(qpi.numberOfShares(locals.qcapAsset) - qpi.numberOfShares(locals.qcapAsset, AssetOwnershipSelect::byOwner(SELF), AssetPossessionSelect::byPossessor(SELF)) + state.totalStakedQcapAmount);
        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).result == 0 && state.FundP.get(locals._t).proposedEpoch + 1 < qpi.epoch())
            {
                output.qcapMarketCap = output.circulatingSupply * state.FundP.get(locals._t).pricePerOneQcap;
                break;
            }
        }
        output.lastRoundPriceOfQcap = state.lastRoundPriceOfQcap;
        output.revenueByQearn = state.revenueByQearn;
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for stake function
     */
    struct stake_locals
    {
        stakingInfo user;               // User staking information
        sint32 _t;                      // Loop counter variable
        bool isNewStaker;               // Flag to check if the user is a new staker
    };

    /**
     * Stakes QCAP tokens to earn voting power and revenue
     * Transfers QCAP tokens from user to contract and updates staking records
     * 
     * @param input Amount of QCAP tokens to stake
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(stake)
    {
        if (input.amount > (uint32)qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX))
        {
            output.returnCode = QVAULT_INSUFFICIENT_QCAP;
            return ;
        }

        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == qpi.invocator())
            {
                break;
            }
        }

        if (locals._t == state.numberOfStaker)
        {
            if (state.numberOfStaker >= QVAULT_X_MULTIPLIER)
            {
                output.returnCode = QVAULT_OVERFLOW_STAKER;
                return ;
            }
            locals.user.amount = input.amount;
            locals.user.stakerAddress = qpi.invocator();
            state.numberOfStaker++;
            locals.isNewStaker = 1;
        }
        else 
        {
            locals.user.amount = state.staker.get(locals._t).amount + input.amount;
            locals.user.stakerAddress = state.staker.get(locals._t).stakerAddress;
        }

        if (qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, qpi.invocator(), qpi.invocator(), input.amount, SELF) < 0)
        {
            if (locals.isNewStaker == 1)
            {
                state.numberOfStaker--;
            }
            output.returnCode = QVAULT_ERROR_TRANSFER_ASSET;
            return ;
        }

        state.totalStakedQcapAmount += input.amount;
        state.staker.set(locals._t, locals.user);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for unStake function
     */
    struct unStake_locals
    {
        stakingInfo user;               // User staking information
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Unstakes QCAP tokens, reducing voting power
     * Transfers QCAP tokens back to user and updates staking records
     * 
     * @param input Amount of QCAP tokens to unstake
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(unStake)
    {
        if (input.amount == 0)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }

        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.staker.get(locals._t).amount < input.amount)
                {
                    output.returnCode = QVAULT_NOT_ENOUGH_STAKE;
                }
                else if (state.staker.get(locals._t).amount >= input.amount)
                {
                    if (qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, input.amount, qpi.invocator()) < 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_QCAP;
                        return ;
                    }

                    locals.user.stakerAddress = state.staker.get(locals._t).stakerAddress;
                    locals.user.amount = state.staker.get(locals._t).amount - input.amount;
                    state.staker.set(locals._t, locals.user);

                    state.totalStakedQcapAmount -= input.amount;
                    output.returnCode = QVAULT_SUCCESS;
                    if (locals.user.amount == 0)
                    {
                        state.numberOfStaker--;
                        state.staker.set(locals._t, state.staker.get(state.numberOfStaker));
                    }
                }
                return ;
            }
        }

        output.returnCode = QVAULT_NOT_STAKER;
    }

    /**
     * Local variables for submitGP function
     */
    struct submitGP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        GPInfo newProposal;             // New general proposal to create
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Submits a General Proposal (GP) for governance voting
     * Requires minimum voting power (10000) or QVAULT shares
     * Charges proposal fee and creates new proposal record
     * 
     * @param input URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitGP)
    {
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;

        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }
        
        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfGP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;

        state.GP.set(state.numberOfGP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitQCP function
     */
    struct submitQCP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        QCPInfo newProposal;            // New quorum change proposal to create
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Submits a Quorum Change Proposal (QCP) to modify voting quorum percentage
     * Requires minimum voting power (10000) or QVAULT shares
     * Charges proposal fee and creates new proposal record
     * 
     * @param input New quorum percentage and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitQCP)
    {
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;

        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfQCP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        
        locals.newProposal.newQuorumPercent = input.newQuorumPercent;

        state.QCP.set(state.numberOfQCP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitIPOP function
     */
    struct submitIPOP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        IPOPInfo newProposal;           // New IPO participation proposal to create
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Submits an IPO Participation Proposal (IPOP) to participate in IPO contracts
     * Requires minimum voting power (10000) or QVAULT shares
     * Requires sufficient reinvesting fund (minimum 1B qubic)
     * Charges proposal fee and creates new proposal record
     * 
     * @param input IPO contract index and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitIPOP)
    {
        if (state.reinvestingFund < QVAULT_IPO_PARTICIPATION_MIN_FUND)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;
        
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfIPOP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        locals.newProposal.assignedFund = 0;
        
        locals.newProposal.ipoContractIndex = input.ipoContractIndex;
        locals.newProposal.totalWeight = 0;

        state.IPOP.set(state.numberOfIPOP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitQEarnP function
     */
    struct submitQEarnP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        QEarnPInfo newProposal;         // New QEarn participation proposal to create
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Submits a QEarn Participation Proposal (QEarnP) to invest in QEarn contracts
     * Requires minimum voting power (10000) or QVAULT shares
     * Maximum participation period is 52 epochs
     * Requires sufficient reinvesting fund for the investment amount
     * Charges proposal fee and creates new proposal record
     * 
     * @param input Investment amount per epoch, number of epochs, and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitQEarnP)
    {
        if (input.numberOfEpoch > 52)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        if (input.amountPerEpoch * input.numberOfEpoch > state.reinvestingFund)
        {
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;
        
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }


        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfQEarnP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        
        locals.newProposal.assignedFundPerEpoch = input.amountPerEpoch;
        locals.newProposal.amountOfInvestPerEpoch = input.amountPerEpoch;
        locals.newProposal.numberOfEpoch = input.numberOfEpoch;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }

        state.QEarnP.set(state.numberOfQEarnP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitFundP function
     */
    struct submitFundP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        FundPInfo newProposal;          // New fundraising proposal to create
        sint32 _t;                      // Loop counter variable
        uint32 curDate;                 // Current date (packed)
        uint8 year;                     // Current year
        uint8 month;                    // Current month
        uint8 day;                      // Current day
        uint8 hour;                     // Current hour
        uint8 minute;                   // Current minute
        uint8 second;                   // Current second
    };

    /**
     * Submits a Fundraising Proposal (FundP) to sell QCAP tokens
     * Requires minimum voting power (10000) or QVAULT shares
     * Validates yearly QCAP sale limits (2025-2027)
     * Charges proposal fee and creates new proposal record
     * 
     * @param input Price per QCAP, amount to sell, and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitFundP)
    {
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;
        
        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }

        packQvaultDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        unpackQvaultDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year == 25 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 26 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 27 && state.qcapSoldAmount + input.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (state.qcapSoldAmount + input.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfFundP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        
        locals.newProposal.restSaleAmount = input.amountOfQcap;
        locals.newProposal.amountOfQcap = input.amountOfQcap;
        locals.newProposal.pricePerOneQcap = input.priceOfOneQcap;

        state.FundP.set(state.numberOfFundP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitMKTP function
     */
    struct submitMKTP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        MKTPInfo newProposal;           // New marketplace proposal to create
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Submits a Marketplace Proposal (MKTP) to purchase shares from marketplace
     * Requires proposal fee payment
     * Transfers shares from user to contract as collateral
     * Creates new proposal record for voting
     * 
     * @param input Qubic amount, share details, QCAP amount, and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitMKTP)
    {
        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (qpi.transferShareOwnershipAndPossession(input.shareName, NULL_ID, qpi.invocator(), qpi.invocator(), input.amountOfShare, SELF) < 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NOT_TRANSFERRED_SHARE;
            return ;
        }
        if (state.numberOfMKTP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        
        locals.newProposal.shareIndex = input.indexOfShare;
        locals.newProposal.amountOfShare = input.amountOfShare;
        locals.newProposal.amountOfQubic = input.amountOfQubic;
        locals.newProposal.amountOfQcap = input.amountOfQcap;
        locals.newProposal.shareName = input.shareName;

        state.MKTP.set(state.numberOfMKTP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for submitAlloP function
     */
    struct submitAlloP_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        AlloPInfo newProposal;          // New allocation proposal to create
        sint32 _t;                      // Loop counter variable
        uint32 curDate;                 // Current date (packed)
        uint8 year;                     // Current year
        uint8 month;                    // Current month
        uint8 day;                      // Current day
        uint8 hour;                     // Current hour
        uint8 minute;                   // Current minute
        uint8 second;                   // Current second
    };

    /**
     * Submits an Allocation Proposal (AlloP) to change revenue allocation percentages
     * Requires minimum voting power (10000) or QVAULT shares
     * Validates allocation percentages sum to 970 (per mille)
     * Charges proposal fee and creates new proposal record
     * 
     * @param input Allocation percentages and URL containing proposal details
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(submitAlloP)
    {
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;

        for (locals._t = 0 ; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    break;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_VOTING_POWER;
                        if (qpi.invocationReward() > 0)
                        {
                            qpi.transfer(qpi.invocator(), qpi.invocationReward());
                        }
                        return ;
                    }
                    break;
                }
            }
        }

        if(locals._t == state.numberOfVotingPower && qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(qpi.invocator()), AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_NO_VOTING_POWER;
            return ;
        }

        packQvaultDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        unpackQvaultDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year < 29 && input.burn != 0)
        {
            output.returnCode = QVAULT_NOT_IN_TIME;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (input.burn + input.distribute + input.reinvested != QVAULT_SUM_OF_ALLOCATION_PERCENTAGES)
        {
            output.returnCode = QVAULT_NOT_FAIR;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        if (qpi.invocationReward() < QVAULT_PROPOSAL_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_INSUFFICIENT_FUND;
            return ;
        }
        if (state.numberOfAlloP >= QVAULT_MAX_NUMBER_OF_PROPOSAL)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
            return ;
        }
        if (qpi.invocationReward() > QVAULT_PROPOSAL_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - QVAULT_PROPOSAL_FEE);
        }
        state.proposalCreateFund += QVAULT_PROPOSAL_FEE;

        locals.newProposal.currentQuorumPercent = state.quorumPercent;
        locals.newProposal.currentTotalVotingPower = state.totalVotingPower;
        for (locals._t = 0; locals._t < QVAULT_MAX_URLS_COUNT; locals._t++)
        {
            locals.newProposal.url.set(locals._t, input.url.get(locals._t));
        }
        locals.newProposal.proposedEpoch = qpi.epoch();
        locals.newProposal.numberOfYes = 0;
        locals.newProposal.numberOfNo = 0;
        locals.newProposal.proposer = qpi.invocator();
        locals.newProposal.result = QVAULT_PROPOSAL_NOT_STARTED;
        
        locals.newProposal.burnQcap = input.burn;
        locals.newProposal.distributed = input.distribute;
        locals.newProposal.reinvested = input.reinvested;

        state.AlloP.set(state.numberOfAlloP++, locals.newProposal);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Local variables for voteInProposal function
     */
    struct voteInProposal_locals
    {
        GPInfo updatedGProposal;                    // Updated general proposal
        QCPInfo updatedQCProposal;                  // Updated quorum change proposal
        IPOPInfo updatedIPOProposal;                // Updated IPO participation proposal
        QEarnPInfo updatedQEarnProposal;            // Updated QEarn participation proposal
        FundPInfo updatedFundProposal;              // Updated fundraising proposal
        MKTPInfo updatedMKTProposal;                // Updated marketplace proposal
        AlloPInfo updatedAlloProposal;              // Updated allocation proposal
        Array<voteStatusInfo, QVAULT_MAX_USER_VOTES> newVoteList;      // Updated vote list for user
        voteStatusInfo newVote;                     // New vote to add
        uint32 numberOfYes;                         // Number of yes votes to add
        uint32 numberOfNo;                          // Number of no votes to add
        sint32 _t, _r;                              // Loop counter variables
        uint8 countOfVote;                          // Current vote count for user
        bit statusOfProposal;                       // Whether proposal is still active
    };

    /**
     * Votes on active proposals of various types
     * Supports 7 different proposal types with different voting logic
     * Updates proposal vote counts and user voting history
     * Prevents duplicate votes and enforces voting rules
     * 
     * @param input Proposal type, ID, voting decision, and IPO price (if applicable)
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
    {
        if (input.proposalType > 7 || input.proposalType < 1)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        switch (input.proposalType)
        {
            case 1:
                if (input.proposalId >= state.numberOfGP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 2:
                if (input.proposalId >= state.numberOfQCP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 3:
                if (input.proposalId >= state.numberOfIPOP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 4:
                if (input.proposalId >= state.numberOfQEarnP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 5:
                if (input.proposalId >= state.numberOfFundP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 6:
                if (input.proposalId >= state.numberOfMKTP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
            case 7:
                if (input.proposalId >= state.numberOfAlloP)
                {
                    output.returnCode = QVAULT_OVERFLOW_PROPOSAL;
                    return ;
                }
                break;
        }

        if (state.countOfVote.get(qpi.invocator(), locals.countOfVote))
        {
            state.vote.get(qpi.invocator(), locals.newVoteList);
            for (locals._r = 0; locals._r < locals.countOfVote; locals._r++)
            {
                if (locals.newVoteList.get(locals._r).proposalId == input.proposalId && locals.newVoteList.get(locals._r).proposalType == input.proposalType)
                {
                    break;
                }
            }
        }
        if (locals.countOfVote == QVAULT_MAX_USER_VOTES && locals._r == locals.countOfVote)
        {
            output.returnCode = QVAULT_OVERFLOW_VOTES;
            return ;
        }
        if (locals._r < locals.countOfVote && locals.newVoteList.get(locals._r).decision == input.yes)
        {
            output.returnCode = QVAULT_SAME_DECISION;
            return ;
        }
        
        switch (input.proposalType)
        {
            case 1:
                locals.updatedGProposal = state.GP.get(input.proposalId);
                locals.statusOfProposal = state.GP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 2:
                locals.updatedQCProposal = state.QCP.get(input.proposalId);
                locals.statusOfProposal = state.QCP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 3:
                locals.updatedIPOProposal = state.IPOP.get(input.proposalId);
                locals.statusOfProposal = state.IPOP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 4:
                locals.updatedQEarnProposal = state.QEarnP.get(input.proposalId);
                locals.statusOfProposal = state.QEarnP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 5:
                locals.updatedFundProposal = state.FundP.get(input.proposalId);
                locals.statusOfProposal = state.FundP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 6:
                locals.updatedMKTProposal = state.MKTP.get(input.proposalId);
                locals.statusOfProposal = state.MKTP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            case 7:
                locals.updatedAlloProposal = state.AlloP.get(input.proposalId);
                locals.statusOfProposal = state.AlloP.get(input.proposalId).proposedEpoch == qpi.epoch() ? 1 : 0;
                break;
            default:
                break;
        }

        if (locals.statusOfProposal == 0)
        {
            output.returnCode = QVAULT_ENDED_PROPOSAL;
            return ;
        }

        if (locals.statusOfProposal == 1)
        {
            locals.numberOfYes = 0;
            locals.numberOfNo = 0;
            for (locals._t = 0 ; locals._t < (sint32)state.numberOfVotingPower; locals._t++)
            {
                if (state.votingPower.get(locals._t).stakerAddress == qpi.invocator())
                {
                    if (input.yes == 1)
                    {
                        locals.numberOfYes = state.votingPower.get(locals._t).amount;
                        if (locals._r < locals.countOfVote)
                        {
                            locals.numberOfNo -= state.votingPower.get(locals._t).amount;
                        }
                    }
                    else 
                    {
                        locals.numberOfNo = state.votingPower.get(locals._t).amount;
                        if (locals._r < locals.countOfVote)
                        {
                            locals.numberOfYes -= state.votingPower.get(locals._t).amount;
                        }
                    }
                    switch (input.proposalType)
                    {
                        case 1:
                            locals.updatedGProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedGProposal.numberOfNo += locals.numberOfNo;
                            state.GP.set(input.proposalId, locals.updatedGProposal);
                            break;
                        case 2:
                            locals.updatedQCProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedQCProposal.numberOfNo += locals.numberOfNo;
                            state.QCP.set(input.proposalId, locals.updatedQCProposal);
                            break;
                        case 3:
                            locals.updatedIPOProposal.totalWeight += locals.numberOfYes * input.priceOfIPO;
                            locals.updatedIPOProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedIPOProposal.numberOfNo += locals.numberOfNo;
                            state.IPOP.set(input.proposalId, locals.updatedIPOProposal);
                            break;
                        case 4:
                            locals.updatedQEarnProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedQEarnProposal.numberOfNo += locals.numberOfNo;
                            state.QEarnP.set(input.proposalId, locals.updatedQEarnProposal);
                            break;
                        case 5:
                            locals.updatedFundProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedFundProposal.numberOfNo += locals.numberOfNo;
                            state.FundP.set(input.proposalId, locals.updatedFundProposal);
                            break;
                        case 6:
                            locals.updatedMKTProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedMKTProposal.numberOfNo += locals.numberOfNo;
                            state.MKTP.set(input.proposalId, locals.updatedMKTProposal);
                            break;
                        case 7:
                            locals.updatedAlloProposal.numberOfYes += locals.numberOfYes;
                            locals.updatedAlloProposal.numberOfNo += locals.numberOfNo;
                            state.AlloP.set(input.proposalId, locals.updatedAlloProposal);
                            break;
                        default:
                            break;
                    }
                    if (state.countOfVote.get(qpi.invocator(), locals.countOfVote))
                    {
                        locals.newVote.proposalId = input.proposalId;
                        locals.newVote.proposalType = input.proposalType;
                        locals.newVote.decision = input.yes;
                        if (locals._r < locals.countOfVote)
                        {
                            locals.newVoteList.set(locals._r, locals.newVote);
                        }
                        else
                        {
                            locals.newVoteList.set(locals.countOfVote, locals.newVote);
                            state.countOfVote.set(qpi.invocator(), locals.countOfVote + 1);
                        }
                    }
                    else 
                    {
                        locals.newVote.proposalId = input.proposalId;
                        locals.newVote.proposalType = input.proposalType;
                        locals.newVote.decision = input.yes;
                        locals.newVoteList.set(0, locals.newVote);
                        state.countOfVote.set(qpi.invocator(), 1);
                    }
                    state.vote.set(qpi.invocator(), locals.newVoteList);
                    output.returnCode = QVAULT_SUCCESS;
                    return ;
                }
            }
        }

        output.returnCode = QVAULT_NO_VOTING_POWER;
    }

    /**
     * Local variables for buyQcap function
     */
    struct buyQcap_locals
    {
        QX::TransferShareManagementRights_input transferShareManagementRights_input;    // Input for QX contract call
        QX::TransferShareManagementRights_output transferShareManagementRights_output;  // Output from QX contract call
        FundPInfo updatedFundProposal;                                                  // Updated fundraising proposal
        sint32 _t;                                                                      // Loop counter variable
        uint32 curDate;                                                                 // Current date (packed)
        uint8 year;                                                                     // Current year
        uint8 month;                                                                    // Current month
        uint8 day;                                                                      // Current day
        uint8 hour;                                                                     // Current hour
        uint8 minute;                                                                   // Current minute
        uint8 second;                                                                   // Current second
    };

    /**
     * Purchases QCAP tokens from active fundraising proposals
     * Validates yearly QCAP sale limits (2025-2027)
     * Finds best available price from passed fundraising proposals
     * Transfers QCAP tokens to buyer and updates proposal sale amounts
     * 
     * @param input Amount of QCAP tokens to purchase
     * @param output Status code indicating success or failure
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(buyQcap)
    {
        packQvaultDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
        unpackQvaultDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
        if (locals.year == 25 && state.qcapSoldAmount + input.amount > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 26 && state.qcapSoldAmount + input.amount > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (locals.year == 27 && state.qcapSoldAmount + input.amount > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }
        else if (state.qcapSoldAmount + input.amount > QVAULT_QCAP_MAX_SUPPLY)
        {
            output.returnCode = QVAULT_OVERFLOW_SALE_AMOUNT;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return ;
        }

        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).result == 0 && state.FundP.get(locals._t).proposedEpoch + 1 < qpi.epoch() && state.FundP.get(locals._t).restSaleAmount >= input.amount)
            {
                if (qpi.invocationReward() >= (sint64)state.FundP.get(locals._t).pricePerOneQcap * input.amount)
                {
                    if (qpi.invocationReward() > (sint64)state.FundP.get(locals._t).pricePerOneQcap * input.amount)
                    {
                        qpi.transfer(qpi.invocator(), qpi.invocationReward() - (state.FundP.get(locals._t).pricePerOneQcap * input.amount));
                    }

                    state.rasiedFundByQcap += state.FundP.get(locals._t).pricePerOneQcap * input.amount;
                    locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
                    locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
                    locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
                    locals.transferShareManagementRights_input.numberOfShares = input.amount;

                    INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

                    qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, input.amount, qpi.invocator());

                    state.qcapSoldAmount += input.amount;
                    locals.updatedFundProposal = state.FundP.get(locals._t);
                    locals.updatedFundProposal.restSaleAmount -= input.amount;
                    state.FundP.set(locals._t, locals.updatedFundProposal);

                    state.reinvestingFund += state.FundP.get(locals._t).pricePerOneQcap * input.amount;
                    output.returnCode = QVAULT_SUCCESS;
                    return ;
                }
            }
        }

        qpi.transfer(qpi.invocator(), qpi.invocationReward());

        output.returnCode = QVAULT_FAILED;
    }

    /**
     * Transfers share management rights to another contract
     * Requires payment of transfer rights fee
     * Releases shares from current contract to new managing contract
     * Used for QCAP token transfers and other asset management
     * 
     * @param input Asset information, number of shares, and new managing contract index
     * @param output Number of shares transferred and status code
     */
	PUBLIC_PROCEDURE(TransferShareManagementRights)
    {
		if (qpi.invocationReward() < state.transferRightsFee)
		{
			output.returnCode = QVAULT_INSUFFICIENT_FUND;
			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = QVAULT_INSUFFICIENT_SHARE;
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, state.transferRightsFee) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				output.returnCode = QVAULT_ERROR_TRANSFER_ASSET;
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				if (qpi.invocationReward() > state.transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.transferRightsFee);
				}

				output.returnCode = QVAULT_SUCCESS;
			}
		}
    }

public:
    /**
     * Input structure for getStakedAmountAndVotingPower function
     * @param address User address to query staking information for
     */
    struct getStakedAmountAndVotingPower_input
    {
        id address;
    };

    /**
     * Output structure for getStakedAmountAndVotingPower function
     * @param stakedAmount Amount of QCAP tokens staked by the user
     * @param votingPower Voting power of the user
     */
    struct getStakedAmountAndVotingPower_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        uint32 stakedAmount;
        uint32 votingPower;
    };

    /**
     * Local variables for getStakedAmountAndVotingPower function
     */
    struct getStakedAmountAndVotingPower_locals
    {
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Retrieves staking information for a specific user address
     * Returns both staked amount and voting power
     * 
     * @param input User address to query
     * @param output Staked amount and voting power for the user
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getStakedAmountAndVotingPower)
    {
        for (locals._t = 0; locals._t < (sint64)state.numberOfStaker; locals._t++)
        {
            if (state.staker.get(locals._t).stakerAddress == input.address)
            {
                output.stakedAmount = state.staker.get(locals._t).amount;
                break;
            }
        }
        for (locals._t = 0; locals._t < (sint64)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == input.address)
            {
                output.votingPower = state.votingPower.get(locals._t).amount;
                break;
            }
        }
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getGP function
     * @param proposalId ID of the general proposal to retrieve
     */
    struct getGP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getGP function
     * @param proposal General proposal information
     */
    struct getGP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        GPInfo proposal;
    };

    /**
     * Retrieves information about a specific General Proposal (GP)
     * 
     * @param input Proposal ID to query
     * @param output General proposal information
     */
    PUBLIC_FUNCTION(getGP)
    {
        if (input.proposalId >= state.numberOfGP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.GP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getQCP function
     * @param proposalId ID of the quorum change proposal to retrieve
     */
    struct getQCP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getQCP function
     * @param proposal Quorum change proposal information
     */
    struct getQCP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        QCPInfo proposal;
    };

    /**
     * Retrieves information about a specific Quorum Change Proposal (QCP)
     * 
     * @param input Proposal ID to query
     * @param output Quorum change proposal information
     */
    PUBLIC_FUNCTION(getQCP)
    {
        if (input.proposalId >= state.numberOfQCP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.QCP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getIPOP function
     * @param proposalId ID of the IPO participation proposal to retrieve
     */
    struct getIPOP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getIPOP function
     * @param proposal IPO participation proposal information
     */
    struct getIPOP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        IPOPInfo proposal;
    };

    /**
     * Retrieves information about a specific IPO Participation Proposal (IPOP)
     * 
     * @param input Proposal ID to query
     * @param output IPO participation proposal information
     */
    PUBLIC_FUNCTION(getIPOP)
    {
        if (input.proposalId >= state.numberOfIPOP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.IPOP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getQEarnP function
     * @param proposalId ID of the QEarn participation proposal to retrieve
     */
    struct getQEarnP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getQEarnP function
     * @param proposal QEarn participation proposal information
     */
    struct getQEarnP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        QEarnPInfo proposal;
    };

    /**
     * Retrieves information about a specific QEarn Participation Proposal (QEarnP)
     * 
     * @param input Proposal ID to query
     * @param output QEarn participation proposal information
     */
    PUBLIC_FUNCTION(getQEarnP)
    {
        if (input.proposalId >= state.numberOfQEarnP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.QEarnP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getFundP function
     * @param proposalId ID of the fundraising proposal to retrieve
     */
    struct getFundP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getFundP function
     * @param proposal Fundraising proposal information
     */
    struct getFundP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        FundPInfo proposal;
    };

    /**
     * Retrieves information about a specific Fundraising Proposal (FundP)
     * 
     * @param input Proposal ID to query
     * @param output Fundraising proposal information
     */
    PUBLIC_FUNCTION(getFundP)
    {
        if (input.proposalId >= state.numberOfFundP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.FundP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getMKTP function
     * @param proposalId ID of the marketplace proposal to retrieve
     */
    struct getMKTP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getMKTP function
     * @param proposal Marketplace proposal information
     */
    struct getMKTP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        MKTPInfo proposal;
    };

    /**
     * Retrieves information about a specific Marketplace Proposal (MKTP)
     * 
     * @param input Proposal ID to query
     * @param output Marketplace proposal information
     */
    PUBLIC_FUNCTION(getMKTP)
    {
        if (input.proposalId >= state.numberOfMKTP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.MKTP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getAlloP function
     * @param proposalId ID of the allocation proposal to retrieve
     */
    struct getAlloP_input
    {
        uint32 proposalId;
    };

    /**
     * Output structure for getAlloP function
     * @param proposal Allocation proposal information
     */
    struct getAlloP_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        AlloPInfo proposal;
    };

    /**
     * Retrieves information about a specific Allocation Proposal (AlloP)
     * 
     * @param input Proposal ID to query
     * @param output Allocation proposal information
     */
    PUBLIC_FUNCTION(getAlloP)
    {
        if (input.proposalId >= state.numberOfAlloP)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        output.proposal = state.AlloP.get(input.proposalId);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getIdentitiesHvVtPw function
     * @param offset Starting index for pagination
     * @param count Number of identities to retrieve (max 256)
     */
    struct getIdentitiesHvVtPw_input
    {
        uint32 offset;
        uint32 count;
    };

    /**
     * Output structure for getIdentitiesHvVtPw function
     * @param idList Array of user addresses with voting power
     * @param amountList Array of voting power amounts corresponding to each address
     */
    struct getIdentitiesHvVtPw_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        Array<id, QVAULT_MAX_URLS_COUNT> idList;
        Array<uint32, QVAULT_MAX_URLS_COUNT> amountList;
    };

    /**
     * Local variables for getIdentitiesHvVtPw function
     */
    struct getIdentitiesHvVtPw_locals
    {
        sint32 _t, _r;                  // Loop counter variables
    };

    /**
     * Retrieves a paginated list of identities that have voting power
     * Returns both addresses and their corresponding voting power amounts
     * Useful for governance and analytics purposes
     * 
     * @param input Offset and count for pagination
     * @param output Arrays of addresses and voting power amounts
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getIdentitiesHvVtPw)
    {
        if(input.count > QVAULT_MAX_URLS_COUNT || input.offset + input.count > QVAULT_X_MULTIPLIER)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        for (locals._t = (sint32)input.offset, locals._r = 0; locals._t < (sint32)(input.offset + input.count); locals._t++)
        {
            if (locals._t > (sint32)state.numberOfVotingPower)
            {
                output.returnCode = QVAULT_INPUT_ERROR;
                return ;
            }
            output.idList.set(locals._r, state.votingPower.get(locals._t).stakerAddress);
            output.amountList.set(locals._r++, state.votingPower.get(locals._t).amount);
        }
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for ppCreationPower function
     * @param address User address to check proposal creation power for
     */
    struct ppCreationPower_input
    {
        id address;
    };

    /**
     * Output structure for ppCreationPower function
     * @param status 0 = no proposal creation power, 1 = has proposal creation power
     */
    struct ppCreationPower_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        bit status;
    };

    /**
     * Local variables for ppCreationPower function
     */
    struct ppCreationPower_locals
    {
        Asset qvaultShare;              // QVAULT share asset information
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Checks if a user has the power to create proposals
     * User must have either minimum voting power (10000) or QVAULT shares
     * 
     * @param input User address to check
     * @param output Status indicating whether user can create proposals
     */
    PUBLIC_FUNCTION_WITH_LOCALS(ppCreationPower)
    {
        locals.qvaultShare.assetName = QVAULT_QVAULT_ASSETNAME;
        locals.qvaultShare.issuer = NULL_ID;

        for (locals._t = 0; locals._t < (sint32)state.numberOfVotingPower; locals._t++)
        {
            if (state.votingPower.get(locals._t).stakerAddress == input.address)
            {
                if (state.votingPower.get(locals._t).amount >= QVAULT_MIN_VOTING_POWER)
                {
                    output.returnCode = QVAULT_SUCCESS;
                    output.status = 1;
                }
                else 
                {
                    if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(input.address), AssetPossessionSelect::byPossessor(input.address)) > 0)
                    {
                        output.returnCode = QVAULT_SUCCESS;
                        output.status = 1;
                    }
                    else
                    {
                        output.returnCode = QVAULT_INSUFFICIENT_SHARE_OR_VOTING_POWER;
                        output.status = 0;
                    }
                }
                return ;
            }
        }

        if (qpi.numberOfShares(locals.qvaultShare, AssetOwnershipSelect::byOwner(input.address), AssetPossessionSelect::byPossessor(input.address)) > 0)
        {
            output.returnCode = QVAULT_SUCCESS;
            output.status = 1;
        }
        else 
        {
            output.returnCode = QVAULT_NOT_FOUND_STAKER_ADDRESS;
            output.status = 0;
        }
    }

    /**
     * Input structure for getQcapBurntAmountInLastEpoches function
     * @param numberOfLastEpoches Number of recent epochs to check (max 100)
     */
    struct getQcapBurntAmountInLastEpoches_input
    {
        uint32 numberOfLastEpoches;
    };

    /**
     * Output structure for getQcapBurntAmountInLastEpoches function
     * @param burntAmount Total amount of QCAP tokens burned in the specified epochs
     */
    struct getQcapBurntAmountInLastEpoches_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        uint32 burntAmount;
    };

    /**
     * Local variables for getQcapBurntAmountInLastEpoches function
     */
    struct getQcapBurntAmountInLastEpoches_locals
    {
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Calculates the total amount of QCAP tokens burned in recent epochs
     * Useful for tracking token deflation and economic metrics
     * Maximum lookback period is 100 epochs
     * 
     * @param input Number of recent epochs to check
     * @param output Total amount of QCAP tokens burned
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getQcapBurntAmountInLastEpoches)
    {
        if (input.numberOfLastEpoches > 100)
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        for (locals._t = (sint32)qpi.epoch(); locals._t >= (sint32)(qpi.epoch() - input.numberOfLastEpoches); locals._t--)
        {
            output.burntAmount += state.burntQcapAmPerEpoch.get(locals._t);
        }
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getAmountToBeSoldPerYear function
     * @param year Year to check available QCAP sales for
     */
    struct getAmountToBeSoldPerYear_input
    {
        uint32 year;
    };

    /**
     * Output structure for getAmountToBeSoldPerYear function
     * @param amount Amount of QCAP tokens available for sale in the specified year
     */
    struct getAmountToBeSoldPerYear_output
    {
        uint32 amount;
    };

    /**
     * Calculates the amount of QCAP tokens available for sale in a specific year
     * Takes into account yearly limits and already sold amounts
     * Returns 0 for years before 2025
     * 
     * @param input Year to check
     * @param output Available QCAP amount for sale
     */
    PUBLIC_FUNCTION(getAmountToBeSoldPerYear)
    {
        if (input.year < 2025)
        {
            output.amount = 0;
        }
        else if (input.year == 2025)
        {
            output.amount = QVAULT_2025MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year == 2026)
        {
            output.amount = QVAULT_2026MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year == 2027)
        {
            output.amount = QVAULT_2027MAX_QCAP_SALE_AMOUNT - state.qcapSoldAmount;
        }
        else if (input.year > 2027)
        {
            output.amount = QVAULT_QCAP_MAX_SUPPLY - state.qcapSoldAmount;
        }
    }

    /**
     * Input structure for getTotalRevenueInQcap function
     * No input parameters required
     */
    struct getTotalRevenueInQcap_input
    {

    };

    /**
     * Output structure for getTotalRevenueInQcap function
     * @param revenue Total historical revenue in QCAP
     */
    struct getTotalRevenueInQcap_output
    {
        uint64 revenue;
    };

    /**
     * Retrieves the total historical revenue accumulated by the contract
     * Represents the sum of all revenue across all epochs
     * 
     * @param input No input parameters
     * @param output Total historical revenue amount
     */
    PUBLIC_FUNCTION(getTotalRevenueInQcap)
    {
        output.revenue = state.totalHistoryRevenue;
    }

    /**
     * Input structure for getRevenueInQcapPerEpoch function
     * @param epoch Epoch number to retrieve revenue data for
     */
    struct getRevenueInQcapPerEpoch_input
    {
        uint32 epoch;
    };

    /**
     * Output structure for getRevenueInQcapPerEpoch function
     * @param epochTotalRevenue Total revenue for the specified epoch
     * @param epochOneQcapRevenue Revenue per QCAP token for the epoch
     * @param epochOneQvaultRevenue Revenue per QVAULT share for the epoch
     * @param epochReinvestAmount Amount allocated for reinvestment in the epoch
     */
    struct getRevenueInQcapPerEpoch_output
    {
        uint64 epochTotalRevenue;
        uint64 epochOneQcapRevenue;
        uint64 epochOneQvaultRevenue;
        uint64 epochReinvestAmount;
    };

    /**
     * Retrieves detailed revenue information for a specific epoch
     * Includes total revenue, per-token revenue, and reinvestment amounts
     * Useful for historical analysis and user reward calculations
     * 
     * @param input Epoch number to query
     * @param output Detailed revenue breakdown for the epoch
     */
    PUBLIC_FUNCTION(getRevenueInQcapPerEpoch)
    {
        output.epochTotalRevenue = state.revenueInQcapPerEpoch.get(input.epoch);
        output.epochOneQcapRevenue = state.revenueForOneQcapPerEpoch.get(input.epoch);
        output.epochOneQvaultRevenue = state.revenueForOneQvaultPerEpoch.get(input.epoch);
        output.epochReinvestAmount = state.revenueForReinvestPerEpoch.get(input.epoch);
    }

    /**
     * Input structure for getRevenuePerShare function
     * @param contractIndex Index of the contract to get revenue for
     */
    struct getRevenuePerShare_input
    {
        uint32 contractIndex;
    };

    /**
     * Output structure for getRevenuePerShare function
     * @param revenue Revenue amount for the specified contract
     */
    struct getRevenuePerShare_output
    {
        uint64 revenue;
    };

    /**
     * Retrieves the revenue amount for a specific contract index
     * Used for tracking revenue distribution across different contracts
     * 
     * @param input Contract index to query
     * @param output Revenue amount for the contract
     */
    PUBLIC_FUNCTION(getRevenuePerShare)
    {
        output.revenue = state.revenuePerShare.get(input.contractIndex);
    }

    /**
     * Input structure for getAmountOfShareQvaultHold function
     * @param assetInfo Asset information (name and issuer) to check
     */
    struct getAmountOfShareQvaultHold_input
    {
        Asset assetInfo;
    };

    /**
     * Output structure for getAmountOfShareQvaultHold function
     * @param amount Amount of shares held by QVAULT contract
     */
    struct getAmountOfShareQvaultHold_output
    {
        uint32 amount;
    };

    /**
     * Retrieves the amount of a specific asset held by the QVAULT contract
     * Useful for checking contract's asset holdings and portfolio composition
     * 
     * @param input Asset information to query
     * @param output Amount of shares held by the contract
     */
    PUBLIC_FUNCTION(getAmountOfShareQvaultHold)
    {
        output.amount = (uint32)qpi.numberOfShares(input.assetInfo, AssetOwnershipSelect::byOwner(SELF), AssetPossessionSelect::byPossessor(SELF));    
    }

    /**
     * Input structure for getNumberOfHolderAndAvgAm function
     * No input parameters required
     */
    struct getNumberOfHolderAndAvgAm_input
    {

    };

    /**
     * Output structure for getNumberOfHolderAndAvgAm function
     * @param numberOfQcapHolder Number of unique QCAP token holders
     * @param avgAmount Average QCAP amount per holder
     */
    struct getNumberOfHolderAndAvgAm_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        uint32 numberOfQcapHolder;
        uint32 avgAmount;
    };

    /**
     * Local variables for getNumberOfHolderAndAvgAm function
     */
    struct getNumberOfHolderAndAvgAm_locals
    {
        AssetPossessionIterator iter;   // Iterator for asset possession
        Asset QCAPId;                   // QCAP asset identifier
        uint32 count;                   // Counter for holders
        uint32 numberOfDuplicatedPossesor; // Counter for duplicate possessors
    };

    /**
     * Calculates the number of unique QCAP token holders and average amount per holder
     * Accounts for duplicate possessors across different contracts
     * Useful for token distribution analysis and economic metrics
     * 
     * @param input No input parameters
     * @param output Number of holders and average amount per holder
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getNumberOfHolderAndAvgAm)
    {
        locals.QCAPId.assetName = QVAULT_QCAP_ASSETNAME;
        locals.QCAPId.issuer = state.QCAP_ISSUER;

        locals.iter.begin(locals.QCAPId);
        while (!locals.iter.reachedEnd())
        {
            if (locals.iter.numberOfPossessedShares() > 0 && locals.iter.possessor() != SELF)
            {
                locals.count++;
            }

            if (qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.iter.possessor(), locals.iter.possessor(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) > 0 && qpi.numberOfPossessedShares(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, locals.iter.possessor(), locals.iter.possessor(), QVAULT_CONTRACT_INDEX, QVAULT_CONTRACT_INDEX) > 0 && locals.iter.possessor() != SELF)
            {
                locals.numberOfDuplicatedPossesor++;
            }

            locals.iter.next();
        }
    
        locals.numberOfDuplicatedPossesor = (uint32)div(locals.numberOfDuplicatedPossesor * 1ULL, 2ULL);

        output.numberOfQcapHolder = locals.count - locals.numberOfDuplicatedPossesor;
        output.avgAmount = (uint32)div(qpi.numberOfShares(locals.QCAPId) - qpi.numberOfShares(locals.QCAPId, AssetOwnershipSelect::byOwner(SELF), AssetPossessionSelect::byPossessor(SELF)) + state.totalStakedQcapAmount * 1ULL, output.numberOfQcapHolder * 1ULL);
        output.returnCode = QVAULT_SUCCESS;
    }

    /**
     * Input structure for getAmountForQearnInUpcomingEpoch function
     * @param epoch Future epoch to check QEarn funding for
     */
    struct getAmountForQearnInUpcomingEpoch_input
    {
        uint32 epoch;
    };

    /**
     * Output structure for getAmountForQearnInUpcomingEpoch function
     * @param amount Total amount allocated for QEarn in the specified epoch
     */
    struct getAmountForQearnInUpcomingEpoch_output
    {
        sint32 returnCode;                 // Status code indicating success or failure
        uint64 amount;
    };

    /**
     * Local variables for getAmountForQearnInUpcomingEpoch function
     */
    struct getAmountForQearnInUpcomingEpoch_locals
    {
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Calculates the total amount allocated for QEarn participation in a future epoch
     * Only considers passed QEarn proposals that are still active
     * Returns 0 for past or current epochs
     * 
     * @param input Future epoch number to check
     * @param output Total QEarn funding amount for the epoch
     */
    PUBLIC_FUNCTION_WITH_LOCALS(getAmountForQearnInUpcomingEpoch)
    {
        if (input.epoch <= qpi.epoch())
        {
            output.returnCode = QVAULT_INPUT_ERROR;
            return ;
        }
        for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QEarnP.get(locals._t).proposedEpoch + 1 < qpi.epoch() && state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).numberOfEpoch + state.QEarnP.get(locals._t).proposedEpoch + 1 >= input.epoch)
            {
                output.amount += state.QEarnP.get(locals._t).assignedFundPerEpoch;
            }
        }
        output.returnCode = QVAULT_SUCCESS;
    }

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(getData, 1);
        REGISTER_USER_FUNCTION(getStakedAmountAndVotingPower, 2);
        REGISTER_USER_FUNCTION(getGP, 3);
        REGISTER_USER_FUNCTION(getQCP, 4);
        REGISTER_USER_FUNCTION(getIPOP, 5);
        REGISTER_USER_FUNCTION(getQEarnP, 6);
        REGISTER_USER_FUNCTION(getFundP, 7);
        REGISTER_USER_FUNCTION(getMKTP, 8);
        REGISTER_USER_FUNCTION(getAlloP, 9);
        REGISTER_USER_FUNCTION(getIdentitiesHvVtPw, 11);
        REGISTER_USER_FUNCTION(ppCreationPower, 12);
        REGISTER_USER_FUNCTION(getQcapBurntAmountInLastEpoches, 13);
        REGISTER_USER_FUNCTION(getAmountToBeSoldPerYear, 14);
        REGISTER_USER_FUNCTION(getTotalRevenueInQcap, 15);
        REGISTER_USER_FUNCTION(getRevenueInQcapPerEpoch, 16);
        REGISTER_USER_FUNCTION(getRevenuePerShare, 17);
        REGISTER_USER_FUNCTION(getAmountOfShareQvaultHold, 18);
        REGISTER_USER_FUNCTION(getNumberOfHolderAndAvgAm, 19);
        REGISTER_USER_FUNCTION(getAmountForQearnInUpcomingEpoch, 20);

        REGISTER_USER_PROCEDURE(stake, 1);
        REGISTER_USER_PROCEDURE(unStake, 2);
        REGISTER_USER_PROCEDURE(submitGP, 3);
        REGISTER_USER_PROCEDURE(submitQCP, 4);
        REGISTER_USER_PROCEDURE(submitIPOP, 5);
        REGISTER_USER_PROCEDURE(submitQEarnP, 6);
        REGISTER_USER_PROCEDURE(submitFundP, 7);
        REGISTER_USER_PROCEDURE(submitMKTP, 8);
        REGISTER_USER_PROCEDURE(submitAlloP, 9);
        REGISTER_USER_PROCEDURE(voteInProposal, 11);
        REGISTER_USER_PROCEDURE(buyQcap, 12);
        REGISTER_USER_PROCEDURE(TransferShareManagementRights, 13);
    }

	INITIALIZE()
    {
        state.QCAP_ISSUER = ID(_Q, _C, _A, _P, _W, _M, _Y, _R, _S, _H, _L, _B, _J, _H, _S, _T, _T, _Z, _Q, _V, _C, _I, _B, _A, _R, _V, _O, _A, _S, _K, _D, _E, _N, _A, _S, _A, _K, _N, _O, _B, _R, _G, _P, _F, _W, _W, _K, _R, _C, _U, _V, _U, _A, _X, _Y, _E);
        state.qcapSoldAmount = 1652235;
        state.transferRightsFee = 100;
        state.quorumPercent = 670;
        state.qcapBurnPermille = 0;
        state.burnPermille = 0;
        state.QCAPHolderPermille = 500;
        state.reinvestingPermille = 450;
        state.shareholderDividend = 30;

    }

    /**
     * Local variables for BEGIN_EPOCH function
     */
    struct BEGIN_EPOCH_locals
    {
        QEARN::lock_input lock_input;                                           // Input for QEARN lock function
        QEARN::lock_output lock_output;                                         // Output from QEARN lock function
        QX::AssetAskOrders_input assetAskOrders_input;                          // Input for QX asset ask orders
        QX::AssetAskOrders_output assetAskOrders_output;                        // Output from QX asset ask orders
        QX::AddToBidOrder_input addToBidOrder_input;                            // Input for QX add to bid order
        QX::AddToBidOrder_output addToBidOrder_output;                          // Output from QX add to bid order
        QX::TransferShareManagementRights_input transferShareManagementRights_input;    // Input for QX transfer rights
        QX::TransferShareManagementRights_output transferShareManagementRights_output;  // Output from QX transfer rights
        uint32 purchasedQcap;                                                   // Amount of QCAP purchased for burning
        sint32 _t;                                                              // Loop counter variable
    };

    /**
     * Executes at the beginning of each epoch
     * Handles QEarn fund locking, quorum changes, allocation updates, and QCAP burning
     * Processes passed proposals and updates contract state accordingly
     */
    BEGIN_EPOCH_WITH_LOCALS()
    {
        for (locals._t = 0 ; locals._t < (sint32)state.numberOfQEarnP; locals._t++)
        {
            if (state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).proposedEpoch + state.QEarnP.get(locals._t).numberOfEpoch + 1 >= qpi.epoch())
            {
                if (state.QEarnP.get(locals._t).proposedEpoch + 1 == qpi.epoch())
                {
                    continue;
                }
                INVOKE_OTHER_CONTRACT_PROCEDURE(QEARN, lock, locals.lock_input, locals.lock_output, state.QEarnP.get(locals._t).assignedFundPerEpoch);
            }
        }

        for (locals._t = state.numberOfQCP - 1 ; locals._t >= 0; locals._t--)
        {
            if (state.QCP.get(locals._t).result == 0 && state.QCP.get(locals._t).proposedEpoch + 2 == qpi.epoch())
            {
                state.quorumPercent = state.QCP.get(locals._t).newQuorumPercent;
                break;
            }
            if (state.QCP.get(locals._t).proposedEpoch + 2 < qpi.epoch())
            {
                break;
            }
        }

        for (locals._t = state.numberOfAlloP - 1; locals._t >= 0; locals._t--)
        {
            if (state.AlloP.get(locals._t).result == 0 && state.AlloP.get(locals._t).proposedEpoch + 2 == qpi.epoch())
            {
                state.QCAPHolderPermille = state.AlloP.get(locals._t).distributed;
                state.reinvestingPermille = state.AlloP.get(locals._t).reinvested;
                state.qcapBurnPermille = state.AlloP.get(locals._t).burnQcap;
                break;
            }
            if (state.AlloP.get(locals._t).proposedEpoch + 2 < qpi.epoch())
            {
                break;
            }
        }

        locals.purchasedQcap = 0;
        while(state.fundForBurn > 0)
        {
            locals.assetAskOrders_input.assetName = QVAULT_QCAP_ASSETNAME;
            locals.assetAskOrders_input.issuer = state.QCAP_ISSUER;
            locals.assetAskOrders_input.offset = 0;
            CALL_OTHER_CONTRACT_FUNCTION(QX, AssetAskOrders, locals.assetAskOrders_input, locals.assetAskOrders_output);
            if (locals.assetAskOrders_output.orders.get(0).price <= (sint64)state.fundForBurn && locals.assetAskOrders_output.orders.get(0).price != 0)
            {
                locals.addToBidOrder_input.assetName = QVAULT_QCAP_ASSETNAME;
                locals.addToBidOrder_input.issuer = state.QCAP_ISSUER;
                locals.addToBidOrder_input.numberOfShares = (sint64)div(state.fundForBurn, locals.assetAskOrders_output.orders.get(0).price * 1ULL) > locals.assetAskOrders_output.orders.get(0).numberOfShares ? locals.assetAskOrders_output.orders.get(0).numberOfShares : div(state.fundForBurn, locals.assetAskOrders_output.orders.get(0).price * 1ULL);
                locals.addToBidOrder_input.price = locals.assetAskOrders_output.orders.get(0).price;

                INVOKE_OTHER_CONTRACT_PROCEDURE(QX, AddToBidOrder, locals.addToBidOrder_input, locals.addToBidOrder_output, locals.addToBidOrder_input.price * locals.addToBidOrder_input.numberOfShares);

                state.fundForBurn -= locals.addToBidOrder_input.price * locals.addToBidOrder_input.numberOfShares;
                state.burntQcapAmPerEpoch.set(qpi.epoch(), state.burntQcapAmPerEpoch.get(qpi.epoch()) + (uint32)locals.addToBidOrder_input.numberOfShares);
                state.totalQcapBurntAmount += (uint32)locals.addToBidOrder_input.numberOfShares;

                locals.purchasedQcap += (uint32)locals.addToBidOrder_output.addedNumberOfShares;
            }
            else 
            {
                break;
            }
        }

        locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
        locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
        locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
        locals.transferShareManagementRights_input.numberOfShares = locals.purchasedQcap;

        INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

        qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, locals.purchasedQcap, NULL_ID);
    }

    /**
     * Local variables for END_EPOCH function
     */
    struct END_EPOCH_locals 
    {
        QX::TransferShareManagementRights_input transferShareManagementRights_input;    // Input for QX transfer rights
        QX::TransferShareManagementRights_output transferShareManagementRights_output;  // Output from QX transfer rights
        GPInfo updatedGProposal;                    // Updated general proposal
        QCPInfo updatedQCProposal;                  // Updated quorum change proposal
        IPOPInfo updatedIPOProposal;                // Updated IPO participation proposal
        QEarnPInfo updatedQEarnProposal;            // Updated QEarn participation proposal
        FundPInfo updatedFundProposal;              // Updated fundraising proposal
        MKTPInfo updatedMKTProposal;                // Updated marketplace proposal
        AlloPInfo updatedAlloProposal;              // Updated allocation proposal
        Entity entity;                               // Entity information
        AssetPossessionIterator iter;                // Iterator for asset possession
        Asset QCAPId;                                // QCAP asset identifier
        id possessorPubkey;                          // Possessor public key
        uint64 paymentForShareholders;               // Payment amount for shareholders
        uint64 paymentForQCAPHolders;                // Payment amount for QCAP holders
        uint64 paymentForReinvest;                   // Payment amount for reinvestment
        uint64 amountOfBurn;                         // Amount to burn
        uint64 circulatedSupply;                     // Circulating supply of QCAP
        uint64 requiredFund;                         // Required fund amount
        uint64 tmpAmount;                            // Temporary amount variable
        uint64 paymentForQcapBurn;                   // Payment amount for QCAP burning
        uint32 numberOfVote;                         // Number of votes
        sint32 _t, _r;                               // Loop counter variables
        uint32 curDate;                              // Current date (packed)
        uint8 year;                                  // Current year
        uint8 month;                                 // Current month
        uint8 day;                                   // Current day
        uint8 hour;                                  // Current hour
        uint8 minute;                                // Current minute
        uint8 second;                                // Current second
    };

    /**
     * Executes at the end of each epoch
     * Distributes revenue to stakeholders, processes proposal results, and updates voting power
     * Handles all proposal voting outcomes and state updates
     */
    END_EPOCH_WITH_LOCALS()
    {
        locals.paymentForShareholders = div(state.totalEpochRevenue * state.shareholderDividend, 1000ULL);
        locals.paymentForQCAPHolders = div(state.totalEpochRevenue * state.QCAPHolderPermille, 1000ULL);
        locals.paymentForReinvest = div(state.totalEpochRevenue * state.reinvestingPermille, 1000ULL);
        locals.paymentForQcapBurn = div(state.totalEpochRevenue * state.qcapBurnPermille, 1000ULL);
        locals.amountOfBurn = div(state.totalEpochRevenue * state.burnPermille, 1000ULL);

        qpi.distributeDividends(div(locals.paymentForShareholders + state.proposalCreateFund, 676ULL));
        qpi.burn(locals.amountOfBurn);

        locals.QCAPId.assetName = QVAULT_QCAP_ASSETNAME;
        locals.QCAPId.issuer = state.QCAP_ISSUER;

        locals.circulatedSupply = qpi.numberOfShares(locals.QCAPId) - qpi.numberOfShares(locals.QCAPId, AssetOwnershipSelect::byOwner(SELF), AssetPossessionSelect::byPossessor(SELF)) + state.totalStakedQcapAmount;

        state.revenueForOneQcapPerEpoch.set(qpi.epoch(), div(locals.paymentForQCAPHolders, locals.circulatedSupply * 1ULL));
        state.revenueForOneQvaultPerEpoch.set(qpi.epoch(), div(locals.paymentForShareholders + state.proposalCreateFund, 676ULL));
        state.revenueForReinvestPerEpoch.set(qpi.epoch(), locals.paymentForReinvest);

        state.reinvestingFund += locals.paymentForReinvest;
        state.fundForBurn += locals.paymentForQcapBurn;
        state.totalEpochRevenue = 0;
        state.proposalCreateFund = 0;

        locals.iter.begin(locals.QCAPId);
        while (!locals.iter.reachedEnd())
        {
            locals.possessorPubkey = locals.iter.possessor();

            if (locals.possessorPubkey != SELF)
            {
                qpi.transfer(locals.possessorPubkey, div(locals.paymentForQCAPHolders, locals.circulatedSupply) * locals.iter.numberOfPossessedShares());
            }

            locals.iter.next();
        }

        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            qpi.transfer(state.staker.get(locals._t).stakerAddress, div(locals.paymentForQCAPHolders, locals.circulatedSupply) * state.staker.get(locals._t).amount);
        }

        /*
            General Proposal Result
        */

        for (locals._t = state.numberOfGP - 1; locals._t >= 0; locals._t--)
        {
            if (state.GP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedGProposal = state.GP.get(locals._t);
                if (state.GP.get(locals._t).numberOfYes + state.GP.get(locals._t).numberOfNo < div(state.GP.get(locals._t).currentQuorumPercent * state.GP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedGProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.GP.get(locals._t).numberOfYes <= state.GP.get(locals._t).numberOfNo)
                {
                    locals.updatedGProposal.result = QVAULT_PROPOSAL_REJECTED;
                }
                else 
                {
                    locals.updatedGProposal.result = QVAULT_PROPOSAL_PASSED;
                }
                state.GP.set(locals._t, locals.updatedGProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            Quorum Proposal Result
        */

        locals.numberOfVote = 0;

        for (locals._t = state.numberOfQCP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QCP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedQCProposal = state.QCP.get(locals._t);
                if (state.QCP.get(locals._t).numberOfYes + state.QCP.get(locals._t).numberOfNo < div(state.QCP.get(locals._t).currentQuorumPercent * state.QCP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedQCProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.QCP.get(locals._t).numberOfYes <= state.QCP.get(locals._t).numberOfNo)
                {
                    locals.updatedQCProposal.result = QVAULT_PROPOSAL_REJECTED;
                }   
                else 
                {
                    locals.updatedQCProposal.result = QVAULT_PROPOSAL_PASSED;
                    if (locals.numberOfVote < state.QCP.get(locals._t).numberOfYes)
                    {
                        locals.numberOfVote = state.QCP.get(locals._t).numberOfYes;
                    }
                    
                }
                state.QCP.set(locals._t, locals.updatedQCProposal);
            }
            else 
            {
                break;
            }
        }

        for (locals._t = state.numberOfQCP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QCP.get(locals._t).proposedEpoch == qpi.epoch() && state.QCP.get(locals._t).numberOfYes != locals.numberOfVote)
            {
                locals.updatedQCProposal = state.QCP.get(locals._t);
                locals.updatedQCProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_VOTING_POWER;
                state.QCP.set(locals._t, locals.updatedQCProposal);
            }
            if (state.QCP.get(locals._t).proposedEpoch != qpi.epoch())
            {
                break;
            }
        }

        /*
            IPO Proposal Result
        */

        locals.requiredFund = 0;

        for (locals._t = state.numberOfIPOP - 1; locals._t >= 0; locals._t--)
        {
            if (state.IPOP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedIPOProposal = state.IPOP.get(locals._t);
                if (state.IPOP.get(locals._t).numberOfYes + state.IPOP.get(locals._t).numberOfNo < div(state.IPOP.get(locals._t).currentQuorumPercent * state.IPOP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedIPOProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.IPOP.get(locals._t).numberOfYes <= state.IPOP.get(locals._t).numberOfNo)
                {
                    locals.updatedIPOProposal.result = QVAULT_PROPOSAL_REJECTED;
                }
                else 
                {
                    locals.updatedIPOProposal.result = QVAULT_PROPOSAL_PASSED;
                    locals.requiredFund += div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL);
                    locals.updatedIPOProposal.assignedFund = div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL);
                }
                state.IPOP.set(locals._t, locals.updatedIPOProposal);
            }
            else 
            {
                break;
            }
        }

        locals.tmpAmount = 0;

        for (locals._t = state.numberOfIPOP - 1; locals._t >= 0; locals._t--)
        {
            if (state.IPOP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                if (state.IPOP.get(locals._t).result != 0)
                {
                    continue;
                }
                locals.updatedIPOProposal = state.IPOP.get(locals._t);
                if (state.reinvestingFund < locals.requiredFund)
                {
                    locals.updatedIPOProposal.assignedFund = div(div(div(locals.updatedIPOProposal.totalWeight * 1ULL, locals.updatedIPOProposal.numberOfYes * 1ULL) * 1000, locals.requiredFund) * state.reinvestingFund, 1000ULL);
                }
                state.IPOP.set(locals._t, locals.updatedIPOProposal);

                for (locals._r = 675 ; locals._r >= 0; locals._r--)
                {
                    if ((676 - locals._r) * (qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r) + 1) > (sint64)locals.updatedIPOProposal.assignedFund)
                    {
                        if (locals._r == 675)
                        {
                            break;
                        }
                        qpi.bidInIPO(locals.updatedIPOProposal.ipoContractIndex, qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r + 1) + 1, 675 - locals._r);
                        locals.tmpAmount += (qpi.ipoBidPrice(locals.updatedIPOProposal.ipoContractIndex, locals._r + 1) + 1) * (675 - locals._r);
                        break;
                    }
                }
            }
            else 
            {
                break;
            }
        }

        state.reinvestingFund -= locals.tmpAmount;

        /*
            QEarn Proposal Result  
        */

        locals.requiredFund = 0;

        for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
        {
            if (state.QEarnP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedQEarnProposal = state.QEarnP.get(locals._t);
                if (state.QEarnP.get(locals._t).numberOfYes + state.QEarnP.get(locals._t).numberOfNo < div(state.QEarnP.get(locals._t).currentQuorumPercent * state.QEarnP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedQEarnProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.QEarnP.get(locals._t).numberOfYes <= state.QEarnP.get(locals._t).numberOfNo)
                {
                    locals.updatedQEarnProposal.result = QVAULT_PROPOSAL_REJECTED;
                }
                else 
                {
                    locals.updatedQEarnProposal.result = QVAULT_PROPOSAL_PASSED;
                    locals.requiredFund += locals.updatedQEarnProposal.amountOfInvestPerEpoch * locals.updatedQEarnProposal.numberOfEpoch;
                }
                state.QEarnP.set(locals._t, locals.updatedQEarnProposal);
            }
            else 
            {
                break;
            }
        }

        locals.tmpAmount = 0;

        if (state.reinvestingFund < locals.requiredFund)
        {
            for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
            {
                if (state.QEarnP.get(locals._t).proposedEpoch == qpi.epoch())
                {
                    if (state.QEarnP.get(locals._t).result != 0)
                    {
                        continue;
                    }
                    locals.updatedQEarnProposal = state.QEarnP.get(locals._t);
                    locals.updatedQEarnProposal.assignedFundPerEpoch = div(div(div(locals.updatedQEarnProposal.numberOfEpoch * locals.updatedQEarnProposal.amountOfInvestPerEpoch * 1000, locals.requiredFund) * state.reinvestingFund, 1000ULL), locals.updatedQEarnProposal.numberOfEpoch * 1ULL);
                    locals.tmpAmount += locals.updatedQEarnProposal.assignedFundPerEpoch * locals.updatedQEarnProposal.numberOfEpoch;
                    state.QEarnP.set(locals._t, locals.updatedQEarnProposal);
                }
                else 
                {
                    break;
                }
            }
            state.reinvestingFund -= locals.tmpAmount;
        }

        else 
        {
            state.reinvestingFund -= locals.requiredFund;
        }

        /*
            Fundrasing Proposal Result
        */

        for (locals._t = state.numberOfFundP - 1; locals._t >= 0; locals._t--)
        {
            if (state.FundP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedFundProposal = state.FundP.get(locals._t);
                if (state.FundP.get(locals._t).numberOfYes + state.FundP.get(locals._t).numberOfNo < div(state.FundP.get(locals._t).currentQuorumPercent * state.FundP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedFundProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.FundP.get(locals._t).numberOfYes <= state.FundP.get(locals._t).numberOfNo)
                {
                    locals.updatedFundProposal.result = QVAULT_PROPOSAL_REJECTED;
                }
                else 
                {
                    locals.updatedFundProposal.result = QVAULT_PROPOSAL_PASSED;

                    packQvaultDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
                    unpackQvaultDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
                    if (locals.year == 25 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                    }
                    else if (locals.year == 26 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                    }
                    else if (locals.year == 27 && state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedFundProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                    }
                    else if (state.qcapSoldAmount + locals.updatedFundProposal.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
                    {
                        locals.updatedFundProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                    }
                }
                state.FundP.set(locals._t, locals.updatedFundProposal);
            }
            else 
            {
                break;
            }
        }

        /*
            Marketplace Proposal Result
        */

        for (locals._t = state.numberOfMKTP - 1; locals._t >= 0; locals._t--)
        {
            if (state.MKTP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedMKTProposal = state.MKTP.get(locals._t);
                if (state.MKTP.get(locals._t).numberOfYes + state.MKTP.get(locals._t).numberOfNo < div(state.MKTP.get(locals._t).currentQuorumPercent * state.MKTP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                    qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                }
                else if (state.MKTP.get(locals._t).numberOfYes <= state.MKTP.get(locals._t).numberOfNo)
                {
                    locals.updatedMKTProposal.result = QVAULT_PROPOSAL_REJECTED;
                    qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                }
                else 
                {
                    locals.updatedMKTProposal.result = QVAULT_PROPOSAL_PASSED;

                    packQvaultDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
                    unpackQvaultDate(locals.year, locals.month, locals.day, locals.hour, locals.minute, locals.second, locals.curDate);
                    if (locals.year == 25 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2025MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (locals.year == 26 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2026MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (locals.year == 27 && state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_2027MAX_QCAP_SALE_AMOUNT)
                    {
                        locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (state.qcapSoldAmount + locals.updatedMKTProposal.amountOfQcap > QVAULT_QCAP_MAX_SUPPLY)
                    {
                        locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else if (state.reinvestingFund < locals.updatedMKTProposal.amountOfQubic)
                    {
                        locals.updatedMKTProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QCAP;
                        qpi.transferShareOwnershipAndPossession(locals.updatedMKTProposal.shareName, NULL_ID, SELF, SELF, locals.updatedMKTProposal.amountOfShare, locals.updatedMKTProposal.proposer);
                    }
                    else 
                    {
                        state.qcapSoldAmount += locals.updatedMKTProposal.amountOfQcap;

                        qpi.transfer(locals.updatedMKTProposal.proposer, locals.updatedMKTProposal.amountOfQubic);

                        state.reinvestingFund -= locals.updatedMKTProposal.amountOfQubic;

                        locals.transferShareManagementRights_input.asset.assetName = QVAULT_QCAP_ASSETNAME;
                        locals.transferShareManagementRights_input.asset.issuer = state.QCAP_ISSUER;
                        locals.transferShareManagementRights_input.newManagingContractIndex = SELF_INDEX;
                        locals.transferShareManagementRights_input.numberOfShares = locals.updatedMKTProposal.amountOfQcap;

                        INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

                        qpi.transferShareOwnershipAndPossession(QVAULT_QCAP_ASSETNAME, state.QCAP_ISSUER, SELF, SELF, locals.updatedMKTProposal.amountOfQcap, locals.updatedMKTProposal.proposer);
                    }
                }
                state.MKTP.set(locals._t, locals.updatedMKTProposal);
            }
            else 
            {
                break;
            }
        }
        

        /*
            Allocation Proposal Result
        */

        locals.numberOfVote = 0;

        for (locals._t = state.numberOfAlloP - 1; locals._t >= 0; locals._t--)
        {
            if (state.AlloP.get(locals._t).proposedEpoch == qpi.epoch())
            {
                locals.updatedAlloProposal = state.AlloP.get(locals._t);
                if (state.AlloP.get(locals._t).numberOfYes + state.AlloP.get(locals._t).numberOfNo < div(state.AlloP.get(locals._t).currentQuorumPercent * state.AlloP.get(locals._t).currentTotalVotingPower * 1ULL, 1000ULL))
                {
                    locals.updatedAlloProposal.result = QVAULT_PROPOSAL_INSUFFICIENT_QUORUM;
                }
                else if (state.AlloP.get(locals._t).numberOfYes <= state.AlloP.get(locals._t).numberOfNo)
                {
                    locals.updatedAlloProposal.result = QVAULT_PROPOSAL_REJECTED;
                }
                else 
                {
                    locals.updatedAlloProposal.result = QVAULT_PROPOSAL_PASSED;
                    if (locals.numberOfVote < state.AlloP.get(locals._t).numberOfYes)
                    {
                        locals.numberOfVote = state.AlloP.get(locals._t).numberOfYes;
                    }
                    
                }
                state.AlloP.set(locals._t, locals.updatedAlloProposal);
            }
            else 
            {
                break;
            }
        }

        for (locals._t = state.numberOfAlloP - 1; locals._t >= 0; locals._t--)
        {
            if (state.AlloP.get(locals._t).proposedEpoch == qpi.epoch() && state.AlloP.get(locals._t).numberOfYes != locals.numberOfVote)
            {
                locals.updatedAlloProposal = state.AlloP.get(locals._t);
                locals.updatedAlloProposal.result = 3;
                state.AlloP.set(locals._t, locals.updatedAlloProposal);
            }
            if (state.AlloP.get(locals._t).proposedEpoch != qpi.epoch())
            {
                break;
            }
        }

        state.totalVotingPower = 0;
        for (locals._t = 0 ; locals._t < (sint32)state.numberOfStaker; locals._t++)
        {
            state.votingPower.set(locals._t, state.staker.get(locals._t));
            state.totalVotingPower += state.staker.get(locals._t).amount;
        }
        state.numberOfVotingPower = state.numberOfStaker;

        state.vote.reset();
        state.countOfVote.reset();

    }

    /**
     * Local variables for POST_INCOMING_TRANSFER function
     */
    struct POST_INCOMING_TRANSFER_locals
    {
        AssetPossessionIterator iter;   // Iterator for asset possession
        Asset QCAPId;                   // QCAP asset identifier
        id possessorPubkey;             // Possessor public key
        uint64 lockedFund;              // Amount of locked funds
        sint32 _t;                      // Loop counter variable
    };

    /**
     * Handles incoming transfers to the contract
     * Processes different transfer types (standard, QPI, IPO refunds, dividends)
     * Updates revenue tracking and fund allocations accordingly
     */
    POST_INCOMING_TRANSFER_WITH_LOCALS()
    {
        switch (input.type)
        {
            case TransferType::standardTransaction:
                state.totalHistoryRevenue += input.amount;
                state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount);

                state.totalEpochRevenue += input.amount;
            break;
            case TransferType::qpiTransfer:
                if (input.sourceId.u64._0 == QEARN_CONTRACT_INDEX)
                {
                    locals.lockedFund = 0;
                    for (locals._t = state.numberOfQEarnP - 1; locals._t >= 0; locals._t--)
                    {
                        if (state.QEarnP.get(locals._t).result == 0 && state.QEarnP.get(locals._t).proposedEpoch + 54 == qpi.epoch())
                        {
                            locals.lockedFund += state.QEarnP.get(locals._t).assignedFundPerEpoch;
                        }
                    }
                    state.reinvestingFund += locals.lockedFund;
                    state.totalEpochRevenue += input.amount - locals.lockedFund;
                    state.totalHistoryRevenue += input.amount - locals.lockedFund;
                    state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount - locals.lockedFund);
                    state.revenueByQearn += input.amount - locals.lockedFund;

                }
                else
                {
                    state.totalHistoryRevenue += input.amount;
                    state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount);

                    state.totalEpochRevenue += input.amount;
                }
                break;
            case TransferType::ipoBidRefund:
                state.reinvestingFund += input.amount;
                break;
            case TransferType::qpiDistributeDividends:
                state.totalHistoryRevenue += input.amount;
                state.revenueInQcapPerEpoch.set(qpi.epoch(), state.revenueInQcapPerEpoch.get(qpi.epoch()) + input.amount);
                state.revenuePerShare.set(input.sourceId.u64._0, state.revenuePerShare.get(input.sourceId.u64._0) + input.amount);
                
                state.totalEpochRevenue += input.amount;
                break;
            default:
                break;
        }
    }

    /**
     * Pre-acquire shares hook function
     * Always allows share transfers to the contract
     * Used for controlling share acquisition behavior
     */
    PRE_ACQUIRE_SHARES()
    {
		output.allowTransfer = true;
    }
};