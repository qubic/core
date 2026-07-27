#pragma once

#include "qpi_types.h"
#include "qpi_containers.h"
#include "qpi_proposals.h"

namespace QPI
{
	// Management rights transfer: pre-transfer input
	struct PreManagementRightsTransfer_input
	{
		Asset asset;
		id owner;
		id possessor;
		sint64 numberOfShares;
		sint64 offeredFee;
		uint16 otherContractIndex;
	};

	// Management rights transfer: pre-transfer output (default is all-zeroed = don't allow transfer)
	struct PreManagementRightsTransfer_output
	{
		bool allowTransfer;
		sint64 requestedFee;
	};

	// Management rights transfer: post-transfer input
	struct PostManagementRightsTransfer_input
	{
		Asset asset;
		id owner;
		id possessor;
		sint64 numberOfShares;
		sint64 receivedFee;
		uint16 otherContractIndex;
	};

	// Input of POST_INCOMING_TRANSFER notification system call
	struct PostIncomingTransfer_input
	{
		id sourceId;
		sint64 amount;
		uint8 type;
	};

	// Input of SET_SHAREHOLDER_PROPOSAL system procedure (buffer for passing the contract-dependent proposal data)
	typedef Array<uint8, 1024> SET_SHAREHOLDER_PROPOSAL_input;

	// Output of SET_SHAREHOLDER_PROPOSAL system procedure (proposal index, or INVALID_PROPOSAL_INDEX on error)
	typedef uint16 SET_SHAREHOLDER_PROPOSAL_output;

	// Input of SET_SHAREHOLDER_VOTES system procedure (vote data)
	typedef ProposalMultiVoteDataV1 SET_SHAREHOLDER_VOTES_input;

	// Output of SET_SHAREHOLDER_VOTES system procedure (success flag)
	typedef bit SET_SHAREHOLDER_VOTES_output;

	//////////

#define STATIC_ASSERT(condition, identifier) static_assert(condition, #identifier);

// Internal macro for defining the system procedure macros
#define NO_IO_SYSTEM_PROC(CapLetterName, FuncName, InputType, OutputType) \
		public: \
			typedef NoData CapLetterName##_locals; \
			NO_IO_SYSTEM_PROC_WITH_LOCALS(CapLetterName, FuncName, InputType, OutputType)

