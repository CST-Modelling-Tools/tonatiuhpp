# Updating a complete Tonatiuh++ installation

The `tonatiuhpp-gui` Help > Updates dialog and the `tonatiuhpp-cli` commands
below use **the same** Qt Installer Framework `MaintenanceTool`. The tool
updates the installed Tonatiuh++ package as one unit, including GUI and CLI
executables, shared libraries, official plug-ins, and packaged runtime data.

```text
tonatiuhpp-cli check-updates
tonatiuhpp-cli update
```

`check-updates` checks the official IFW repository; it does not install anything.
Its exit codes are: `0` no updates, `10` update available, `1` update check error
(including missing MaintenanceTool or a 60-second timeout), and `2` invalid
arguments. Parse the exit code, not the human-readable message.

`update` checks for an available update and, only when found, launches the
**interactive** MaintenanceTool. It does not perform an unattended update.
Exit `0` means either no update was available or handoff was launched; it does
**not** certify that installation succeeded. The user must finish the update
in MaintenanceTool and restart the application manually. Exit `1` or `2`
indicates an error. For scripted deployments use the supported IFW maintenance
mechanism administered by the system rather than invoking this UI command.

Never call `update` in a running scientific study: finish jobs and close all
Tonatiuh++ processes before installing, to avoid replacing binaries in use.
No automatic update is performed on startup, `trace-scene`, `benchmark`,
`run-script`, or Python automation. The GUI continues to check for available
updates and asks for user approval before launching the updater.

These commands only work from an official installed Qt IFW package containing
MaintenanceTool. They do not update extracted ZIP/tar distributions, Python
virtual environments, or build-tree executables. No separate updater exists
for the CLI.

## Release acceptance (not covered by build-tree tests)

1. Install the previous official version using each supported IFW installer.
2. Confirm the GUI sees the new version in Help > Updates and the installed
   CLI reports `check-updates` as available (exit 10) when the new online
   repository is deployed.
3. Invoke `tonatiuhpp-cli update` from a desktop terminal; complete the IFW
   transaction after closing all Tonatiuh++ executables.
4. Verify the installed GUI, CLI, scientific plugins, runtime libraries and
   resources all reflect the new version; verify each executable starts.
5. Run a bounded CLI scene-validation/trace and a bounded GUI smoke test;
   check MaintenanceTool's uninstall and repair/update metadata.
6. Confirm failure modes (no MaintenanceTool, repository unreachable,
   malformed output, nonzero MaintenanceTool exit, and cancellation) never
   claim the installation is updated.

The immutable 500-million-ray scientific benchmark must not be run or changed
as part of this acceptance.
