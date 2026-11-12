# rampart-lang notes for agents (DRAFT)

rampart-lang is at S0: the kit and the plan. The base names, the Mealy
steps and the laws of `design/DESIGN.md` are not built yet.

## When to use it

Use it to state and prove attack properties of the telcoin-network p2p
layer at a pinned SHA. The table in `README.md` says when to use something
else.

## Build and test

- Gate: `make check` in the repo root. Run it serially when you keep the
  log.
- Tests are in C (host) and in rampart-lang (guest). Do not add a node,
  Python or shell test harness.

## Rules for changes

- `design/DESIGN.md` section 11 is verbatim. Do not edit it.
- Each base name cites `path:start-end` in telcoin-network at the pin. Do
  not add a base name without a citation.
- Read telcoin-network only at the pin, for example with
  `git show <pin>:<path>`. Do not copy Rust source into this repo.
- A question that is not ruled stays open in `docs/PLAN.md` section 1.2.
- Do not put a wallet address, a facilitator URL or a deploy secret in the
  repo.

## Tools for agents (planned)

- `langc check --json`: refusal codes, fix hints, progress counts (S12).
- `langc fmt`: the canonical printer (S12).
- `langc pins verify` and `langc pins bump`: span drift against the pin
  (S5).
- `langc mcp`: an MCP server over stdio for authoring (S16).
- `langc serve`: the same tools over HTTP behind x402 (S17).
- `SKILL.md`: graded examples (S16).

## Funding

GitHub Sponsors is on for this repo. `funding.json` and `llms.txt` come in
S18.
