#include "cleanup.hpp"
#include "organize.hpp"
#include "config.hpp"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

namespace CleanUp {

namespace {

bool RunProcess(const std::wstring& appPath, const std::wstring& commandLine, const std::wstring& taskName, DWORD timeoutMs = INFINITE) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back(L'\0');

    LogMessage(L"Launching " + taskName + L": " + commandLine);

    BOOL success = CreateProcessW(
        appPath.empty() ? nullptr : appPath.c_str(),
        cmdBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        DWORD err = GetLastError();
        LogMessage(L"Failed to launch " + taskName + L". Error code: " + std::to_wstring(err));
        return false;
    }

    LogMessage(taskName + L" started (PID: " + std::to_wstring(pi.dwProcessId) + L"). Waiting for completion...");

    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
    if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        LogMessage(taskName + L" completed with exit code: " + std::to_wstring(exitCode));
    } else if (waitResult == WAIT_TIMEOUT) {
        LogMessage(taskName + L" timed out.");
    } else {
        LogMessage(L"WaitForSingleObject failed for " + taskName + L". Error: " + std::to_wstring(GetLastError()));
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// Emulates: Remove-Item -Path "<dir>\*" -Recurse -Force -ErrorAction SilentlyContinue
void CleanDirectoryContents(const fs::path& dirPath) {
    std::error_code ec;
    if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec)) {
        return;
    }

    uint64_t removedCount = 0;
    uint64_t failedCount = 0;

    for (const auto& entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        std::error_code removeEc;
        fs::remove_all(entry.path(), removeEc);
        if (removeEc) {
            // Silently continue on locked or in-use files (same as -ErrorAction SilentlyContinue)
            failedCount++;
        } else {
            removedCount++;
        }
    }

    LogMessage(L"Cleaned [" + dirPath.wstring() + L"]: " +
               std::to_wstring(removedCount) + L" items removed, " +
               std::to_wstring(failedCount) + L" locked/skipped.");
}

} // anonymous namespace

void LogMessage(const std::wstring& message) {
    std::wcout << L"[CleanUpService] " << message << std::endl;

    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        std::wstring logFilePath = std::wstring(tempPath) + L"CleanUpService.log";
        std::wofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            tm localTime{};
            localtime_s(&localTime, &now);
            wchar_t timeBuffer[64];
            wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &localTime);

            logFile << L"[" << timeBuffer << L"] " << message << std::endl;
        }
    }

    HANDLE hEventSource = RegisterEventSourceW(nullptr, L"CleanUpService");
    if (hEventSource) {
        LPCWSTR strings[1] = { message.c_str() };
        ReportEventW(
            hEventSource,
            EVENTLOG_INFORMATION_TYPE,
            0,
            0x1000,
            nullptr,
            1,
            0,
            strings,
            nullptr
        );
        DeregisterEventSource(hEventSource);
    }
}

