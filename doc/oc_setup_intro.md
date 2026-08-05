# Testing your OC setup with the Mock interface

## What you are testing

Outsourced Computation (OC) lets a smart contract ask an off-chain machine to do
work. The **Mock interface** (interface index 0) is the trivial one: it takes a
single `uint64`, writes it to a file, and forwards the authorized bundle to a
public service. It computes nothing useful — it exists so you can verify the
whole pipeline end to end.

The chain of hops you are testing:

```
your Core node ──▶ your OC machine ──▶ ocmock.qubic.org
   (bare metal)      (Docker)            (provided)
```

You only run the first two. The mock service is hosted at
**https://ocmock.qubic.org** — you do not need to deploy one.

1. You send a transaction to the **QUtil** contract asking for a Mock invocation.
2. Once that tick reaches consensus, computors sign the invocation and each Core
   node pushes the authorized bundle to the OC machine it is configured with.
3. The OC machine writes the value to `mock_oc_sink.txt` and forwards the **raw
   bundle bytes** to the mock service.
4. The mock service cryptographically verifies the bundle — it requires **451
   distinct valid computor signatures** — deduplicates it by `invocationId`, and
   shows it on its homepage with a replication count (how many distinct OC
   machines reported the same invocation).

The last step is the point: the mock service trusts nobody. Anyone may POST to
it, so a bundle is only accepted if it carries real quorum signatures. If your
invocation shows up there, your whole setup — node, consensus, delivery,
machine, forwarding — is working.

## Before you start

Point `ocMachineIPs` in your node's `private_settings.h` at the OC machine's
public IP, then bring the machine up on its own host
(`https://github.com/qubic/oc-machine`):

```bash
cd oc-machine/docker
cp ../example_env .env       # then edit it — see below
mkdir -p data && chown -R 999:999 data
docker compose up -d --build
```

In `.env`, with inbound TCP **21841** open on that host:

- `OC_MACHINE_INTERFACE_INDEX=0` — the Mock interface.
- `OC_MACHINE_WHITELIST` — your Core nodes' public IPs, comma-separated. You
  **must** fill this in: `example_env` ships it empty, and an empty value falls
  back to localhost only, which silently drops every real node.
- `OC_MACHINE_MOCK_SERVICE_URL` — already set to `https://ocmock.qubic.org`;
  leave it alone unless you are deliberately pointing at your own service.

Quick sanity check:

```bash
curl -s -o /dev/null -w "%{http_code}\n" https://ocmock.qubic.org/   # expect 200
docker logs -t oc-machine-node | tail -20   # expect "starting (port 21841 ...)"
                                            # and    "Mock service forwarding: ..."
```

## Trigger a test invocation

Send a `TriggerOC` transaction to QUtil (contract 4, inputType 103). The attached
amount pays the invocation fee — 10 qu for Mock — and the surplus is refunded.

```bash
qubic-cli -nodeip <your node IP> -nodeport 21841 \
  -seed <your seed> \
  -scheduletick 20 \
  -invokecontractprocedure 4 103 10 "{ 42uint64 }"
```

`42` is the value that should travel all the way to the homepage — pick
something recognisable so you can spot it.

## What success looks like

Allow ~3–5 ticks after the invocation's tick reaches consensus, then check each
hop in order:

1. **OC machine received it** — `docker logs -t oc-machine-node` shows the pair:

```text
MockOcService: wrote value 42 (invocationId <id>) to mock_oc_sink.txt
MockOcService: forwarded invocationId <id> to ocmock.qubic.org:443 (HTTP 200)
```

2. **Value landed on disk** — `42` appears in `oc-machine/docker/data/mock_oc_sink.txt`.
3. **Service accepted it** — https://ocmock.qubic.org/ lists the invocation, with
   a replication count equal to the number of distinct OC machines that reported
   it. Your invocation may well show a count above 1: other operators' machines
   report the same invocation, and that is the system working as intended.

If all three are there, your setup passes.
