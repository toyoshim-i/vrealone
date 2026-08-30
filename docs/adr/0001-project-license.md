# ADR 0001: License First-Party Code Under Apache-2.0

- Status: Accepted
- Date: 2026-08-31

## Context

The driver must link permissively licensed OpenVR, XREAL One transport, and
sensor-fusion components while loading into the proprietary SteamVR runtime.
Some useful reference implementations are GPL-licensed and cannot be copied
into a permissively licensed implementation.

## Decision

All original first-party source, documentation, configuration, and scripts in
this repository are licensed under Apache License 2.0 unless a file is clearly
marked otherwise.

Use `Copyright 2026 The vrealone Authors` for the initial project notice. New
first-party source files should carry:

```text
SPDX-License-Identifier: Apache-2.0
```

The project will use a permissive, independently implemented architecture.
XRLinuxDriver and `psvr2-linux-adapter` remain reference-only. Their source
must not be copied, closely translated, or adapted into this repository.

OpenVR, `xreal_one_driver`, Fusion, and `byteorder` may be direct or transitive
dependencies after their exact revisions and notices are recorded in
`THIRD_PARTY.yml` and release notices.

Contributions are accepted under Apache-2.0 by default as described in section
5 of the license. A DCO or CLA may be added before accepting contributions from
outside the initial authors; no relicensing right is implied.

## Consequences

- The driver may be used and redistributed in open-source or commercial
  environments subject to Apache-2.0 and third-party notice obligations.
- The project receives Apache-2.0's explicit contributor patent grant.
- GPL implementation code cannot enter the permissive codebase without a new
  license decision and compatibility review.
- Every dependency and imported file needs revision-level provenance and a
  release-time license audit.
