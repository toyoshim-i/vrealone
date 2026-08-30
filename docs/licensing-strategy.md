# Licensing and Source-Provenance Strategy

## Purpose

This document defines the licensing boundary that must be established before implementation begins. It is an engineering compliance plan based on the license texts currently present in the upstream repositories; it is not legal advice.

No upstream source may be copied, translated, or adapted into this repository merely because it is publicly visible. Every imported file and linked dependency must have a recorded license, revision, source URL, and required notices.

## Project license decision

The project is licensed under **Apache License 2.0**, as recorded in [ADR 0001](adr/0001-project-license.md). The root `LICENSE` contains the complete terms and `NOTICE` identifies the initial project authors collectively as `The vrealone Authors`.

### Selected path: permissive independent implementation

Use **Apache License 2.0** for newly written project code.

Reasons for this recommendation:

- It permits open-source and commercial use of the driver and does not conflict with loading the driver into the proprietary SteamVR runtime.
- Its explicit contributor patent grant is useful for a hardware-integration project.
- It is compatible with the planned BSD-3-Clause and MIT dependencies.
- It keeps the driver reusable by other Linux XR projects without imposing whole-project copyleft.

MIT would also have been technically compatible and simpler, but it lacks Apache-2.0's explicit patent terms. The selected Apache-2.0 path requires an independently structured driver. GPL-licensed projects may be used to learn public behavior and identify standards, APIs, device IDs, and experimental questions, but their source code must not be copied or closely translated.

### Alternative path: reuse GPL implementation code

If direct reuse of XRLinuxDriver source is a project requirement, the project would need a GPL-compatible licensing and distribution strategy. XRLinuxDriver currently carries GPL version 3, while the inspected `psvr2-linux-adapter` carries GPL version 2. Those grants must not be assumed mutually compatible, and code from both projects must not be combined without verifying an explicit compatible grant or obtaining permission.

This path also deserves legal review before distributing a GPL SteamVR plugin designed to interact with a proprietary runtime. It is not necessary for the proposed implementation and is therefore not recommended.

## Dependency classification

### Permitted direct dependencies for the recommended path

