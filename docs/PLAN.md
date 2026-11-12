# rampart-lang plan

rampart-lang is a Cubical TT language for one job: to prove attack
properties of the telcoin-network p2p layer at a pinned commit. The classes
are OOM, DoS, privilege escalation, arbitrage, eclipse, ban evasion and
stale records. The base is a minimal span of structs, impls, free fns and
constants from three paths of telcoin-network at `6167184c0`. Each base
name cites `path:start-end`, and a gate row hashes each cited span. A model
is a Mealy step over a codata event trace. A host (`rampartd`) replays a
trace through the compiled module and reports verdicts. No program can have
a runtime error.

The design is `design/DESIGN.md`: the sources, the threat model, the model
shape, the attack classes, the laws, the numbers, the span pins, the four C
programs and the request, verbatim. This file gives the readings of the
request, the route, the slices and the open questions. "design sec N" is
section N of `design/DESIGN.md`. A reading that is not ruled stays a
recommendation.

Status: S1 (rulings): the USER ruled sections 1.2 and 1.3 on 2026-11-12.
Next: S2 (port the kernel, Q-S0-20).

## 1. Rulings and questions

### 1.1 Rulings

| Date | Ruling | Source |
|---|---|---|
| 2026-11-12 | Name: rampart-lang. Repo MavenRain/rampart-lang, private at creation. The USER makes it public. Not chosen: siege-lang, bulwark-lang. | USER, S0 |
| 2026-11-12 | T1 (primary): a Wasm module and a TinyCC C host `rampartd`, from the lang-template `hosts/tcc-wasm` kit (HEAD 5265a2a). One total Wasm module. `rampartd` replays recorded or generated peer, gossip and request traces and reports verdicts. Differential tests under node and wasmtime. | USER, S0 |
| 2026-11-12 | Extra target: T2 JSON report. For each property: the verdict, the resource bound, the law counts and the cited Rust `path:start-end` spans. | USER, S0 |
| 2026-11-12 | Not chosen: native C through TinyCC (a new host kit), Rust test export as primary, Lean 4 export as primary, Rust PoC or regression export, Lean 4 theorem export, SMT-LIB 2 export. | USER, S0 |
| 2026-11-12 | Fidelity: pinned reading and span hashes. Each base type and op cites `path:start-end` at telcoin-network `6167184c0`. A gate row hashes the cited spans. When an upstream bump changes a span, the gate fails until someone reads the span again. Constants are copied with their citations. Not chosen: tool-extracted constants, no drift check. | USER, S0 |
| 2026-11-12 | x402 route: `langc serve`, an HTTP server in C that tcc builds. It links libcurl for the facilitator verify and settle calls and runs behind a TLS proxy. Same tools as `langc mcp`, plus MCP streamable HTTP at `/mcp`. The repo ships `server.json`, `openapi.yaml`, `llms.txt` and `funding.json`. Stated as the default in the question; the USER did not change it. | USER, S0 |
| 2026-11-12 | Host: TinyCC. `langc`, `langc mcp`, `langc serve` and `rampartd` are C99 that TinyCC builds. | USER, S0 |
| 2026-11-12 | Each deploy, each registry submission, each AgentFund campaign and the payTo wallet are USER steps. The repo holds no wallet address that Claude made up. | USER, S0 |
| 2026-11-12 | Q-S0-1 (a): A failed check is a value of a sum type (`Verdict`, `Admit`, `Outcome`). A Rust panic site in a cited span is an explicit `Outcome` branch. A trace that reaches it is a FINDING in the T2 report, not a trap (pontoon 9ab466c:39). | USER, S1 |
| 2026-11-12 | Q-S0-2 (a): The event trace is codata (`Stream Event`, the dual of the cell monad). Each modeled impl is a Mealy step `S -> Event -> S x Out`. The state is in a state cell monad. No other former. | USER, S1 |
| 2026-11-12 | Q-S0-3 (a): The four-part rule of design sec 8: (i) inside the three paths, (ii) cites `path:start-end`, (iii) one example uses it, (iv) no admitted op is a composition of other admitted ops. A minimality table and a gate row in S21. "Minimal" is relative to the example corpus. It is not a theorem. | USER, S1 |
| 2026-11-12 | Q-S0-4 (a): Opaque carriers with no op except the decidable equality that the span uses. Each cites its first use in the three paths. | USER, S1 |
| 2026-11-12 | Q-S0-5 (a): The Swarm, tokio and the transport are the environment. Each async fn is atomic steps between the cited await points. A race below that grain is EXTERNAL (law X2). A bounded channel is a bounded queue value with a cited capacity. | USER, S1 |
| 2026-11-12 | Q-S0-6 (a): It controls the event order, the bytes of up to `f` peers within the frame cap, and any number of fresh PeerIds under the span's admission rules. It cannot forge a signature or break a hash. `n` and `f < n/3` are in `Params`. | USER, S1 |
| 2026-11-12 | Q-S0-7 (a): Logical time is the trace index. Each Rust `Duration` or `Instant` is a count of `Tick` events from a cited constant. No wall clock. | USER, S1 |
| 2026-11-12 | Q-S0-8 (a): Each law is an inductive invariant over the Mealy step. Its proof is a core term that the kernel checks. Evaluation decides closed goals. A later slice (S11) adds a linear integer arithmetic procedure that emits a core certificate, which the kernel checks again. No trusted oracle. The USER saw the FOR USER ATTENTION note in the question. | USER, S1 |
| 2026-11-12 | Q-S0-9 (a): Integers only. `u64` and `usize` are bounded naturals. Each `saturating_*` and `checked_*` op maps to one total op (gavel 86c2f30). The `f64` of `peers/score.rs` is a fixed-point integer at a cited scale, rounded against the property under proof. The gap to IEEE `f64` is EXTERNAL (law X1). | USER, S1 |
| 2026-11-12 | Q-S0-10 (a): The memory measure stays at or below a cap that depends only on `Params`, for each trace. | USER, S1 |
| 2026-11-12 | Q-S0-11 (b): The same, and liveness (an honest request is served within `k` steps) in v1. "The same" is the text of (a) up to "committee peers". Law X4 leaves EXTERNAL; section 1.3 gives its form (law D3). Edits: PLAN sections 1.3, 6, 12, 14 (S8), 15 (OI-13); design secs 2.2, 4, 10. | USER, S1 |
| 2026-11-12 | Q-S0-12 (a): The role lattice comes from the span (for example `unknown < known < committee < trusted < self`). Non-interference on the high projection. | USER, S1 |
| 2026-11-12 | Q-S0-13 (a): Resource arbitrage (no positive cycle in a scored resource) and information arbitrage (a bound on the gossip lead for a batch or a certificate). The USER saw the FOR USER ATTENTION note in the question. | USER, S1 |
| 2026-11-12 | Q-S0-14 (a): Eclipse, ban evasion, stale or replayed records. | USER, S1 |
| 2026-11-12 | Q-S0-15 (a): The draft table of section 13 is the starting list for S6. S6 confirms each span. | USER, S1 |
| 2026-11-12 | Q-S0-16 (a): One row per name with the sha256 of the exact line range. `langc pins verify` prints SAME or CHANGED. The gate row prints `pins: SKIPPED (no RAMPART_TN)` when it cannot run. `langc pins bump NEWSHA` finds each item again by name. | USER, S1 |
| 2026-11-12 | Q-S0-17 (a): One schema, with a version. It is also the property section of `check --json`. | USER, S1 |
| 2026-11-12 | Q-S0-18 (a): A line-based text format in `test/traces/`, one event per line, that `rampartd` reads. | USER, S1 |
| 2026-11-12 | Q-S0-19 (a): The draft in `README.md`. | USER, S1 |
| 2026-11-12 | Q-S0-20 (a): pontoon-lang a82331d: the Cubical TT kernel and the total Wasm writer, a port of barter-lang df78228 and a8f0f52 (pontoon 9ab466c:64). | USER, S1 |
| 2026-11-12 | Q-S0-21 (a): Its own C99 Wasm interpreter; node and wasmtime are oracles in the gate (pontoon 9ab466c:65). | USER, S1 |
| 2026-11-12 | Q-S0-22 (a): S3 changes it: R0 makes a trap a failure on each target (pontoon 9ab466c:80; barter a8f0f52). | USER, S1 |
| 2026-11-12 | Q-S0-23 (a): The four buckets; `laws_discharged` counts DISCHARGED rows only (pontoon 9ab466c:78). | USER, S1 |
| 2026-11-12 | Q-S0-24 (a): The ten codes of section 7. | USER, S1 |
| 2026-11-12 | Q-S0-25 (a): JSON POST routes for the tools and MCP streamable HTTP at `/mcp`, each behind x402 (pontoon 9ab466c:67). | USER, S1 |
| 2026-11-12 | Q-S0-26 (a): Four: check a property (with its T2 report), explain a refusal, list the base span with citations, report span drift against a newer telcoin-network SHA. | USER, S1 |
| 2026-11-12 | Q-S0-27 (a): Tiered: $0.01 for fmt, explain, list-base, show-law and show-span; $0.05 for a property check with its T2 report; $0.20 for a drift report (pontoon 9ab466c:68 shape). | USER, S1 |
| 2026-11-12 | Q-S0-28 (a): USDC on Base mainnet; base-sepolia in tests (pontoon 9ab466c:69). | USER, S1 |
| 2026-11-12 | Q-S0-29 (a): A `packages` entry and a `remotes` entry, name `io.github.MavenRain/rampart-lang` (pontoon 9ab466c:70). | USER, S1 |
| 2026-11-12 | Q-S0-30 (a): OpenAPI 3.1; each route lists its 402 response with the x402 body schema and the price (pontoon 9ab466c:71). | USER, S1 |
| 2026-11-12 | Q-S0-31 (a): `.github/FUNDING.yml`, `funding.json`, `llms.txt`; no `.well-known/ai-donate.json` (pontoon 9ab466c:72). | USER, S1 |
| 2026-11-12 | Q-S0-32 (a): `langc serve` writes a local count log; nothing leaves the host (pontoon 9ab466c:73). | USER, S1 |
| 2026-11-12 | Q-S0-33 (a): This plan lists candidate milestones only; the USER runs any campaign (pontoon 9ab466c:74). | USER, S1 |
| 2026-11-12 | Q-S0-34 (a): The README gets the section "What an agent gets" in ROI terms (pontoon 9ab466c:75). | USER, S1 |
| 2026-11-12 | Q-S0-35 (a): A local C stub facilitator and a public test-network row in the gate; the gate fails when the network or a test input is absent (pontoon 9ab466c:76). | USER, S1 |
| 2026-11-12 | Q-S0-36 (a): count_tokens on programs with equal outputs (pontoon 9ab466c:77). | USER, S1 |
| 2026-11-12 | Q-S1-1 (a): X4 becomes law D3 in DISCHARGED: in each trace that meets the premise of Q-S1-2, an admitted request from an honest peer (scope Q-S1-3) gets its response or an explicit refusal Outcome within k step events; k depends only on Params. Kernel-checked inductive invariant, S11 LIA certificate. Counts: DISCHARGED 9 to 10, EXTERNAL 4 to 3. Law D3 names the bound `t`. Edits: PLAN sections 1.3, 6, 12, 14 (S8), 15 (OI-13); design secs 2.2, 4, 10. | USER, S1 |
| 2026-11-12 | Q-S1-2 (b): Same window premise inside the D3 statement, but no ASSUMED row: the scheduler gap joins law X3 (transport and runtime internals, EXTERNAL). ASSUMED stays 2. The window premise (from (a)): in each window of `w` events (`w` in `Params`), the environment delivers a step to each ready handler; the adversary keeps all other order control. | USER, S1 |
| 2026-11-12 | Q-S1-3 (a): Only requests from honest committee peers, which have reserved capacity under law D2. Unknown and known non-committee peers have no liveness claim in v1. | USER, S1 |

