#pragma once

#include "logging/logging.h"
#include "network_core/peers.h"


// Request: ranges of log ID
void qLogger::processRequestLog(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header)
{
#if ENABLED_LOGGING
    RequestLog* request = header->getPayload<RequestLog>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3])
    {
        BlobInfo startIdBufferRange = logBuf.getBlobInfo(request->fromID);
        BlobInfo endIdBufferRange = logBuf.getBlobInfo(request->toID); // inclusive
        if (startIdBufferRange.startIndex != -1 && startIdBufferRange.length != -1
            && endIdBufferRange.startIndex != -1 && endIdBufferRange.length != -1)
        {
            unsigned long long toID = request->toID;
            long long startFrom = startIdBufferRange.startIndex;
            long long length = endIdBufferRange.length + endIdBufferRange.startIndex - startFrom;
            constexpr long long maxPayloadSize = RequestResponseHeader::max_size - sizeof(sizeof(RequestResponseHeader));

            if (length > maxPayloadSize)
            {
                while (length > maxPayloadSize && toID > request->fromID)
                {
                    toID = request->fromID + (toID - request->fromID) / 2;
                    endIdBufferRange = logBuf.getBlobInfo(toID);
                    length = endIdBufferRange.length + endIdBufferRange.startIndex - startFrom;
                }
            }
            if (length < maxPayloadSize)
            {
                char* rBuffer = responseBuffers[processorNumber];
                logBuffer.getMany(rBuffer, startFrom, length);
                enqueueResponse(peer, (unsigned int)(length), RespondLog::type, header->dejavu(), rBuffer);
            }
            else
            {
                enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
            }
        }
        else
        {
            enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
        }
        return;
    }
#endif
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

void qLogger::processRequestTxLogInfo(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header)
{
#if ENABLED_LOGGING
    RequestLogIdRangeFromTx* request = header->getPayload<RequestLogIdRangeFromTx>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3]
        && request->tick >= tickBegin
        )
    {
        ResponseLogIdRangeFromTx resp;
        if (request->tick <= lastUpdatedTick)
        {
            BlobInfo info = tx.getLogIdInfo(processorNumber, request->tick, request->txId);
            resp.fromLogId = info.startIndex;
            resp.length = info.length;
        }
        else
        {
            // logging of this tick hasn't generated
            resp.fromLogId = -3;
            resp.length = -3;
        }
        
        enqueueResponse(peer, sizeof(ResponseLogIdRangeFromTx), ResponseLogIdRangeFromTx::type, header->dejavu(), &resp);
        return;
    }
#endif
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

void qLogger::processRequestTickTxLogInfo(unsigned long long processorNumber, Peer* peer, RequestResponseHeader* header)
{
#if ENABLED_LOGGING
    RequestAllLogIdRangesFromTick* request = header->getPayload<RequestAllLogIdRangesFromTick>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3]
        && request->tick >= tickBegin
        )
    {
        ResponseAllLogIdRangesFromTick* resp = (ResponseAllLogIdRangesFromTick * )responseBuffers[processorNumber];
        int txId = 0;
        if (request->tick <= lastUpdatedTick)
        {
            tx.getTickLogIdInfo((TickBlobInfo*)resp, request->tick);
        }
        else
        {
            // logging of this tick hasn't generated
            for (txId = 0; txId < LOG_TX_PER_TICK; txId++)
            {
                resp->fromLogId[txId] = -3;
                resp->length[txId] = -3;
            }
        }
        enqueueResponse(peer, sizeof(ResponseAllLogIdRangesFromTick), ResponseAllLogIdRangesFromTick::type, header->dejavu(), resp);
        return;
    }
#endif
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

void qLogger::processRequestPrunePageFile(Peer* peer, RequestResponseHeader* header)
{
#if ENABLED_LOGGING
    RequestPruningLog* request = header->getPayload<RequestPruningLog>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3])
    {
        ResponsePruningLog resp;
        bool isValidRange = mapLogIdToBufferIndex.isIndexValid(request->fromLogId) && mapLogIdToBufferIndex.isIndexValid(request->toLogId);
        isValidRange &= (request->toLogId >= mapLogIdToBufferIndex.pageCap());
        isValidRange &= (request->toLogId >= request->fromLogId + mapLogIdToBufferIndex.pageCap());
        if (!isValidRange)
        {
            resp.success = 4;
        }
        else
        {
            auto pageCap = mapLogIdToBufferIndex.pageCap();
            // here we round up FROM but round down TO
            long long fromPageId = (request->fromLogId + pageCap - 1) / pageCap;
            long long toPageId = (request->toLogId - pageCap + 1) / pageCap;
            long long fromLogId = fromPageId * pageCap;
            long long toLogId = toPageId * pageCap + pageCap - 1;

            BlobInfo bis = mapLogIdToBufferIndex[fromLogId];
            BlobInfo bie = mapLogIdToBufferIndex[toLogId];
            long long logBufferStart = bis.startIndex;
            long long logBufferEnd = bie.startIndex + bie.length;
            resp.success = 0;
            bool res0 = mapLogIdToBufferIndex.pruneRange(fromLogId, toLogId);
            if (!res0)
            {
                resp.success = 1;
            }

            if (logBufferEnd > logBufferStart)
            {
                bool res1 = logBuffer.pruneRange(logBufferStart, logBufferEnd);
                if (!res1)
                {
                    resp.success = (resp.success << 1) | 1;
                }
            }
        }

        enqueueResponse(peer, sizeof(ResponsePruningLog), ResponsePruningLog::type, header->dejavu(), &resp);
        return;
    }
#endif
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

void qLogger::processRequestGetLogDigest(Peer* peer, RequestResponseHeader* header)
{
#if LOG_STATE_DIGEST
    RequestLogStateDigest* request = header->getPayload<RequestLogStateDigest>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3]
        && request->requestedTick >= tickBegin
        && request->requestedTick <= lastUpdatedTick)
    {
        ResponseLogStateDigest resp;
        resp.digest = digests[request->requestedTick - tickBegin];
        enqueueResponse(peer, sizeof(ResponseLogStateDigest), ResponseLogStateDigest::type, header->dejavu(), &resp);
        return;
    }
#endif
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}