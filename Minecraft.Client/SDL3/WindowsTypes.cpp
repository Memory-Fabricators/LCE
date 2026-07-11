#include "WindowsTypes.h"
#include "SDL3/SDL.h"
#include <stdlib.h>

void InitializeCriticalSection(CRITICAL_SECTION *cs)
{
    SDL_zerop(cs);
}

void InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *cs, DWORD spinCount)
{
    SDL_zerop(cs);
    cs->spinCount = spinCount;
}

void DeleteCriticalSection(CRITICAL_SECTION *cs)
{
    SDL_zerop(cs);
}

void EnterCriticalSection(CRITICAL_SECTION *cs)
{
}

BOOL TryEnterCriticalSection(CRITICAL_SECTION *cs)
{
    return TRUE;
}

void LeaveCriticalSection(CRITICAL_SECTION *cs)
{
}

DWORD TlsAlloc()
{
    static DWORD nextKey = 1;
    return nextKey++;
}

LPVOID TlsGetValue(DWORD dwTlsIndex)
{
    return NULL;
}

BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    return TRUE;
}

BOOL TlsFree(DWORD dwTlsIndex)
{
    return TRUE;
}

DWORD GetLastError()
{
    return 0;
}

DWORD GetCurrentThreadId()
{
    return 0;
}

HANDLE GetCurrentThread()
{
    return (HANDLE)(uintptr_t)1;
}

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
    return NULL;
}

DWORD ResumeThread(HANDLE hThread)
{
    return 1;
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{
    return TRUE;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    return WAIT_OBJECT_0;
}

BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode)
{
    *lpExitCode = STILL_ACTIVE;
    return TRUE;
}

void Sleep(DWORD dwMilliseconds)
{
}

HANDLE CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName)
{
    return NULL;
}

HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
{
    return NULL;
}

BOOL SetEvent(HANDLE hEvent)
{
    return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
    return TRUE;
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
    return WAIT_OBJECT_0;
}

LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    return calloc(1, dwSize);
}

BOOL VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    free(lpAddress);
    return TRUE;
}

BOOL VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
    return TRUE;
}

SIZE_T VirtualQuery(LPCVOID lpAddress, void *lpBuffer, SIZE_T dwLength)
{
    return 0;
}

DWORD GetFileType(HANDLE hFile)
{
    return FILE_TYPE_DISK;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    return 0;
}

BOOL SetEndOfFile(HANDLE hFile)
{
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
    return TRUE;
}

HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    return INVALID_HANDLE_VALUE;
}

HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    return INVALID_HANDLE_VALUE;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    *lpNumberOfBytesRead = 0;
    return FALSE;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    *lpNumberOfBytesWritten = 0;
    return FALSE;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    return 0;
}

BOOL CloseHandle(HANDLE hObject)
{
    return TRUE;
}