The seed is `bin/new-lang.sh rampart-lang tcc-wasm` from lang-template
5265a2a. The kit gives the parser, the checker, the evaluator, the lowering
and the Wasm writer. No kit emits a C trace host, a JSON report, a span pin
checker, an MCP server or an HTTP server, so these are new code.

Carried default, not ruled for rampart-lang: proofs are guest-language
terms only (quorum-lang ruling, 2026-10-25). Lean export is not a target.

### 1.2 Questions for the USER (S0)

S1 ruled each item on 2026-11-12. Section 1.1 has one row per
ruling.

Option (a) is the recommendation. Where a pontoon-lang S1 ruling answers
the same question, option (a) is that ruling, cited as `pontoon
9ab466c:<line>` (pontoon-lang `docs/PLAN.md` at 9ab466c), unless the item
says why not. Two items are FOR USER ATTENTION: Q-S0-8 and Q-S0-13.

Readings of the request:

- Q-S0-1 R0 failures (section 2, design sec 5). (a) A failed check is a
  value of a sum type (`Verdict`, `Admit`, `Outcome`). A Rust panic site in
  a cited span is an explicit `Outcome` branch. A trace that reaches it is
  a FINDING in the T2 report, not a trap (pontoon 9ab466c:39). (b) Leave
  panic sites out of the model, and refuse a span that has one.
  RULED: (a).
