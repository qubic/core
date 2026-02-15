using namespace QPI;

/**
* Oracle interface "Football" for querying football match events from top 5 European leagues.
*
* Provides access to match results, scores, and status for:
* - Premier League (England)
* - La Liga (Spain)
* - Serie A (Italy)
* - Bundesliga (Germany)
* - Ligue 1 (France)
*
* Team IDs are numeric (uint32) to avoid string limitations in Qubic contracts.
* Match IDs are also numeric for efficient storage and querying.
*/
struct Football
{
	//-------------------------------------------------------------------------
	// Mandatory oracle interface definitions

	/// Oracle interface index
	static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

	/// Oracle query data / input to the oracle machine
	struct OracleQuery
	{
		/// Oracle provider (e.g., "apifootball", "mock")
		id oracle;

		/// Match ID to query (numeric identifier from API)
		uint32 matchId;

		/// League ID (39=Premier League, 140=La Liga, 135=Serie A, 78=Bundesliga, 61=Ligue 1)
		uint32 leagueId;

		/// Season year (e.g., 2024 for 2024/2025 season)
		uint32 season;

		/// Reserved for future use
		uint32 _reserved;
	};

	/// Oracle reply data / output of the oracle machine
	struct OracleReply
	{
		/// Home team ID (numeric)
		uint32 homeTeamId;

		/// Away team ID (numeric)
		uint32 awayTeamId;

		/// Home team score
		sint32 homeScore;

		/// Away team score
		sint32 awayScore;

		/// Match status: 0=Not Started, 1=In Progress, 2=Finished, 3=Postponed, 4=Cancelled
		uint8 status;

		/// Elapsed minutes (0-90+)
		uint8 elapsedMinutes;

		/// Reserved for future use
		uint16 _reserved;
	};

	/// Return query fee
	static sint64 getQueryFee(const OracleQuery& query)
	{
		return 10;
	}

	/// Return subscription fee
	static sint64 getSubscriptionFee(const OracleQuery& query, uint16 notifyIntervalInMinutes)
	{
		return 1000;
	}

	//-------------------------------------------------------------------------
	// Optional: convenience features

	/// Check if the passed oracle reply is valid
	static bool replyIsValid(const OracleReply& reply)
	{
		// Valid if teams are set and scores are non-negative (or -1 for not started)
		return reply.homeTeamId > 0 && reply.awayTeamId > 0 
			&& reply.homeScore >= -1 && reply.awayScore >= -1
			&& reply.status <= 4;
	}

	/// Match status enum for readability
	enum MatchStatus : uint8
	{
		NOT_STARTED = 0,
		IN_PROGRESS = 1,
		FINISHED = 2,
		POSTPONED = 3,
		CANCELLED = 4
	};

	/// League IDs
	static constexpr uint32 LEAGUE_PREMIER_LEAGUE = 39;
	static constexpr uint32 LEAGUE_LA_LIGA = 140;
	static constexpr uint32 LEAGUE_SERIE_A = 135;
	static constexpr uint32 LEAGUE_BUNDESLIGA = 78;
	static constexpr uint32 LEAGUE_LIGUE_1 = 61;

	/// Get oracle ID of mock oracle
	static id getMockOracleId()
	{
		using namespace Ch;
		return id(m, o, c, k, null);
	}

	/// Get oracle ID of TheSportsDB oracle (no authentication required!)
	static id getTheSportsDBOracleId()
	{
		using namespace Ch;
		return id(t, h, e, s, p, o, r, t, s, d, b, null);
	}
};
