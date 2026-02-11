# Manjaro KDE Plasma System Metrics Dashboard: a highly optimized system metrics daemon with a very small running footprint.

# The Components

The project is modularized into specialized units to ensure high performance and easy debugging.

# Core Engine

daemon.c: The orchestrator. It manages the main loop, handles the 1s "Fast Lane" (power/freq) and 5s "Slow Lane" (thermals) polling, and enforces the 30s UI heartbeat.

config.c / config.h: The configuration parser. It implements "Fail-Fast" validation—if a required value in metrics.conf is missing or malformed, the daemon notifies the user via system notification and exits immediately.

sensors.c / sensors.h: The hardware abstraction layer. This is where we use pread() on low-level file descriptors to bypass the C library's buffering, ensuring frequency data is never "stale."

# Specialized Logic

power_model.c / power_model.h: The brain of the telemetry. It calculates "Wall Watts" using PSU efficiency curves and includes the "Ghost-Buster" filter to ignore 0W reporting spikes common on the Ryzen 8700G or other compatible CPUs and SoCs..

discovery.c / discovery.h: Automates hardware pathing. It probes /sys/class/hwmon and /sys/class/drm to dynamically find the correct sensors for your specific motherboard and GPU.

json_builder.c / json_builder.h: A lightweight, dependency-free JSON generator. It outputs a minified, single-line payload optimized for the Plasma DataEngine.

# Frontend & Configuration

Main.qml: The Plasma 6 widget. It parses the JSON from /dev/shm and handles the visual state, including the dynamic "Red/Amber/Yellow" color-coding for thresholds.

metrics.conf: The user control center. Contains all hardware-specific calibration values, power loss factors, and UI warning limits.

Makefile: Configured for your specific development environment. Supports three build targets: debug, build, and release with appropriate optimization levels.

# Key Documentation Highlights for Contributors:

Zero-Malloc Principle: The daemon avoids dynamic memory allocation during the main loop to prevent fragmentation and memory leaks over long-term runs.

Topology Awareness: In sensors.c, the daemon reads the CPU sibling lists to distinguish between physical cores and logical threads, ensuring the reported MHz average is a realistic representation of system work.

The "Sync" Logic: In daemon.c, high-priority data is written immediately to RAM, while "Thermal Maturity" and long-term accumulators are synced to the SSD every 5 minutes to protect your hardware.
    
    
# Getting Started for Contributors

We follow a high-discipline, "Short Run" development approach. Code is improved in manageable chunks, followed immediately by assessment, debugging, and verification before proceeding to the next iteration 
Development Environment

The lead developer uses Kate as the primary C IDE with Clang. The project is structured around a smart Makefile generator bash script that handles header discovery automatically.

    Project Root: /home/rob/Files/C/.

    Recommended Kate Shortcuts:

        Ctrl+Meta+D: Build Debug Target.

        Ctrl+Meta+B: Build Normal Target.
        
        Ctrl+Meta+R: Build Release Target.
        
# Coding Standards

No malloc in the Fast Lane: To ensure long-term stability on passive systems, all polling logic must use fixed-size buffers or stack allocation

Topology Awareness: All CPU metrics must distinguish between physical cores and SMT threads to ensure accurate system-wide averages

Low-Level I/O: Use pread() on file descriptors rather than buffered fscanf for frequency-sensitive sysfs files to bypass the glibc cache
    
# How to Contribute

Iterate Small: Do not submit massive architectural shifts. We prefer small, verified updates that maintain the 1.2 MB footprint.

Verify on Hardware: If you are testing on Intel or ARM, please note your hardware specs (e.g., NUC, Raspberry Pi) in the PR.

Document Logic: If you add a new filter (like our "Ghost-Buster" or "Maturity" logic), include comments explaining the math and the hardware behavior it addresses.
