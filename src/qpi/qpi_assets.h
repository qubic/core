#pragma once

#include "qpi_types.h"

namespace QPI
{
	struct AssetIssuanceSelect : public Asset
	{
		bool anyIssuer;
		bool anyName;

		inline static AssetIssuanceSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetIssuanceSelect byIssuer(const id& owner)
		{
			return { owner, 0, false, true };
		}

		inline static AssetIssuanceSelect byName(uint64 assetName)
		{
			return { m256i::zero(), assetName, true, false };
		}
	};

	struct AssetOwnershipSelect
	{
		id owner;
		uint16 managingContract;
		bool anyOwner;
		bool anyManagingContract;

		inline static AssetOwnershipSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetOwnershipSelect byOwner(const id& owner)
		{
			return { owner, 0, false, true };
		}

		inline static AssetOwnershipSelect byManagingContract(uint16 managingContract)
		{
			return { m256i::zero(), managingContract, true, false };
		}
	};

	struct AssetPossessionSelect
	{
		id possessor;
		uint16 managingContract;
		bool anyPossessor;
		bool anyManagingContract;

		inline static AssetPossessionSelect any()
		{
			return { id::zero(), 0, true, true };
		}

		inline static AssetPossessionSelect byPossessor(const id& possessor)
		{
			return { possessor, 0, false, true };
		}

		inline static AssetPossessionSelect byManagingContract(uint16 managingContract)
		{
			return { m256i::zero(), managingContract, true, false };
		}
	};

	// Iterator for asset issuance records.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE FOR LOCKING! (not relevant for contract devs)
	class AssetIssuanceIterator
	{
	protected:
		AssetIssuanceSelect _issuance;
		unsigned int _issuanceIdx;

	public:
		AssetIssuanceIterator(const AssetIssuanceSelect& issuance = AssetIssuanceSelect::any())
		{
			begin(issuance);
		}

		// Start iteration with issuance filter (selects first record).
		inline void begin(const AssetIssuanceSelect& issuance);

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next issuance record matching filtering criteria.
		inline bool next();

		// Issuer of current record
		inline id issuer() const;

		// Asset name of current record
		inline uint64 assetName() const;

		// Return asset (pair of issuer and asset name)
		inline Asset asset() const
		{
			return Asset{ issuer(), assetName() };
		}

		// Index of issuance in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if issuance has not been found.
		inline unsigned int issuanceIndex() const
		{
			return _issuanceIdx;
		}
	};

	// Iterator for ownership records of specific issuance also providing filtering options.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE OF LOCKING! (not relevant for contract devs)
	class AssetOwnershipIterator
	{
	protected:
		Asset _issuance;
		unsigned int _issuanceIdx;
		AssetOwnershipSelect _ownership;
		unsigned int _ownershipIdx;

		// Constructor for derived classes, which should call begin() themselves.
		AssetOwnershipIterator()
		{
		}

	public:
		AssetOwnershipIterator(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any())
		{
			begin(issuance, ownership);
		}

		// Start iteration with given issuance and given ownership filter (selects first record).
		inline void begin(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any());

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next ownership record matching filtering criteria.
		inline bool next();

		// Issuer of current record
		inline id issuer() const;

		// Asset name of current record
		inline uint64 assetName() const;

		// Owner of current record
		inline id owner() const;

		// Number of shares in current ownership record
		inline sint64 numberOfOwnedShares() const;

		// Contract index of contract having management rights (can transfer ownership)
		inline uint16 ownershipManagingContract() const;

		// Index of issuance in universe. Should not be used by contracts, because it may change between contract calls.
		// Constant not changed by next(). NO_ASSET_INDEX if issuance has not been found.
		inline unsigned int issuanceIndex() const
		{
			return _issuanceIdx;
		}

		// Index of ownership in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if no (more) matching ownership has not been found.
		inline unsigned int ownershipIndex() const
		{
			return _ownershipIdx;
		}
	};

	// Iterator for possession records of specific issuance also providing filtering options.
	// CAUTION CORE DEVS: DOES NOT TAKE CARE OF LOCKING! (not relevant for contract devs)
	class AssetPossessionIterator : public AssetOwnershipIterator
	{
	protected:
		AssetPossessionSelect _possession;
		unsigned int _possessionIdx;

	public:
		AssetPossessionIterator(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(), const AssetPossessionSelect& possession = AssetPossessionSelect::any())
		{
			begin(issuance, ownership, possession);
		}

		// Start iteration with given issuance and given ownership + possession filters (selects first record).
		inline void begin(const Asset& issuance, const AssetOwnershipSelect& ownership = AssetOwnershipSelect::any(), const AssetPossessionSelect& possession = AssetPossessionSelect::any());

		// Return if iteration with next() has reached end.
		inline bool reachedEnd() const;

		// Step to next possession record matching filtering criteria.
		inline bool next();

		// Owner of current record
		inline id possessor() const;

		// Number of shares in current possession record
		inline sint64 numberOfPossessedShares() const;

		// Index of possession record in universe. Should not be used by contracts, because it may change between contract calls.
		// Changed by next(). NO_ASSET_INDEX if no (more) matching ownership has not been found.
		inline unsigned int possessionIndex() const
		{
			return _possessionIdx;
		}

		// Contract index of contract having management rights (can transfer possession)
		inline uint16 possessionManagingContract() const;
	};
}
