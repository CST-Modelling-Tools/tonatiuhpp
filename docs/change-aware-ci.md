# Continuous integration (current behavior)

The workflow `.github/workflows/ci.yml` checks pull requests into `master`, pushes to `master`, and manually started runs. Its three jobs use Ubuntu, macOS, and Windows.

- **Only Markdown files added, modified, or deleted:** each CI job checks the changed documentation for valid UTF-8 and unresolved merge markers, and runs the CI change-classifier unit tests. The expensive native build and CTests are skipped.
- **Any non-Markdown change:** all three jobs run the configured native build and tests. This includes changes to C++, CMake, Python, workflow YAML, scientific fixtures, benchmark reference files, and the benchmark JSON schema.
- **Manually started workflow:** runs the complete native CI suite, even when the last commit changed only Markdown.
- **Unknown or malformed change set:** falls back to full native validation or fails closed, rather than silently skipping relevant tests.

The three existing job names remain `build (ubuntu-24.04)`, `build (macos-latest)`, and `build (windows-latest)` so required-status checks retain their identity. Full native CI does not automatically run a new 500-million-ray benchmark.

Test the classifier locally without building Tonatiuh++:

```powershell
python -m unittest discover -s tests/ci -p test_ci_change_classifier.py -v
```

If Markdown files ever become inputs to application code generation or native tests, revise `scripts/ci_change_classifier.py` before relying on the Markdown-only shortcut. The separate release workflow is not covered by these rules.
