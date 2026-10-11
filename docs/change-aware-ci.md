# Change-aware CI policy

The `.github/workflows/ci.yml` workflow runs for pull requests to `master`, pushes to `master`, and manual dispatch. **Do not add workflow-level `paths-ignore` to skip documentation-only runs:** a required check might never be reported and block merging.

## Classification and checks

All three existing job names remain unchanged: `build (ubuntu-24.04)`, `build (macos-latest)`, and `build (windows-latest)`. Every job checks out history, sets up Python, classifies the changed paths, and runs the small classifier unit tests.

- **Only `.md` files changed:** run a lightweight document check (UTF-8 decoding and unresolved merge-marker detection) on each of the three platforms. Skip Qt, Boost/Eigen/Coin dependencies, native builds, CTest, benchmark integrity tests, and build artifact uploads. Deleted Markdown files count as documentation changes. This also covers `PROJECT_STATUS.md` and Markdown release notes.
- **Any non-Markdown path changed:** run the original complete CI matrix, including scientific tests. This includes C++ files, CMake and packaging inputs, scientific reference/scene/fixture files, `.rst` help sources, scripts, YAML workflows, and mixed code-plus-documentation changes.
- **Manual `workflow_dispatch`:** always run complete CI, regardless of the commit's changed paths.
- **Cannot determine a diff / empty change set / unknown event:** default to complete CI. Invalid changed-path input makes the classifier fail rather than silently skipping work.

On pull requests, the workflow uses the merge-base-to-PR-head difference (`git diff BASE...HEAD`); on pushes, it uses the `before`-to-`after` difference. Diff results are NUL-delimited and rename detection is disabled, so a code-to-Markdown rename still triggers the full build. Full history is fetched for reliable revision resolution.

## Branch protection and scope

The **matrix job names and status-check identity are preserved** even for documentation-only changes. Instead of skipping an entire workflow or job, the heavy steps inside each job are skipped; the document check and classifier tests still have to pass. If GitHub branch protection requires these existing `build (...)` checks, they remain present.

This reduces heavy builds on documentation PRs without weakening checks on code, science or workflow changes. It does not alter the independent `Release` workflow. The CI implementation itself modifies non-Markdown files, so its first PR necessarily runs **full CI**. Exercise the lightweight path on the next Markdown-only PR before relying on it as an established hosted-run result.

To test the classifier locally, without Qt or native compilation:

```console
python -m unittest discover -s tests/ci -p test_ci_change_classifier.py -v
```

If an `.md` file ever becomes an input to production code generation or native test behavior, narrow the documentation-only allowlist in `scripts/ci_change_classifier.py` before changing that input.
