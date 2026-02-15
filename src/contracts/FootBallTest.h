using namespace QPI;
struct FootballTest : public ContractBase
{
	//---------------------------------------------------------------
	// FOOTBALL ORACLE TESTING

	struct NotificationLog
	{
		uint32 contractIndex;
		uint8 interface;
		uint8 status;
		uint16 dataCheck;
		uint32 matchId;
		sint64 queryId;
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	// Optional: additional contract data associated with oracle query
	HashMap<uint64, uint32, 64> oracleQueryExtraData;

	struct QueryFootballOracle_input
	{
		OI::Football::OracleQuery footballOracleQuery;
		uint32 timeoutMilliseconds;
	};

	struct QueryFootballOracle_output
	{
		sint64 oracleQueryId;
	};

	PUBLIC_PROCEDURE(QueryFootballOracle)
	{
		output.oracleQueryId = QUERY_ORACLE(
			OI::Football,
			input.footballOracleQuery,
			NotifyFootballOracleReply,
			input.timeoutMilliseconds);

		if (output.oracleQueryId < 0)
		{
			// Error
			return;
		}

		// Example: store additional data related to oracle query
		state.oracleQueryExtraData.set(output.oracleQueryId, 0);
	}

	typedef OracleNotificationInput<OI::Football> NotifyFootballOracleReply_input;
	typedef NoData NotifyFootballOracleReply_output;

	struct NotifyFootballOracleReply_locals
	{
		OI::Football::OracleQuery query;
		uint32 queryExtraData;
		NotificationLog notificationLog;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(NotifyFootballOracleReply)
	{
		locals.notificationLog = NotificationLog{
			CONTRACT_INDEX,
			OI::Football::oracleInterfaceIndex,
			input.status,
			OI::Football::replyIsValid(input.reply),
			0,
			input.queryId};
		LOG_INFO(locals.notificationLog);

		if (input.status == ORACLE_QUERY_STATUS_SUCCESS)
		{
			// Get and use query info if needed
			if (!qpi.getOracleQuery<OI::Football>(input.queryId, locals.query))
				return;

			// Get and use additional query info stored by contract if needed
			if (!state.oracleQueryExtraData.get(input.queryId, locals.queryExtraData))
				return;

			// Use example convenience function provided by oracle interface
			if (!OI::Football::replyIsValid(input.reply))
				return;

			// Use the match data
			// input.reply.homeTeamId - Home team ID
			// input.reply.awayTeamId - Away team ID
			// input.reply.homeScore - Home team score
			// input.reply.awayScore - Away team score
			// input.reply.status - Match status (0-4)
			// input.reply.elapsedMinutes - Elapsed minutes

			// Example: Check if match is finished
			if (input.reply.status == OI::Football::FINISHED)
			{
				// Match is finished, process final score
				// TODO: Implement betting logic, payouts, etc.
			}
		}
		else
		{
			// Handle failure (TIMEOUT or UNRESOLVABLE)
		}
	}

	struct END_TICK_locals
	{
		OI::Football::OracleQuery footballOracleQuery;
		sint64 oracleQueryId;
		uint32 c;
		NotificationLog notificationLog;
	};

	END_TICK_WITH_LOCALS()
	{
		// Query football oracles automatically every 13 ticks
		if (qpi.tick() % 13 == 1)
		{
			locals.c = (qpi.tick() / 13) % 3;

			// Setup query
			if (locals.c == 0)
			{
				// Query mock oracle - Match 1001 (Man Utd vs Liverpool)
				// For testing without external API
				using namespace Ch;
				locals.footballOracleQuery.oracle = OI::Football::getMockOracleId();
				locals.footballOracleQuery.matchId = 1001;
				locals.footballOracleQuery.leagueId = OI::Football::LEAGUE_PREMIER_LEAGUE;
				locals.footballOracleQuery.season = 2024;
				locals.footballOracleQuery._reserved = 0;
				
				// Alternative: Query TheSportsDB for real data (no API key needed!)
				// locals.footballOracleQuery.oracle = OI::Football::getTheSportsDBOracleId();
				// locals.footballOracleQuery.matchId = 2279631;  // Real Madrid vs Real Sociedad
			}
			else if (locals.c == 1)
			{
				// Query mock oracle - Match 1002 (Real Madrid vs Barcelona)
				using namespace Ch;
				locals.footballOracleQuery.oracle = OI::Football::getMockOracleId();
				locals.footballOracleQuery.matchId = 1002;
				locals.footballOracleQuery.leagueId = OI::Football::LEAGUE_LA_LIGA;
				locals.footballOracleQuery.season = 2024;
				locals.footballOracleQuery._reserved = 0;
				
				// Alternative: Query TheSportsDB for real data
				// locals.footballOracleQuery.oracle = OI::Football::getTheSportsDBOracleId();
				// locals.footballOracleQuery.matchId = 2267320;  // Man Utd vs Tottenham
			}
			else if (locals.c == 2)
			{
				// Query mock oracle - Match 1003 (Bayern vs Dortmund)
				using namespace Ch;
				locals.footballOracleQuery.oracle = OI::Football::getMockOracleId();
				locals.footballOracleQuery.matchId = 1003;
				locals.footballOracleQuery.leagueId = OI::Football::LEAGUE_BUNDESLIGA;
				locals.footballOracleQuery.season = 2024;
				locals.footballOracleQuery._reserved = 0;
				
				// Alternative: Query TheSportsDB for real data
				// locals.footballOracleQuery.oracle = OI::Football::getTheSportsDBOracleId();
				// locals.footballOracleQuery.matchId = 2279631;  // Any real match ID
			}

			locals.oracleQueryId = QUERY_ORACLE(
				OI::Football,
				locals.footballOracleQuery,
				NotifyFootballOracleReply,
				60000); // 60 second timeout

			ASSERT(qpi.getOracleQueryStatus(locals.oracleQueryId) == ORACLE_QUERY_STATUS_PENDING);

			locals.notificationLog = NotificationLog{
				CONTRACT_INDEX,
				OI::Football::oracleInterfaceIndex,
				ORACLE_QUERY_STATUS_PENDING,
				0,
				locals.footballOracleQuery.matchId,
				locals.oracleQueryId};
			LOG_INFO(locals.notificationLog);
		}
	}

	//---------------------------------------------------------------
	// COMMON PARTS

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(QueryFootballOracle, 100);
		REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyFootballOracleReply);
	}
};
