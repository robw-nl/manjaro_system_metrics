# manjaro_system_metrics

1. Project Overview
Manjaro System Metrics is a high-efficiency, low-latency dashboard written in C for monitoring system power, thermals, and costs on Manjaro Linux (KDE Plasma).
It consists of two decoupled components:

The Daemon (C Backend): Reads hardware sensors, performs math, and writes atomic JSON files to RAM (/dev/shm).

The Plasmoid (QML Frontend): A lightweight Plasma widget that parses the JSON and displays the UI.

2. Architecture Principles
Decoupled Design: The frontend (QML) and backend (C) are completely independent. They communicate via text files in shared memory (/dev/shm), ensuring zero latency and no process locking.

Metric gathering: performed in a fast lane (1 second) and slow lane (5 second) to ensure minimum resource usage.

Atomic Updates: The daemon writes to a temporary file (.tmp) and renames it (rename()). This guarantees the QML never reads a "half-written" file, preventing UI flickers or crashes.

Minified JSON: The daemon outputs single-line JSON to ensure the Plasma DataEngine reads the entire payload in one event.

Fail-Fast Configuration: The system refuses to guess. If the configuration file is missing or values are undefined, the daemon alerts the user and exits immediately rather than running with incorrect defaults.

3. The Components
File Description

daemon.c
The main engine. Runs in the background, loops indefinitely, manages timing/syncing.

config.c
Loads metrics.conf. Enforces strict validation: if the file is missing, it sends a system notification and kills the process.

sensors.c
Handles low-level Linux file reads (CPU freq, temp, uptime, brightness).

metrics.conf
User-editable configuration. All values must be defined here.

Main.qml
The visual layout. Defines the text, colors, and popup behavior.

4. Configuration (metrics.conf)
Edit this file to tune the system without recompiling. Note: If you delete this file, the daemon will not start.

Hardware Calibration:
mobo_overhead: Power loss from motherboard VRMs (e.g., 0.05 = 5%).
psu_efficiency: Power supply efficiency curve (e.g., 0.85 = 85%).
mon_base / mon_delta: Monitor power consumption (Base + Brightness factor).

Costs:
euro_per_kwh: Cost of electricity (e.g., 0.26).

Thresholds:
limit_mhz_warn: CPU frequency that triggers yellow text.
limit_temp_crit: CPU temp that triggers red text.

5. Installation & Deployment
Standard Update Procedure:
Edit source files.
Run make to compile.
Stop the running daemon (killall daemon).
Copy the new binary to the production folder.
Restart the daemon.

6. Troubleshooting
Problem: The dashboard shows empty -- lines.
Cause: Daemon is not running, or QML cannot find the file.
Fix: Run pidof daemon. Check if /dev/shm/dashboard_panel.txt exists.

Problem: The popup shows "JSON ERROR".
Cause: The daemon is outputting invalid JSON (e.g., unexpected newlines).
Fix: Ensure daemon.c is using the "Minified JSON" format (no \n inside the string).

Problem: Daemon starts but immediately quits.
Cause: metrics.conf is missing or unreadable.
Fix: Check your desktop notifications for a "Startup Failed" error. Verify the config file is in the same folder as the binary.

