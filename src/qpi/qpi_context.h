#pragma once

#include "qpi_types.h"
#include "qpi_date_time.h"
#include "qpi_proposals.h"

// ASSERT can be used to support debugging and speed-up development
#include "../platform/assert.h"

namespace QPI
{
	// QPI context base class (common data, by default has no stack for locals)
	struct QpiContext
	{
	protected:
		// Construction is done in core, not allowed in contracts
		QpiContext(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward,
			unsigned char entryPoint
		) {
			init(contractIndex, originator, invocator, invocationReward, entryPoint, -1);
		}

		void init(
			unsigned int contractIndex,
			const m256i& originator,
			const m256i& invocator,
			long long invocationReward,
			unsigned char entryPoint,
			int stackIndex
		) {
			ASSERT(invocationReward >= 0);
			_currentContractIndex = contractIndex;
			_currentContractId = m256i(contractIndex, 0, 0, 0);
			_originator = originator;
			_invocator = invocator;
			_invocationReward = invocationReward;
			_entryPoint = entryPoint;
			_stackIndex = stackIndex;
		}

		unsigned int _currentContractIndex;
		int _stackIndex;
		m256i _currentContractId, _originator, _invocator;
		long long _invocationReward;
		unsigned char _entryPoint;

	private:
		// Disabling copy and move
		QpiContext(const QpiContext&) = delete;
		QpiContext(QpiContext&&) = delete;
		QpiContext& operator=(const QpiContext&) = delete;
		QpiContext& operator=(QpiContext&&) = delete;
	};

	// QPI function available to contract functions and procedures
	struct QpiContextFunctionCall : public QpiContext
	{
		inline id arbitrator(
		) const;

		inline id computor(
			uint16 computorIndex // [0..675]
		) const;

		inline uint8 day(
		) const; // [1..31]

		inline uint8 dayOfWeek(
			uint8 year, // (0 = 2000, 1 = 2001, ..., 99 = 2099)
			uint8 month,
			uint8 day
		) const; // [0..6] (0 = Wednesday)

		inline uint16 epoch(
		) const; // [0..9'999]

		inline bit getEntity(
			const id& id,
			Entity& entity
		) const; // Returns "true" if the entity has been found, returns "false" otherwise

		inline uint8 hour(
		) const; // [0..23]

		// Return the invocation reward (amount transferred to contract immediately before invoking)
		sint64 invocationReward() const { return _invocationReward; }

		// Returns the id of the user/contract who has triggered this contract; returns NULL_ID if there has been no user/contract
		id invocator() const { return _invocator; }

		// Returns the ID of the entity who has made this IPO bid or NULL_ID if the ipoContractIndex or ipoBidIndex are invalid.
		inline id ipoBidId(
			uint32 ipoContractIndex,
			uint32 ipoBidIndex
		) const;

		// Returns the price of an IPO bid, -1 if contract index is invalid, -2 if contract is not in IPO, -3 if bid index is invalid.
		inline sint64 ipoBidPrice(
			uint32 ipoContractIndex,
			uint32 ipoBidIndex
		) const;

		// Returns true if the id passed belongs to a contract (no user entity).
		inline bit isContractId(
			const id& id
		) const;

		template <typename T>
		inline id K12(
			const T& data
		) const;

		inline uint16 millisecond(
		) const; // [0..999]

		inline uint8 minute(
		) const; // [0..59]

		inline uint8 month(
		) const; // [1..12]

		inline id nextId(
			const id& currentId
		) const;

		inline id prevId(
			const id& currentId
		) const;

		inline sint64 numberOfPossessedShares(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			uint16 ownershipManagingContractIndex,
			uint16 possessionManagingContractIndex
		) const;

		inline sint64 numberOfShares(
			const Asset& asset,
			const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(),
			const AssetPossessionSelect& possession = AssetPossessionSelect::any()
		) const;

		inline bool isAssetIssued(
			const m256i& id,
			unsigned long long assetName
		) const;

		// Returns -1 if the current tick is empty, returns the number of the transactions in the tick otherwise, including 0.
		inline sint32 numberOfTickTransactions(
		) const;

		// Returns the id of the user who has triggered the whole chain of invocations with their transaction; returns NULL_ID if there has been no user
		id originator() const { return _originator; }

		inline uint8 second(
		) const; // [0..59]

		// return current datetime (year, month, day, hour, minute, second, millisec)
		inline DateAndTime now() const;

		// return last spectrum digest on etalonTick
		inline m256i getPrevSpectrumDigest() const;

