# Agent Working Rules

## General principles

- Optimize for minimal token consumption.
- Keep responses concise and focused.
- Work in milestone mode, not micro-step mode.
- Prefer the fewest meaningful commits that preserve clarity.
- Batch related changes together instead of splitting into many tiny steps.

## Development cadence

- Alternate meaningful user-facing functionality milestones with focused architecture and engineering-quality milestones. Do not start a succession of micro-refactors or defensive checks without an identified risk, defect, or measurable benefit.
- Every architecture milestone should have a concrete rationale, bounded scope, and behavior-preserving validation; then return to product functionality work.

- Before switching development focus, complete validation and **publish a new accepted release**. A failed or pending release leaves the current cycle open; never ship just to advance the cadence.
- Compare every release candidate against the **previous published stable version** with controlled reduced-ray same-machine paired runs, scientific-output checks and a documented stability assessment. Inconclusive results remain unresolved rather than passing; investigate confirmed regressions.
- Follow `docs/release-cycle-checklist.md` for release evidence. Publication and documented performance exceptions require explicit maintainer authorization. For each release, explicitly run a new 500-million-ray candidate simulation as a manual scientific release gate, without regenerating or changing the immutable historical 500M reference. Preserve exact-grid equivalence for unchanged physics; for reviewed intentional physics changes, independently audit outputs, compare power/peak tolerances, document cellwise distribution differences and obtain scientific approval. This full-scale run is separate from routine tests and reduced-ray performance comparisons.

## Engineering discipline

- Avoid cosmetic refactors, renames, or wording changes unless they improve:
  - correctness
  - maintainability
  - clarity
- Only propose further edits if they materially improve:
  - correctness
  - robustness
  - cross-platform behavior
  - CI stability
  - release readiness

## C++ lambda policy

- Eliminate C++ lambda expressions from all Tonatiuh++ production C++ code under `source/`, including the scientific kernel, application, plugins, and libraries. The target is zero production-code lambdas; tests are exempt as long as they pass.
- Prefer named free functions, named member functions, function pointers, or explicit named function objects when equivalent functionality is needed.
- Introduce or retain a lambda only when controlled, reproducible benchmarks clearly demonstrate a **substantial performance improvement** over a functionally equivalent lambda-free alternative. Convenience, brevity, stylistic preference, or unverified assumptions about compiler optimization are not sufficient exceptions.
- Complete the removal of existing production-code lambdas in small, independently validated milestones rather than a risky single rewrite. Confirm zero remaining lambdas with a repository-wide production-source audit, while preserving scientific correctness, deterministic behavior, and performance where relevant.
- Do not duplicate scientific algorithms merely to eliminate lambdas. Preserve a single authoritative propagation and physics implementation.

## Architecture guidelines

- Keep clear separation between core logic and interface layers.
- Do not duplicate domain logic across modules.
- Prefer simple, maintainable designs over over-engineering.

## Workflow behavior

- Before starting Tonatiuh++ tasks, read `PROJECT_STATUS.md` for current project and release context.
- Keep durable working rules in this file; keep time-sensitive release context in `PROJECT_STATUS.md`.
- Keep progress updates brief.
- Keep final summaries concise.
- When a feature is mostly complete, stop incremental tweaks and switch to a completion review.
- Ask for confirmation only when there is a real tradeoff or product decision.

## Local-first delivery and documentation currency

- Perform implementation, debugging and relevant Windows Release builds/tests in the maintainer's local checkout before committing or pushing changes to GitHub. Review the diff and run focused regressions plus the applicable full local CTest suite; use hosted CI only as subsequent cross-platform integration verification, not as the first testing environment.
- When direct access to the maintainer's machine is unavailable, provide a reviewable patch and local commands rather than committing untested edits remotely. Wait for the maintainer's local validation evidence and explicit authorization before committing, pushing or creating a PR. Never merge, tag or release without the maintainer's explicit approval.
- Keep maintained documentation current as part of **every merged change**, not only releases or large features. Update the affected architecture, testing, user/developer documentation and `PROJECT_STATUS.md` in the same reviewed milestone; check for stale current-state claims, counts and instructions before merge. Do not postpone documentation until the next feature.
- Distinguish local test evidence from hosted CI, proposed PRs from merged code, and current instructions from dated historical checkpoints. After each merge, verify that documentation reflects the merged revision and record the verified merge SHA and CI evidence in `PROJECT_STATUS.md` when available; complete any separately approved post-merge status update before declaring the handoff complete.

## Output requirements

For each task:
- Provide a concise summary.
- List modified files.
- Provide a standardized, detailed commit message using Conventional Commits 1.0.0.
- Format commit messages as `<type>[optional scope]: <description>`, followed by an optional blank line and detailed body paragraphs when useful.
- Use conventional types such as `feat`, `fix`, `docs`, `refactor`, `test`, `build`, `ci`, `perf`, `chore`, or `revert`; add `!` and a `BREAKING CHANGE:` footer when the change is breaking.
- Keep the subject concise and imperative. Use the body to explain what changed, why it changed, important validation, and any compatibility or packaging impact.
- Use CR/LF only to end the subject line or a paragraph; do not split one sentence or paragraph across multiple lines for formatting.
- Do not print full file contents unless explicitly requested.

## Validation

When relevant:
- Run linting and syntax checks.
- Perform minimal functional validation if applicable.
