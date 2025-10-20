Short, focused instructions to help an AI coding agent be productive in SynchronousAudioRouter.

High-level summary
------------------
- This repository implements Synchronous Audio Router (SAR): a Windows kernel-mode virtual audio driver plus supporting user-mode tools and a small web UI.
- Major components:
  - Kernel driver (SynchronousAudioRouter/): KS/WaveRT driver and core logic (entry.cpp, device.cpp, wavert.cpp, sar.h).
    - Responsibilities: create control interface, manage endpoints, allocate shared buffer sections, expose KS filters/pins.
  - User-mode configuration tool / ASIO helper (SarAsio/): configuration, registry mapping, and helper utilities (`config.cpp`, `tinyasio.*`).
  - Installer / UI / Web (SarInstaller/, SarWeb/): WiX installer scripts and static web UI for the configuration dialog.
  - Examples and tools (Examples/, nm_ping/, RioPing/): helpful sample projects and test utilities.

Why the structure matters
------------------------
- The driver is split between a control interface (IRP-based ioctls) and KS filter factories that get created on-demand. Changes to buffer layout or endpoints go through the control IRPs (see `sar.h` and `entry.cpp`).
- Endpoint lifetime, shared buffer mapping and the handle/notification queue are central: inspect `SarControlContext`, `SarEndpoint` and SarHandleQueue implementations in `sar.h` and `entry.cpp`/`wavert.cpp` to understand concurrency and IRQL constraints.
- Many operations require PASSIVE_LEVEL (e.g., calls to `KsFilterFactoryGetSymbolicLink`, registry operations). When modifying code, follow the locking and work-item patterns used in `SarProcessPendingEndpoints` (queue work items, drop mutexes before KS calls).

Key files to read first (quick tour)
----------------------------------
- `SynchronousAudioRouter/entry.cpp` — DriverEntry, IRP handlers (create/close/device_control) and control ioctls (SAR_SET_BUFFER_LAYOUT, SAR_CREATE_ENDPOINT).
- `SynchronousAudioRouter/wavert.cpp` — WaveRT / real-time pin helpers; buffer mapping and RT APIs (SarKsPinRtGetBufferCore etc.). Good for understanding the buffer mapping & notification flows.
- `SynchronousAudioRouter/sar.h` — Core data structures (SarControlContext, SarEndpoint, ioctls and constants). Use this as the canonical reference for types and limits (e.g., SAR_MAX_BUFFER_SIZE, SAR_BUFFER_CELL_SIZE).
- `SarAsio/config.cpp` — User-mode configuration parsing and saving (picojson). Shows the JSON config format used by SarConfig/GUI.
- `README.md` — Project goals, how SAR is used (DAW, ASIO, test signing notes). Useful for understanding expected runtime environment.

Build / test / debug notes (developer workflows)
----------------------------------------------
- Solution file: `SynchronousAudioRouter.sln` contains multiple projects (driver and user-mode). Use Visual Studio (Windows) to build the kernel driver projects.
- Kernel driver specifics:
  - Requires Windows driver build environment (WDK). Build with Visual Studio + WDK matching target OS.
  - Test-signed driver note: prerelease builds are unsigned; enable `testsigning` for local testing (see `README.md`).
  - Many debug messages are printed using DbgPrintEx macros (SAR_INFO/SAR_DEBUG). Use WinDbg or kernel debugger to view them.
- Running locally:
  - Install via the WiX project in `SarInstaller/` or use the installer project outputs. For quick dev, build and use test-signing then load with `pnputil`/`devcon` or install from the WiX output.
  - To exercise endpoints: start your DAW (ASIO host) and create endpoints via the SAR configuration UI or the asio config helper.
- User-mode tools:
  - `SarAsio` reads/writes JSON config files (see `DriverConfig::fromFile` and `writeFile` in `SarAsio/config.cpp`). Useful example config keys: `driverClsid`, `endpoints`, `applications`.

Project-specific conventions and patterns
--------------------------------------
- IRQL and locking:
  - Kernel objects use `FAST_MUTEX` around control context and endpoint lists. Work-items are used to perform operations that require PASSIVE_LEVEL (see `SarProcessPendingEndpoints`). Follow their pattern: acquire short locks, extract items, release, then call KS APIs.
- Buffer management:
  - Shared audio buffers are created as named sections (`ZwCreateSection`) in `SarSetBufferLayout` and mapped into processes. Buffers are partitioned into fixed-sized cells (`SAR_BUFFER_CELL_SIZE`) and tracked using an RTL bitmap. When allocating views, code maps specific cells using `ZwMapViewOfSection`.
- Endpoint IDs on Windows 10:
  - The code mangles endpoint IDs with sample rate/size/channel information for Windows 10 compatibility (`deviceIdMangled` in `wavert.cpp`). When changing format-related behavior, update the mangling logic.
- Registry hooking and redirecting:
  - The driver registers a callback to redirect certain registry lookups (for MMDeviceEnumerator compatibility). See `SarRegistryCallback` and `SarAddAllRegistryRedirects` in `entry.cpp`.

Integration points & external dependencies
-----------------------------------------
- Kernel APIs: KS (Kernel Streaming), WaveRT, Zw* / Ke* calls. Changes must compile with WDK headers and be IRQL-correct.
- ASIO: user-mode SarAsio and the ASIO driver interface are integration points — the DAW will host the ASIO driver.
- Installer: WiX scripts under `SarInstaller/` produce the final installer. If changing device interface registrations, update WiX metadata.

Change guidance / quick examples
------------------------------
- Adding a new control IOCTL:
  - Add new CTL_CODE in `sar.h`, handle it in `SarIrpDeviceControl` in `entry.cpp`. Follow existing patterns for reading user buffers (`SarReadUserBuffer`) and completing the IRP; set STATUS_PENDING only when you actually queue an IRP for later completion.
- Creating endpoints from user-mode config (example): `SarAsio/config.cpp` serializes `DriverConfig` and `EndpointConfig`. To add a config field, update the JSON load/save functions and ensure the driver-side parameter is consumed when creating endpoints.
- Fixing a race: if you need to call KS APIs that require PASSIVE_LEVEL, mirror `SarProcessPendingEndpoints` — move work into an IoQueueWorkItem and ensure you drop FAST_MUTEX before calling KS functions.

Limits, magic numbers and constants to respect
---------------------------------------------
- `SAR_BUFFER_CELL_SIZE` (65536) — cell size used for buffer partitioning (see `sar.h` and `wavert.cpp`).
- `SAR_MAX_BUFFER_SIZE` and sample rate/size bounds in `sar.h` — validation is performed in `SarSetBufferLayout`.
- `MAX_ENDPOINT_NAME_LENGTH` — used when writing endpoint names/IDs; ensure strings are NUL-terminated.

Testing hints
-------------
- Use REAPER or any ASIO host: select SAR ASIO driver and create channels. The driver will register device interfaces only after the KS filter factories are created.
- For kernel debugging, enable test-signing and use WinDbg to capture DbgPrint output. Many trace macros use function name prefixes making it easy to filter logs.

What I couldn't infer
---------------------
- Exact WDK/Visual Studio versions targeted are not declared in repo files. Recommend asking the maintainer which WDK/VS combo they use for CI/builds.
- Installer signing and CI details are external to repository (AppVeyor status badge exists in the README but no CI config files found in repo root).

If anything here looks wrong or you want a different focus (tests, refactor suggestions, or wiring up small unit tests), tell me which area to expand and I will iterate.
