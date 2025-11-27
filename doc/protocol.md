# Qubic Protocol

This documents parts of the target Qubic Protocol.
This is a non-final incomplete draft and may change.

Also, the current implementation may differ in some details.
If you find such protocol violations in the code, feel free to [contribute](contributing.md) via a pull request or via contacting one of the Core developers on Discord.


## List of network message types

The network message types are defined in `src/network_messages/`.
This is its current list ordered by type.
The type number is the identifier used in `RequestResponseHeader` (defined in `header.h`).

- `ExchangePublicPeers`, type 0, defined in `public_peers.h`.
- `BroadcastMessage`, type 1, defined in `broadcast_message.h`.
- `BroadcastComputors`, type 2, defined in `computors.h`.
- `BroadcastTick`, type 3, defined in `tick.h`.
- `BroadcastFutureTickData`, type 8, defined in `tick.h`.
- `RequestComputors`, type 11, defined in `computors.h`.
- `RequestQuorumTick`, type 14, defined in `tick.h`.
- `RequestTickData`, type 16, defined in `tick.h`.
- `BROADCAST_TRANSACTION`, type 24, defined in `transactions.h`.
- `REQUEST_TRANSACTION_INFO`, type 26, defined in `transactions.h`.
- `REQUEST_CURRENT_TICK_INFO`, type 27, defined in `tick.h`.
- `RESPOND_CURRENT_TICK_INFO`, type 28, defined in `tick.h`.
- `REQUEST_TICK_TRANSACTIONS`, type 29, defined in `transactions.h`.
- `RequestedEntity`, type 31, defined in `entity.h`.
- `RespondedEntity`, type 32, defined in `entity.h`.
- `RequestContractIPO`, type 33, defined in `contract.h`.
- `RespondContractIPO`, type 34, defined in `contract.h`.
- `EndResponse`, type 35, defined in `common_response.h`.
- `RequestIssuedAssets`, type 36, defined in `assets.h`.
- `RespondIssuedAssets`, type 37, defined in `assets.h`.
- `RequestOwnedAssets`, type 38, defined in `assets.h`.
- `RespondOwnedAssets`, type 39, defined in `assets.h`.
- `RequestPossessedAssets`, type 40, defined in `assets.h`.
- `RespondPossessedAssets`, type 41, defined in `assets.h`.
- `RequestContractFunction`, type 42, defined in `contract.h`.
- `RespondContractFunction`, type 43, defined in `contract.h`.
- `RequestLog`, type 44, defined in `logging.h`.
- `RespondLog`, type 45, defined in `logging.h`.
- `REQUEST_SYSTEM_INFO`, type 46, defined in `system_info.h`.
- `RespondSystemInfo`, type 47, defined in `system_info.h`.
- `RequestLogIdRangeFromTx`, type 48, defined in `logging.h`.
- `ResponseLogIdRangeFromTx`, type 49, defined in `logging.h`.
- `RequestAllLogIdRangesFromTick`, type 50, defined in `logging.h`.
- `ResponseAllLogIdRangesFromTick`, type 51, defined in `logging.h`.
- `RequestAssets`, type 52, defined in `assets.h`.
- `RespondAssets` and `RespondAssetsWithSiblings`, type 53, defined in `assets.h`.
- `TryAgain`, type 54, defined in `common_response.h`.
- `RequestPruningLog`, type 56, defined in `logging.h`.
- `ResponsePruningLog`, type 57, defined in `logging.h`.
- `RequestLogStateDigest`, type 58, defined in `logging.h`.
- `ResponseLogStateDigest`, type 59, defined in `logging.h`.
- `RequestedCustomMiningData`, type 60, defined in `custom_mining.h`.
- `RespondCustomMiningData`, type 61, defined in `custom_mining.h`.
- `RequestedCustomMiningSolutionVerification`, type 62, defined in `custom_mining.h`.
- `RespondCustomMiningSolutionVerification`, type 63, defined in `custom_mining.h`.
- `RequestActiveIPOs`, type 64, defined in `contract.h`.
- `RespondActiveIPO`, type 65, defined in `contract.h`.
- `SpecialCommand`, type 255, defined in `special_command.h`.

Addon messages (supported if addon is enabled):
- `REQUEST_TX_STATUS`, type 201, defined in `src/addons/tx_status_request.h`.
- `RESPOND_TX_STATUS`, type 202, defined in `src/addons/tx_status_request.h`.


## Peer Sharing

Peers are identified by IPv4 addresses in context of peer sharing.
They are referred to as "public peers" in the source code.
Each node needs an initial set of known public peers (ideally at least 4), which are entered in the list `knownPublicPeers` in `src/private_settings.h`.
The own IP should be included into `knownPublicPeers` as an ordinary peer.
IPs in `knownPublicPeers` get the verified status by default.

Peers are shared through the `ExchangePublicPeers` message defined in `src/network_messages/public_peers.h`.
The message is sent after a new connection has been established (the node connects to randomly selected public peers).
The IPs for sharing are picked randomly among verified peer IPs (however, there may be duplicate IPs in the `ExchangePublicPeers` message).
If there are no verified peers in the list of peers, then "0.0.0.0" must be sent as IPs with `ExchangePublicPeers`.

If an outgoing connection to a verified peer is rejected, the peer loses the verified status.
If an outgoing connection to a non-verified peer is rejected, the peer is removed from the list of peers.
If an outgoing connection to a non-verified peer is accepted and an `ExchangePublicPeers` message is received, the peer gets the verified status.
If a protocol violation is detected at any moment during communication (allowing to assume the remote end runs something else, not Qubic node), then the IP is removed even if it is verified.
An IP is only removed from the list of peers if the list still has at least 10 entries afterwards and if it is not in the initial `knownPublicPeers`.

## Broadcast Message

Defined in https://github.com/qubic/core/blob/main/src/network_messages/broadcast_message.h

```
struct BroadcastMessage
{
    m256i sourcePublicKey;
    m256i destinationPublicKey;
    m256i gammingNonce;

    enum {
        type = 1,
    };
};
```

- SourcePublicKey must not be NULL.
- The message signature must be verified with SourcePublicKey.
- The message is broadcasted if sourcePublicKey's balance is greater than or equal MESSAGE_DISSEMINATION_THRESHOLD or sourcePublicKey is a computor's public key.
- destinationPublicKey needs to be in the computorPublicKeys of the node:
  - If sourcePublicKey is the same as computorPublicKey:
    - computorPrivateKeys and sourcePublicKey are used to generate sharedKey.
    - sharedKey is used for generating gammingKey.
  - If sourcePublicKey is not the same as computorPublicKey, its balance must be greater than or equal MESSAGE_DISSEMINATION_THRESHOLD.
    - gammingKey can be extracted from the message.
    - gammingKey is used for decrypting the message's payload.
- The first byte of gammingKey (gammingKey[0]) is used to define the message type.

The message is processed as follows, depending on the message type:

### MESSAGE_TYPE_SOLUTION
- Payload size check.
- Solution mining seed is extracted from the first 32 bytes of the payload.
- Solution nonce is extracted from the next 32 bytes of the payload.
- The solution will be verified and recorded if it does not already exist in the current node.

## ...


