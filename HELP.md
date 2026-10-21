# LGL SCX Scheduler Manager Help

Use the **Control** tab to select a scheduler and mode. **Start** activates the
selection, **Switch** replaces the running scheduler, and **Stop** returns the
system to its normal scheduler. Administrative actions show a Polkit prompt.

The **Status** tab refreshes every five seconds. **Reference** describes the
schedulers, **Flags** lists common custom arguments, and **Log** records command
results. A missing `scxctl` installation opens the **Setup** tab.

## Shortcuts

- **Ctrl+Q** - quit
- **Ctrl+Shift+M** - switch between application and system title bars
- **F1** - help
- **F5** - refresh status
- **F10** - open the main menu

Closing the window hides it when a compatible StatusNotifierItem tray host is
available. Otherwise, closing the window quits the application.
