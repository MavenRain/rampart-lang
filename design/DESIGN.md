# rampart-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
rampart-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---
This design has no pasted source. The USER gave the ask and the pasted
block of section 11. The source of the base is the telcoin-network code at
a pinned commit. Each section below is a reading by Claude. Each reading is
a question in `docs/PLAN.md` section 1.2 until the USER rules it.

## 1. Sources

- telcoin-network at origin/main `6167184c0` (2026-11-11). The license is
  MIT OR Apache-2.0. This repo stores citations and span hashes. It does not
  copy Rust source.
- Three paths, and only these:

| Path | Non-test lines (about) | Main files |
|---|---|---|
| `crates/network-libp2p` | 22k | `peers/`, `admission/`, `source_admission/`, `sync/`, `stream/`, `kad.rs`, `codec.rs`, `serve_permits.rs`, `retention.rs`, `freshness.rs`, `consensus.rs` |
| `crates/consensus/primary/src/network` | 9.3k | `mod.rs`, `handler.rs`, `message.rs`, `gap_policy.rs`, `sync_codec.rs` |
| `crates/consensus/worker/src/network` | 6.5k | `mod.rs`, `handle.rs`, `handler.rs`, `message.rs`, `stream_codec.rs`, `primary.rs`, `error.rs` |

- Test files show intent only. A test file is never a base span.
- The model covers the public behavior at the pin. A bump of the pin is a
  slice of its own (section 7).

## 2. Threat model

### 2.1 Environment (Q-S0-5)

The libp2p Swarm, the tokio runtime and the transport are the environment.
The model does not run them. An event trace is the schedule that the
adversary picks. Each admitted async fn is a sequence of atomic steps. A
step boundary is an await point that the cited span shows. A race inside one
step is out of scope (EXTERNAL, law X2). A bounded mpsc channel is a bounded
queue value. Its capacity is a cited constant.

### 2.2 Adversary (Q-S0-6)

- The adversary controls the order of events.
- It controls the bytes of the messages of up to `f` peers, within the
  transport frame cap.
- It can make any number of fresh PeerIds (Sybil). The admission rules of
  the span still apply to each one.
- It cannot forge a BLS or an ed25519 signature (law S1, ASSUMED). It cannot
  find a hash collision (law H1, ASSUMED).
- `Params` holds the committee size `n`, with `f < n/3`.
- For law D3 only, the event order meets a window premise: in each window
  of `w` events, each ready handler gets a step (Q-S1-2 (b)). The
  adversary keeps all other order control. `Params` holds `w` and the
  response bound `t` of law D3.

### 2.3 Time (Q-S0-7)

Logical time is the trace index. A Rust `Duration` or `Instant` (a ban
duration, a heartbeat, a freshness window, a retention period) reads as a
count of `Tick` events from the environment. Each duration is a tick count
from a cited constant. The language has no wall clock.

## 3. Model shape (Q-S0-2)

- R1 admits only cell monads and their duals, as Cubical TT represents
  them.
- The event trace is codata: `Stream Event`, the cofree side, the dual of
  the cell monad. The sibling source is leaf-lang (codata `Stream`, sealed
  codata, coinduction).
- Each modeled impl is a Mealy step `S -> Event -> S x Out`, a coalgebra.
- The state is carried in a state cell monad. `St S A := S -> Prod S A`
  and the step `Mealy S E O` are defs in `domain/domain.lang`, not kernel
  formers (Q-S4-1 (a)).
- `Verdict`, `Admit` and `Outcome` are sum types, not formers.
- A former outside this table gives `REFUSE_FORMER`.

## 4. Attack classes (Q-S0-10 to Q-S0-14)

Each class is a property shape. Each has one example program in the S0
corpus draft (`docs/PLAN.md` section 12). Each example names the cited
mechanism that it checks.

| Class | Law | Shape |
|---|---|---|
| OOM | O1 | For each trace, the memory measure stays at or below a cap. The measure counts bytes held in queues, caches, pending-request maps, peer tables and kad records. The cap depends only on `Params` (cited limits). It never depends on the trace length or on the attacker volume. |
| DoS, amplification | D1 | Honest work units (bytes sent, decode steps, store reads) are at most `a * attacker bytes + b`, per peer and in total. |
| DoS, reserved capacity | D2 | Committee peers keep `k` inbound slots or permits under any flood. |
| DoS, bounded response | D3 | In each trace that meets the window premise of sec 2.2, a request from an honest committee peer that the span admits gets its response or an explicit refusal `Outcome` within `t` step events. `t` and `w` depend only on `Params`. The premise is part of the law. That the runtime scheduler meets it is EXTERNAL (law X3). Q-S0-11 (b), Q-S1-1 to Q-S1-3. |
| Privilege escalation | P1 | A role lattice comes from the span, for example `unknown < known < committee < trusted < self`. No event from a peer at role `r` changes state that the span reserves for a role above `r` (non-interference on the high projection). This covers ban lifting, score reset, slot reservation and gossip topic authority. |
| Arbitrage, resource | A1 | No cycle of attacker actions gives a net gain in a scored resource (peer score, permits, slots, bandwidth credit, retention priority) at no net cost. This is the no-positive-cycle rule of exchange rates. |
| Arbitrage, information | A2 | The lead (in trace steps) that one peer gets over another, when it learns a batch or a certificate by gossip fan-out, has a bound. FOR USER ATTENTION (Q-S0-13). |
| Eclipse | E1 | The attacker cannot fill all the peer slots. |
| Ban evasion | E2 | A fresh PeerId after a ban gets no more than the budget of a new peer. |
| Stale or replayed records | E3 | A record outside the freshness window, or a replayed record, is a `reject`, never a state change (`freshness.rs`, `kad.rs`, `record_exchange.rs`). |

