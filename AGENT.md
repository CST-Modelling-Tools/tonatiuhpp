# Agent Working Rules

## General principles

- Optimize for minimal token consumption.
- Keep responses concise and focused.
- Work in milestone mode, not micro-step mode.
- Prefer the fewest meaningful commits that preserve clarity.
- Batch related changes together instead of splitting into many tiny steps.

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
