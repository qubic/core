#pragma once

#include "oracle_core/oracle_engine.h"
#include "network_messages/oracles.h"
#include "network_core/peers.h"

template <uint16_t ownComputorSeedsCount>
void OracleEngine<ownComputorSeedsCount>::processRequestOracleData(Peer* peer, RequestResponseHeader* header) const
{
	// check input
	ASSERT(header && peer);
	ASSERT(header->type() == RequestOracleData::type());
	if (!header->checkPayloadSize(sizeof(RequestOracleData)))
		return;

	// prepare buffer
	constexpr int maxQueryIdCount = 128;
	constexpr int payloadBufferSize = math_lib::max(
		(int)math_lib::max(MAX_ORACLE_QUERY_SIZE, MAX_ORACLE_REPLY_SIZE),
		(int)math_lib::max(maxQueryIdCount * 8ull, sizeof(revenuePoints)));
	static_assert(payloadBufferSize >= sizeof(RespondOracleDataQueryMetadata), "Buffer too small.");
	static_assert(payloadBufferSize < 32 * 1024, "Large alloc in stack may need reconsideration.");
	uint8_t responseBuffer[sizeof(RespondOracleData) + payloadBufferSize];
	RespondOracleData* response = (RespondOracleData*)responseBuffer;
	void* payload = responseBuffer + sizeof(RespondOracleData);
	int64_t* payloadQueryIds = (int64_t*)(responseBuffer + sizeof(RespondOracleData));

	// lock for accessing engine data
	LockGuard lockGuard(lock);

	// process request
	const RequestOracleData* request = header->getPayload<RequestOracleData>();
	switch (request->reqType)
	{

	case RequestOracleData::requestAllQueryIdsByTick:
	case RequestOracleData::requestUserQueryIdsByTick:
	case RequestOracleData::requestContractDirectQueryIdsByTick:
	case RequestOracleData::requestContractSubscriptionQueryIdsByTick:
	{
		// check if data for tick is available
		if (request->reqTickOrId < system.initialTick || request->reqTickOrId >= system.tick) // TODO: > or >=
		{
			// data isn't available -> send RespondOracleDataValidTickRange message
			response->resType = RespondOracleData::respondTickRange;
			auto* payloadTickRange = (RespondOracleDataValidTickRange*)payload;
			payloadTickRange->firstTick = system.initialTick;
			payloadTickRange->currentTick = system.tick;
			enqueueResponse(peer, sizeof(RespondOracleData) + sizeof(RespondOracleDataValidTickRange),
				RespondOracleData::type(), header->dejavu(), response);
			break;
		}

		// select filter function for the request type
		static_assert(RequestOracleData::requestAllQueryIdsByTick == 0);
		static_assert(RequestOracleData::requestUserQueryIdsByTick == 1);
		static_assert(RequestOracleData::requestContractDirectQueryIdsByTick == 2);
		static_assert(RequestOracleData::requestContractSubscriptionQueryIdsByTick == 3);
		typedef bool (*FilterFunc)(const OracleQueryMetadata& oqm);
		const FilterFunc allFilterFunc[] = {
			nullptr,
			[](const OracleQueryMetadata& oqm) { return oqm.type == ORACLE_QUERY_TYPE_USER_QUERY; },
			[](const OracleQueryMetadata& oqm) { return oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY; },
			[](const OracleQueryMetadata& oqm) { return oqm.type == ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION; },
		};
		const FilterFunc filterFunc = allFilterFunc[request->reqType];

		// send query IDs of queries of a given tick, splitting the array in multiple messages if needed
		if (request->reqTickOrId >= UINT32_MAX)
			break;
		const unsigned int tick = (unsigned int)request->reqTickOrId;
		unsigned int queryIndex = findFirstQueryIndexOfTick(tick);
		if (queryIndex == UINT32_MAX)
			break;
		response->resType = RespondOracleData::respondQueryIds;
		bool moreQueries = true;
		do
		{
			unsigned int idxInMsg = 0;
			while (idxInMsg < maxQueryIdCount && moreQueries)
			{
				if (!filterFunc || filterFunc(queries[queryIndex]))
				{
					payloadQueryIds[idxInMsg] = queries[queryIndex].queryId;
					++idxInMsg;
				}
				++queryIndex;
				moreQueries = queryIndex < oracleQueryCount && queries[queryIndex].queryTick == tick;
			}
			enqueueResponse(peer, sizeof(RespondOracleData) + idxInMsg * 8,
				RespondOracleData::type(), header->dejavu(), response);
		} while (moreQueries);
		break;
	}

	case RequestOracleData::requestPendingQueryIds:
	{
		// send query IDs of pending queries, splitting the array in multiple messages if needed
		response->resType = RespondOracleData::respondQueryIds;
		const unsigned int numMessages = (pendingQueryIndices.numValues + maxQueryIdCount - 1) / maxQueryIdCount;
		unsigned int idIdx = 0;
		for (unsigned int msgIdx = 0; msgIdx < numMessages; ++msgIdx)
		{
			unsigned int idxInMsg = 0;
			for (; idxInMsg < maxQueryIdCount && idIdx < pendingQueryIndices.numValues; ++idxInMsg, ++idIdx)
			{
				const uint32_t queryIndex = pendingQueryIndices.values[idIdx];
				payloadQueryIds[idxInMsg] = queries[queryIndex].queryId;
			}
			enqueueResponse(peer, sizeof(RespondOracleData) + idxInMsg * 8,
				RespondOracleData::type(), header->dejavu(), response);
		}
		break;
	}

	case RequestOracleData::requestQueryAndResponse:
	{
		// get query metadata
		const int64_t queryId = request->reqTickOrId;
		uint32_t queryIndex;
		if (queryId < 0 || !queryIdToIndex->get(queryId, queryIndex))
			break;
		const OracleQueryMetadata& oqm = queries[queryIndex];

		// prepare metadata response
		response->resType = RespondOracleData::respondQueryMetadata;
		auto* payloadOqm = (RespondOracleDataQueryMetadata*)payload;
		setMemory(*payloadOqm, 0);
		payloadOqm->queryId = oqm.queryId;
		payloadOqm->type = oqm.type;
		payloadOqm->status = oqm.status;
		payloadOqm->statusFlags = oqm.statusFlags;
		payloadOqm->queryTick = oqm.queryTick;
		if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_QUERY)
		{
			payloadOqm->queryingEntity = m256i(oqm.typeVar.contract.queryingContract, 0, 0, 0);
		}
		else if (oqm.type == ORACLE_QUERY_TYPE_USER_QUERY)
		{
			payloadOqm->queryingEntity = oqm.typeVar.user.queryingEntity;
		}
		payloadOqm->timeout = *(uint64_t*)&oqm.timeout;
		payloadOqm->interfaceIndex = oqm.interfaceIndex;
		if (oqm.type == ORACLE_QUERY_TYPE_CONTRACT_SUBSCRIPTION)
		{
			payloadOqm->subscriptionId = oqm.typeVar.subscription.subscriptionId;
		}
		if (oqm.status == ORACLE_QUERY_STATUS_SUCCESS)
		{
			payloadOqm->revealTick = oqm.statusVar.success.revealTick;
		}
		if (oqm.status == ORACLE_QUERY_STATUS_PENDING || oqm.status == ORACLE_QUERY_STATUS_COMMITTED)
		{
			const OracleReplyState& replyState = replyStates[oqm.statusVar.pending.replyStateIndex];
			payloadOqm->agreeingCommits = replyState.replyCommitHistogramCount[replyState.mostCommitsHistIdx];
			payloadOqm->totalCommits = replyState.totalCommits;
		}
		else if (oqm.status == ORACLE_QUERY_STATUS_TIMEOUT || oqm.status == ORACLE_QUERY_STATUS_UNRESOLVABLE)
		{
			payloadOqm->agreeingCommits = oqm.statusVar.failure.agreeingCommits;
			payloadOqm->totalCommits = oqm.statusVar.failure.totalCommits;
		}

		// send metadata response
		enqueueResponse(peer, sizeof(RespondOracleData) + sizeof(RespondOracleDataQueryMetadata),
			RespondOracleData::type(), header->dejavu(), response);

		// get and send query data
		const uint16_t querySize = (uint16_t)OI::oracleInterfaces[oqm.interfaceIndex].querySize;
		ASSERT(querySize <= payloadBufferSize);
		if (getOracleQueryWithoutLocking(queryId, payload, querySize))
		{
			response->resType = RespondOracleData::respondQueryData;
			enqueueResponse(peer, sizeof(RespondOracleData) + querySize,
				RespondOracleData::type(), header->dejavu(), response);
		}

		// get and send reply data
		if (oqm.status == ORACLE_QUERY_STATUS_SUCCESS)
		{
			const uint16_t replySize = (uint16_t)OI::oracleInterfaces[oqm.interfaceIndex].replySize;
			const void* replyData = getReplyDataFromTickTransactionStorage(oqm);
			copyMem(payload, replyData, replySize);
			response->resType = RespondOracleData::respondReplyData;
			enqueueResponse(peer, sizeof(RespondOracleData) + replySize,
				RespondOracleData::type(), header->dejavu(), response);
		}

		break;
	}

	case RequestOracleData::requestSubscription:
		// TODO
		break;

	case RequestOracleData::requestQueryStatistics:
	{
		// prepare response
		response->resType = RespondOracleData::respondQueryStatistics;
		auto* p = (RespondOracleDataQueryStatistics*)payload;
		setMemory(*p, 0);
		p->pendingCount = pendingQueryIndices.numValues;
		p->pendingOracleMachineCount = pendingQueryIndices.numValues - pendingCommitReplyStateIndices.numValues - pendingRevealReplyStateIndices.numValues;
		p->pendingCommitCount = pendingCommitReplyStateIndices.numValues;
		p->pendingRevealCount = pendingRevealReplyStateIndices.numValues;
		p->successfulCount = stats.successCount;
		p->unresolvableCount = stats.unresolvableCount;
		const uint64_t totalTimeouts = stats.timeoutNoReplyCount + stats.timeoutNoCommitCount + stats.timeoutNoRevealCount;
		p->timeoutCount = totalTimeouts;
		p->timeoutNoReplyCount = stats.timeoutNoReplyCount;
		p->timeoutNoCommitCount = stats.timeoutNoCommitCount;
		p->timeoutNoRevealCount = stats.timeoutNoRevealCount;
		p->oracleMachineRepliesDisagreeCount = stats.oracleMachineRepliesDisagreeCount;
		p->oracleMachineReplyAvgMilliTicksPerQuery = (stats.oracleMachineReplyCount) ? stats.oracleMachineReplyTicksSum * 1000 / stats.oracleMachineReplyCount : 0;
		p->commitAvgMilliTicksPerQuery = (stats.commitCount) ? stats.commitTicksSum * 1000 / stats.commitCount : 0;
		p->successAvgMilliTicksPerQuery = (stats.successCount) ? stats.successTicksSum * 1000 / stats.successCount : 0;
		p->timeoutAvgMilliTicksPerQuery = (totalTimeouts) ? stats.timeoutTicksSum * 1000 / totalTimeouts : 0;
		p->revealTxCount = stats.revealTxCount;
		p->wrongKnowledgeProofCount = stats.wrongKnowledgeProofCount;

		// send response
		enqueueResponse(peer, sizeof(RespondOracleData) + sizeof(RespondOracleDataQueryStatistics),
			RespondOracleData::type(), header->dejavu(), response);
		break;
	}

	case RequestOracleData::requestOracleRevenuePoints:
	{
		// prepare response
		response->resType = RespondOracleData::respondOracleRevenuePoints;
		uint64_t* payloadRevPoints = (uint64_t*)(responseBuffer + sizeof(RespondOracleData));
		copyMem(payloadRevPoints, revenuePoints, sizeof(revenuePoints));

		// send response
		enqueueResponse(peer, sizeof(RespondOracleData) + sizeof(revenuePoints),
			RespondOracleData::type(), header->dejavu(), response);

		break;
	}

	}

	enqueueResponse(peer, 0, EndResponse::type(), header->dejavu(), nullptr);
}