// Equivalent to WinUtil WPFTweaksDeleteTempFiles:
// Remove-Item -Path "$Env:Temp\*" -Recurse -Force -ErrorAction SilentlyContinue
// Remove-Item -Path "$Env:SystemRoot\Temp\*" -Recurse -Force -ErrorAction SilentlyContinue
void DeleteTempFiles() {
    LogMessage(L"=== Starting DeleteTempFiles (WinUtil equivalent) ===");

    // 1. $Env:SystemRoot\Temp\* (e.g., C:\Windows\Temp)
    wchar_t winDir[MAX_PATH];
    if (GetWindowsDirectoryW(winDir, MAX_PATH)) {
        fs::path systemTemp = fs::path(winDir) / L"Temp";
        CleanDirectoryContents(systemTemp);
    }

    // 2. $Env:Temp\*
    wchar_t envTemp[MAX_PATH];
    DWORD tempLen = GetEnvironmentVariableW(L"TEMP", envTemp, MAX_PATH);
    if (tempLen > 0 && tempLen < MAX_PATH) {
        CleanDirectoryContents(fs::path(envTemp));
    } else {
        wchar_t fallbackTemp[MAX_PATH];
        if (GetTempPathW(MAX_PATH, fallbackTemp)) {
            CleanDirectoryContents(fs::path(fallbackTemp));
        }
    }

    // 3. User profiles Temp directories (%SystemDrive%\Users\*\AppData\Local\Temp\*)
    // Since Windows Service runs as LocalSystem, this ensures all logged-in user temp folders are cleaned
    wchar_t sysDrive[MAX_PATH] = L"C:\\";
    if (GetWindowsDirectoryW(winDir, MAX_PATH) && wcslen(winDir) >= 3) {
        sysDrive[0] = winDir[0];
        sysDrive[1] = winDir[1];
        sysDrive[2] = winDir[2];
        sysDrive[3] = L'\0';
    }

    fs::path usersDir = fs::path(sysDrive) / L"Users";
    std::error_code ec;
    if (fs::exists(usersDir, ec)) {
        for (const auto& userEntry : fs::directory_iterator(usersDir, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (userEntry.is_directory(ec)) {
                fs::path userTemp = userEntry.path() / L"AppData" / L"Local" / L"Temp";
                CleanDirectoryContents(userTemp);
            }
        }
    }

    LogMessage(L"=== DeleteTempFiles completed ===");
}

// Equivalent to WinUtil WPFTweaksDiskCleanup:
// cleanmgr.exe /d C: /VERYLOWDISK
// Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase
void DiskCleanup() {
    LogMessage(L"=== Starting DiskCleanup (WinUtil equivalent) ===");

    wchar_t systemDir[MAX_PATH];
    if (!GetSystemDirectoryW(systemDir, MAX_PATH)) {
        LogMessage(L"Failed to retrieve System32 directory.");
        return;
    }

    // Step 1: cleanmgr.exe /d C: /VERYLOWDISK
    std::wstring cleanmgrPath = std::wstring(systemDir) + L"\\cleanmgr.exe";
    std::wstring cleanmgrCmd = L"\"" + cleanmgrPath + L"\" /d C: /VERYLOWDISK";
    // Wait up to 15 minutes for cleanmgr
    RunProcess(cleanmgrPath, cleanmgrCmd, L"cleanmgr.exe", 15 * 60 * 1000);

    // Step 2: Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase
    std::wstring dismPath = std::wstring(systemDir) + L"\\Dism.exe";
    std::wstring dismCmd = L"\"" + dismPath + L"\" /online /Cleanup-Image /StartComponentCleanup /ResetBase";
    // Wait up to 30 minutes for DISM component cleanup & base reset
    RunProcess(dismPath, dismCmd, L"Dism.exe", 30 * 60 * 1000);

    LogMessage(L"=== DiskCleanup completed ===");
}

void RunAllCleanups(const AppConfig* configPtr) {
    AppConfig localConfig;
    const AppConfig& config = configPtr ? *configPtr : (localConfig = LoadConfig());

    LogMessage(L"**************************************************");
    LogMessage(L"* CleanUpService: Starting Boot Maintenance Sequence *");
    LogMessage(L"**************************************************");

    if (config.deleteTempFiles) {
        DeleteTempFiles();
    } else {
        LogMessage(L"DeleteTempFiles disabled in config.ini. Skipping.");
    }

    if (config.diskCleanup) {
        DiskCleanup();
    } else {
        LogMessage(L"DiskCleanup disabled in config.ini. Skipping.");
    }

    if (config.organizeDownloads) {
        Organizer::OrganizeDownloads(config.downloadsPath, &config);
    } else {
        LogMessage(L"OrganizeDownloads disabled in config.ini. Skipping.");
    }

    LogMessage(L"**************************************************");
    LogMessage(L"* CleanUpService: Maintenance Sequence Complete  *");
    LogMessage(L"**************************************************");
}

} // namespace CleanUp
