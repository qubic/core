# K12 state cache: incremental contract state digests

Every tick, the computer digest covers each contract state that changed (`state.mut()`). Until now
a changed state was rehashed in full with KangarooTwelve, so one written byte in a 600 MB state cost
a 600 MB hash. The K12 state cache rehashes only the parts that were written. The digest value is
unchanged: it is bit-identical to the one-shot `KangarooTwelve(state, size, out, 32)`.

## How it works

KangarooTwelve is a tree hash. After the first 8 KiB chunk, every further 8 KiB leaf yields a 32-byte
chaining value that depends on that leaf alone; the final node absorbs the chaining values in order.
The cache keeps the chaining values of every large contract between ticks (0.4 percent of the state
size) and recomputes only the leaves whose pages were written. The first chunk and the trailing
partial leaf are always re-read.

Which pages were written comes from the hardware: the state's page tables are split into 4 KiB
entries at start-up, and the CPU sets the Dirty bit of a page-table entry on the first write to
that page. Before hashing, the contract processor (the only core that writes tracked states while
the node ticks) scans those bits, marks the leaves, clears the bits and flushes the pages from its
TLB. States written on other cores (contract 0 with the fee reserves, contracts in their IPO epoch,
state files loaded at start-up, migrations) stay on the full-hash path or invalidate their cache.

Only states of at least `PARALLEL_K12_LEAVES_MIN_STATE_SIZE` (4 MiB) are cached; the parallel leaf
pool on the request processors still hashes whatever has to be rehashed.

## Settings (`private_settings.h`)

- `USE_K12_STATE_CACHE 1`: enable the cache (requires `USE_PARALLEL_K12_LEAVES 1`).
- `K12_STATE_CACHE_FULL_CHECK 0`: validation only. With 1 every cached digest is compared against
  a one-shot K12 and the one-shot value is used on mismatch; costs a full hash per changed contract.

## What the node reports

At start-up:

    K12 state cache: hardware dirty tracking on, contracts not trackable: 0, page tables split: N, cache bytes: M

`off` means the firmware uses 5-level paging, which the walker does not handle; the node then hashes
as before. A non-zero "not trackable" count means the page tables of that many states could not be
split; those states hash as before.

In the health status (F2):

    K12 state cache: cached digests A, full rebuilds B, collections C, dirty pages D, tracking failures E, sample mismatches F, full-check mismatches G

`B` grows by the number of cached contracts at start-up and once per epoch transition. `E`, `F` and
`G` must stay 0. A sample mismatch means a write reached a state without being observed; the
contract is then permanently taken off the cache and its digest is recomputed in full from then on,
so consensus is not affected.

## Safety

The digest is a pure function of the state bytes, never of the dirty tracking. A node whose tracking
fell back, a node without the cache and a node with perfect tracking produce the same digest for the
same bytes. The only failure mode is a stale leaf, which the sample check and the full check exist
to catch, and both respond by leaving the cache.

Consensus is unchanged: no digest, wire format or file format differs from a node without the cache,
so nodes with and without it can run in the same network.

## Cost

Measured on a 600 MiB state with 6 or 7 helper cores (`tools/k12_bench cache`): about 3 ms per
changed contract when nothing or little was written, versus about 500 ms for the full hash on one
core or 70 to 90 ms with helpers. With every page dirty the cost equals the parallel full hash.
Contract code runs on 4 KiB mappings instead of 2 MiB after the split; on random access this costs
about 15 percent, on sequential access nothing.