- Q-S0-2 R1 formers (section 3, design sec 3). (a) The event trace is
  codata (`Stream Event`, the dual of the cell monad). Each modeled impl is
  a Mealy step `S -> Event -> S x Out`. The state is in a state cell monad.
  No other former. (b) A list trace (finite traces only) and no codata.
  RULED: (a).
- Q-S0-3 R2 minimal span (section 4, design sec 8). (a) The four-part rule
  of design sec 8: (i) inside the three paths, (ii) cites `path:start-end`,
  (iii) one example uses it, (iv) no admitted op is a composition of other
  admitted ops. A minimality table and a gate row in S21. "Minimal" is
  relative to the example corpus. It is not a theorem. (b) The one-way
  rule only: (i) to (iii).
  RULED: (a).
- Q-S0-4 Foreign types (design sec 8). (a) Opaque carriers with no op
  except the decidable equality that the span uses. Each cites its first
  use in the three paths. (b) Refused: refuse each item whose signature
  names a foreign type. Nearly every handler takes a `PeerId`. (c) Admit
  foreign types with their own ops. This breaks "only".
  RULED: (a).
- Q-S0-5 Async and the Swarm (design sec 2.1). (a) The Swarm, tokio and the
  transport are the environment. Each async fn is atomic steps between the
  cited await points. A race below that grain is EXTERNAL (law X2). A
  bounded channel is a bounded queue value with a cited capacity. (b)
  Refused in v1: model interleavings inside a step.
  RULED: (a).
- Q-S0-6 Adversary (design sec 2.2). (a) It controls the event order, the
  bytes of up to `f` peers within the frame cap, and any number of fresh
  PeerIds under the span's admission rules. It cannot forge a signature or
  break a hash. `n` and `f < n/3` are in `Params`. (b) A variant that the
  USER names.
  RULED: (a).
- Q-S0-7 Time (section 5). (a) Logical time is the trace index. Each Rust
  `Duration` or `Instant` is a count of `Tick` events from a cited
  constant. No wall clock. (b) A clock value in each event.
  RULED: (a).
- Q-S0-8 Law discharge. FOR USER ATTENTION. A property over all traces
  needs induction. (a) Each law is an inductive invariant over the Mealy
  step. Its proof is a core term that the kernel checks. Evaluation decides
  closed goals. A later slice (S11) adds a linear integer arithmetic
  procedure that emits a core certificate, which the kernel checks again.
  No trusted oracle. (b) A trusted Presburger oracle in C. Its laws go to
  ASSUMED. (c) Refused as the main mode: bounded checking to depth `k`. It
  stays as a counterexample search that feeds T2.
  RULED: (a).
- Q-S0-9 Numbers (design sec 6.2). (a) Integers only. `u64` and `usize`
  are bounded naturals. Each `saturating_*` and `checked_*` op maps to one
  total op (gavel 86c2f30). The `f64` of `peers/score.rs` is a fixed-point
  integer at a cited scale, rounded against the property under proof. The
  gap to IEEE `f64` is EXTERNAL (law X1). (b) Refused: exact rationals (no
  static size bound). (c) Refused: model `f64` (R0 and exactness).
  RULED: (a).

The attack classes (design sec 4):

- Q-S0-10 OOM (law O1). (a) The memory measure stays at or below a cap
  that depends only on `Params`, for each trace. (b) A cap per structure
  only, with no total.
  RULED: (a).
- Q-S0-11 DoS (laws D1, D2). (a) Amplification `work <= a * attacker bytes
  + b`, per peer and in total, and reserved capacity for committee peers.
  Liveness is not in v1 (law X4, EXTERNAL). (b) The same, and liveness (an
  honest request is served within `k` steps) in v1.
  RULED: (b). Section 1.3 gives the form of the liveness law.
- Q-S0-12 Privilege escalation (law P1). (a) The role lattice comes from
  the span (for example `unknown < known < committee < trusted < self`).
  Non-interference on the high projection. (b) A lattice that the USER
  names.
  RULED: (a).
