#pragma once

using namespace QPI;

/**
* Shared building blocks for all EVM cross-chain oracle interfaces.
*
* Keep generic types here (chain ids, address/word representations, helpers) so that future EVM
* oracle interfaces (e.g. reading event logs, storage slots, balances) can reuse them instead of
* redefining their own. Each concrete EVM interface (such as EvmErc20Transfer) includes this header.
*
* Representation conventions (chosen for determinism and to mirror the EVM ABI):
* - All hashes are raw 32-byte big-endian values.
* - All EVM addresses are 20 bytes, stored left-zero-padded into a 32-byte word (the same way the
*   ABI encodes an `address`). The QPI Array<> capacity must be a power of two, so 20-byte arrays
*   are not representable directly anyway.
* - All EVM 256-bit integers (uint256) are stored as 32-byte big-endian values.
*/
namespace Evm
{
    /// Raw 32-byte big-endian value: tx hash, block hash, ABI word, etc.
    typedef Array<uint8, 32> Bytes32;

    /// EVM address (20 bytes) left-zero-padded into a 32-byte ABI word.
    typedef Array<uint8, 32> Address;

    /// EVM 256-bit unsigned integer, big-endian (e.g. an ERC20 token amount).
    typedef Array<uint8, 32> Uint256;

    /// Chain ids (decimal) of supported EVM networks.
    struct ChainId
    {
        static constexpr uint64 ethereum  = 1;       // 0x1
        static constexpr uint64 optimism  = 10;      // 0xa
        static constexpr uint64 bsc       = 56;      // 0x38
        static constexpr uint64 polygon   = 137;     // 0x89
        static constexpr uint64 fantom    = 250;     // 0xfa
        static constexpr uint64 base      = 8453;    // 0x2105
        static constexpr uint64 avalanche = 43114;   // 0xa86a
        static constexpr uint64 arbitrum  = 42161;   // 0xa4b1
        // add new chain ids above this line
    };

    /// Return true if the chain id is one this oracle is expected to serve.
    static bool isSupportedChain(uint64 chainId)
    {
        return chainId == ChainId::ethereum
            || chainId == ChainId::optimism
            || chainId == ChainId::bsc
            || chainId == ChainId::polygon
            || chainId == ChainId::fantom
            || chainId == ChainId::base
            || chainId == ChainId::avalanche
            || chainId == ChainId::arbitrum;
    }
}
