using namespace QPI;

// ---------------------------------------------------------------------
// Constants / configuration
// ---------------------------------------------------------------------

static constexpr uint32 QSB_MAX_ORACLES = 64;
static constexpr uint32 QSB_MAX_PAUSERS = 32;
static constexpr uint32 QSB_MAX_FILLED_ORDERS = 2048;
static constexpr uint32 QSB_MAX_LOCKED_ORDERS = 1024;

struct QSB2
{
};

struct QSB : public ContractBase
{
public:
	// Role identifiers for addRole / removeRole
	enum class Role : uint8
	{
		Oracle = 1,
		Pauser = 2
	};

	// ---------------------------------------------------------------------
	// Core data structures
	// ---------------------------------------------------------------------

	// Order object used for unlock()
	struct Order
	{
		id fromAddress;              // sender on source chain (mirrored id)
		id toAddress;                // Qubic recipient when minting/unlocking
		uint64 tokenIn;              // identifier for incoming asset
		uint64 tokenOut;             // identifier for outgoing asset
		uint64 amount;               // total bridged amount (Qubic units)
		uint64 relayerFee;           // fee paid to relayer (subset of amount)
		uint32 destinationChainId;   // e.g. Solana chain-id as used off-chain
		uint32 networkIn;            // incoming network id
		uint32 networkOut;           // outgoing network id
		uint32 nonce;                // unique order nonce
	};

	// Compact order-hash representation (K12 digest)
	typedef Array<uint8, 32> OrderHash;

	// Signature wrapper compatible with QPI::signatureValidity
	struct SignatureData
	{
		id signer;     // oracle id (public key)
		Array<sint8, 64> signature;  // raw 64-byte signature
	};

	// Storage entry for filledOrders mapping
	struct FilledOrderEntry
	{
		OrderHash hash;
		bit used;
	};

	// Storage entry for role mappings (oracles / pausers)
	struct RoleEntry
	{
		id account;
		bit active;
	};

	// Storage entry for lock() orders (for overrideLock / off-chain reference)
	struct LockedOrderEntry
	{
		id sender;
		uint64 amount;
		uint64 relayerFee;
		uint32 networkOut;
		uint32 nonce;
		Array<uint8, 64> toAddress;
		OrderHash orderHash;
		bit active;
	};

	// ---------------------------------------------------------------------
	// User-facing I/O structures
	// ---------------------------------------------------------------------

	// 1) lock()
	struct Lock_input
	{
		// Recipient on Solana (fixed-size buffer, zero-padded)
		uint64 amount;
		uint64 relayerFee;
		Array<uint8, 64> toAddress;
		uint32 networkOut;
		uint32 nonce;
	};

	struct Lock_output
	{
		OrderHash orderHash;
		bit success;
	};

	// 2) overrideLock()
	struct OverrideLock_input
	{
		Array<uint8, 64> toAddress;
		uint64 relayerFee;
		uint32 nonce;
	};

	struct OverrideLock_output
	{
		OrderHash orderHash;
		bit success;
	};

	// 3) unlock()
	struct Unlock_input
	{
		Order order;
		uint32 numSignatures;
		Array<SignatureData, QSB_MAX_ORACLES> signatures;
	};

	struct Unlock_output
	{
		OrderHash orderHash;
		bit success;
	};

	// 4) transferAdmin()
	struct TransferAdmin_input
	{
		id newAdmin;
	};

	struct TransferAdmin_output
	{
		bit success;
	};

	// 5) editOracleThreshold()
	struct EditOracleThreshold_input
	{
		uint8 newThreshold;
	};

	struct EditOracleThreshold_output
	{
		uint8 oldThreshold;
		bit success;
	};

	// 6) addRole()
	struct AddRole_input
	{
		id account;
		uint8 role;    // see Role enum
	};

	struct AddRole_output
	{
		bit success;
	};

	// 7) removeRole()
	struct RemoveRole_input
	{
		id account;
		uint8 role;
	};

	struct RemoveRole_output
	{
		bit success;
	};

	// 8) pause() / unpause()
	struct Pause_input
	{
	};

	struct Pause_output
	{
		bit success;
	};

	typedef Pause_input  Unpause_input;
	typedef Pause_output Unpause_output;

