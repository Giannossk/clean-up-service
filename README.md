# Clean-Up Service

A lightweight, robust Windows Service written in **C++17** that automatically performs temporary file cleanup, disk cleanup, and file organization on every system boot.

---

## Features

- **Boot Automation**: Configured with `SERVICE_AUTO_START` to automatically run every time Windows starts up.
- **INI-Style Configuration (`config.ini`)**: Easily enable or disable features and customize destination folders and file extensions.
- **`OrganizeDownloads`**:
  - Automatically scans your Downloads folder (e.g. `D:\Downloads`).
  - Categorizes and moves loose files into designated category folders:
    - `/Archives` (`.zip`, `.rar`, `.7z`, `.tar`, `.iso`, etc.)
    - `/PowerPoint` (`.ppt`, `.pptx`, `.pps`, etc.)
    - `/PDF` (`.pdf`)
    - `/Word` (`.doc`, `.docx`, etc.)
    - `/Excel` (`.xls`, `.xlsx`, `.csv`, etc.)
    - `/Images` (`.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, etc.)
    - `/Videos` (`.mp4`, `.mkv`, `.avi`, etc.)
    - `/Audio` (`.mp3`, `.wav`, `.flac`, etc.)
    - `/Installers` (`.exe`, `.msi`)
    - `/Code` (`.cpp`, `.py`, `.js`, etc.)
  - Resolves duplicate filename collisions safely (`file (1).ext`).
  - Skips in-progress downloads (`.crdownload`, `.part`, `.tmp`) and directory structures.
- **`DeleteTempFiles`**:
  - Recursively purges Windows system temporary folders (`C:\Windows\Temp`).
  - Cleans user temporary directories (`C:\Users\<User>\AppData\Local\Temp`).
  - Handles locked files silently and gracefully using `std::filesystem`.
- **`DiskCleanup`**:
  - Spawns `cleanmgr.exe /d C: /VERYLOWDISK` and `Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase`.
- **Logging**:
  - Windows Application Event Log (`CleanUpService`).
  - Local log file (`%TEMP%\CleanUpService.log`).
  - Real-time console output during manual runs.

---

## Configuration (`config.ini`)

The configuration file is automatically placed alongside `CleanUpService.exe`:

```ini
[General]
; Remove temporary folders on boot (WinUtil DeleteTempFiles equivalent)
delete_temp_files = true

; Run Windows Disk Cleanup & DISM on boot (cleanmgr.exe + Dism.exe)
disk_cleanup = false

[OrganizeDownloads]
; Automatically organize Downloads folder on boot
enabled = true

; Path to the downloads directory to organize
target_directory = D:\Downloads

[Folders]
; FolderName = .ext1, .ext2, .ext3 ...
Archives = .zip, .rar, .7z, .tar, .gz, .bz2, .xz, .iso
PowerPoint = .ppt, .pptx, .pps, .ppsx, .potx
PDF = .pdf
Word = .doc, .docx, .dotx, .odt, .rtf
Excel = .xls, .xlsx, .xlsm, .csv
Images = .jpg, .jpeg, .png, .gif, .bmp, .svg, .webp, .ico, .tiff
Videos = .mp4, .mkv, .mov, .avi, .wmv, .flv, .webm
Audio = .mp3, .wav, .flac, .aac, .ogg, .m4a
Installers = .exe, .msi
Code = .cpp, .hpp, .c, .h, .py, .js, .ts, .html, .css, .json, .xml
Torrents = .torrent
```

---

## Project Structure

```text
clean-up-service/
├── CMakeLists.txt         # C++17 CMake build configuration
├── config.ini             # INI configuration template
├── include/
│   ├── cleanup.hpp        # DeleteTempFiles & DiskCleanup declarations
│   ├── config.hpp         # INI configuration loader declarations
│   ├── organize.hpp       # OrganizeDownloads declarations
│   └── service.hpp        # Windows Service Control Manager interfaces
├── src/
│   ├── cleanup.cpp        # Temp file deletion & cleanmgr/DISM execution
│   ├── config.cpp         # Lightweight INI parser implementation
│   ├── organize.cpp       # Download organizer & categorization implementation
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

> **Note**: Installing and uninstalling Windows Services requires an elevated command prompt (**Run as Administrator**).

### Commands

| Command | Description |
| :--- | :--- |
| `CleanUpService.exe --organize [path]` | Immediately organizes downloads in the specified path (or `config.ini` default). |
| `CleanUpService.exe --run-now` | Executes all enabled tasks (`DeleteTempFiles`, `OrganizeDownloads`, `DiskCleanup`) in console mode. |
| `CleanUpService.exe --install` | Installs the service into Windows SCM with **Automatic Startup** (runs on every boot). |
| `CleanUpService.exe --uninstall` | Stops and deletes the service from Windows SCM. |
| `CleanUpService.exe --start` | Manually starts the installed service. |
| `CleanUpService.exe --stop` | Stops the running service. |
| `CleanUpService.exe --help` | Displays command line usage. |

---

## Verification & Logs

- **Log File**: Check `%TEMP%\CleanUpService.log`.
- **Windows Event Viewer**: Open `eventvwr.msc` -> **Windows Logs** -> **Application** -> Filter for source **CleanUpService**.
- **Services Applet**: Open `services.msc` and look for **Clean-Up Boot Service**.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.