| Component | Observed license | Intended use | Distribution action |
| --- | --- | --- | --- |
| [ValveSoftware/openvr](https://github.com/ValveSoftware/openvr) | BSD-3-Clause | Pin `openvr_driver.h` and the required OpenVR driver library/object code. | Preserve Valve's copyright, conditions, disclaimer, and non-endorsement clause in source and binary documentation. |
| [`xreal_one_driver`](https://github.com/rohitsangwan01/xreal_one_driver) | MIT | Initially link its Rust static library through the published C ABI. | Preserve its copyright and MIT text in source and binary distributions. Record the exact commit and local modifications. |
| [xioTechnologies/Fusion](https://github.com/xioTechnologies/Fusion) | MIT | Compile the AHRS implementation into the driver. | Preserve its copyright and MIT text in source and binary distributions. Keep upstream files clearly marked. |
| [`byteorder`](https://github.com/BurntSushi/byteorder) 1.5.0 | MIT OR Unlicense | Transitive Rust dependency of `xreal_one_driver`. | Elect the MIT option for compliance records and include its MIT notice. Keep the Cargo checksum and locked version. |
| Rust standard library/runtime portions included by `staticlib` | Apache-2.0 OR MIT, with component-specific exceptions possible | Produced by the Rust toolchain when building the static library. | Generate and review the actual release artifact's Rust license inventory; include applicable notices rather than relying only on `Cargo.lock`. |

Direct dependency approval is conditional on a release-time license scan confirming that upstream licensing has not changed at the pinned revision.

### Reference-only projects

| Project | Observed license | Allowed use in this project | Prohibited without changing the licensing plan |
| --- | --- | --- | --- |
| [wheaney/XRLinuxDriver](https://github.com/wheaney/XRLinuxDriver) | GPL-3.0 license text | Run it, compare hardware behavior, inspect logs, verify model IDs and experimental hypotheses, and consult public documentation. | Copying, adapting, or translating its implementation into an Apache-2.0/MIT codebase. |
| [unterschall/psvr2-linux-adapter](https://github.com/unterschall/psvr2-linux-adapter) | GPL-2.0 license text; inspected driver files carry GPL-2.0 identifiers | Use it to validate that EDID-based DRM leasing is a viable public OpenVR design pattern. Implement from Valve's API documentation and our own measurements. | Copying its driver classes, control flow, comments, scripts, or settings into a permissive codebase. |
| [wheaney/OpenVR-xrealAirGlassesHMD](https://github.com/wheaney/OpenVR-xrealAirGlassesHMD) | Unlicense at repository root; dependencies have their own licenses | Compare observable behavior and, only after a file-level provenance audit, consider small reusable portions. | Assuming the root Unlicense covers submodules or upstream-derived files; importing its vendored dependencies without auditing them. |
| [opentrack/opentrack](https://github.com/opentrack/opentrack) | Unlicense at repository level; bundled components may differ | Compare packet interpretation and Fusion configuration. | Importing tracker or bundled third-party source without a file and dependency audit. |

DXMT and all macOS implementation references are out of scope for the Linux driver and must not enter the dependency graph.

## Facts versus copyrighted expression

The implementation may rely on independently verified facts such as:

- TCP endpoint, message boundaries, byte offsets, numeric encoding, device IDs, EDID values, display modes, coordinate systems, and observed timing.
- Public API signatures and behavior required to implement the OpenVR interface.
- Test results produced with owned hardware.

The implementation must not reproduce GPL source structure, function bodies, comments, tests, or distinctive control flow under the label of a reimplementation. Where a fact was first discovered in a reference project, confirm it through packet captures, device output, public specifications, or a permissively licensed implementation when practical, and record that evidence.

This is a provenance discipline, not a claim that every protocol fact is copyrightable or non-copyrightable in every jurisdiction.

## Source intake procedure

Before adding any third-party source or binary:

1. Record project name, canonical URL, exact commit/tag, file paths, SPDX identifier, copyright holders, modifications, link type, and redistribution obligations in `THIRD_PARTY.yml`.
2. Preserve the upstream license and per-file copyright headers.
3. Place vendored license texts under `LICENSES/` using SPDX-style names, and generate a human-readable `THIRD_PARTY_NOTICES.md` for release packages.
4. Keep vendored source in an explicitly named `third_party/` subtree or fetch it reproducibly at build time. Do not paste third-party code into first-party files.
5. Mark local modifications in patches or commits and retain a reproducible path to the unmodified upstream revision.
6. Review submodules and transitive dependencies separately; a parent repository's license does not automatically cover them.
7. Reject dependencies with a missing, ambiguous, non-redistributable, or incompatible license until written permission or a replacement is available.

## Contribution provenance

After the project license is selected:

- Add a Developer Certificate of Origin sign-off policy or an equivalent contributor declaration before accepting external patches.
- Require contributors to identify copied or adapted third-party material in the pull request.
- Prefer SPDX headers such as `SPDX-License-Identifier: Apache-2.0` in first-party source files.
- Do not accept code generated by reading a reference-only GPL implementation and reproducing its structure. Reviews must check provenance as well as functionality.

A CLA is not required for the initial plan unless the project intends to relicense contributions later. If dual licensing or proprietary relicensing may be desired, decide that before accepting contributors.

## Build and release controls

Add these checks before the first binary release:

- `cargo deny` or `cargo about` for the Rust dependency graph and the exact features used.
- REUSE/SPDX linting for repository files and license texts.
- An SBOM for each release artifact, preferably CycloneDX or SPDX JSON.
- A CI allowlist containing only licenses approved by the project.
- A release test that verifies `LICENSE`, `LICENSES/`, `THIRD_PARTY_NOTICES.md`, the SBOM, source offer/source archive where required, and build scripts are present in the package.

Do not let an automated scanner overrule an ambiguous license. Ambiguities remain blocking until manually resolved.

## Current compatibility conclusion

The Linux driver can be implemented under the selected Apache-2.0 project license without importing GPL code, provided required notices are preserved and the Rust release artifact is audited.

The implementation plan changes materially if the project chooses to copy XRLinuxDriver or `psvr2-linux-adapter` code. That decision must be made before source is imported, not repaired after implementation.

## Accepted decision record

ADR 0001 records:

- Chosen project license: Apache-2.0.
- Initial notice: `Copyright 2026 The vrealone Authors`.
- Contributions are accepted under Apache-2.0 section 5; a DCO or CLA may be added before accepting external contributions.
- Confirmation that GPL projects remain reference-only.
- Approved direct-dependency license allowlist.

The root `LICENSE`, `NOTICE`, and initial `THIRD_PARTY.yml` are present. First-party implementation files must use Apache-2.0 SPDX headers, and exact dependency revisions must replace the `null` placeholders before source is imported or binaries are distributed.