		// return last universe digest on etalonTick
		inline m256i getPrevUniverseDigest() const;

		// return last computer digest on etalonTick
		inline m256i getPrevComputerDigest() const;

		// run the score function (in qubic mining) and return first 256 bit of output
		inline m256i computeMiningFunction(const m256i miningSeed, const m256i publicKey, const m256i nonce) const;

		inline bit signatureValidity(
			const id& entity,
			const id& digest,
			const Array<sint8, 64>& signature
		) const;

		inline uint32 tick(
		) const; // [0..999'999'999]

		inline uint8 year(
		) const; // [0..99] (0 = 2000, 1 = 2001, ..., 99 = 2099)

		// Return the amount of Qu in the fee reserve for the specified contract.
		// If the provided index is invalid (< 1 or >= contractCount) the currentContractIndex is used instead.
		inline sint64 queryFeeReserve(uint32 contractIndex = 0) const;

		/**
		* @brief Get oracle query by queryId.
		* @param queryId Identifier of oracle query to get query data from.
		* @param query Output query data (only set if true is returned).
		* @return Whether queryId is found and matches the oracle interface.
		*/
		template <typename OracleInterface>
		inline bool getOracleQuery(sint64 queryId, typename OracleInterface::OracleQuery& query) const;

		/**
		* @brief Get oracle reply by queryId.
		* @param queryId Identifier of oracle query.
		* @param reply Output reply data (only set if true is returned).
		* @return Whether queryId is found, matches the oracle interface, and a valid reply is available.
		*/
		template <typename OracleInterface>
		inline bool getOracleReply(sint64 queryId, typename OracleInterface::OracleReply& reply) const;

		/**
		* @brief Get status of oracle query by queryId.
		* @param queryId Identifier of oracle query to get query status from.
		* @return One of the values ORACLE_QUERY_STATUS_* listed below.
		*
		* - ORACLE_QUERY_STATUS_UNKNOWN: Query not found / not valid.
		* - ORACLE_QUERY_STATUS_PENDING: Query is being processed.
		* - ORACLE_QUERY_STATUS_COMMITTED: The quorum has committed to an oracle reply, but it has not been revealed yet.
		* - ORACLE_QUERY_STATUS_SUCCESS: The oracle reply has been confirmed and is available.
		* - ORACLE_QUERY_STATUS_UNRESOLVABLE: No valid oracle reply is available, because computors disagreed about the value.
		* - ORACLE_QUERY_STATUS_TIMEOUT: No valid oracle reply is available and timeout has hit.
		*/
		inline uint8 getOracleQueryStatus(sint64 queryId) const;

		/**
		* @brief Get status of an OC invocation by invocationId.
		* @param invocationId Identifier returned from INVOKE_OC().
		* @return One of the values OC_INVOCATION_STATUS_* listed below.
		*
		* - OC_INVOCATION_STATUS_UNKNOWN: Invocation not found / not valid.
		* - OC_INVOCATION_STATUS_PENDING_AUTH: Recorded; waiting for QUORUM authorization signatures.
		* - OC_INVOCATION_STATUS_AUTHORIZED: QUORUM signatures counted; bundle eligible for delivery.
		* - OC_INVOCATION_STATUS_TIMEOUT: Authorization did not reach quorum before the timeout.
		*/
		inline uint8 getOcInvocationStatus(sint64 invocationId) const;

		// Access proposal functions with qpi(proposalVotingObject).func().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalFunctionCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			const ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
			) const;

