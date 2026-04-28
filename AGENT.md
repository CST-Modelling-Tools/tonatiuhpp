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

## Architecture guidelines

- Keep clear separation between core logic and interface layers.
- Do not duplicate domain logic across modules.
- Prefer simple, maintainable designs over over-engineering.

## Workflow behavior

- Keep progress updates brief.
- Keep final summaries concise.
- When a feature is mostly complete, stop incremental tweaks and switch to a completion review.
- Ask for confirmation only when there is a real tradeoff or product decision.

## Output requirements

For each task:
- Provide a concise summary.
- List modified files.
- Provide an standardize and detailed commit message.
- Provide commit messages with real lines and paragraphs: a subject line, optional blank line, and optional body paragraphs when useful.
- Use CR/LF only to end the subject line or a paragraph; do not split one sentence or paragraph across multiple lines for formatting.
- Do not print full file contents unless explicitly requested.

## Validation

When relevant:
- Run linting and syntax checks.
- Perform minimal functional validation if applicable.
