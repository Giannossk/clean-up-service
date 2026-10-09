#pragma once

#include <string>

namespace CleanUp {

// Removes temporary files from system and user temp directories,
// SoftwareDistribution cache, and empties the Recycle Bin.
void DeleteTempFiles();

// Runs Windows Disk Cleanup (cleanmgr.exe /AUTOCLEAN).
void DiskCleanup();

// Executes both DeleteTempFiles and DiskCleanup in sequence.
void RunAllCleanups();

// Logging helper for cleanup events
void LogMessage(const std::wstring& message);

} // namespace CleanUp
