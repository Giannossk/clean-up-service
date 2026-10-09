#pragma once

#include "config.hpp"
#include <string>

namespace CleanUp {

// Removes temporary files from system and user temp directories
void DeleteTempFiles();

// Runs Windows Disk Cleanup (cleanmgr.exe /VERYLOWDISK and Dism.exe)
void DiskCleanup();

// Executes all enabled maintenance routines according to config.ini
void RunAllCleanups(const AppConfig* config = nullptr);

// Logging helper for cleanup events
void LogMessage(const std::wstring& message);

} // namespace CleanUp
