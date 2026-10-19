# Contributing

This is an evidence-first research project. Do not implement the full port or speculative compatibility systems before the executable and platform gates in [the roadmap](docs/roadmap.md) have been reviewed.

## Research contributions

Distinguish **VERIFIED** (direct inspection or reproducible observation), **INFERRED** (reasoned proposal), and **UNKNOWN** (not established). Repository source inspection confirms an implementation exists, not that it works on hardware. Label README performance statements as author reports unless independently reproduced.

For each finding, include the exact commit, file/function or line, method, sanitized result, and limitations. Negative findings are useful. Use Conventional Commits. Public documentation is English.

## Data boundary

Never add original executables, assets, commercial archives, copyrighted music, shader data, generated proprietary code, console keys, credentials, personal paths, saves, or dumps. This applies to branches, pull requests, issues, CI logs and artifacts as well as the main branch. Do not paste proprietary disassembly or source translations into reports.

All game inputs and derived output must remain in a private directory outside the checkout. Do not create symlinks into private data. Use neutral placeholders in public commands. Before sharing diagnostics, inspect them for paths, keys, game bytes and personal data; publish only a concise sanitized summary.

## Validation

Stage only intended public files, run `python3 tools/check_public_tree.py`, inspect the staged diff and file list, and review history before pushing. The check is a limited research-stage allowlist, not a copyright or secret scanner and not a substitute for review. Update it deliberately if the project later gains reviewed original source files. Do not relax it to admit game-derived outputs.

Do not enable public codegen/game-build CI. Do not create a release until there is a reproducible, tested, redistributable artifact and its complete dependency/license obligations have been reviewed.

## Licensing

Original contributions are GPL-3.0-only. Preserve all upstream notices on future reused code. Record exact component, revision, files, license and modifications before importing it. A repository's detected root license is not a complete component audit. Do not copy code from a project whose applicable license is unclear.
