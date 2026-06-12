# Tonatiuh++ Runtime Help

The Tonatiuh++ GUI opens installed documentation from:

```text
help/html/index.html
```

The source for that runtime help is the Sphinx project in `help/Sphinx`.
Generated HTML under `help/html` is build output and should not be committed.

Build the runtime help from the repository root with:

```bash
python scripts/build_runtime_help.py
```

On Windows, use `py` if that is how Python is installed:

```powershell
py scripts/build_runtime_help.py
```

The release workflow builds this HTML before packaging so the installed
application contains `help/html/index.html`. GitHub Pages is intentionally not
used for this documentation because the existing Pages root serves Qt IFW update
metadata.