		// Internal functions, calling not allowed in contracts
		inline void* __qpiAllocLocals(unsigned int sizeOfLocals) const;
		inline void __qpiFreeLocals() const;
		inline const QpiContextFunctionCall* __qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex, InterContractCallError& callError) const;
		inline void __qpiFreeContext() const;
		inline void* __qpiAcquireStateForReading(unsigned int contractIndex) const;
		inline void __qpiReleaseStateForReading(unsigned int contractIndex) const;
		inline void __qpiAbort(unsigned int errorCode) const;

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextFunctionCall(unsigned int contractIndex, const m256i& originator, long long invocationReward, unsigned char entryPoint) : QpiContext(contractIndex, originator, originator, invocationReward, entryPoint) {}
	};



	template <typename OracleInterface>
	struct OracleNotificationInput
	{
		sint64 queryId;         ///< ID of the oracle query that led to this notification.
		sint32 subscriptionId;  ///< ID of the oracle subscription or -1 in case of a one-time oracle query.
		uint8 status;           ///< Oracle query status as defined in `network_messages/common_def.h`
		uint8 __reserved0;
		uint16 __reserved1;
		typename OracleInterface::OracleReply reply;	///< Oracle reply if status == ORACLE_QUERY_STATUS_SUCCESS
	};

	// QPI procedures available to contract procedures (not to contract functions)
	struct QpiContextProcedureCall : public QPI::QpiContextFunctionCall
	{
		inline sint64 acquireShares(
			const Asset& asset,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 sourceOwnershipManagingContractIndex,
			uint16 sourcePossessionManagingContractIndex,
			sint64 offeredTransferFee
		) const; // Returns payed fee on success (>= 0), -requestedFee if offeredTransferFee or contract balance is not sufficient, INVALID_AMOUNT in case of other error.

		// Burns Qus from the current contract's balance to fill the contract fee reserve of the contract specified via contractIndexBurnedFor.
		// If the provided index is invalid (< 1 or >= contractCount), the Qus are burned for the currentContractIndex.
		// Returns the remaining balance (>= 0) of the current contract if the burning is successful. A negative return value indicates failure.  
		inline sint64 burn(
			sint64 amount,
			uint32 contractIndexBurnedFor = 0
		) const;

		inline bool distributeDividends( //  Attempts to pay dividends
			sint64 amountPerShare // Total amount will be 676x of this
		) const; // "true" if the contract has had enough qus, "false" otherwise

		inline sint64 issueAsset(
			uint64 name,
			const id& issuer,
			sint8 numberOfDecimalPlaces,
			sint64 numberOfShares,
			uint64 unitOfMeasurement
		) const; // Returns number of shares or 0 on error

		// Bid in contract IPO, deducting price * quantity QU. Bids that don't get shares are refunded.
		// Returns number of bids registered or -1 if any invalid value is passed or the owned funds aren't sufficient.
		// If the return value >= 0, the full amount has been deducted, but if return value < quantity it has been partially
		// refunded.
		inline sint64 bidInIPO(
			uint32 IPOContractIndex,
			sint64 price,
			uint32 quantity
		) const;

		inline sint64 releaseShares(
			const Asset& asset,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			uint16 destinationOwnershipManagingContractIndex,
			uint16 destinationPossessionManagingContractIndex,
			sint64 offeredTransferFee
		) const; // Returns payed fee on success (>= 0), -requestedFee if offeredTransferFee or contract balance is not sufficient, INVALID_AMOUNT in case of other error.

		/**
		* @brief Add/change/cancel shareholder proposal as shareholder of another contract.
		* @param contractIndex Index of the other contract, that SELF is shareholder of and that the proposal is about.
		* @param proposalDataBuffer Buffer for passing the contract-dependent proposal data. You may use copyToBuffer() to fill it.
		* @param invocationReward Invocation reward sent to contractIndex when invoking it's procedure.
		* @return Proposal index on success, INVALID_PROPOSAL_INDEX on error.
		* @note Invokes SET_SHAREHOLDER_PROPOSAL of contractIndex without checking shareholder status and proposalDataBuffer.
		*/
		inline uint16 setShareholderProposal(
			uint16 contractIndex,
			const Array<uint8, 1024>& proposalDataBuffer,
			sint64 invocationReward
		) const;

		/**
		* @brief Add/change/cancel shareholder vote(s) in another contract.
		* @param contractIndex Index of the other contract, that SELF is shareholder of and that the proposal is about.
		* @param shareholderVoteData Vote(s) to cast. See ProposalMultiVoteDataV1 for details.
		* @param invocationReward Invocation reward sent to contractIndex when invoking it's procedure.
		* @return Proposal index on success, INVALID_PROPOSAL_INDEX on error.
		* @note Invokes SET_SHAREHOLDER_VOTES of contractIndex without checking shareholder status and shareholderVoteData.
		*/
		inline bool setShareholderVotes(
			uint16 contractIndex,
			const ProposalMultiVoteDataV1& shareholderVoteData,
			sint64 invocationReward
		) const;

		inline sint64 transfer( // Attempts to transfer energy from this qubic
			const id& destination, // Destination to transfer to, use NULL_ID to destroy the transferred energy
			sint64 amount // Energy amount to transfer, must be in [0..1'000'000'000'000'000] range
		) const; // Returns remaining energy amount; if the value is less than 0 then the attempt has failed, in this case the absolute value equals to the insufficient amount

		inline sint64 transferShareOwnershipAndPossession(
			uint64 assetName,
			const id& issuer,
			const id& owner,
			const id& possessor,
			sint64 numberOfShares,
			const id& newOwnerAndPossessor // New owner and possessor. Pass NULL_ID to burn shares (not allowed for contract shares).
		) const; // Returns remaining number of possessed shares satisfying all the conditions; if the value is less than 0, the attempt has failed, in this case the absolute value equals to the insufficient number, INVALID_AMOUNT indicates another error

		/// Unsubscribe oracle based on subscription ID (returning false if oracleSubscriptionId is invalid).
		inline bool unsubscribeOracle(
			sint32 oracleSubscriptionId
		) const;

		// Bring base class const operator() into scope (otherwise hidden by non-const overload below)
		using QpiContextFunctionCall::operator();

		// Access proposal procedures with qpi(proposalVotingObject).proc().
		template <typename ProposerAndVoterHandlingType, typename ProposalDataType>
		inline QpiContextProposalProcedureCall<ProposerAndVoterHandlingType, ProposalDataType> operator()(
			ProposalVoting<ProposerAndVoterHandlingType, ProposalDataType>& proposalVoting
			) const;


		// Internal functions, calling not allowed in contracts
		inline const QpiContextProcedureCall* __qpiConstructProcedureCallContext(unsigned int otherContractIndex, sint64 invocationReward, InterContractCallError& callError, bool skipFeeCheck = false) const;
		inline void* __qpiAcquireStateForWriting(unsigned int contractIndex) const;
		inline void __qpiReleaseStateForWriting(unsigned int contractIndex) const;
		template <unsigned int sysProcId, typename InputType, typename OutputType>
		bool __qpiCallSystemProc(unsigned int otherContractIndex, InputType& input, OutputType& output, sint64 invocationReward) const;
		inline void __qpiNotifyPostIncomingTransfer(const id& source, const id& dest, sint64 amount, uint8 type) const;

		// Internal version of QUERY_ORACLE (macro ensures that proc pointer and id match)
		template <typename OracleInterface, typename ContractStateType, typename LocalsType>
		inline sint64 __qpiQueryOracle(
			const typename OracleInterface::OracleQuery& query,
			void (*notificationProcPtr)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, OracleNotificationInput<OracleInterface>& input, NoData& output, LocalsType& locals),
			unsigned int notificationProcId,
			uint32 timeoutMillisec
		) const;

		// Internal version of SUBSCRIBE_ORACLE (macro ensures that proc pointer and id match)
		template <typename OracleInterface, typename ContractStateType, typename LocalsType>
		inline sint32 __qpiSubscribeOracle(
			const typename OracleInterface::OracleQuery& query,
			void (*notificationProcPtr)(const QPI::QpiContextProcedureCall& qpi, ContractStateType& state, OracleNotificationInput<OracleInterface>& input, NoData& output, LocalsType& locals),
			unsigned int notificationProcId,
			uint32 notificationPeriodInMilliseconds = 60000,
			bool notifyWithPreviousReply = true
		) const;

		// Internal version of INVOKE_OC.
		template <typename OcInterface>
		inline sint64 __qpiInvokeOC(
			const typename OcInterface::OcRequest& request
		) const;

		// Internal version of transfer() that takes the TransferType as additional argument.
		inline sint64 __transfer( // Attempts to transfer energy from this qubic
			const id& destination, // Destination to transfer to, use NULL_ID to destroy the transferred energy
			sint64 amount, // Energy amount to transfer, must be in [0..1'000'000'000'000'000] range
			uint8 transferType // the type of transfer
		) const; // Returns remaining energy amount; if the value is less than 0 then the attempt has failed, in this case the absolute value equals to the insufficient amount

	protected:
		// Construction is done in core, not allowed in contracts
		QpiContextProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward, unsigned char entryPoint) : QpiContextFunctionCall(contractIndex, originator, invocationReward, entryPoint) {}
	};

	// QPI available in REGISTER_USER_FUNCTIONS_AND_PROCEDURES
	struct QpiContextForInit : public QPI::QpiContext
	{
		inline void __registerUserFunction(USER_FUNCTION, unsigned short, unsigned short, unsigned short, unsigned int) const;
		inline void __registerUserProcedure(USER_PROCEDURE, unsigned short, unsigned short, unsigned short, unsigned int) const;
		inline void __registerUserProcedureNotification(USER_PROCEDURE, unsigned int, unsigned short, unsigned short, unsigned int) const;

		// Construction is done in core, not allowed in contracts
		inline QpiContextForInit(unsigned int contractIndex);
	};

}
