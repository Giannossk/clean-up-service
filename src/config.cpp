#include "config.hpp"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;

namespace {

std::wstring Trim(const std::wstring& str) {
    auto start = str.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"";
    auto end = str.find_last_not_of(L" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::wstring ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return result;
}

bool ParseBool(const std::wstring& value, bool defaultValue) {
    std::wstring lower = ToLower(Trim(value));
    if (lower == L"true" || lower == L"1" || lower == L"yes" || lower == L"on") return true;
    if (lower == L"false" || lower == L"0" || lower == L"no" || lower == L"off") return false;
    return defaultValue;
}

} // anonymous namespace

fs::path GetDefaultConfigPath() {
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        fs::path p(exePath);
        return p.parent_path() / L"config.ini";
    }
    return fs::current_path() / L"config.ini";
}

bool CreateDefaultConfigFile(const fs::path& configPath) {
    std::wofstream out(configPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << L"[General]\n"
        << L"; Remove temporary folders on boot (equivalent to WinUtil DeleteTempFiles)\n"
        << L"delete_temp_files = true\n"
        << L"; Run Windows Disk Cleanup & DISM on boot (cleanmgr.exe + Dism.exe)\n"
        << L"disk_cleanup = false\n\n"
        << L"[OrganizeDownloads]\n"
        << L"; Automatically organize Downloads folder on boot\n"
        << L"enabled = true\n"
        << L"; Path to the downloads directory to organize\n"
        << L"target_directory = D:\\Downloads\n\n"
        << L"[Folders]\n"
        << L"; DestinationFolder = .ext1, .ext2, .ext3\n"
        << L"Archives = .zip, .rar, .7z, .tar, .gz, .bz2, .xz, .iso\n"
        << L"PowerPoint = .ppt, .pptx, .pps, .ppsx, .potx\n"
        << L"PDF = .pdf\n"
        << L"Word = .doc, .docx, .dotx, .odt, .rtf\n"
        << L"Excel = .xls, .xlsx, .xlsm, .csv\n"
        << L"Images = .jpg, .jpeg, .png, .gif, .bmp, .svg, .webp, .ico, .tiff\n"
        << L"Videos = .mp4, .mkv, .mov, .avi, .wmv, .flv, .webm\n"
        << L"Audio = .mp3, .wav, .flac, .aac, .ogg, .m4a\n"
        << L"Installers = .exe, .msi\n"
        << L"Code = .cpp, .hpp, .c, .h, .py, .js, .ts, .html, .css, .json, .xml\n"
        << L"Torrents = .torrent\n";

    return true;
}

AppConfig LoadConfig(const fs::path& configPath) {
    AppConfig config;

    if (!fs::exists(configPath)) {
        CreateDefaultConfigFile(configPath);
    }

    std::wifstream in(configPath);
    if (!in.is_open()) {
        // Populate standard default folder rules in memory if file cannot be opened
        config.extensionToFolder[L".zip"] = L"Archives";
        config.extensionToFolder[L".rar"] = L"Archives";
        config.extensionToFolder[L".7z"] = L"Archives";
        config.extensionToFolder[L".tar"] = L"Archives";
        config.extensionToFolder[L".gz"] = L"Archives";
        config.extensionToFolder[L".bz2"] = L"Archives";
        config.extensionToFolder[L".xz"] = L"Archives";
        config.extensionToFolder[L".iso"] = L"Archives";
        config.extensionToFolder[L".pdf"] = L"PDF";
        config.extensionToFolder[L".ppt"] = L"PowerPoint";
        config.extensionToFolder[L".pptx"] = L"PowerPoint";
        config.extensionToFolder[L".pps"] = L"PowerPoint";
        config.extensionToFolder[L".ppsx"] = L"PowerPoint";
        return config;
    }

    std::wstring line;
    std::wstring currentSection;

    while (std::getline(in, line)) {
        std::wstring trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == L';' || trimmed[0] == L'#') {
            continue;
        }

        // Section header
        if (trimmed.front() == L'[' && trimmed.back() == L']') {
            currentSection = ToLower(Trim(trimmed.substr(1, trimmed.size() - 2)));
            continue;
        }

        // Key = Value
        auto eqPos = trimmed.find(L'=');
        if (eqPos == std::wstring::npos) {
            continue;
        }

        std::wstring key = Trim(trimmed.substr(0, eqPos));
        std::wstring value = Trim(trimmed.substr(eqPos + 1));

        if (currentSection == L"general") {
            std::wstring keyLower = ToLower(key);
            if (keyLower == L"delete_temp_files") {
                config.deleteTempFiles = ParseBool(value, config.deleteTempFiles);
            } else if (keyLower == L"disk_cleanup") {
                config.diskCleanup = ParseBool(value, config.diskCleanup);
            }
        } else if (currentSection == L"organizedownloads") {
            std::wstring keyLower = ToLower(key);
            if (keyLower == L"enabled") {
                config.organizeDownloads = ParseBool(value, config.organizeDownloads);
            } else if (keyLower == L"target_directory") {
                config.downloadsPath = value;
            }
        } else if (currentSection == L"folders") {
            std::wstring folderName = key;
            // Parse comma-separated list of extensions
            std::wstringstream ss(value);
            std::wstring extToken;
            while (std::getline(ss, extToken, L',')) {
                std::wstring ext = ToLower(Trim(extToken));
                if (!ext.empty()) {
                    if (ext.front() != L'.') {
                        ext = L"." + ext;
                    }
                    config.extensionToFolder[ext] = folderName;
                }
            }
        }
    }

    return config;
}
