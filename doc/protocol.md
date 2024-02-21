# Qubic Protocol

This documents parts of the target Qubic Protocol.
This is a non-final incomplete draft and may change.

Also, the current implementation may differ in some details.
If you find such protocol violations in the code, feel free to [contribute](contributing.md) via a pull request or via contacting one of the Core developers on Discord.


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


## ...

