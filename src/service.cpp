#include "service.hpp"
#include "cleanup.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>

namespace Service {

namespace {

SERVICE_STATUS        g_ServiceStatus{};
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
HANDLE                g_StopEvent = nullptr;
std::thread           g_WorkerThread;

void SetStatus(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0) {
    static DWORD checkPoint = 1;

    g_ServiceStatus.dwCurrentState = currentState;
    g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
    g_ServiceStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING) {
        g_ServiceStatus.dwControlsAccepted = 0;
    } else {
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED) {
        g_ServiceStatus.dwCheckPoint = 0;
    } else {
        g_ServiceStatus.dwCheckPoint = checkPoint++;
    }

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

void ServiceWorker() {
    CleanUp::LogMessage(L"Service worker started. Executing boot cleanup tasks...");
    
    // Execute the boot cleanup sequence
    CleanUp::RunAllCleanups();

    CleanUp::LogMessage(L"Boot cleanup tasks finished. Waiting for service stop or shutdown...");

    // Wait until stop signal is received
    WaitForSingleObject(g_StopEvent, INFINITE);

    CleanUp::LogMessage(L"Service worker shutting down.");
}

} // anonymous namespace

bool InstallService() {
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        std::wcerr << L"Failed to determine executable path." << std::endl;
        return false;
    }

    std::wstring quotedPath = L"\"" + std::wstring(exePath) + L"\"";

    SC_HANDLE schSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (!schSCManager) {
        std::wcerr << L"OpenSCManager failed. Error: " << GetLastError() << L" (Make sure to run as Administrator)" << std::endl;
        return false;
    }

    SC_HANDLE schService = CreateServiceW(
        schSCManager,
        kServiceName,
        kServiceDisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, // Starts automatically on boot!
        SERVICE_ERROR_NORMAL,
        quotedPath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr, // Runs as LocalSystem
        nullptr
    );

    if (!schService) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            std::wcout << L"Service is already installed." << std::endl;
        } else {
            std::wcerr << L"CreateService failed. Error: " << err << std::endl;
        }
        CloseServiceHandle(schSCManager);
        return false;
    }

    // Configure service description
    SERVICE_DESCRIPTIONW sd{};
    sd.lpDescription = const_cast<LPWSTR>(kServiceDescription);
    ChangeServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, &sd);

    std::wcout << L"Service installed successfully with Auto-Start (runs on every boot)." << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool UninstallService() {
    SC_HANDLE schSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!schSCManager) {
        std::wcerr << L"OpenSCManager failed. Error: " << GetLastError() << L" (Make sure to run as Administrator)" << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenServiceW(schSCManager, kServiceName, SERVICE_STOP | DELETE);
    if (!schService) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            std::wcout << L"Service does not exist." << std::endl;
        } else {
            std::wcerr << L"OpenService failed. Error: " << err << std::endl;
        }
        CloseServiceHandle(schSCManager);
        return false;
    }

    // Attempt to stop before deletion
    SERVICE_STATUS status;
    ControlService(schService, SERVICE_CONTROL_STOP, &status);

    if (!DeleteService(schService)) {
        std::wcerr << L"DeleteService failed. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    std::wcout << L"Service removed successfully." << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool StartServiceInstance() {
    SC_HANDLE schSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!schSCManager) {
        std::wcerr << L"OpenSCManager failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenServiceW(schSCManager, kServiceName, SERVICE_START);
    if (!schService) {
        std::wcerr << L"OpenService failed. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (!StartServiceW(schService, 0, nullptr)) {
        std::wcerr << L"StartService failed. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    std::wcout << L"Service started successfully." << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool StopServiceInstance() {
    SC_HANDLE schSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!schSCManager) {
        std::wcerr << L"OpenSCManager failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenServiceW(schSCManager, kServiceName, SERVICE_STOP);
    if (!schService) {
        std::wcerr << L"OpenService failed. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_STATUS status;
    if (!ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
        std::wcerr << L"ControlService failed. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    std::wcout << L"Service stopped successfully." << std::endl;

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool RunServiceDispatcher() {
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(kServiceName), ServiceMain },
        { nullptr, nullptr }
    };

    return StartServiceCtrlDispatcherW(serviceTable) != FALSE;
}

VOID WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/) {
    g_StatusHandle = RegisterServiceCtrlHandlerExW(kServiceName, ServiceCtrlHandler, nullptr);
    if (!g_StatusHandle) {
        return;
    }

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    SetStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_StopEvent) {
        SetStatus(SERVICE_STOPPED, GetLastError());
        return;
    }

    SetStatus(SERVICE_RUNNING);

    g_WorkerThread = std::thread(ServiceWorker);

    // Wait until worker thread is ready to exit or stopped
    if (g_WorkerThread.joinable()) {
        g_WorkerThread.join();
    }

    CloseHandle(g_StopEvent);
    SetStatus(SERVICE_STOPPED);
}

DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD /*eventType*/, LPVOID /*eventData*/, LPVOID /*context*/) {
    switch (control) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            SetStatus(SERVICE_STOP_PENDING, NO_ERROR, 5000);
            if (g_StopEvent) {
                SetEvent(g_StopEvent);
            }
            return NO_ERROR;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

} // namespace Service