- Q-S0-13 Arbitrage (laws A1, A2). FOR USER ATTENTION: the least settled
  class. (a) Resource arbitrage (no positive cycle in a scored resource)
  and information arbitrage (a bound on the gossip lead for a batch or a
  certificate). (b) Resource arbitrage only in v1. (c) A reading that the
  USER names.
  RULED: (a).
- Q-S0-14 Similar attacks, v1 set (laws E1, E2, E3). (a) Eclipse, ban
  evasion, stale or replayed records. (b) The four named classes only.
  RULED: (a).

The base and its pins:

- Q-S0-15 Base draft (section 13). (a) The draft table of section 13 is
  the starting list for S6. S6 confirms each span. (b) A smaller list that
  the USER names.
  RULED: (a).
- Q-S0-16 Span pins (design sec 7). (a) One row per name with the sha256
  of the exact line range. `langc pins verify` prints SAME or CHANGED. The
  gate row prints `pins: SKIPPED (no RAMPART_TN)` when it cannot run.
  `langc pins bump NEWSHA` finds each item again by name. (b) Hash whole
  files. Any upstream edit then fails the gate.
  RULED: (a).
- Q-S0-17 T2 report (design sec 7). (a) One schema, with a version. It is
  also the property section of `check --json`. (b) Two schemas.
  RULED: (a).
- Q-S0-18 Trace format (S14). (a) A line-based text format in
  `test/traces/`, one event per line, that `rampartd` reads. (b) JSON
  lines.
  RULED: (a).
- Q-S0-19 When-to table (README draft). (a) The draft in `README.md`. (b)
  A narrower scope that the USER names.
  RULED: (a).

The route:

- Q-S0-20 Kernel source (section 8.1). (a) pontoon-lang a82331d: the
  Cubical TT kernel and the total Wasm writer, a port of barter-lang
  df78228 and a8f0f52 (pontoon 9ab466c:64). (b) barter-lang a8f0f52
  directly.
  RULED: (a).
- Q-S0-21 `rampartd` (section 8.3). (a) Its own C99 Wasm interpreter; node
  and wasmtime are oracles in the gate (pontoon 9ab466c:65). (b) Link a
  Wasm runtime library.
  RULED: (a).
- Q-S0-22 The kit row "a trap is equal to a trap" (section 2). (a) S3
  changes it: R0 makes a trap a failure on each target (pontoon
  9ab466c:80; barter a8f0f52). (b) Keep the row until S13 (T1).
  RULED: (a).
- Q-S0-23 Law buckets (section 6). (a) The four buckets; `laws_discharged`
  counts DISCHARGED rows only (pontoon 9ab466c:78). (b) Three buckets.
  RULED: (a).
- Q-S0-24 Refusal codes (section 7). (a) The ten codes of section 7. (b)
  One generic code with a text.
  RULED: (a).

Agent adoption and x402 (section 9; option (a) ports pontoon 9ab466c):

- Q-S0-25 `langc serve` routes. (a) JSON POST routes for the tools and MCP
  streamable HTTP at `/mcp`, each behind x402 (pontoon 9ab466c:67). (b)
  `/mcp` only.
  RULED: (a).
- Q-S0-26 Paid tools. (a) Four: check a property (with its T2 report),
  explain a refusal, list the base span with citations, report span drift
  against a newer telcoin-network SHA. (b) The first three only. The drift
  tool needs a telcoin-network checkout on the server (OI-7).
  RULED: (a).
- Q-S0-27 Prices. (a) Tiered: $0.01 for fmt, explain, list-base,
  show-law and show-span; $0.05 for a property check with its T2 report;
  $0.20 for a drift report (pontoon 9ab466c:68 shape). (b) $0.01 flat.
  RULED: (a).
- Q-S0-28 Payment network. (a) USDC on Base mainnet; base-sepolia in tests
  (pontoon 9ab466c:69). (b) A network that the USER names.
  RULED: (a).
- Q-S0-29 `server.json`. (a) A `packages` entry and a `remotes` entry,
  name `io.github.MavenRain/rampart-lang` (pontoon 9ab466c:70). (b)
  `packages` only.
  RULED: (a).
- Q-S0-30 `openapi.yaml`. (a) OpenAPI 3.1; each route lists its 402
  response with the x402 body schema and the price (pontoon 9ab466c:71).
  (b) OpenAPI 3.0.
  RULED: (a).
- Q-S0-31 Funding files. (a) `.github/FUNDING.yml`, `funding.json`,
  `llms.txt`; no `.well-known/ai-donate.json` (pontoon 9ab466c:72). (b)
  Also `.well-known/ai-donate.json`.
  RULED: (a).
- Q-S0-32 Measurement. (a) `langc serve` writes a local count log; nothing
  leaves the host (pontoon 9ab466c:73). (b) No count log.
  RULED: (a).
- Q-S0-33 AgentFund. (a) This plan lists candidate milestones only; the
  USER runs any campaign (pontoon 9ab466c:74). (b) No milestones.
  RULED: (a).
- Q-S0-34 Agent framing. (a) The README gets the section "What an agent
  gets" in ROI terms (pontoon 9ab466c:75). (b) No such section.
  RULED: (a).
- Q-S0-35 x402 test network. (a) A local C stub facilitator and a public
  test-network row in the gate; the gate fails when the network or a test
  input is absent (pontoon 9ab466c:76). (b) The local stub only.
  RULED: (a).
- Q-S0-36 Token comparison. (a) count_tokens on programs with equal
  outputs (pontoon 9ab466c:77). (b) A word count.
  RULED: (a).

### 1.3 Questions for the USER (S1)

Q-S0-11 (b) puts liveness in v1. These items give the form of the
liveness law. The bound `k` of Q-S0-11 (b) is `t` in law D3, because
law D2 uses `k`.