	// 9) editFeeParameters()
	struct EditFeeParameters_input
	{
		id protocolFeeRecipient; // updated when not zero-id
		id oracleFeeRecipient;   // updated when not zero-id
		uint32 bpsFee;           // basis points fee (0..10000)
		uint32 protocolFee;      // share of BPS fee for protocol (0..100)
	};

	struct EditFeeParameters_output
	{
		bit success;
	};

protected:
	// ---------------------------------------------------------------------
	// State variables
	// ---------------------------------------------------------------------

	id admin;
	id protocolFeeRecipient; // receives protocolFeeAmount
	id oracleFeeRecipient;   // receives oracleFeeAmount
	Array<RoleEntry, QSB_MAX_ORACLES> oracles;
	Array<RoleEntry, QSB_MAX_PAUSERS> pausers;
	Array<FilledOrderEntry, QSB_MAX_FILLED_ORDERS> filledOrders;
	Array<LockedOrderEntry, QSB_MAX_LOCKED_ORDERS> lockedOrders;
	uint32 oracleCount;
	uint32 bpsFee;               // fee taken in BPS (base 10000) from netAmount
	uint32 protocolFee;          // percent of BPS fee sent to protocol (base 100)
	uint8 oracleThreshold; // percent [1..100]
	bit paused;

	// ---------------------------------------------------------------------
	// Internal helpers
	// ---------------------------------------------------------------------

	// Truncate digest to OrderHash (full 32 bytes)
	inline static void digestToOrderHash(const id& digest, OrderHash& outHash)
	{
		// Copy digest directly to OrderHash (both are 32 bytes)
		// Use setMem which handles 32-byte types specially
		outHash.setMem(digest);
	}

	// Check if caller is current admin (or if admin is not yet set, allow bootstrap)
	inline static bool isAdmin(const QSB& state, const id& who)
	{
		if (isZero(state.admin))
			return true;
		return who == state.admin;
	}

	// Check if caller is admin or has pauser role
	inline static bool isAdminOrPauser(const QSB& state, const id& who)
	{
		if (isAdmin(state, who))
			return true;

		for (uint32 i = 0; i < state.pausers.capacity(); ++i)
		{
			const RoleEntry entry = state.pausers.get(i);
			if (entry.active && entry.account == who)
				return true;
		}
		return false;
	}

	// Find oracle index; returns NULL_INDEX if not found
	inline static sint32 findOracleIndex(const QSB& state, const id& account)
	{
		for (uint32 i = 0; i < state.oracles.capacity(); ++i)
		{
			const RoleEntry entry = state.oracles.get(i);
			if (entry.active && entry.account == account)
				return (sint32)i;
		}
		return NULL_INDEX;
	}

	// Find pauser index; returns NULL_INDEX if not found
	inline static sint32 findPauserIndex(const QSB& state, const id& account)
	{
		for (uint32 i = 0; i < state.pausers.capacity(); ++i)
		{
			const RoleEntry entry = state.pausers.get(i);
			if (entry.active && entry.account == account)
				return (sint32)i;
		}
		return NULL_INDEX;
	}

	// Mark an orderHash as filled (idempotent, linear storage)
	inline static void markOrderFilled(QSB& state, const OrderHash& hash)
	{
		// First, see if it already exists
		for (uint32 i = 0; i < state.filledOrders.capacity(); ++i)
		{
			FilledOrderEntry entry = state.filledOrders.get(i);
			if (entry.used)
			{
				bool same = true;
				for (uint32 j = 0; j < hash.capacity(); ++j)
				{
					if (entry.hash.get(j) != hash.get(j))
					{
						same = false;
						break;
					}
				}
				if (same)
					return;
			}
		}

		// Otherwise, insert into first free slot (or overwrite slot 0 as fallback)
		for (uint32 i = 0; i < state.filledOrders.capacity(); ++i)
		{
			FilledOrderEntry entry = state.filledOrders.get(i);
			if (!entry.used)
			{
				entry.hash = hash;
				entry.used = true;
				state.filledOrders.set(i, entry);
				return;
			}
		}

		// If storage is full, overwrite slot 0 (still provides replay protection for recent orders)
		FilledOrderEntry entry;
		entry.hash = hash;
		entry.used = true;
		state.filledOrders.set(0, entry);
	}

