#pragma once

#include <windows.h>
#include <string>

namespace Service {

inline constexpr const wchar_t* kServiceName = L"CleanUpService";
inline constexpr const wchar_t* kServiceDisplayName = L"Clean-Up Boot Service";
inline constexpr const wchar_t* kServiceDescription = L"Removes temporary files and executes Windows Disk Cleanup on every system boot.";

// Installs the Windows Service with SERVICE_AUTO_START so it runs every boot
bool InstallService();

// Uninstalls (removes) the service from SCM
bool UninstallService();

// Starts the installed service manually
bool StartServiceInstance();

// Stops the running service
bool StopServiceInstance();

// Registers with Service Control Manager when started by Windows
bool RunServiceDispatcher();

// Windows Service entry point and control handler
VOID WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID context);

} // namespace Service