- Q-S1-1 Liveness form and bucket (raised by Q-S0-11 (b); law X4 leaves
  EXTERNAL). A bounded response is a safety property over finite prefixes,
  so the Q-S0-8 (a) induction can prove it. (a) X4 becomes law D3 in
  DISCHARGED: in each trace that meets the premise of Q-S1-2, an admitted
  request from an honest peer (scope Q-S1-3) gets its response or an
  explicit refusal `Outcome` within `t` step events; `t` depends only on
  `Params`. (b) The same statement AT TARGET: `rampartd` replays the trace
  corpus and checks the bound, with no proof.
  RULED: (a).
- Q-S1-2 Fairness premise. Under Q-S0-6 (a) the adversary controls the event
  order, so a schedule that never runs the handler breaks any liveness law.
  (a) D3 holds for traces where, in each window of `w` events (`w` in
  `Params`), the environment delivers a step to each ready handler; the
  adversary keeps all other order control. That the tokio runtime meets this
  premise is a stated law F1 in ASSUMED. (b) The same window premise inside
  the D3 statement, but no ASSUMED row: the scheduler gap joins law X3
  (EXTERNAL).
  RULED: (b).
- Q-S1-3 Scope of "honest request". The adversary can make any number of
  fresh PeerIds (Q-S0-6 (a)), so it can fill capacity that unknown peers
  share. (a) Only requests from honest committee peers, which have reserved
  capacity under law D2. (b) Requests from each honest peer, unknown peers
  too; the checker will likely find a counterexample.
  RULED: (a).

## 2. The reading of R0: no runtime error

R0 says that langc accepts no program that can lead to a runtime error.
The reading ports permit-lang, barter-lang and pontoon-lang: every function
is total, and every failure in the domain is a value (Q-S0-1).

- A rejected frame, a full queue, an exhausted permit, a banned peer, a
  stale record, a score under a threshold and a decode error are values of
  `Verdict`, `Admit` or `Outcome`. None is a trap.
- A Rust panic site in a cited span is an `Outcome` branch. The T2 report
  lists a reached branch as a FINDING.
- Arithmetic is total (Q-S0-9). No `f64`.
- Host input that does not decode is a `reject`, never a trap (permit-lang
  `test/notraptool.c` precedent).
- The kit row "a trap is equal to a trap" (`docs/host/README.md:172`)
  changes in S3 (Q-S0-22, OI-4).

## 3. The reading of R1

R1 admits only cell monads and their duals, as Cubical TT represents them
(Q-S0-2).

- `Stream Event` is codata, the cofree side, the dual of the cell monad
  (leaf-lang precedent).
- A model is a Mealy step `S -> Event -> S x Out`. The state is in a state
  cell monad.
- `Verdict`, `Admit` and `Outcome` are sums, not formers.
- A former outside this table gives `REFUSE_FORMER`.

## 4. The base signature (R2)

R2 admits only a minimal span of structs, impls and free fns from the three
paths (Q-S0-3). Foreign types are opaque carriers (Q-S0-4). Section 14
gives the draft. A name outside the pin table gives `REFUSE_BASE`. With
Q-S0-3 (a), a gate row in S21 fails on a name with no use in the example
corpus, and on an op that is a composition of other admitted ops.

## 5. Time

Progress is the trace index (Q-S0-7). A duration is a count of `Tick`
events from a cited constant. There is no wall clock; a program that names
one gives `REFUSE_CLOCK`.

## 6. Laws and obligations

The laws are design sec 4. Buckets (Q-S0-23):

| Bucket | Laws | Count |
|---|---|---|
| DISCHARGED (the checker proves it) | O1, D1, D2, D3, P1, A1, A2, E1, E2, E3 | 10 |
| ASSUMED (a crypto assumption, stated) | S1 (signature unforgeability), H1 (hash collision resistance) | 2 |
| AT TARGET (the target shows it; differential tests) | C1 (the Wasm module agrees with `langc eval` on the trace corpus), C2 (the T2 report agrees with `check --json`) | 2 |
| EXTERNAL (outside langc) | X1 (the gap from fixed point to IEEE `f64`), X2 (races below the await grain), X3 (the transport, the libp2p internals and the runtime scheduler, also that the scheduler meets the window premise of law D3) | 3 |

Q-S0-13 (a) keeps law A2 and Q-S0-14 (a) keeps laws E1 to E3 in v1.
Q-S0-11 (b) puts liveness in v1: law X4 is now law D3 in DISCHARGED, a
bounded response (section 1.3, design sec 4). There is no law X4.

## 7. Refusals

Ten codes (Q-S0-24). `langc check --json` gives each with a fix hint.

| Code | Cause |
|---|---|
| `REFUSE_PARTIAL` | A function that is not total. |
| `REFUSE_FORMER` | A former outside the R1 table. |
| `REFUSE_BASE` | A base name outside the pin table. |
| `REFUSE_FOREIGN` | An op on an opaque carrier other than its equality. |
| `REFUSE_PIN` | A base name whose pin row is CHANGED or absent. |
| `REFUSE_CLOCK` | A wall clock. |
| `REFUSE_FLOAT` | A floating-point type or literal. |
| `REFUSE_BOUND` | An arithmetic bound obligation that is not discharged. |
| `REFUSE_LAW` | A stated DISCHARGED law that the checker cannot prove. |
| `REFUSE_FORGE` | An adversary event that forges a signature or breaks a hash. |

## 8. Route

### 8.1 Kernel

S2 ports the Cubical TT kernel and the total Wasm writer (Q-S0-20). S3
makes arithmetic total (Q-S0-9) and changes the trap row (Q-S0-22).

### 8.2 The two targets

