#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

struct AppConfig {
    // [General]
    bool deleteTempFiles = true;
    bool diskCleanup = false; // Off by default on boot since DISM can be time-consuming

    // [OrganizeDownloads]
    bool organizeDownloads = true;
    std::wstring downloadsPath = L"D:\\Downloads";

    // [Folders] - Mapping: lowercase extension (e.g. L".pdf") -> destination folder name (e.g. L"PDF")
    std::unordered_map<std::wstring, std::wstring> extensionToFolder;
};

// Returns the path to config.ini located alongside the current executable
std::filesystem::path GetDefaultConfigPath();

// Loads configuration from config.ini, generating a default file if it doesn't exist
AppConfig LoadConfig(const std::filesystem::path& configPath = GetDefaultConfigPath());

// Generates a default config.ini with standard rules and comments
bool CreateDefaultConfigFile(const std::filesystem::path& configPath);
