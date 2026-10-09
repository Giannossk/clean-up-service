# Clean-Up Service

A lightweight, robust Windows Service written in **C++17** that automatically performs temporary file cleanup and disk cleanup on every system boot.

---

## Features

- **Boot Automation**: Configured with `SERVICE_AUTO_START` to automatically run every time Windows starts up.
- **`DeleteTempFiles`**:
  - Recursively purges Windows system temporary folders (`C:\Windows\Temp`).
  - Clears Windows Update cached download files (`C:\Windows\SoftwareDistribution\Download`).
  - Cleans user temporary directories (`C:\Users\<User>\AppData\Local\Temp`).
  - Empties the Windows Recycle Bin (`SHEmptyRecycleBin`).
  - Gracefully handles locked files using `std::filesystem` and error codes.
- **`DiskCleanup`**:
  - Spawns Windows Disk Cleanup tool (`cleanmgr.exe /AUTOCLEAN`) silently in the background and monitors completion.
- **Logging**:
  - Windows Application Event Log (`CleanUpService`).
  - Local log file (`%TEMP%\CleanUpService.log`).
  - Console output for interactive execution.

---

## Project Structure

```text
clean-up-service/
├── CMakeLists.txt         # C++17 CMake configuration
├── include/
│   ├── cleanup.hpp        # DeleteTempFiles & DiskCleanup declarations
│   └── service.hpp        # Windows Service Control Manager interfaces
├── src/
│   ├── cleanup.cpp        # File deletion & cleanmgr process implementation
│   ├── service.cpp        # Service lifecycle & auto-start configuration
│   └── main.cpp           # CLI and SCM entry point
└── README.md
```

---

## Building the Project

### Prerequisites
- Windows 10 / 11 or Windows Server
- Visual Studio 2019 / 2022 / 2026 (with C++ Desktop Development)
- CMake 3.20 or newer

### Build Instructions
Run the following commands in PowerShell or Command Prompt:

```powershell
# Generate build files
cmake -B build -A x64

# Compile Release binary
cmake --build build --config Release
```

The resulting executable will be located at:
```text
build\Release\CleanUpService.exe
```

---

## Usage & Service Management

> **Note**: Installing, starting, and uninstalling Windows Services requires an elevated command prompt (**Run as Administrator**).

### Commands

| Command | Description |
| :--- | :--- |
| `CleanUpService.exe --install` | Installs the service into Windows SCM with **Automatic Startup** (runs every boot). |
| `CleanUpService.exe --uninstall` | Stops and deletes the service from Windows SCM. |
| `CleanUpService.exe --start` | Manually starts the installed service. |
| `CleanUpService.exe --stop` | Stops the running service. |
| `CleanUpService.exe --run-now` | Executes `DeleteTempFiles` and `DiskCleanup` immediately in console mode (no service install needed). |
| `CleanUpService.exe --help` | Displays command line usage. |

---

## Verification & Logs

- **Log File**: Check `%TEMP%\CleanUpService.log` (or `C:\Windows\Temp\CleanUpService.log` when running as `LocalSystem`).
- **Windows Event Viewer**: Open `eventvwr.msc` -> **Windows Logs** -> **Application** -> Filter for source **CleanUpService**.
- **Services Applet**: Open `services.msc` and look for **Clean-Up Boot Service**.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.