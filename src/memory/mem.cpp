#include "mem.hpp"
#include <cwchar>
#include <iostream>
#include <cstring>

uintptr_t mem::GetModuleBase(HANDLE processHandle, string& sModuleName) {
    DWORD cbNeeded = 0;
    HMODULE* hModules = nullptr;
    uintptr_t dwBase = 0;
    
    if (!EnumProcessModules(processHandle, nullptr, 0, &cbNeeded)) {
        return 0;
    }

    size_t moduleCount = cbNeeded / sizeof(HMODULE);
    hModules = new HMODULE[moduleCount];

    if (EnumProcessModules(processHandle, hModules, cbNeeded, &cbNeeded)) {
        WCHAR szModuleName[MAX_PATH];
        for (size_t i = 0; i < moduleCount; ++i) {
            if (GetModuleBaseNameW(processHandle, hModules[i], szModuleName, MAX_PATH)) {
                char szConverted[MAX_PATH];
                wcstombs(szConverted, szModuleName, MAX_PATH);
                if (sModuleName == szConverted) {
                    dwBase = reinterpret_cast<uintptr_t>(hModules[i]);
                    break;
                }
            }
        }
    }

    delete[] hModules;
    return dwBase;
}

BYTE* mem::LocateModuleBase(DWORD pid, const char* modulename) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snapshot, &moduleEntry)) {
        do {
            #ifdef UNICODE
                char szModuleName[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, moduleEntry.szModule, -1, szModuleName, MAX_PATH, NULL, NULL);
                if (_stricmp(szModuleName, modulename) == 0) {
            #else
                if (_stricmp(moduleEntry.szModule, modulename) == 0) {
            #endif
                CloseHandle(snapshot);
                return moduleEntry.modBaseAddr;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }

    CloseHandle(snapshot);
    return nullptr;
}