Liveness is in v1 as law D3, a bounded response (Q-S0-11 (b), Q-S1-1
(a)). A bound in steps is a safety property over finite prefixes, so the
induction of section 6.1 proves it. D3 covers honest committee peers
only (Q-S1-3 (a)). The window premise is in the law, not in ASSUMED
(Q-S1-2 (b)). There is no law X4.

## 5. No runtime error (R0, Q-S0-1)

- A rejected frame, a full queue, an exhausted permit, a banned peer, a
  stale record, a score under a threshold and a decode error are values of
  sum types (`Verdict`, `Admit`, `Outcome`). None is a trap.
- A Rust panic site inside a cited span (`unwrap`, `expect`, a slice index,
  a debug overflow) is an explicit `Outcome` branch in the model. If a trace
  reaches that branch, the T2 report lists it as a FINDING. It is not a trap
  of the module.
- Host input that does not decode is a `reject`, never a trap.

## 6. Laws and numbers

### 6.1 Law discharge (Q-S0-8, FOR USER ATTENTION)

A property over all traces needs induction. Each law of section 4 is an
inductive invariant over the Mealy step. Its proof is a core term (the
eliminator of the trace, or its coinduction principle). The kernel checks
it. Evaluation decides a closed arithmetic goal. A later slice adds a linear
integer arithmetic procedure that emits a core certificate. The kernel
checks that certificate again, so no oracle is trusted. `check --json`
counts the laws discharged. Bounded checking to depth `k` is a
counterexample search only. It feeds the T2 report. It is not a proof.

### 6.2 Numbers (Q-S0-9)

- Integers only. `u64` and `usize` are bounded naturals with the cited
  width.
- Each Rust `saturating_*` and `checked_*` op maps to one total op (port of
  gavel-lang 86c2f30).
- `crates/network-libp2p/src/peers/score.rs` uses `f64` at the pin. The
  model reads each score as a fixed-point integer at a cited scale. The
  rounding goes against the property under proof. For example, a law on the
  ban threshold rounds toward the attacker. This is a sound
  over-approximation. The gap to IEEE `f64` is EXTERNAL (law X1).

## 7. Span pins and the report (Q-S0-16, Q-S0-17)

- `pins/spans.tsv` has one row per base name: `name path start end sha256`.
  The hash is the sha256 of the exact bytes of the line range at the pin.
- `langc pins verify --tn DIR` reads each file with
  `git -C DIR show <pin>:<path>` through `popen`. It prints SAME or CHANGED
  for each row.
- The gate row runs when `RAMPART_TN` names a checkout that has the pin
  object. If it does not, the row prints `pins: SKIPPED (no RAMPART_TN)`. A
  silent pass is not allowed.
- `langc pins bump NEWSHA` lists the CHANGED spans and finds each item again
  by name. A human or an agent confirms each new span.
- The T2 JSON report gives, for each property: the name, the class, the
  verdict (PROVED, REFUTED with a trace, OPEN), the resource bound, the law
  counts by bucket, the cited spans with SAME or CHANGED, and a schema
  version. It is also the property section of `check --json`.

## 8. The base (R2, Q-S0-3, Q-S0-4, Q-S0-15)

- A name enters the base only if (i) it is a struct, an impl method on such
  a struct, a free fn or a cited constant inside the three paths at the
  pin, (ii) it cites `path:start-end`, (iii) at least one example program
  uses it, and (iv) no admitted op is a composition of other admitted ops.
- "Minimal" is relative to the example corpus. It is not a theorem. A
  minimality table lists each op with the example that needs it.
- Foreign types in a signature (`PeerId`, `Multiaddr`, `BlsPublicKey`,
  `Certificate`, `Batch`, `Header`, `Round`, `Epoch`, `Duration`,
  `Instant`, channel types) are opaque carriers. A carrier has no op except
  the decidable equality that the admitted span uses. Each carrier cites its
  first use inside the three paths and counts as a span name.
- `docs/PLAN.md` section 13 gives the base draft: 40 names keyed to
  the examples E1 to E9. It is a draft for S1 and S6.

## 9. Four C programs

TinyCC builds each one. Keep them separate.