	// Check whether an orderHash has already been filled
	inline static bit isOrderFilled(const QSB& state, const OrderHash& hash)
	{
		for (uint32 i = 0; i < state.filledOrders.capacity(); ++i)
		{
			const FilledOrderEntry entry = state.filledOrders.get(i);
			if (!entry.used)
				continue;

			bool same = true;
			for (uint32 j = 0; j < hash.capacity(); ++j)
			{
				if (entry.hash.get(j) != hash.get(j))
				{
					same = false;
					break;
				}
			}
			if (same)
				return true;
		}
		return false;
	}

	// Find index of locked order by nonce; returns NULL_INDEX if not found
	inline static sint32 findLockedOrderIndexByNonce(const QSB& state, uint32 nonce)
	{
		for (uint32 i = 0; i < state.lockedOrders.capacity(); ++i)
		{
			const LockedOrderEntry entry = state.lockedOrders.get(i);
			if (entry.active && entry.nonce == nonce)
				return (sint32)i;
		}
		return NULL_INDEX;
	}

public:
	// ---------------------------------------------------------------------
	// Core user procedures
	// ---------------------------------------------------------------------

	struct Lock_locals
	{
		id digest;
		LockedOrderEntry existing;
		Order tmpOrder;
		LockedOrderEntry entry;
		uint32 freeIdx, i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(Lock)
	{
		output.success = false;
		setMemory(output.orderHash, 0);

		// Must not be paused
		if (state.paused)
		{
			// Refund attached funds if any
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// Basic validation
		if (input.amount == 0 || input.relayerFee >= input.amount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// Ensure funds sent with call match the amount to be locked
		if (qpi.invocationReward() < (sint64)input.amount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// Any excess over `amount` is refunded
		if (qpi.invocationReward() > (sint64)input.amount)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount);
		}

		// Funds equal to `amount` now remain locked in the contract balance

		// Ensure nonce unused
		if (findLockedOrderIndexByNonce(state, input.nonce) != NULL_INDEX)
		{
			// Nonce already used; reject
			qpi.transfer(qpi.invocator(), input.amount);
			return;
		}

		// Find storage slot for new locked order
		locals.freeIdx = NULL_INDEX;
		for (locals.i = 0; locals.i < state.lockedOrders.capacity(); ++locals.i)
		{
			locals.existing = state.lockedOrders.get(locals.i);
			if (!locals.existing.active)
			{
				locals.freeIdx = (sint32)locals.i;
				break;
			}
		}

		if (locals.freeIdx == NULL_INDEX)
		{
			// No space left to track orders, refund and abort
			qpi.transfer(qpi.invocator(), input.amount);
			return;
		}

		// Construct a lightweight order object for hashing and event usage
		locals.tmpOrder.destinationChainId = input.networkOut;
		locals.tmpOrder.networkIn = 0; // Qubic network id (can be refined off-chain)
		locals.tmpOrder.networkOut = input.networkOut;
		locals.tmpOrder.tokenIn = 0;
		locals.tmpOrder.tokenOut = 0;
		locals.tmpOrder.fromAddress = qpi.invocator();
		locals.tmpOrder.toAddress = NULL_ID; // Off-chain destination encoded only in toAddress bytes
		locals.tmpOrder.amount = input.amount;
		locals.tmpOrder.relayerFee = input.relayerFee;
		locals.tmpOrder.nonce = input.nonce;

		locals.digest = qpi.K12(locals.tmpOrder);
		digestToOrderHash(locals.digest, output.orderHash);

		// Persist locked order so that overrideLock can modify it
		locals.entry.active = true;
		locals.entry.sender = qpi.invocator();
		locals.entry.networkOut = input.networkOut;
		locals.entry.amount = input.amount;
		locals.entry.relayerFee = input.relayerFee;
		locals.entry.nonce = input.nonce;
		copyMemory(locals.entry.toAddress, input.toAddress);
		locals.entry.orderHash = output.orderHash;
		state.lockedOrders.set((uint32)locals.freeIdx, locals.entry);

		output.success = true;
	}

	struct OverrideLock_locals
	{
		LockedOrderEntry entry;
		Order tmpOrder;
		id digest;
		sint32 idx;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(OverrideLock)
	{
		output.success = false;
		setMemory(output.orderHash, 0);

		// Always refund invocationReward (locking was done in original lock() call)
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// Contract must not be paused
		if (state.paused)
		{
			return;
		}

		// Find existing order by nonce
		locals.idx = findLockedOrderIndexByNonce(state, input.nonce);
		if (locals.idx == NULL_INDEX)
		{
			return;
		}

		locals.entry = state.lockedOrders.get((uint32)locals.idx);

		// Only original sender can override
		if (locals.entry.sender != qpi.invocator())
		{
			return;
		}

		// Validate new relayer fee
		if (input.relayerFee >= locals.entry.amount)
		{
			return;
		}

		// Update mutable fields
		copyMemory(locals.entry.toAddress, input.toAddress);
		locals.entry.relayerFee = input.relayerFee;

		// Rebuild order for hashing
		locals.tmpOrder.destinationChainId = locals.entry.networkOut;
		locals.tmpOrder.networkIn = 0;
		locals.tmpOrder.networkOut = locals.entry.networkOut;
		locals.tmpOrder.tokenIn = 0;
		locals.tmpOrder.tokenOut = 0;
		locals.tmpOrder.fromAddress = locals.entry.sender;
		locals.tmpOrder.toAddress = NULL_ID;
		locals.tmpOrder.amount = locals.entry.amount;
		locals.tmpOrder.relayerFee = locals.entry.relayerFee;
		locals.tmpOrder.nonce = locals.entry.nonce;

		locals.digest = qpi.K12(locals.tmpOrder);
		digestToOrderHash(locals.digest, locals.entry.orderHash);
		output.orderHash = locals.entry.orderHash;

		state.lockedOrders.set((uint32)locals.idx, locals.entry);
		output.success = true;
	}

	struct Unlock_locals
	{
		id digest;
		OrderHash hash;
		uint32 validSignatureCount;
		uint32 requiredSignatures;
		Array<id, QSB_MAX_ORACLES> seenSigners;
		SignatureData sig;
		uint32 seenCount;
		uint32 i;
		uint32 j;
		uint64 netAmount;
		uint128 tmpMul;
		uint128 tmpMul2;
		uint64 bpsFeeAmount;
		uint64 protocolFeeAmount;
		uint64 oracleFeeAmount;
		uint64 recipientAmount;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(Unlock)
	{
		output.success = false;
		setMemory(output.orderHash, 0);

		// Must not be paused
		if (state.paused)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		// Refund any invocation reward (relayer is paid from order.amount, not from reward)
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		// Basic order validation
		if (input.order.amount == 0 || input.order.relayerFee >= input.order.amount)
		{
			return;
		}

		// Compute digest and orderHash
		locals.digest = qpi.K12(input.order);
		digestToOrderHash(locals.digest, locals.hash);
		output.orderHash = locals.hash;

		// Ensure orderHash not yet filled
		if (isOrderFilled(state, locals.hash))
		{
			return;
		}

		// Verify oracle signatures against threshold
		if (state.oracleCount == 0 || input.numSignatures == 0)
		{
			return;
		}

		// requiredSignatures = ceil(oracleCount * oracleThreshold / 100)
		locals.tmpMul  = uint128(state.oracleCount) * uint128(state.oracleThreshold);
		locals.tmpMul2 = div(locals.tmpMul, uint128(100));
		locals.requiredSignatures = (uint32)locals.tmpMul2.low;
		if (locals.requiredSignatures * 100 < state.oracleCount * state.oracleThreshold)
		{
			++locals.requiredSignatures;
		}
		if (locals.requiredSignatures == 0)
		{
			locals.requiredSignatures = 1;
		}

		locals.validSignatureCount = 0;
		locals.seenCount = 0;

		for (locals.i = 0; locals.i < input.numSignatures && locals.i < input.signatures.capacity(); ++locals.i)
		{
			locals.sig = input.signatures.get(locals.i);

			// Check signer is authorized oracle
			if (findOracleIndex(state, locals.sig.signer) == NULL_INDEX)
				return; // unknown signer -> fail fast

			// Check duplicates
			for (locals.j = 0; locals.j < locals.seenCount; ++locals.j)
			{
				if (locals.seenSigners.get(locals.j) == locals.sig.signer)
					return; // duplicate signer -> fail
			}

			// Verify signature
			if (!qpi.signatureValidity(locals.sig.signer, locals.digest, locals.sig.signature))
			{
				return;
			}

			// Record signer and increment count
			if (locals.seenCount < locals.seenSigners.capacity())
			{
				locals.seenSigners.set(locals.seenCount, locals.sig.signer);
				++locals.seenCount;
			}
			++locals.validSignatureCount;
		}

		if (locals.validSignatureCount < locals.requiredSignatures)
		{
			return;
		}

		// -----------------------------------------------------------------
		// Fee calculations
		// -----------------------------------------------------------------
		locals.netAmount = input.order.amount - input.order.relayerFee;

		// bpsFeeAmount = netAmount * bpsFee / 10000
		locals.tmpMul  = uint128(locals.netAmount) * uint128(state.bpsFee);
		locals.tmpMul2 = div(locals.tmpMul, uint128(10000));
		locals.bpsFeeAmount = (uint64)locals.tmpMul2.low;

		// protocolFeeAmount = bpsFeeAmount * protocolFee / 100
		locals.tmpMul  = uint128(locals.bpsFeeAmount) * uint128(state.protocolFee);
		locals.tmpMul2 = div(locals.tmpMul, uint128(100));
		locals.protocolFeeAmount = (uint64)locals.tmpMul2.low;

		// oracleFeeAmount = bpsFeeAmount - protocolFeeAmount
		if (locals.bpsFeeAmount >= locals.protocolFeeAmount)
			locals.oracleFeeAmount = locals.bpsFeeAmount - locals.protocolFeeAmount;
		else
			locals.oracleFeeAmount = 0;

		// recipientAmount = netAmount - bpsFeeAmount
		if (locals.netAmount >= locals.bpsFeeAmount)
			locals.recipientAmount = locals.netAmount - locals.bpsFeeAmount;
		else
			locals.recipientAmount = 0;

		// -----------------------------------------------------------------
		// Token transfers
		// -----------------------------------------------------------------

		// Relayer fee to caller
		if (input.order.relayerFee > 0)
		{
			qpi.transfer(qpi.invocator(), (sint64)input.order.relayerFee);
		}

		// Protocol fee
		if (locals.protocolFeeAmount > 0 && !isZero(state.protocolFeeRecipient))
		{
			qpi.transfer(state.protocolFeeRecipient, (sint64)locals.protocolFeeAmount);
		}

		// Oracle fee
		if (locals.oracleFeeAmount > 0 && !isZero(state.oracleFeeRecipient))
		{
			qpi.transfer(state.oracleFeeRecipient, (sint64)locals.oracleFeeAmount);
		}

		// Recipient payout
		if (locals.recipientAmount > 0 && !isZero(input.order.toAddress))
		{
			qpi.transfer(input.order.toAddress, (sint64)locals.recipientAmount);
		}

		// Mark order as filled
		markOrderFilled(state, locals.hash);
		output.success = true;
	}

	// ---------------------------------------------------------------------
	// Admin procedures
	// ---------------------------------------------------------------------

	PUBLIC_PROCEDURE(TransferAdmin)
	{
		output.success = false;

		// Refund any attached funds
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdmin(state, qpi.invocator()))
		{
			return;
		}

		state.admin = input.newAdmin;
		output.success = true;
	}

	PUBLIC_PROCEDURE(EditOracleThreshold)
	{
		output.success = false;
		output.oldThreshold = state.oracleThreshold;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdmin(state, qpi.invocator()))
		{
			return;
		}

		if (input.newThreshold == 0 || input.newThreshold > 100)
		{
			return;
		}

		state.oracleThreshold  = input.newThreshold;
		output.success   = true;
	}

	struct AddRole_locals
	{
		RoleEntry entry;
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(AddRole)
	{
		output.success = false;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdmin(state, qpi.invocator()))
		{
			return;
		}

		if (input.role == (uint8)Role::Oracle)
		{
			if (findOracleIndex(state, input.account) != NULL_INDEX)
			{
				output.success = true;
				return;
			}

			for (locals.i = 0; locals.i < state.oracles.capacity(); ++locals.i)
			{
				locals.entry = state.oracles.get(locals.i);
				if (!locals.entry.active)
				{
					locals.entry.account = input.account;
					locals.entry.active  = true;
					state.oracles.set(locals.i, locals.entry);
					++state.oracleCount;
					output.success = true;
					return;
				}
			}
		}
		else if (input.role == (uint8)Role::Pauser)
		{
			if (findPauserIndex(state, input.account) != NULL_INDEX)
			{
				output.success = true;
				return;
			}

			for (locals.i = 0; locals.i < state.pausers.capacity(); ++locals.i)
			{
				locals.entry = state.pausers.get(locals.i);
				if (!locals.entry.active)
				{
					locals.entry.account = input.account;
					locals.entry.active  = true;
					state.pausers.set(locals.i, locals.entry);
					output.success = true;
					return;
				}
			}
		}
	}

	struct RemoveRole_locals
	{
		RoleEntry entry;
		sint32 idx;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(RemoveRole)
	{
		output.success = false;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdmin(state, qpi.invocator()))
		{
			return;
		}

		if (input.role == (uint8)Role::Oracle)
		{
			locals.idx = findOracleIndex(state, input.account);
			if (locals.idx != NULL_INDEX)
			{
				locals.entry = state.oracles.get((uint32)locals.idx);
				locals.entry.active = false;
				state.oracles.set((uint32)locals.idx, locals.entry);
				if (state.oracleCount > 0)
					--state.oracleCount;
				output.success = true;
			}
		}
		else if (input.role == (uint8)Role::Pauser)
		{
			locals.idx = findPauserIndex(state, input.account);
			if (locals.idx != NULL_INDEX)
			{
				locals.entry = state.pausers.get((uint32)locals.idx);
				locals.entry.active = false;
				state.pausers.set((uint32)locals.idx, locals.entry);
				output.success = true;
			}
		}
	}

	PUBLIC_PROCEDURE(Pause)
	{
		output.success = false;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdminOrPauser(state, qpi.invocator()))
		{
			return;
		}

		state.paused = true;
		output.success = true;
	}

