#include "service.hpp"
#include "cleanup.hpp"
#include "organize.hpp"
#include "config.hpp"

#include <windows.h>
#include <objbase.h>
#include <iostream>
#include <string>

void PrintHelp() {
    std::wcout << L"CleanUpService - Windows Service for Boot Maintenance & Download Organization\n\n"
               << L"Usage:\n"
               << L"  CleanUpService.exe --install              Install the service (auto-starts on boot)\n"
               << L"  CleanUpService.exe --uninstall            Uninstall the service\n"
               << L"  CleanUpService.exe --start                Start the installed service\n"
               << L"  CleanUpService.exe --stop                 Stop the running service\n"
               << L"  CleanUpService.exe --organize [path]      Organize downloads (e.g. D:\\Downloads)\n"
               << L"  CleanUpService.exe --run-now              Run enabled boot tasks (cleanup + organize) immediately\n"
               << L"  CleanUpService.exe --help                 Show this help message\n\n"
               << L"Configuration file: config.ini (loaded alongside the executable)\n"
               << L"When started by Windows Service Control Manager, runs automatically without arguments.\n";
}

int wmain(int argc, wchar_t* argv[]) {
    // Initialize COM for shell operations (Recycle Bin, KnownFolders, etc.)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library." << std::endl;
        return 1;
    }

    int exitCode = 0;

    if (argc > 1) {
        std::wstring command = argv[1];

        if (command == L"--install" || command == L"/install" || command == L"-i") {
            exitCode = Service::InstallService() ? 0 : 1;
        } else if (command == L"--uninstall" || command == L"/uninstall" || command == L"-u") {
            exitCode = Service::UninstallService() ? 0 : 1;
        } else if (command == L"--start") {
            exitCode = Service::StartServiceInstance() ? 0 : 1;
        } else if (command == L"--stop") {
            exitCode = Service::StopServiceInstance() ? 0 : 1;
        } else if (command == L"--organize" || command == L"/organize" || command == L"-o") {
            std::wstring targetDir = (argc > 2) ? argv[2] : L"";
            std::wcout << L"Executing OrganizeDownloads..." << std::endl;
            Organizer::OrganizeDownloads(targetDir);
            std::wcout << L"OrganizeDownloads finished." << std::endl;
        } else if (command == L"--run-now" || command == L"/run-now" || command == L"-r") {
            std::wcout << L"Executing manual maintenance run..." << std::endl;
            CleanUp::RunAllCleanups();
            std::wcout << L"Maintenance finished." << std::endl;
        } else if (command == L"--help" || command == L"/?") {
            PrintHelp();
        } else {
            std::wcerr << L"Unknown argument: " << command << L"\n\n";
            PrintHelp();
            exitCode = 1;
        }
    } else {
        // Run as Windows Service via Service Control Manager
        if (!Service::RunServiceDispatcher()) {
            DWORD error = GetLastError();
            if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
                // Executed directly in console without arguments
                PrintHelp();
            } else {
                std::wcerr << L"Failed to connect to Service Control Manager. Error: " << error << std::endl;
                exitCode = 1;
            }
        }
    }

    CoUninitialize();
    return exitCode;
}
