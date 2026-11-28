# Security Policy

## Reporting a Vulnerability

Please **do not** report security vulnerabilities through public GitHub issues, pull requests, or discussions.

Instead, submit a private report through GitHub Security Advisories:

**https://github.com/qubic/core/security/advisories**

Click **"Report a vulnerability"** to open a private report. Only you and the Qubic core team can see it, so the team can review and fix the issue before it is disclosed publicly.

Please include as much of the following as you can:

- A description of the vulnerability and its impact
- The affected component, file(s), and version or commit
- Steps to reproduce, or a proof of concept
- Any suggested fix or mitigation

## Scope and Bounties

**In scope (eligible for bounty):** the Qubic Core protocol code in this repository, such as consensus, networking, tick processing, scoring/mining, spectrum/assets, cryptography, and the smart contract engine (`src/contract_core/`, `src/contracts/qpi.h`).

**Not eligible for bounty:** the individual smart contracts in `src/contracts/` (for example `Qx.h`, `Qearn.h`, `QVAULT.h`). These contracts are written and maintained by community developers, and the Qubic team does not offer bounties for vulnerabilities in them. You can still report such issues through Security Advisories so they can be forwarded to the responsible developers.