	PUBLIC_PROCEDURE(Unpause)
	{
		output.success = false;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdminOrPauser(state, qpi.invocator()))
		{
			return;
		}

		state.paused = false;
		output.success = true;
	}

	PUBLIC_PROCEDURE(EditFeeParameters)
	{
		output.success = false;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isAdmin(state, qpi.invocator()))
		{
			return;
		}

		// Only non-zero values are updated
		if (input.bpsFee != 0)
		{
			state.bpsFee = input.bpsFee;
		}

		if (input.protocolFee != 0)
		{
			state.protocolFee = input.protocolFee;
		}

		if (!isZero(input.protocolFeeRecipient))
		{
			state.protocolFeeRecipient = input.protocolFeeRecipient;
		}

		if (!isZero(input.oracleFeeRecipient))
		{
			state.oracleFeeRecipient = input.oracleFeeRecipient;
		}

		output.success = true;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// User procedures
		REGISTER_USER_PROCEDURE(Lock, 1);
		REGISTER_USER_PROCEDURE(OverrideLock, 2);
		REGISTER_USER_PROCEDURE(Unlock, 3);

		// Admin procedures
		REGISTER_USER_PROCEDURE(TransferAdmin, 10);
		REGISTER_USER_PROCEDURE(EditOracleThreshold, 11);
		REGISTER_USER_PROCEDURE(AddRole, 12);
		REGISTER_USER_PROCEDURE(RemoveRole, 13);
		REGISTER_USER_PROCEDURE(Pause, 14);
		REGISTER_USER_PROCEDURE(Unpause, 15);
		REGISTER_USER_PROCEDURE(EditFeeParameters, 16);
	}

	// ---------------------------------------------------------------------
	// Initialization
	// ---------------------------------------------------------------------

	INITIALIZE()
	{
		// No admin set initially; first TransferAdmin call bootstraps admin.
		state.admin = id(100, 200, 300, 400);
		state.paused = false;

		state.oracleThreshold = 67; // default 67% (2/3 + 1 style)
		state.oracleCount     = 0;

		// Clear role mappings and filled order table
		setMemory(state.oracles, 0);
		setMemory(state.pausers, 0);
		setMemory(state.filledOrders, 0);
		setMemory(state.lockedOrders, 0);

		// Default fee configuration: no fees(it will be decided later)
		state.bpsFee = 0;
		state.protocolFee = 0;
		state.protocolFeeRecipient = NULL_ID;
		state.oracleFeeRecipient = NULL_ID;
	}
};