- T1: one Wasm module (the kit writer) for the Mealy steps, run by
  `rampartd` on a trace (Q-S0-21, Q-S0-18).
- T2: a JSON report (Q-S0-17).

### 8.3 Four C programs

`langc`, `langc mcp`, `langc serve` and `rampartd`, as design sec 9. Keep
them separate: `langc mcp` and `langc serve` are for agents that WRITE
rampart-lang; `rampartd` runs a compiled module. `langc pins` is a
subcommand of `langc`.

### 8.4 The x402 route

`langc serve` gives the `langc mcp` tools over HTTP (Q-S0-25) at the
prices of Q-S0-27 on the network of Q-S0-28. A request with no payment gets
a 402 response with the price. `langc serve` calls the facilitator verify
and settle routes through libcurl. The facilitator URL and the payTo
address are USER inputs; `langc serve` does not start without them (OI-5).
The x402 shape follows mandate-lang `docs/VALIDATION.md` "The x402 route
(S8)". Field names are checked against the x402 text in S17 (OI-1).

### 8.5 The demo

A trace where a Sybil set floods inbound connections and sync requests,
and one banned peer comes back under a fresh PeerId. `rampartd` replays it.
The T2 report shows that committee peers keep their slots (D2), the memory
measure stays under its cap (O1), a score cycle gives no gain (A1), and the
returning peer gets only the budget of a new peer (E2).

## 9. Agent adoption

Each item of the pasted block (design sec 11.2) maps to a slice. USER marks
an outward step that the USER does.

| Pasted item | Where |
|---|---|
| One canonical form per operation, `langc fmt` | S12 |
| `langc check --json`, fix hints, progress counts (parsed, typed, laws discharged) | S12 |
| `SKILL.md` with 3 to 5 graded examples | S16 |
| An MCP server in TinyCC (`langc mcp`) | S16 |
| `AGENTS.md` and a README table on when to use the language | S0 DRAFT, S21 final |
| Token-count comparison against Python and Rust | S20 (Q-S0-36) |
| MCP server entry in the public registries | S18 `server.json` (Q-S0-29); submission USER |
| An OpenAPI description | S18 `openapi.yaml` (Q-S0-30) |
| The price in the 402 response | S17 (Q-S0-27) |
| Sec 1: MCP tools and x402 endpoints, price $0.01 to $0.20 | S16, S17 (Q-S0-26) |
| Sec 1: Agent Skills | S16 |
| Sec 2: FUNDING.yml, funding.json, llms.txt, ai-donate.json | S18 (Q-S0-31) |
| Sec 2: AGENTS.md or Skills in the repo | S16, S21 |
| Sec 3: AgentFund milestones | S21 candidates (Q-S0-33); campaign USER |
| Sec 3: human channels (GitHub Sponsors) | Sponsor flag at the seed; channels USER |
| Sec 4: frame the value in agent terms | S21 (Q-S0-34) |
| Start 1: funding metadata | S18 |
| Start 2: one capability as MCP tool or x402 endpoint | S16, S17 |
| Start 3: a short Skill | S16 |
| Start 4: list the tool in discovery surfaces | S18 files; listing USER |
| Start 5: measure agent traffic and payments | S17 count log (Q-S0-32); reading USER |

USER steps: each deploy of `langc serve`, the TLS proxy, each registry
submission, the payTo wallet, the facilitator account, any AgentFund
campaign, and the public flip of the repo.

## 10. Tests

The dev-language rule (USER ruling 2026-10-09) holds. Unit and integration
tests are in C (host) and in rampart-lang (guest). The x402 test uses a C
client against a local `langc serve` and a stub facilitator in C
(Q-S0-35). The trace corpus is in `test/traces/` (S14). No new node, Python
or shell harness. The kit's node differential stays. `make check` is the
gate; logs that are kept come from a serial run.

## 11. Port sources

| Source | What |
|---|---|
| pontoon-lang ac4369e, 9ab466c, a82331d | The S0 shape, the S1 rulings, the kernel and total Wasm writer port. |
| barter-lang a5ae739, df78228, a8f0f52 | The S1 rulings, the kernel, the R0 trap row. |
| gavel-lang 86c2f30 | Total arithmetic. |
| permit-lang | The R0 no-trap reading, `REFUSE_PARTIAL`, `test/notraptool.c`, `langc mcp`, `check --json`, `SKILL.md`, the token comparison. |
| mandate-lang | The x402 shape (`docs/VALIDATION.md` "The x402 route (S8)"). |
| leaf-lang | Codata `Stream`, sealed codata, coinduction. |
| quotient-lang | The trace-monoid reading of a history. |

Overlap check (2026-11-12). No sibling models the telcoin-network p2p
layer. Six siblings name Telcoin Network for another reason:

- lend-lang: names Telcoin Network as an EVM deploy chain (`README.md:10`).
- intent-lang: names it as an EVM deploy chain (`README.md:9`).
- orbit-lang: names it as an EVM deploy chain (`README.md:9`).
- glue-lang: names it as an EVM L1 for a shielded-transfer circuit
  (`design/DESIGN.md:202`).
- lien-lang: names it as an EVM deploy chain (`design/DESIGN.md:73`).
- tranche-lang: names it as an EVM deploy chain (`design/DESIGN.md:277`).

## 12. Examples (S0 corpus draft)

| Id | Example | Laws |
|---|---|---|
| E1 | `oom-bound`: queues, caches, pending maps, peer tables and kad records stay under a `Params` cap | O1 |
| E2 | `dos-amplify`: honest work is at most `a * attacker bytes + b` | D1 |
| E3 | `reserved-slots`: committee peers keep `k` slots or permits under a flood | D2, D3 |
| E4 | `privesc-role`: no event at role `r` changes state above `r` | P1 |
| E5 | `arbitrage-cycle`: no attacker cycle gains score, permits, slots or retention priority | A1 |
| E6 | `gossip-lead`: a bound on the lead to a batch or a certificate | A2 |
| E7 | `eclipse`: the attacker cannot fill all peer slots | E1 |
| E8 | `ban-evasion`: a fresh PeerId after a ban gets the budget of a new peer | E2 |
| E9 | `stale-record`: a stale or replayed record is a `reject` | E3 |

