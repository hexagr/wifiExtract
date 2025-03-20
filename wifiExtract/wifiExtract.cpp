#include <windows.h>
#include <wlanapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "wlanapi.lib")

void error(const std::string& msg) {
    std::cerr << "[Uhoh] " << msg << std::endl;
}

int main() {
    DWORD negotiatedVersion;
    HANDLE hClient = NULL;

    DWORD result = WlanOpenHandle(2, NULL, &negotiatedVersion, &hClient);
    if (result != ERROR_SUCCESS) {
        error("WlanOpenHandle failed: " + std::to_string(result));
        return 1;
    }

    PWLAN_INTERFACE_INFO_LIST plist = NULL;
    result = WlanEnumInterfaces(hClient, NULL, &plist);
    if (result != ERROR_SUCCESS) {
        error("WlanEnumInterfaces failed: " + std::to_string(result));
        WlanCloseHandle(hClient, NULL);
        return 1;
    }

    for (DWORD i = 0; i < plist->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO& interface_info = plist->InterfaceInfo[i];
        PWLAN_PROFILE_INFO_LIST profileList = NULL;

        result = WlanGetProfileList(hClient, &interface_info.InterfaceGuid, NULL, &profileList);
        if (result != ERROR_SUCCESS) {
            error("WlanGetProfileList failed: " + std::to_string(result));
            continue;
        }

        for (DWORD j = 0; j < profileList->dwNumberOfItems; ++j) {
            const WLAN_PROFILE_INFO& profileInfo = profileList->ProfileInfo[j];
            std::wstring profileName(profileInfo.strProfileName);

            LPWSTR xmlProfile = NULL;
            DWORD flags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
            result = WlanGetProfile(hClient, &interface_info.InterfaceGuid, profileName.c_str(), NULL, &xmlProfile, &flags, NULL);
            if (result != ERROR_SUCCESS) {
                error("WlanGetProfile failed: " + std::to_string(result));
                continue;
            }

            std::wstring profileXml(xmlProfile);
            size_t key = profileXml.find(L"<keyMaterial>");
            if (key != std::wstring::npos) {
                size_t keyEnd = profileXml.find(L"</keyMaterial>", key);
                if (keyEnd != std::wstring::npos) {
                    std::wstring keyContent = profileXml.substr(key + 13, keyEnd - (key + 13));
                    std::wcout << L"[+] Found Wifi Profile:" << std::endl;
                    std::wcout << L"[+] SSID: " << profileName << std::endl;
                    std::wcout << L"[+] Key: " << keyContent << std::endl;
                    std::wcout << L" " << std::endl;
                }
            }
            else {
                std::wcout << L"[+] Found Wifi Profile:" << std::endl;
                std::wcout << L"[+] SSID: " << profileName << std::endl;
                std::wcout << L"[+] No key material found" << std::endl;
                std::wcout << L" " << std::endl;
            }

            WlanFreeMemory(xmlProfile);
        }

        WlanFreeMemory(profileList);
    }

    WlanFreeMemory(plist);
    WlanCloseHandle(hClient, NULL);

    return 0;
}