| Program | Job |
|---|---|
| `langc` | The compiler: parse, check, eval, lower, Wasm. `langc pins` (verify, bump) is a subcommand, not a fifth program. |
| `langc mcp` | An authoring MCP server over stdio for agents that WRITE rampart-lang: check, fmt, explain, list-base, show-law, show-span. |
| `langc serve` | The same tools over HTTP behind x402, plus MCP streamable HTTP at `/mcp`. It links libcurl for the facilitator verify and settle calls. It runs behind a TLS proxy. It does not start without a USER facilitator URL and payTo address. |
| `rampartd` | The T1 host. It runs a compiled module on an event trace and reports verdicts. |

## 10. Not in v1

- Consensus safety of the Bullshark DAG outside the network modules.
- EVM or execution bugs.
- Other libp2p networks. The base is specific to telcoin-network.
- Crypto soundness (laws S1, H1 are ASSUMED).
- A working exploit. No Rust PoC target was chosen.
- Races below the await grain (law X2).
- Liveness for a peer that is not a committee peer (Q-S1-3 (a)).
- Rust PoC export, Lean 4 export and SMT-LIB 2 export (not chosen).

## 11. The request (USER, 2026-11-12, verbatim)

Copied from `rampart-lang-work/USER-ASK.md` sections 1 and 2. Two changes:
trailing white space is removed, and the en dash in `$0.01-$0.20` is a
hyphen (the kit gate refuses en dashes). The curly quotes, the leading spaces
and the spelling stay.

### 11.1 The ask, verbatim

I want to write a new programming language for proving properties about DoS, OOM, privilege escalation, arbitrage, and similar attacks on telcoin-network, with the following characteristics:

The language should not allow anything to compile that will lead to a runtime error, period.

As for type constructors/higher-kinded types, admit only cell monads and their duals as computationally represented in cubical type theory.

As for base types and operations, admit only a minimal span of structs, their associated impls, and free functions, from the following crates in telcoin-network: network-libp2p, consensus/primary/src/network, and consensus/worker/src/network

The host language shall be TinyCC.  Use your judgment to surface multiple-choice option(s) for compilation target(s).

Finally, I want agents to reach for this language when it fits their use case(s).  To that end, admit the following additional criteria:

### 11.2 The pasted block, verbatim

```
The plan for making agents reach for the language includes:
  - one canonical form per operation and a langc fmt canonical printer;
  - langc check --json, giving fix hints and progress counts (parsed, typed, laws
    discharged);
  - a SKILL.md with 3 to 5 graded examples;
  - an MCP server written in TinyCC (langc mcp);
  - AGENTS.md and a README table saying when to use the language;
  - a token-count comparison against Python and Rust.
  - List where agents look. That means an MCP server entry in the public
     registries, an OpenAPI description, and the price in the 402 response.


1. Make the work agent-consumable and payable (highest leverage)
* Wrap existing formalizations or verification workflows as MCP tools or simple x402-gated endpoints. Examples: “verify this Halo2 gadget against the formalization,” “search formalized properties of this circuit,” “check this snippet for known invariants,” or “request a formalization sketch of gadget X.”
* Price per use in the $0.01-$0.20 range (or higher for heavier verification). Agents (or their principals) will use tools that improve success rate or reduce verification cost.
* Publish Agent Skills (SKILL.md with name + description frontmatter) that teach agents how and when to use your libraries or tools. Skills are an open standard (originally from Anthropic, now under the Agentic AI Foundation) and load progressively, so discovery is cheap.
2. Publish machine-readable discovery and funding signals
* Add FUNDING.yml plus a simple funding.json (or equivalent) to GitHub repos.
* Put an llms.txt (and, where relevant, .well-known/ai-donate.json or similar) on any site or content property that describes the projects, transparency, and explicit welcome for agent payments.
* Include AGENTS.md or Skills in the repos so coding agents that encounter your code can surface funding options.
Agents parse these files the way humans read a README or Sponsors page.
3. Use agent-native funding rails for larger efforts
* Run specific formalization milestones on AgentFund (or similar) with escrow and clear deliverables. Agents can discover, evaluate, contribute, and vote.
* Keep human channels (GitHub Sponsors, Open Collective, Patreon) as the primary near-term path; treat agent micropayments and campaigns as complementary. Better tools that agents actually use also generate recommendations and visibility that drive human sponsors.
4. Frame the value in agent terms
Position the work as shared infrastructure for agentic systems that need verifiable correctness: formalized payment or agent protocols, ZK compliance layers, verified gadgets that other agents can rely on. Agents (and the people who fund them) respond to clear ROI on reliability and reduced verification cost more than to general appeals.
Practical starting sequence
1. Add machine-readable funding metadata (FUNDING.yml, llms.txt, funding manifest) to the main repos.
2. Expose one useful Lean/ZK capability as an MCP tool or lightweight x402 endpoint (Cloudflare Workers + facilitator, or a simple Express + x402 middleware, are common starting points).
3. Publish a short Skill that documents how to use it.
4. List the endpoint/tool in relevant MCP or x402 discovery surfaces and mention it in content that reaches agent developers.
5. Measure actual agent traffic and payments before investing heavily in larger campaigns.
```
