# Third-Party Notices

The build fetches reviewed upstream revisions rather than copying their source
into this repository. Binary distributions must include this file and the
corresponding license texts under `LICENSES/`.

| Component | Revision or version | License | Use |
| --- | --- | --- | --- |
| OpenVR | `0924064316de3effbcd1acf1e309182a2deb1c05` (2.15.6) | BSD-3-Clause | Driver API header |
| xreal_one_driver | `3e912315ec16e94e9a47688770d649cfef293624` | MIT | Statically linked sensor parser |
| byteorder | 1.5.0, checksum `1fd0f2584146f6f2ef48085050886acf353beff7305ebd1ae69500e27c67f64b` | MIT OR Unlicense; MIT selected | Rust parser dependency |
| Fusion | `9325424011892abacc0ce42b8bb1a8ae20264b9b` (1.3.3) | MIT | Planned AHRS implementation |

`xreal_one_driver` is built as a Rust `staticlib`. Rust standard-library and
runtime portions may therefore enter the resulting binary under Apache-2.0 OR
MIT and component-specific exceptions. The exact release artifact must be
inventoried and its applicable Rust notices added before distribution. Current
probe binaries are development artifacts, not release packages.

The GPL-licensed projects listed as `reference_only` in `THIRD_PARTY.yml` are
not dependencies and no source from them is included in this project.