## 13. Base draft (R2, Q-S0-15)

DRAFT, not ruled. Each row cites telcoin-network at `6167184c0`. S6
confirms each span and S5 writes its hash. "Minimal" is relative to the
examples of section 12.

| # | Name | Kind | path:start-end | Examples | Note |
|---|---|---|---|---|---|
| 1 | `Score` | struct | crates/network-libp2p/src/peers/score.rs:18-33 | E4, E5 | `f64` fields (lines 25, 29); fixed point per Q-S0-9 |
| 2 | `Score::apply_penalty` | method | crates/network-libp2p/src/peers/score.rs:69-83 | E5, E8 | penalty grades lower the score |
| 3 | `Score::update_at` | method | crates/network-libp2p/src/peers/score.rs:105-114 | E5, E8 | decay over time; ban lockout |
| 4 | `PenaltyHistory::permits_forgiveness` | method | crates/network-libp2p/src/peers/penalty.rs:98-100 | E5 | forgiveness depends on the penalty history |
| 5 | `PeerPolicy::grant` | method | crates/network-libp2p/src/peers/policy.rs:95-117 | E4 | role grants scoring exemptions |
| 6 | `PeerPolicy::applies` | method | crates/network-libp2p/src/peers/policy.rs:140-143 | E4, E5 | whether a penalty counts |
| 7 | `AllPeers::process_penalty` | method | crates/network-libp2p/src/peers/all_peers.rs:311-354 | E5, E8 | policy-filtered penalty to ban or disconnect |
| 8 | `PeerManager::forgive_temporarily_banned` | method | crates/network-libp2p/src/peers/manager.rs:1823-1831 | E4, E8 | committee members leave the temp-ban cache |
| 9 | `BannedPeers::ip_banned` | method | crates/network-libp2p/src/peers/banned.rs:126-128 | E8 | IP ban by banned-peer count |
| 10 | `BANNED_PEERS_PER_IP_THRESHOLD` | const | crates/network-libp2p/src/peers/banned.rs:21-21 | E8 | 1 |
| 11 | `BannedPeerCache::insert` | method | crates/network-libp2p/src/peers/cache.rs:137-170 | E1, E8 | capped at `max_len` |
| 12 | `PeerManager::peer_limit_reached` | method | crates/network-libp2p/src/peers/manager.rs:1301-1310 | E7 | dial and listen peer limits |
| 13 | `PeerManager::prune_connected_peers` | method | crates/network-libp2p/src/peers/manager.rs:1504-1535 | E5, E7 | prune above the target peer count |
| 14 | `PeerManager::process_peer_exchange` | method | crates/network-libp2p/src/peers/manager.rs:1555-1593 | E1, E7 | discovery set cap |
| 15 | `MAX_PEER_EXCHANGE_ENTRIES` | const | crates/network-libp2p/src/peers/types.rs:155-155 | E1, E7 | 128 |
| 16 | `PublicConnectionShare::reserve` | method | crates/network-libp2p/src/peers/public_share.rs:75-91 | E3, E7 | public share and per-peer ceiling |
| 17 | `MAX_ESTABLISHED_CONNECTIONS` | const | crates/network-libp2p/src/consensus.rs:255-255 | E3, E7 | 128 |
| 18 | `PRIVILEGED_ESTABLISHED_CONNECTION_RESERVE` | const | crates/network-libp2p/src/consensus.rs:267-267 | E3 | 50 |
| 19 | `MAX_ESTABLISHED_CONNECTIONS_PER_PUBLIC_PEER` | const | crates/network-libp2p/src/consensus.rs:276-276 | E3, E7 | 2 |
| 20 | `ReservedPool::try_acquire` | method | crates/network-libp2p/src/sync/admission.rs:119-126 | E3, E4 | committee reserve, then the public pool |
| 21 | `HoldBudget::try_lease` | method | crates/network-libp2p/src/sync/admission.rs:284-292 | E2, E5 | leaky budget per key |
| 22 | `ClassPermits::try_acquire` | method | crates/network-libp2p/src/serve_permits.rs:174-198 | E2, E3 | class semaphore and a per-peer cap |
| 23 | `MAX_PERMITS_PER_PEER` | const | crates/network-libp2p/src/serve_permits.rs:64-64 | E2, E3 | 2 |
| 24 | `admit_and_serve_sync_stream` | fn | crates/consensus/primary/src/network/mod.rs:1144-1237 | E2, E3 | read bound, serve pools, stall charge |
| 25 | `MAX_CONCURRENT_EPOCH_STREAMS` | const | crates/consensus/primary/src/network/mod.rs:78-78 | E2, E3 | 5 |
| 26 | `COMMITTEE_RESERVED_SYNC_STREAMS` | const | crates/consensus/primary/src/network/mod.rs:89-89 | E3 | 1 |
| 27 | `MAX_BATCH_DIGESTS_PER_REQUEST` | const | crates/consensus/worker/src/network/mod.rs:91-91 | E2 | 500 |
| 28 | `decode_message` | fn | crates/network-libp2p/src/codec.rs:32-101 | E1, E2 | size caps before and after decompression |
| 29 | `read_frame` | fn | crates/network-libp2p/src/sync/frame.rs:223-234 | E2 | frame size bound |
| 30 | `MAX_SYNC_REQUEST_FRAME_SIZE` | const | crates/consensus/primary/src/network/mod.rs:322-322 | E1, E2 | 4 MiB |
| 31 | `gossipsub_config` | fn | crates/network-libp2p/src/gossip_config.rs:212-238 | E2, E6 | heartbeat, validation, size caps |
| 32 | `GOSSIP_MAX_TRANSMIT_SIZE` | const | crates/network-libp2p/src/gossip_config.rs:79-79 | E2, E6 | frame bound plus read chunk |
| 33 | `RequestHandler::process_gossip` (worker) | method | crates/consensus/worker/src/network/handler.rs:218-290 | E2, E6 | topic and epoch check, prefetch |
| 34 | `ConsensusNetwork::verify_gossip` | method | crates/network-libp2p/src/consensus.rs:2741-2766 | E4, E6 | size cap, publishers per topic |
| 35 | `Budget::acquire` | method | crates/network-libp2p/src/source_admission/core.rs:162-198 | E7, E8 | process, peer, address and prefix caps |
| 36 | `KadStore::put_with_timestamp` | method | crates/network-libp2p/src/kad.rs:598-661 | E1, E9 | retention gate, value and record caps |
| 37 | `KAD_MAX_STORED_RECORDS` | const | crates/network-libp2p/src/peers/manager.rs:128-128 | E1 | 1024 |
| 38 | `RecordTimestamp::supersedes` | method | crates/network-libp2p/src/freshness.rs:57-76 | E9 | newer signed record wins; future skew clamp |
| 39 | `FUTURE_SKEW` | const | crates/network-libp2p/src/freshness.rs:13-13 | E9 | 300 s |
| 40 | `RecordExchange::allow_request` | method | crates/network-libp2p/src/record_exchange.rs:59-63 | E1, E9 | pending and interval caps |

