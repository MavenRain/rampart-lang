# rampart-lang

rampart-lang is a Cubical TT language that proves attack properties of the
telcoin-network p2p layer at a pinned commit: OOM, DoS, privilege
escalation, arbitrage, eclipse, ban evasion and stale records. Its base is
a minimal span of structs, impls and free fns from three telcoin-network
paths. Each base name cites a file and a line range, and a gate row hashes
each cited span. The checker refuses any program that can have a runtime
error. It comes from lang-template with the tcc-wasm host.

Status: S3 DONE (the kernel, the total Wasm writer, `Wide` and the trap row).
The plan is `docs/PLAN.md`.

## When to reach for rampart-lang (DRAFT)

| Use rampart-lang when | Use something else when |
|---|---|
| You state an attack property of the telcoin-network p2p layer (network-libp2p, primary and worker `src/network`) at a pinned SHA. | The property is about consensus safety of the Bullshark DAG outside the network modules. |
| The property is a resource bound: memory, work per attacker byte, reserved slots or permits. | The bug is in the EVM or in execution. |
| The property is about roles: no low-role peer changes high-role state. | The network is another libp2p network. The base is specific to telcoin-network. |
| The property is a no-gain rule: no attacker cycle gains score, permits or slots. | You need crypto soundness. Signatures and hashes are assumptions here. |
| You must know when an upstream change makes a proof stale (span hashes). | You need a working exploit. There is no Rust PoC target. |

## Files

| Path | What |
|---|---|
| `design/DESIGN.md` | Sources, threat model, model shape, attack classes, laws, span pins, the request. |
| `docs/PLAN.md` | Readings, route, base draft, slices, open questions. |
| `AGENTS.md` | Notes for coding agents. |
| `SPEC.md` | The specification. |
| `formers/FORMERS.md` | The type formers. `formers/tcc-wasm.md` gives their status on the tcc-wasm host. |
| `probe/CAPABILITY.md` | The host probe. |
| `docs/STATUS.md`, `docs/VALIDATION.md` | The status and the gate results. |
| `docs/host/README.md` | How to build and test the host kit. |
| `src/`, `test/` | The language and the kit. |

## Build and test

`make check` runs the gate.

## License

MIT OR Apache-2.0. See `LICENSE-MIT` and `LICENSE-APACHE`.
