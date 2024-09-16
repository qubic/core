#pragma once

#include "logging/logging.h"
#include "network_core/peers.h"


// Request: ranges of log ID
void qLogger::processRequestLog(Peer* peer, RequestResponseHeader* header)
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
            if (endIdBufferRange.startIndex < startIdBufferRange.startIndex)
            {
                // round buffer case, only response first packet - let the client figure out and request the rest
                unsigned long long i = 0;
                for (i = request->fromID; i <= request->toID; i++)
                {
                    BlobInfo iBufferRange = logBuf.getBlobInfo(i);
                    if (iBufferRange.startIndex < startIdBufferRange.startIndex)
                    {
                        i--;
                        break;
                    }
                }
                // first packet: from startID to end of buffer
                {
                    BlobInfo iBufferRange = logBuf.getBlobInfo(i);
                    unsigned long long startFrom = startIdBufferRange.startIndex;
                    unsigned long long length = iBufferRange.length + iBufferRange.startIndex - startFrom;
                    if (length > RequestResponseHeader::max_size)
                    {
                        length = RequestResponseHeader::max_size;
                    }
                    enqueueResponse(peer, (unsigned int)(length), RespondLog::type, header->dejavu(), logBuffer + startFrom);
                }
                // second packet: from start buffer to endID - NOT SEND THIS
            }
            else
            {
                unsigned long long startFrom = startIdBufferRange.startIndex;
                unsigned long long length = endIdBufferRange.length + endIdBufferRange.startIndex - startFrom;
                if (length > RequestResponseHeader::max_size)
                {
                    length = RequestResponseHeader::max_size;
                }
                enqueueResponse(peer, (unsigned int)(length), RespondLog::type, header->dejavu(), logBuffer + startFrom);
            }
        }
        else
        {
            enqueueResponse(peer, 0, RespondLog::type, header->dejavu(), NULL);
        }
        return;
    }
#endif
    enqueueResponse(peer, 0, RespondLog::type, header->dejavu(), NULL);
}

void qLogger::processRequestTxLogInfo(Peer* peer, RequestResponseHeader* header)
{
#if ENABLED_LOGGING
    RequestLogIdRangeFromTx* request = header->getPayload<RequestLogIdRangeFromTx>();
    if (request->passcode[0] == logReaderPasscodes[0]
        && request->passcode[1] == logReaderPasscodes[1]
        && request->passcode[2] == logReaderPasscodes[2]
        && request->passcode[3] == logReaderPasscodes[3]
        && request->tick < system.tick
        && request->tick >= system.initialTick
        )
    {
        ResponseLogIdRangeFromTx resp;
        BlobInfo info = tx.getLogIdInfo(request->tick, request->txId);
        resp.fromLogId = info.startIndex;
        resp.length = info.length;
        enqueueResponse(peer, sizeof(ResponseLogIdRangeFromTx), ResponseLogIdRangeFromTx::type, header->dejavu(), &resp);
        return;
    }
#endif
    enqueueResponse(peer, 0, ResponseLogIdRangeFromTx::type, header->dejavu(), NULL);
}
