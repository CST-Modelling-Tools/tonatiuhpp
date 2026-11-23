# Production C++ lambda policy check

Production C++ sources in `source/` must not introduce lambda expressions under the policy in `AGENT.md`. The scanner is a conservative lexical screen, **not** a compiler AST proof. A candidate makes CI fail and requires manual grammatical review; do not automatically rewrite production C++ to satisfy a false positive.

The CI matrix invokes this check on **every** PR and push, including Markdown-only diffs, before any native build. It uses only Python's standard library. Local equivalents from the repository root:

```powershell
python -m unittest discover -s tests/tooling -p "test_production_lambda_audit.py" -v
python scripts/audit_production_lambdas.py --root .
```

The optional `--expected-head SHA` argument pins an audit to a specific Git revision. A local check without this argument evaluates the current working tree; it does not imply the files are committed. On a nonzero exit, review each candidate against C++ grammar and add a focused regression test for any confirmed false positive. Do not bypass the gate with a file/line allowlist.

The original false positive was `char bytes[sizeof(double)] = {};` (an array declaration), corrected in the tracked scanner. Do not run or regenerate the frozen 500-million-ray scientific benchmark to validate this check.
