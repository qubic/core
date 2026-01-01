# Qubic Protocol

This documents parts of the target Qubic Protocol.
This is a non-final incomplete draft and may change.

Also, the current implementation may differ in some details.
If you find such protocol violations in the code, feel free to [contribute](contributing.md) via a pull request or via contacting one of the Core developers on Discord.


## List of network message types

The network message types are defined in a single `enum` in [src/network_messages/network_message_type.h](https://github.com/qubic/core/blob/main/src/network_messages/network_message_type.h).
The type number is the identifier used in `RequestResponseHeader` (defined in `header.h`).
The type number is usually available from the network message type via the `static constexpr unsigned char type()` method.

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