	// Internal macro for defining the system procedure macros
#define NO_IO_SYSTEM_PROC_WITH_LOCALS(CapLetterName, FuncName, InputType, OutputType) \
		 public: \
			enum { FuncName##Empty = 0, FuncName##LocalsSize = sizeof(CapLetterName##_locals) }; \
			static_assert(sizeof(CapLetterName##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #CapLetterName "_locals size too large"); \
			inline static void FuncName(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, InputType& input, OutputType& output, CapLetterName##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##FuncName(qpi, state, input, output, locals); } \
			static void __impl_##FuncName(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, InputType& input, OutputType& output, CapLetterName##_locals& locals)

	// Define contract system procedure called to initialize contract state after IPO
#define INITIALIZE()  NO_IO_SYSTEM_PROC(INITIALIZE, __initialize, NoData, NoData)

// Define contract system procedure called to initialize contract state after IPO, provides zeroed instance of INITIALIZE_locals struct
#define INITIALIZE_WITH_LOCALS()  NO_IO_SYSTEM_PROC_WITH_LOCALS(INITIALIZE, __initialize, NoData, NoData)

// Define contract system procedure called at beginning of each epoch
#define BEGIN_EPOCH()  NO_IO_SYSTEM_PROC(BEGIN_EPOCH, __beginEpoch, NoData, NoData)

// Define contract system procedure called at beginning of each epoch, provides zeroed instance of BEGIN_EPOCH_locals struct
#define BEGIN_EPOCH_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(BEGIN_EPOCH, __beginEpoch, NoData, NoData)

// Define contract system procedure called at end of each epoch
#define END_EPOCH() NO_IO_SYSTEM_PROC(END_EPOCH, __endEpoch, NoData, NoData)

// Define contract system procedure called at end of each epoch, provides zeroed instance of END_EPOCH_locals struct
#define END_EPOCH_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(END_EPOCH, __endEpoch, NoData, NoData)

// Define contract system procedure called at beginning of each tick
#define BEGIN_TICK() NO_IO_SYSTEM_PROC(BEGIN_TICK, __beginTick, NoData, NoData)

// Define contract system procedure called at beginning of each tick, provides zeroed instance of BEGIN_TICK_locals struct
#define BEGIN_TICK_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(BEGIN_TICK, __beginTick, NoData, NoData)

// Define contract system procedure called at end of each tick
#define END_TICK() NO_IO_SYSTEM_PROC(END_TICK, __endTick, NoData, NoData)

// Define contract system procedure called at end of each tick, provides zeroed instance of BEGIN_TICK_locals struct
#define END_TICK_WITH_LOCALS() NO_IO_SYSTEM_PROC_WITH_LOCALS(END_TICK, __endTick, NoData, NoData)

// Define contract system procedure called before asset management rights transfer with `qpi.releaseShares(). See
// `doc/contracts.md` for details.
#define PRE_ACQUIRE_SHARES() \
        NO_IO_SYSTEM_PROC(PRE_ACQUIRE_SHARES, __preAcquireShares, PreManagementRightsTransfer_input, \
                          PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.releaseShares(). Provides
	// zeroed instance of PRE_ACQUIRE_SHARES_locals struct. See `doc/contracts.md` for details.
#define PRE_ACQUIRE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(PRE_ACQUIRE_SHARES, __preAcquireShares, PreManagementRightsTransfer_input, \
                                      PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.acquireShares(). See
	// `doc/contracts.md` for details.
#define PRE_RELEASE_SHARES() \
        NO_IO_SYSTEM_PROC(PRE_RELEASE_SHARES, __preReleaseShares, PreManagementRightsTransfer_input, \
                          PreManagementRightsTransfer_output)

	// Define contract system procedure called before asset management rights transfer with `qpi.acquireShares(). Provides
	// zeroed instance of PRE_RELEASE_SHARES_locals struct. See `doc/contracts.md` for details.
#define PRE_RELEASE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(PRE_RELEASE_SHARES, __preReleaseShares, PreManagementRightsTransfer_input, \
                                      PreManagementRightsTransfer_output)

	// Define contract system procedure called after asset management rights transfer with `qpi.releaseShares(). See
	// `doc/contracts.md` for details.
#define POST_ACQUIRE_SHARES() \
        NO_IO_SYSTEM_PROC(POST_ACQUIRE_SHARES, __postAcquireShares, PostManagementRightsTransfer_input, NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.releaseShares(). Provides
	// zeroed instance of POST_ACQUIRE_SHARES_locals struct. See `doc/contracts.md` for details.
#define POST_ACQUIRE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_ACQUIRE_SHARES, __postAcquireShares, PostManagementRightsTransfer_input, \
                                      NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.acquireShares(). See
	// `doc/contracts.md` for details.
#define POST_RELEASE_SHARES() \
        NO_IO_SYSTEM_PROC(POST_RELEASE_SHARES, __postReleaseShares, PostManagementRightsTransfer_input, NoData)

	// Define contract system procedure called after asset management rights transfer with `qpi.acquireShares(). Provides
	// zeroed instance of POST_RELEASE_SHARES_locals struct. See `doc/contracts.md` for details.
#define POST_RELEASE_SHARES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_RELEASE_SHARES, __postReleaseShares, PostManagementRightsTransfer_input, \
                                      NoData)

	// Define contract system procedure called when QUs are transferred to the contract. See `doc/contracts.md` for
	// details.
#define POST_INCOMING_TRANSFER() \
        NO_IO_SYSTEM_PROC(POST_INCOMING_TRANSFER, __postIncomingTransfer, PostIncomingTransfer_input, NoData)

	// Define contract system procedure called when QUs are transferred to the contract. Provides zeroed instance of
	// POST_INCOMING_TRANSFER_locals struct. See `doc/contracts.md` for details.
#define POST_INCOMING_TRANSFER_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(POST_INCOMING_TRANSFER, __postIncomingTransfer, PostIncomingTransfer_input, \
                                      NoData)

	// Define contract system procedure called when another contract tries to set/change/cancel a proposal through
	// qpi.setShareholderProposal(). See `doc/contracts.md` for details.
#define SET_SHAREHOLDER_PROPOSAL() \
        NO_IO_SYSTEM_PROC(SET_SHAREHOLDER_PROPOSAL, __setShareholderProposal, SET_SHAREHOLDER_PROPOSAL_input, \
						  SET_SHAREHOLDER_PROPOSAL_output)

	// Define contract system procedure called when another contract tries to set/change/cancel a proposal through
	// qpi.setShareholderProposal(). Provides zeroed instance of SET_SHAREHOLDER_PROPOSAL_locals struct. See
	// `doc/contracts.md` for details.
#define SET_SHAREHOLDER_PROPOSAL_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(SET_SHAREHOLDER_PROPOSAL, __setShareholderProposal, SET_SHAREHOLDER_PROPOSAL_input, \
						              SET_SHAREHOLDER_PROPOSAL_output)

	// Define contract system procedure called when another contract tries to set/change/cancel a vote through
	// qpi.setShareholderVotes(). See `doc/contracts.md` for details.
#define SET_SHAREHOLDER_VOTES() \
        NO_IO_SYSTEM_PROC(SET_SHAREHOLDER_VOTES, __setShareholderVotes, SET_SHAREHOLDER_VOTES_input, \
						  SET_SHAREHOLDER_VOTES_output)

	// Define contract system procedure called when another contract tries to set/change/cancel a vote through
	// qpi.setShareholderVotes(). Provides zeroed instance of SET_SHAREHOLDER_VOTES_locals struct. See
	// `doc/contracts.md` for details.
#define SET_SHAREHOLDER_VOTES_WITH_LOCALS() \
        NO_IO_SYSTEM_PROC_WITH_LOCALS(SET_SHAREHOLDER_VOTES, __setShareholderVotes, SET_SHAREHOLDER_VOTES_input, \
						              SET_SHAREHOLDER_VOTES_output)

#define EXPAND() \
      public: \
        enum { __expandEmpty = 0 }; \
		inline static void __expand(const QPI::QpiContextFunctionCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, QPI::ContractState<CONTRACT_STATE2_TYPE, CONTRACT_INDEX>& state2) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_expand(qpi, state, state2); } \
		static void __impl_expand(const QPI::QpiContextFunctionCall & qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>&state, QPI::ContractState<CONTRACT_STATE2_TYPE, CONTRACT_INDEX>& state2)

#define MIGRATE_WITH_LOCALS() \
      public: \
        enum { __migrateEmpty = 0, __migrateOldStateSize = sizeof(CONTRACT_STATE_TYPE::OldStateData), __migrateLocalsSize = sizeof(MIGRATE_locals) }; \
		static_assert(sizeof(MIGRATE_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, "MIGRATE_locals size too large"); \
		inline static void __migrate(const QPI::QpiContextFunctionCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, const CONTRACT_STATE_TYPE::OldStateData& oldState, MIGRATE_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_migrate(qpi, state, oldState, locals); } \
		static void __impl_migrate(const QPI::QpiContextFunctionCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, const CONTRACT_STATE_TYPE::OldStateData& oldState, MIGRATE_locals& locals)

#define MIGRATE() \
      public: \
		typedef NoData MIGRATE_locals; \
		MIGRATE_WITH_LOCALS()

#define LOG_DEBUG(message) __logContractDebugMessage(CONTRACT_INDEX, message);

#define LOG_ERROR(message) __logContractErrorMessage(CONTRACT_INDEX, message);

#define LOG_INFO(message) __logContractInfoMessage(CONTRACT_INDEX, message);

#define LOG_WARNING(message) __logContractWarningMessage(CONTRACT_INDEX, message);

#define LOG_PAUSE() __pauseLogMessage();

#define LOG_RESUME() __resumeLogMessage();

#define PRIVATE_FUNCTION(function) \
		protected: \
			typedef QPI::NoData function##_locals; \
			PRIVATE_FUNCTION_WITH_LOCALS(function)

#define PRIVATE_FUNCTION_WITH_LOCALS(function) \
		protected: \
			enum { __is_function_##function = true }; \
			inline static void function(const QPI::QpiContextFunctionCall& qpi, const QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##function(qpi, state, input, output, locals); } \
			static void __impl_##function(const QPI::QpiContextFunctionCall& qpi, const QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, function##_input& input, function##_output& output, function##_locals& locals)

#define PRIVATE_PROCEDURE(procedure) \
		protected: \
			typedef QPI::NoData procedure##_locals; \
			PRIVATE_PROCEDURE_WITH_LOCALS(procedure)

#define PRIVATE_PROCEDURE_WITH_LOCALS(procedure) \
		protected: \
			enum { __is_function_##procedure = false, __id_##procedure = (CONTRACT_INDEX << 22) | __LINE__ }; \
			inline static void procedure(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##procedure(qpi, state, input, output, locals); } \
			static void __impl_##procedure(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals)

#define PUBLIC_FUNCTION(function) \
		public: \
			typedef QPI::NoData function##_locals; \
			PUBLIC_FUNCTION_WITH_LOCALS(function)

#define PUBLIC_FUNCTION_WITH_LOCALS(function) \
		public: \
			enum { __is_function_##function = true }; \
			inline static void function(const QPI::QpiContextFunctionCall& qpi, const QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, function##_input& input, function##_output& output, function##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##function(qpi, state, input, output, locals); } \
			static void __impl_##function(const QPI::QpiContextFunctionCall& qpi, const QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, function##_input& input, function##_output& output, function##_locals& locals)

#define PUBLIC_PROCEDURE(procedure) \
		public: \
			typedef QPI::NoData procedure##_locals; \
			PUBLIC_PROCEDURE_WITH_LOCALS(procedure)

#define PUBLIC_PROCEDURE_WITH_LOCALS(procedure) \
		public: \
			enum { __is_function_##procedure = false, __id_##procedure = (CONTRACT_INDEX << 22) | __LINE__ }; \
			static_assert(sizeof(procedure##_input) <= MAX_INPUT_SIZE, #procedure "_input size too large"); \
			inline static void procedure(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl_##procedure(qpi, state, input, output, locals); } \
			static void __impl_##procedure(const QPI::QpiContextProcedureCall& qpi, QPI::ContractState<CONTRACT_STATE_TYPE::StateData, CONTRACT_INDEX>& state, procedure##_input& input, procedure##_output& output, procedure##_locals& locals)

#define REGISTER_USER_FUNCTIONS_AND_PROCEDURES() \
		public: \
			enum { __contract_index = CONTRACT_INDEX }; \
			inline static void __registerUserFunctionsAndProcedures(const QPI::QpiContextForInit& qpi) { ::__FunctionOrProcedureBeginEndGuard<(CONTRACT_INDEX << 22) | __LINE__> __prologueEpilogueCaller; __impl___registerUserFunctionsAndProcedures(qpi); } \
			static void __impl___registerUserFunctionsAndProcedures(const QPI::QpiContextForInit& qpi)

#define REGISTER_USER_FUNCTION(userFunction, inputType) \
		static_assert(__is_function_##userFunction, #userFunction " is procedure"); \
		static_assert(inputType >= 1 && inputType <= 65535, "inputType must be >= 1 and <= 65535"); \
		static_assert(sizeof(userFunction##_output) <= 65535, #userFunction "_output size too large"); \
		static_assert(sizeof(userFunction##_input) <= 65535, #userFunction "_input size too large"); \
		static_assert(sizeof(userFunction##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userFunction "_locals size too large"); \
		qpi.__registerUserFunction((USER_FUNCTION)userFunction, inputType, sizeof(userFunction##_input), sizeof(userFunction##_output), sizeof(userFunction##_locals));

#define REGISTER_USER_PROCEDURE(userProcedure, inputType) \
		static_assert(!__is_function_##userProcedure, #userProcedure " is function"); \
		static_assert(inputType >= 1 && inputType <= 65535, "inputType must be >= 1 and <= 65535"); \
		static_assert(sizeof(userProcedure##_output) <= 65535, #userProcedure "_output size too large"); \
		static_assert(sizeof(userProcedure##_input) <= 65535, #userProcedure "_input size too large"); \
		static_assert(sizeof(userProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userProcedure "_locals size too large"); \
		qpi.__registerUserProcedure((USER_PROCEDURE)userProcedure, inputType, sizeof(userProcedure##_input), sizeof(userProcedure##_output), sizeof(userProcedure##_locals));

	// Register procedure for notifications (such as oracle reply notification)
#define REGISTER_USER_PROCEDURE_NOTIFICATION(userProcedure) \
		static_assert(!__is_function_##userProcedure, #userProcedure " is function"); \
		static_assert(sizeof(userProcedure##_output) <= 65535, #userProcedure "_output size too large"); \
		static_assert(sizeof(userProcedure##_input) <= 65535, #userProcedure "_input size too large"); \
		static_assert(sizeof(userProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #userProcedure "_locals size too large"); \
		qpi.__registerUserProcedureNotification((USER_PROCEDURE)userProcedure, __id_##userProcedure, sizeof(userProcedure##_input), sizeof(userProcedure##_output), sizeof(userProcedure##_locals));

	// Call function or procedure of current contract (without changing invocation reward)
	// WARNING: input may be changed by called function
#define CALL(functionOrProcedure, input, output) \
		static_assert(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #functionOrProcedure "_locals size too large"); \
		functionOrProcedure(qpi, state, input, output, *(functionOrProcedure##_locals*)qpi.__qpiAllocLocals(sizeof(CONTRACT_STATE_TYPE::functionOrProcedure##_locals))); \
		qpi.__qpiFreeLocals()

	// Invoke procedure of current contract with changed invocation reward
	// WARNING: input may be changed by called function
	// TODO: INVOKE

	// Call function of other contract with custom error variable name
	// Use this variant when making multiple inter-contract calls in the same scope
	// WARNING: input may be changed by called function
#define CALL_OTHER_CONTRACT_FUNCTION_E(contractStateType, function, input, output, errorVar) \
		static_assert(sizeof(contractStateType::function##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #function "_locals size too large"); \
		static_assert(contractStateType::__is_function_##function, "CALL_OTHER_CONTRACT_FUNCTION_E() cannot be used to invoke procedures."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		InterContractCallError errorVar; \
		do { \
			const QpiContextFunctionCall* __ctx = qpi.__qpiConstructContextOtherContractFunctionCall(contractStateType::__contract_index, errorVar); \
			if (__ctx) { \
				const QPI::ContractState<contractStateType::StateData, contractStateType::__contract_index>* __state = (const QPI::ContractState<contractStateType::StateData, contractStateType::__contract_index>*)qpi.__qpiAcquireStateForReading(contractStateType::__contract_index); \
				contractStateType::function##_locals* __locals = (contractStateType::function##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::function##_locals)); \
				contractStateType::function(*__ctx, *__state, input, output, *__locals); \
				qpi.__qpiFreeLocals(); \
				qpi.__qpiReleaseStateForReading(contractStateType::__contract_index); \
				qpi.__qpiFreeContext(); \
			} \
		} while(0)

	// Call function of other contract
	// WARNING: input may be changed by called function
#define CALL_OTHER_CONTRACT_FUNCTION(contractStateType, function, input, output) \
		CALL_OTHER_CONTRACT_FUNCTION_E(contractStateType, function, input, output, interContractCallError)

	// Transfer invocation reward and invoke of other contract (procedure only) with custom error variable name
	// Use this variant when making multiple inter-contract calls in the same scope
	// WARNING: input may be changed by called function
#define INVOKE_OTHER_CONTRACT_PROCEDURE_E(contractStateType, procedure, input, output, invocationReward, errorVar) \
		static_assert(sizeof(contractStateType::procedure##_locals) <= MAX_SIZE_OF_CONTRACT_LOCALS, #procedure "_locals size too large"); \
		static_assert(!contractStateType::__is_function_##procedure, "INVOKE_OTHER_CONTRACT_PROCEDURE_E() cannot be used to call functions."); \
		static_assert(!(contractStateType::__contract_index == CONTRACT_STATE_TYPE::__contract_index), "Use CALL() to call a function/procedure of this contract."); \
		static_assert(contractStateType::__contract_index < CONTRACT_STATE_TYPE::__contract_index, "You can only call contracts with lower index."); \
		InterContractCallError errorVar; \
		do { \
			const QpiContextProcedureCall* __ctx = qpi.__qpiConstructProcedureCallContext(contractStateType::__contract_index, invocationReward, errorVar); \
			if (__ctx) { \
				QPI::ContractState<contractStateType::StateData, contractStateType::__contract_index>* __state = (QPI::ContractState<contractStateType::StateData, contractStateType::__contract_index>*)qpi.__qpiAcquireStateForWriting(contractStateType::__contract_index); \
				contractStateType::procedure##_locals* __locals = (contractStateType::procedure##_locals*)qpi.__qpiAllocLocals(sizeof(contractStateType::procedure##_locals)); \
				contractStateType::procedure(*__ctx, *__state, input, output, *__locals); \
				qpi.__qpiFreeLocals(); \
				qpi.__qpiReleaseStateForWriting(contractStateType::__contract_index); \
				qpi.__qpiFreeContext(); \
			} \
		} while(0)

	// Transfer invocation reward and invoke of other contract (procedure only)
	// WARNING: input may be changed by called function
#define INVOKE_OTHER_CONTRACT_PROCEDURE(contractStateType, procedure, input, output, invocationReward) \
		INVOKE_OTHER_CONTRACT_PROCEDURE_E(contractStateType, procedure, input, output, invocationReward, interContractCallError)

	/**
	* @brief Initiate oracle query that will lead to notification later.
	* @param OracleInterface Oracle interface struct of interface to query, e.g., OI::Price
	* @param query Details about which oracle to query for which information, as defined by a specific oracle interface.
	* @param userProcNotification User procedure that shall be executed when the oracle reply is available or an error occurs.
	* @param timeoutMillisec Maximum number of milliseconds to wait for reply.
	* @return Oracle query ID that can be used to get the status of the query, or -1 on error.
	*
	* This will automatically burn the oracle query fee as defined by the oracle interface (burning without
	* adding to the contract's execution fee reserve). It will fail if the contract doesn't have enough QU.
	*
	* The notification callback will be executed when the reply is available or on error.
	* The callback must be a user procedure of the contract calling QUERY_ORACLE() with the procedure input type
	* OracleNotificationInput<OracleInterface> and NoData as output. The procedure must be registered with
	* REGISTER_USER_PROCEDURE_NOTIFICATION() in REGISTER_USER_FUNCTIONS_AND_PROCEDURES().
	*
	* In the notification callback, success is indicated by input.status == ORACLE_QUERY_STATUS_SUCCESS.
	* If an error happened before the query has been created and sent, input.status is ORACLE_QUERY_STATUS_UNKNOWN
	* and input.queryId is -1 (invalid).
	* Other errors that may happen with valid input.queryId are input.status == ORACLE_QUERY_STATUS_TIMEOUT and
	* input.status == ORACLE_QUERY_STATUS_UNRESOLVABLE.
	*/
#define QUERY_ORACLE(OracleInterface, query, userProcNotification, timeoutMillisec) qpi.__qpiQueryOracle<OracleInterface>(query, userProcNotification, __id_##userProcNotification, timeoutMillisec)

	/**
	* @brief Subscribe for regularly querying an oracle.
	* @param query The regular query, which must have a member `DateAndTime timestamp`.
	* @param notificationCallback User procedure that shall be executed when the oracle reply is available or an error occurs.
	* @param notificationPeriodInMilliseconds Number of milliseconds between consecutive queries/replies that the contract
	*           is notified about. Currently, only multiples of 60000 are supported and other values are rejected with an error.
	* @param notifyWithPreviousReply Whether to immediately notify this contract with the most up-to-date value if any is available.
	* @return Oracle subscription ID or -1 on error.
	*
	* Subscriptions automatically expire at the end of each epoch. So, a common pattern is to call SUBSCRIBE_ORACLE
	* in BEGIN_EPOCH.
	*
	* Subscriptions facilitate sharing common oracle queries among multiple contracts. This saves network resources and allows
	* to provide a fixed-price subscription for the whole epoch, which is usually much cheaper than the equivalent series of
	* individual QUERY_ORACLE() calls.
	*
	* The SUBSCRIBE_ORACLE call will automatically burn the oracle subscription fee as defined by the oracle interface
	* (burning without adding to the contract's execution fee reserve). It will fail if the contract doesn't have enough QU.
	*
	* The notification callback will be executed when the reply is available or on error.
	* The callback must be a user procedure of the contract calling SUBSCRIBE_ORACLE with the procedure input type
	* OracleNotificationInput<OracleInterface> and NoData as output. The procedure must be registered with
	* REGISTER_USER_PROCEDURE_NOTIFICATION() in REGISTER_USER_FUNCTIONS_AND_PROCEDURES().
	*
	* In the notification callback, success is indicated by input.status == ORACLE_QUERY_STATUS_SUCCESS.
	* If an error happened before the query has been created and sent, input.status is ORACLE_QUERY_STATUS_UNKNOWN
	* and input.queryId is -1 (invalid).
	* Other errors that may happen with valid input.queryId are input.status == ORACLE_QUERY_STATUS_TIMEOUT and
	* input.status == ORACLE_QUERY_STATUS_UNRESOLVABLE.
	* The timeout of subscription queries is always 60000 milliseconds.
	*
	* A contract may subscribe to the same oracle interface with multiple different queries.
	* However, it cannot subscribe with the same query multiple times.
	* In order to change the notification period of an existing query, it needs to be unsubscribed first and subscribed again afterwards.
	*/
#define SUBSCRIBE_ORACLE(OracleInterface, query, userProcNotification, notificationPeriodInMilliseconds, notifyWithPreviousReply) qpi.__qpiSubscribeOracle<OracleInterface>(query, userProcNotification, __id_##userProcNotification, notificationPeriodInMilliseconds, notifyWithPreviousReply)

	/**
	* @brief Issue an OC (Outsourced Computation) invocation to an external system.
	* @param OcInterface The OC interface type to invoke (e.g. OCI::Mock).
	* @param request An OcRequest value matching OcInterface::OcRequest. The caller MUST
	*           zero-initialize the request struct (e.g. via setMemory) before assigning fields,
	*           because hidden padding bytes are hashed into paramsDigest for consensus.
	* @return Invocation ID (non-negative sint64) on success, -1 on any failure.
	*
	* The call is non-blocking and has no notification callback or return path. The contract
	* observes invocation state by polling qpi.getOcInvocationStatus(invocationId).
	*
	* The invocation fee (OcInterface::getInvocationFee(request)) is deducted from the contract's
	* spectrum balance and destroyed (not added to execution reserve). If the engine cannot record
	* the invocation, the fee is refunded.
	*/
#define INVOKE_OC(OcInterface, request) qpi.__qpiInvokeOC<OcInterface>(request)

#define SELF id(CONTRACT_INDEX, 0, 0, 0)

#define SELF_INDEX CONTRACT_INDEX

	//////////

#define DEFINE_SHAREHOLDER_PROPOSAL_TYPES(numProposalSlots, assetNameInt64) \
		public: \
			typedef ProposalDataYesNo ProposalDataT; \
			typedef ProposalAndVotingByShareholders<numProposalSlots, assetNameInt64> ProposersAndVotersT; \
			typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT

#define IMPLEMENT_SetShareholderProposal(numFeeStateVariables, setProposalFeeVarOrValue) \
		typedef ProposalDataT SetShareholderProposal_input; \
		typedef uint16 SetShareholderProposal_output; \
		PUBLIC_PROCEDURE(SetShareholderProposal) { \
			if (qpi.invocationReward() < setProposalFeeVarOrValue || (input.epoch \
				&& (input.type != ProposalTypes::VariableYesNo || input.data.variableOptions.variable >= numFeeStateVariables \
					|| input.data.variableOptions.value < 0))) { \
				qpi.transfer(qpi.invocator(), qpi.invocationReward()); \
				output = INVALID_PROPOSAL_INDEX; \
				return; } \
			output = qpi(state.mut().proposals).setProposal(qpi.invocator(), input); \
			if (output == INVALID_PROPOSAL_INDEX) { \
				qpi.transfer(qpi.invocator(), qpi.invocationReward()); \
				return;	} \
			qpi.burn(setProposalFeeVarOrValue); \
			if (qpi.invocationReward() > setProposalFeeVarOrValue) { \
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - setProposalFeeVarOrValue); } }

#define IMPLEMENT_GetShareholderProposal() \
		struct GetShareholderProposal_input { uint16 proposalIndex; }; \
		struct GetShareholderProposal_output { ProposalDataT proposal; id proposerPubicKey; }; \
		PUBLIC_FUNCTION(GetShareholderProposal) { \
			output.proposerPubicKey = qpi(state.get().proposals).proposerId(input.proposalIndex); \
			qpi(state.get().proposals).getProposal(input.proposalIndex, output.proposal); }

#define IMPLEMENT_GetShareholderProposalIndices() \
		struct GetShareholderProposalIndices_input { bit activeProposals; sint32 prevProposalIndex; }; \
		struct GetShareholderProposalIndices_output { uint16 numOfIndices; Array<uint16, 64> indices; }; \
		PUBLIC_FUNCTION(GetShareholderProposalIndices) {\
			if (input.activeProposals) { \
				while ((input.prevProposalIndex = qpi(state.get().proposals).nextProposalIndex(input.prevProposalIndex, qpi.epoch())) >= 0) { \
					output.indices.set(output.numOfIndices, input.prevProposalIndex); \
					++output.numOfIndices; \
					if (output.numOfIndices == output.indices.capacity()) break; } } \
			else { \
				while ((input.prevProposalIndex = qpi(state.get().proposals).nextFinishedProposalIndex(input.prevProposalIndex)) >= 0) { \
					output.indices.set(output.numOfIndices, input.prevProposalIndex); \
					++output.numOfIndices; \
					if (output.numOfIndices == output.indices.capacity()) break; } } }

#define IMPLEMENT_GetShareholderProposalFees(setProposalFeeVarOrValue) \
		typedef NoData GetShareholderProposalFees_input; \
		struct GetShareholderProposalFees_output { sint64 setProposalFee; sint64 setVoteFee; }; \
		PUBLIC_FUNCTION(GetShareholderProposalFees) { \
			output.setProposalFee = setProposalFeeVarOrValue; \
			output.setVoteFee = 0; }

#define IMPLEMENT_SetShareholderVotes() \
		typedef ProposalMultiVoteDataV1 SetShareholderVotes_input; \
		typedef bit SetShareholderVotes_output; \
		PUBLIC_PROCEDURE(SetShareholderVotes) { \
			output = qpi(state.mut().proposals).vote(qpi.invocator(), input); } \

#define IMPLEMENT_GetShareholderVotes() \
		struct GetShareholderVotes_input { id voter; uint16 proposalIndex; }; \
		typedef ProposalMultiVoteDataV1 GetShareholderVotes_output; \
		PUBLIC_FUNCTION(GetShareholderVotes) { \
			qpi(state.get().proposals).getVotes(input.proposalIndex, input.voter,	output); }

#define IMPLEMENT_GetShareholderVotingResults() \
		struct GetShareholderVotingResults_input { uint16 proposalIndex; }; \
		typedef ProposalSummarizedVotingDataV1 GetShareholderVotingResults_output; \
		PUBLIC_FUNCTION(GetShareholderVotingResults) { \
			qpi(state.get().proposals).getVotingSummary(input.proposalIndex, output); }

#define IMPLEMENT_SET_SHAREHOLDER_PROPOSAL() \
		struct SET_SHAREHOLDER_PROPOSAL_locals { SetShareholderProposal_input userProcInput; }; \
		SET_SHAREHOLDER_PROPOSAL_WITH_LOCALS() { \
			copyFromBuffer(locals.userProcInput, input); \
			CALL(SetShareholderProposal, locals.userProcInput, output); }

#define IMPLEMENT_SET_SHAREHOLDER_VOTES() \
		SET_SHAREHOLDER_VOTES() { \
			CALL(SetShareholderVotes, input, output); }

	// Define procedures for easily implementing END_EPOCH
#define IMPLEMENT_FinalizeShareholderStateVarProposals() \
		struct FinalizeShareholderProposalSetStateVar_input { \
			sint32 proposalIndex; ProposalDataT proposal; ProposalSummarizedVotingDataV1 results; \
			sint32 acceptedOption; 	sint64 acceptedValue; }; \
		typedef NoData FinalizeShareholderProposalSetStateVar_output; \
		typedef NoData FinalizeShareholderStateVarProposals_input; \
		typedef NoData FinalizeShareholderStateVarProposals_output; \
		struct FinalizeShareholderStateVarProposals_locals { \
			FinalizeShareholderProposalSetStateVar_input p; uint16 proposalClass; }; \
		PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeShareholderStateVarProposals) { \
			locals.p.proposalIndex = -1; \
			while ((locals.p.proposalIndex = qpi(state.get().proposals).nextProposalIndex(locals.p.proposalIndex, qpi.epoch())) >= 0) { \
				if (!qpi(state.get().proposals).getProposal(locals.p.proposalIndex, locals.p.proposal)) \
					continue; \
				locals.proposalClass = ProposalTypes::cls(locals.p.proposal.type); \
				if (locals.proposalClass == ProposalTypes::Class::Variable || locals.proposalClass == ProposalTypes::Class::MultiVariables) { \
					if (!qpi(state.get().proposals).getVotingSummary(locals.p.proposalIndex, locals.p.results)) \
						continue; \
					locals.p.acceptedOption = locals.p.results.getAcceptedOption(); \
					if (locals.p.acceptedOption <= 0) \
						continue; \
					locals.p.acceptedValue = locals.p.proposal.data.variableOptions.value; \
					CALL(FinalizeShareholderProposalSetStateVar, locals.p, output); } } } \
		PRIVATE_PROCEDURE(FinalizeShareholderProposalSetStateVar)

#define IMPLEMENT_DEFAULT_SHAREHOLDER_PROPOSAL_VOTING(numFeeStateVariables, setProposalFeeVarOrValue) \
		IMPLEMENT_SetShareholderProposal(numFeeStateVariables, setProposalFeeVarOrValue) \
		IMPLEMENT_GetShareholderProposal() \
		IMPLEMENT_GetShareholderProposalIndices() \
		IMPLEMENT_GetShareholderProposalFees(setProposalFeeVarOrValue) \
		IMPLEMENT_SetShareholderVotes() \
		IMPLEMENT_GetShareholderVotes() \
		IMPLEMENT_GetShareholderVotingResults() \
		IMPLEMENT_SET_SHAREHOLDER_PROPOSAL() \
		IMPLEMENT_SET_SHAREHOLDER_VOTES()

#define REGISTER_GetShareholderProposalFees() REGISTER_USER_FUNCTION(GetShareholderProposalFees, 65531)
#define REGISTER_GetShareholderProposalIndices() REGISTER_USER_FUNCTION(GetShareholderProposalIndices, 65532)
#define REGISTER_GetShareholderProposal() REGISTER_USER_FUNCTION(GetShareholderProposal, 65533)
#define REGISTER_GetShareholderVotes() REGISTER_USER_FUNCTION(GetShareholderVotes, 65534)
#define REGISTER_GetShareholderVotingResults() REGISTER_USER_FUNCTION(GetShareholderVotingResults, 65535)
#define REGISTER_SetShareholderProposal() REGISTER_USER_PROCEDURE(SetShareholderProposal, 65534)
#define REGISTER_SetShareholderVotes() REGISTER_USER_PROCEDURE(SetShareholderVotes, 65535)

#define REGISTER_SHAREHOLDER_PROPOSAL_VOTING()  REGISTER_GetShareholderProposalFees() \
		REGISTER_GetShareholderProposalIndices(); REGISTER_GetShareholderProposal(); \
		REGISTER_GetShareholderVotes(); REGISTER_GetShareholderVotingResults(); \
		REGISTER_SetShareholderProposal(); REGISTER_SetShareholderVotes()

}
