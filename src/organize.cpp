#include "organize.hpp"
#include "cleanup.hpp"

#include <windows.h>
#include <shlobj.h>

#include <filesystem>
#include <algorithm>
#include <cwctype>
#include <unordered_set>

namespace fs = std::filesystem;

namespace Organizer {

namespace {

std::wstring ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return result;
}

std::wstring GetSystemDownloadsPath() {
    PWSTR pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &pPath))) {
        std::wstring result(pPath);
        CoTaskMemFree(pPath);
        return result;
    }
    return L"";
}

fs::path GenerateUniqueFilename(const fs::path& targetDir, const fs::path& originalFile) {
    fs::path destination = targetDir / originalFile.filename();
    if (!fs::exists(destination)) {
        return destination;
    }

    std::wstring stem = originalFile.stem().wstring();
    std::wstring ext = originalFile.extension().wstring();
    int counter = 1;

    while (fs::exists(destination)) {
        std::wstring newName = stem + L" (" + std::to_wstring(counter++) + L")" + ext;
        destination = targetDir / newName;
    }

    return destination;
}

} // anonymous namespace

void OrganizeDownloads(const std::wstring& targetDirectory, const AppConfig* configPtr) {
    AppConfig localConfig;
    const AppConfig& config = configPtr ? *configPtr : (localConfig = LoadConfig());

    // Determine target directory: argument > config > system Downloads
    std::wstring dirToUse = targetDirectory;
    if (dirToUse.empty()) {
        dirToUse = config.downloadsPath;
    }
    if (dirToUse.empty()) {
        dirToUse = GetSystemDownloadsPath();
    }

    fs::path targetPath(dirToUse);
    std::error_code ec;

    if (!fs::exists(targetPath, ec) || !fs::is_directory(targetPath, ec)) {
        CleanUp::LogMessage(L"OrganizeDownloads: Directory does not exist: " + targetPath.wstring());
        return;
    }

    CleanUp::LogMessage(L"OrganizeDownloads: Scanning " + targetPath.wstring());

    // Build set of protected destination folder names so we never move them
    std::unordered_set<std::wstring> destinationFolders;
    for (const auto& [ext, folder] : config.extensionToFolder) {
        destinationFolders.insert(ToLower(folder));
    }

    uint64_t movedCount = 0;
    uint64_t skippedCount = 0;

    // Scan non-recursively: only organize files located in the root of the target directory
    for (const auto& entry : fs::directory_iterator(targetPath, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        // Skip subdirectories
        if (entry.is_directory(ec)) {
            continue;
        }

        fs::path filePath = entry.path();
        std::wstring filename = filePath.filename().wstring();

        // Skip hidden or system files like desktop.ini
        if (filename == L"desktop.ini" || filename == L"thumbs.db") {
            continue;
        }

        std::wstring ext = ToLower(filePath.extension().wstring());

        // Skip partial in-progress downloads
        if (ext == L".crdownload" || ext == L".part" || ext == L".tmp" || ext == L".downloading") {
            continue;
        }

        // Look up target folder by extension
        auto it = config.extensionToFolder.find(ext);
        if (it != config.extensionToFolder.end()) {
            const std::wstring& folderName = it->second;
            fs::path destDir = targetPath / folderName;

            // Ensure destination category folder exists
            if (!fs::exists(destDir, ec)) {
                fs::create_directories(destDir, ec);
                if (ec) {
                    CleanUp::LogMessage(L"Failed to create category folder: " + destDir.wstring());
                    skippedCount++;
                    ec.clear();
                    continue;
                }
            }

            fs::path uniqueDestPath = GenerateUniqueFilename(destDir, filePath);

            // Move the file into category folder
            fs::rename(filePath, uniqueDestPath, ec);
            if (ec) {
                // If rename fails (e.g. cross-volume or locked), attempt copy + delete
                ec.clear();
                fs::copy_file(filePath, uniqueDestPath, fs::copy_options::overwrite_existing, ec);
                if (!ec) {
                    fs::remove(filePath, ec);
                    movedCount++;
                    CleanUp::LogMessage(L"Organized: " + filename + L" -> /" + folderName + L"/" + uniqueDestPath.filename().wstring());
                } else {
                    skippedCount++;
                    CleanUp::LogMessage(L"Skipped (locked or in-use): " + filename);
                }
            } else {
                movedCount++;
                CleanUp::LogMessage(L"Organized: " + filename + L" -> /" + folderName + L"/" + uniqueDestPath.filename().wstring());
            }
        }
    }

    CleanUp::LogMessage(L"OrganizeDownloads finished: " + std::to_wstring(movedCount) +
                        L" files organized, " + std::to_wstring(skippedCount) + L" skipped.");
}

} // namespace Organizer
