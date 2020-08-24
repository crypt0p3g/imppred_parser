#include <iostream>

#include <windows.h>
#include <shlobj.h>
#include <assert.h>

#pragma pack(push, 1)

typedef struct IMPPRED_HEADER {
    unsigned long long Timestamp;
    DWORD dwAllocatedSize;
    DWORD dwFlags;
    DWORD dwEntryCount;
    DWORD dwConst_0x20;
    DWORD dwUnk1;
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
    BYTE nResultLen;
    BYTE nInputLen;
    DWORD dwFlags;
    //WCHAR wsText[ANYSIZE_ARRAY]; // WCHAR[nInputLen] + WCHAR[nResultLen]
} IMPPRED_SUBSTITUTION;

#pragma pack(pop)

BOOL toAnsiChar(LPCWCH lpWideCharStr, LPSTR lpMultiByteStr, int cbMultiByte)
{
    int res = WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, 0, 0, 0, 0);
    if (res == -1 || res >= cbMultiByte)
        return FALSE;
    WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, -1, lpMultiByteStr, cbMultiByte, 0, 0);
    return TRUE;
}

int wmain(int argc, wchar_t** argv)
{
    WCHAR szFilePath[MAX_PATH];
    GUID local_FOLDERID_RoamingAppData = { 0x3EB685DB, 0x65F9, 0x4CF6, 0xA0, 0x3A, 0xE3, 0xEF, 0x65, 0x72, 0x9F, 0x3D };
    if (argc < 2) {
        PWSTR appdate;
        HRESULT result;
        if ((result = SHGetKnownFolderPath(local_FOLDERID_RoamingAppData, NULL, NULL, &appdate)) != S_OK) {
            printf("SHGetKnownFolderPath failed hresult=%08x\n", result);
            return -1;
        }  
        wcscpy_s(szFilePath, appdate);
        wcscat_s(szFilePath, L"\\Microsoft\\InputMethod\\Shared\\JpnIHDS.dat");
    }
    else {
        wcscpy_s(szFilePath, argv[1]);
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    wprintf(L"Path: %s\n", szFilePath);

    HANDLE hFile = CreateFileW(szFilePath, FILE_READ_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFileW failed lasterror=%08x\n", GetLastError());
        return -1;
    }

    IMPPRED_HEADER header;
    DWORD dwReaded;
    ReadFile(hFile, &header, sizeof(IMPPRED_HEADER), &dwReaded, NULL);

    //printf("Timestamp = %llx\n", header.Timestamp);
    printf("dwEntryCount = %d\n", header.dwEntryCount);
    printf("dwUnk1 = %08x\n", header.dwUnk1);

    assert(header.dwConst_0x20 == 0x20);
    for (DWORD i = 0; i < header.dwEntryCount; i++) {
        IMPPRED_SENTENCE sentence;
        ReadFile(hFile, &sentence, sizeof(IMPPRED_SENTENCE), &dwReaded, NULL);

        FILETIME ft;
        ft.dwLowDateTime = (DWORD)sentence.Timestamp;
        ft.dwHighDateTime = (DWORD)(sentence.Timestamp >> 32);

        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        printf("%04i.%02i.%02i %02i:%02i:%02i ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        //printf("Timestamp = %llx\n", sentence.Timestamp);
        //printf("bSubEntryCount = %d\n", sentence.bSubEntryCount);
        assert(sentence.bConst_0x01 == 0x01);
        assert(sentence.wConst_0x00 == 0x00);
        assert(sentence.wConst_0x10 == 0x10);
        //
        for (int j = 0; j < sentence.bSubEntryCount; j++) {
            IMPPRED_SUBSTITUTION substitution;
            ReadFile(hFile, &substitution, sizeof(IMPPRED_SUBSTITUTION), &dwReaded, NULL);

            CHAR szInputText[256];
            WCHAR wsInputText[256];
            ReadFile(hFile, wsInputText, substitution.nInputLen * 2, &dwReaded, NULL);
            wsInputText[substitution.nInputLen] = 0;
            toAnsiChar(wsInputText, szInputText, sizeof(szInputText));

            WCHAR wsResultText[256];
            CHAR szResultText[256];
            ReadFile(hFile, wsResultText, substitution.nResultLen * 2, &dwReaded, NULL);
            wsResultText[substitution.nResultLen] = 0;
            toAnsiChar(wsResultText, szResultText, sizeof(szResultText));

            if (substitution.nResultLen == 0)
                fwrite(szInputText, 1, strlen(szInputText), stdout);
            else
                fwrite(szResultText, 1, strlen(szResultText), stdout);
        }
        printf("\n");
    }

    DWORD dwReadedSize = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
    //printf("dwReadedSize = %08x\n", dwReadedSize);
    assert(header.dwUsedSize == dwReadedSize);

    CloseHandle(hFile);
}