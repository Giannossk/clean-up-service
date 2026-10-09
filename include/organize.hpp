#pragma once

#include "config.hpp"
#include <string>

namespace Organizer {

// Organizes files in the specified directory (defaults to D:\Downloads or config.downloadsPath)
// into folders based on INI configured extensions (/Archives, /PowerPoint, /PDF, etc.).
void OrganizeDownloads(const std::wstring& targetDirectory = L"", const AppConfig* config = nullptr);

} // namespace Organizer