Coverage notes. E6 has the fewest rows (4). The gossip heartbeat is an
inline literal at `gossip_config.rs:215`, and no mesh constant is inside
the three paths (OI-11). Ban durations come from config, not from
constants in the three paths (OI-12). The longest span is 94 lines
(row 24).

## 14. Slices

| Slice | Work | State |
|---|---|---|
| S0 | Seed from the kit, design, this plan, README and AGENTS drafts | DONE |
| S1 | The USER rules section 1.2 | DONE |
| S2 | Port the kernel and the total Wasm writer (Q-S0-20) | NEXT |
| S3 | Total arithmetic, bounded naturals, fixed point; the trap row (Q-S0-9, Q-S0-22) | |
| S4 | R1 formers: `Stream` codata, the state cell monad, the Mealy step (Q-S0-2) | |
| S5 | Span pins: `pins/spans.tsv`, `langc pins verify`, `langc pins bump`, the gate row (Q-S0-16) | |
| S6 | Base carriers, `Params` and cited constants; confirm the section 13 spans (Q-S0-4, Q-S0-15) | |
| S7 | Peers: score, penalty, bans, the peer table; laws A1, E1, E2, P1 | |
| S8 | Admission and permits: admission, source admission, serve permits, service class, sync admission; laws D1, D2, D3 | |
| S9 | Records: kad, freshness, record exchange, retention; law E3, O1 (records) | |
| S10 | Primary and worker network handlers and codecs; laws O1 (queues), D1 (codecs), A2 | |
| S11 | Law discharge: trace induction, linear integer arithmetic with certificates (Q-S0-8) | |
| S12 | Refusal codes, `check --json`, `langc fmt` | |
| S13 | T1 lowering to one Wasm module | |
| S14 | `rampartd`: interpreter, trace replay, `test/traces/` (Q-S0-18, Q-S0-21) | |
| S15 | T2 JSON report (Q-S0-17) | |
| S16 | `langc mcp` and `SKILL.md` | |
| S17 | `langc serve` and x402 with the C stub facilitator | |
| S18 | `server.json`, `openapi.yaml`, `llms.txt`, `funding.json`, `.github/FUNDING.yml` | |
| S19 | The public test-network row (if Q-S0-35 (a)) | |
| S20 | Token comparison against Python and Rust | |
| S21 | README and AGENTS final, the demo, candidate milestones, the R2 minimality gate row | |
| S22 | Close | |

## 15. Open items

- OI-1 The field names of the x402 402 body and of the facilitator routes.
  Check them against the x402 text in S17. Do not guess.
- OI-2 The `server.json` schema of the MCP Registry. Check it in S18.
- OI-3 The `funding.json` schema. Check it in S18.
- OI-4 The kit row "a trap is equal to a trap". Q-S0-22 (a): S3 changes
  it.
- OI-5 The facilitator URL and the payTo address are USER inputs. No
  default address in the repo.
- OI-6 `doc-check.pl` was not run at S0 (pontoon S0 precedent).
- OI-7 `langc pins verify` and the drift tool need `git` and a
  telcoin-network checkout that has the pin object. A deploy of the drift
  tool is a USER step.
- OI-8 The fixed-point scale for `peers/score.rs`. S7 picks it from the
  cited score ranges and states the rounding direction for each law.
- OI-9 The role lattice of law P1. S7 reads it from the span. If the span
  gives no total order, it is a question for S7.
- OI-10 The base draft is a draft. S6 confirms each span. Rule (iv) of
  Q-S0-3 is checked in S21.
- OI-11 The gossip lead of law A2 depends on the gossipsub mesh
  parameters in the span. If they are not inside the three paths, A2 is a
  question for S10.
- OI-12 Ban durations and the excess-peer reconnection timeout come from
  config, not from constants in the three paths. S6 decides how `Params`
  carries them, with a citation to the config field.
- OI-13 The window `w` and the response bound `t` of law D3 (section 1.3).
  S8 reads them from the cited capacities (reserved slots, queue capacities,
  permits) and puts them in `Params`. If the span gives no bound, it is a
  question for S8.
