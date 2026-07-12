#pragma once

#include <cstdint>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <wchar.h>

typedef int32_t BOOL;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int32_t __int32;
typedef int64_t LONGLONG;
typedef int64_t LONG64;
typedef uint64_t ULONGLONG;
typedef uint64_t DWORD64;
typedef uint64_t DWORD_PTR;
typedef unsigned char boolean;
class C4JStringTable;

// Miles Sound System types (RAD Game Tools) - not vendored in this tree.
// Opaque handles only, sufficient to declare SoundEngine's members until
// the audio backend is reimplemented on top of OpenAL.
typedef float F32;
typedef void *HMSOUNDBANK;
typedef void *HDIGDRIVER;
typedef void *HSTREAM;

typedef void *HANDLE;
typedef void *PVOID;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef const wchar_t *LPCWSTR;
typedef wchar_t *LPWSTR;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef BYTE *PBYTE;
typedef BOOL *PBOOL;
typedef DWORD *PDWORD;
typedef UINT *PUINT;
typedef wchar_t WCHAR;
typedef WCHAR TCHAR;
typedef WCHAR *LPTSTR;
typedef const WCHAR *LPCTSTR;
typedef void *PSECURITY_DESCRIPTOR;
typedef size_t SIZE_T;
typedef uintptr_t ULONG_PTR;
typedef intptr_t LONG_PTR;
typedef float FLOAT;

#ifndef __debugbreak
#define __debugbreak() __builtin_trap()
#endif

#ifndef sprintf_s
#define sprintf_s(buf, sz, ...) snprintf((buf), (sz), __VA_ARGS__)
#endif

#ifndef STILL_ACTIVE
#define STILL_ACTIVE 259
#endif
#define CREATE_SUSPENDED 4
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define INFINITE 0xFFFFFFFF
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_LOWEST -2

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define WINAPI
#define CALLBACK
#define APIENTRY
#define CONST const
#define VOID void

typedef long HRESULT;
typedef long *PLONG;
typedef long long *PLONGLONG;
typedef unsigned long *PULONG;
typedef char *PCHAR;
typedef wchar_t *PWCHAR;
typedef const char *PCSTR;
typedef const wchar_t *PCWSTR;
typedef BYTE *LPBYTE;

#define MAXULONG_PTR ((ULONG_PTR) - 1)

#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_RELEASE 0x8000
#define MEM_DECOMMIT 0x4000
#define MEM_LARGE_PAGES 0x20000000
#define MEM_FREE 0x10000
#define PAGE_READWRITE 0x04
#define PAGE_READONLY 0x02
#define PAGE_WRITECOPY 0x08
#define PAGE_NOACCESS 0x01

typedef struct _MEMORYSTATUS
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
    SIZE_T dwAvailExtendedVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef void *LPVIRTUALALLOC;
LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
BOOL VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
SIZE_T VirtualQuery(LPCVOID lpAddress, void *lpBuffer, SIZE_T dwLength);

#define FILE_TYPE_DISK 1
#define FILE_TYPE_UNKNOWN 0
DWORD GetFileType(HANDLE hFile);
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL SetEndOfFile(HANDLE hFile);
BOOL FlushFileBuffers(HANDLE hFile);

typedef struct _SECURITY_ATTRIBUTES
{
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef DWORD(WINAPI *LPTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);

typedef struct _OVERLAPPED
{
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct
        {
            DWORD Offset;
            DWORD OffsetHigh;
        };
        LPVOID Pointer;
    };
    HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef DWORD *LPDWORD;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_ABORT ((HRESULT)0x80004004L)
#define E_POINTER ((HRESULT)0x80004003L)
#define E_UNEXPECTED ((HRESULT)0x8000FFFFL)

#define FAILED(hr) ((HRESULT)(hr) < 0)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR) - 1)
#define GENERIC_READ (0x80000000L)
#define GENERIC_WRITE (0x40000000L)
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define FILE_FLAG_RANDOM_ACCESS 0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000

#define MAX_PATH 260

typedef struct
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME;

typedef union {
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    };
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER ULARGE_INTEGER;

#define ZeroMemory(d, l) memset((d), 0, (l))
#define MoveMemory(d, s, l) memmove((d), (s), (l))
#define CopyMemory(d, s, l) memcpy((d), (s), (l))

#define swscanf_s swscanf
#define _TRUNCATE ((size_t)-1)
#define CDECL
#define _vsnprintf_s(buf, count, fmt, args) vsnprintf((buf), sizeof(buf), (fmt), (args))
inline void OutputDebugStringW(const wchar_t *)
{
}
inline void OutputDebugStringA(const char *)
{
}
#define ERROR_SUCCESS 0L
#define ERROR_IO_PENDING 997L

// Rich presence context IDs (normally generated per-platform from Minecraft.spa;
// no SDL3 rich presence backend exists yet, so these are just placeholder values).
#define CONTEXT_GAME_STATE_BLANK 0
#define CONTEXT_GAME_STATE_RIDING_PIG 1
#define CONTEXT_GAME_STATE_RIDING_MINECART 2
#define CONTEXT_GAME_STATE_BOATING 3
#define CONTEXT_GAME_STATE_FISHING 4
#define CONTEXT_GAME_STATE_NETHER 7
#define CONTEXT_GAME_STATE_CD 8
#define CONTEXT_GAME_STATE_MAP 9

#define CONTEXT_PRESENCE_IDLE 0
#define CONTEXT_PRESENCE_MULTIPLAYER 2
#define CONTEXT_PRESENCE_MULTIPLAYEROFFLINE 3
#define CONTEXT_PRESENCE_MULTIPLAYER_1P 4
#define CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE 5

static inline std::int64_t InterlockedCompareExchangeRelease64(volatile std::int64_t *dest, std::int64_t exchange, std::int64_t comparand)
{
    std::int64_t expected = comparand;
    __atomic_compare_exchange_n(dest, &expected, exchange, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
    return expected;
}

typedef struct _FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;

#include <chrono>

inline DWORD GetTickCount(void)
{
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return (DWORD)duration_cast<milliseconds>(steady_clock::now() - start).count();
}

inline BOOL SystemTimeToFileTime(const SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime)
{
    struct tm tmVal = {};
    tmVal.tm_year = lpSystemTime->wYear - 1900;
    tmVal.tm_mon = lpSystemTime->wMonth - 1;
    tmVal.tm_mday = lpSystemTime->wDay;
    tmVal.tm_hour = lpSystemTime->wHour;
    tmVal.tm_min = lpSystemTime->wMinute;
    tmVal.tm_sec = lpSystemTime->wSecond;
    time_t tt = timegm(&tmVal);

    const uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
    uint64_t ticks = (uint64_t)tt * 10000000ULL + EPOCH_DIFF_100NS;
    lpFileTime->dwLowDateTime = (DWORD)(ticks & 0xFFFFFFFF);
    lpFileTime->dwHighDateTime = (DWORD)(ticks >> 32);
    return TRUE;
}

void GetSystemTime(SYSTEMTIME *lpSystemTime);

inline LONG SystemTimeAsFileTime(FILETIME *pDestFileTime)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, pDestFileTime);
    return 0;
}

inline void MemSect(int)
{
}

namespace std
{
inline size_t hash_value(const std::wstring &s)
{
    return hash<std::wstring>()(s);
}
} // namespace std

static inline wchar_t *_itow(int value, wchar_t *buffer, int radix)
{
    unsigned int v = (radix == 10 && value < 0) ? (unsigned int)(-value) : (unsigned int)value;
    wchar_t tmp[33];
    int pos = 0;
    do
    {
        unsigned int digit = v % radix;
        tmp[pos++] = digit < 10 ? L'0' + digit : L'a' + digit - 10;
        v /= radix;
    } while (v > 0);
    if (value < 0 && radix == 10)
    {
        tmp[pos++] = L'-';
    }
    int j = 0;
    while (pos > 0)
    {
        buffer[j++] = tmp[--pos];
    }
    buffer[j] = L'\0';
    return buffer;
}
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifndef LOWORD
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l) ((WORD)((DWORD_PTR)(l) >> 16))
#endif
#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xff))
#endif
#ifndef HIBYTE
#define HIBYTE(w) ((BYTE)((DWORD_PTR)(w) >> 8))
#endif

#define MAKEINTRESOURCEA(i) ((char *)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCEW(i) ((wchar_t *)((ULONG_PTR)((WORD)(i))))
#ifdef UNICODE
#define MAKEINTRESOURCE MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#endif

#define UNREFERENCED_PARAMETER(p) (void)(p)

struct CRITICAL_SECTION
{
    // Real backing storage (an opaque pthread_mutex_t, sized to fit on every
    // platform this ships on) - nothing outside WindowsTypes.cpp reads these
    // fields by name, so there's no compatibility reason to match the real
    // Win32 layout here.
    void *debug; // pthread_mutex_t*, allocated in InitializeCriticalSection
    LONG lockCount;
    LONG recursionCount;
    HANDLE owningThread;
    HANDLE lockSemaphore;
    ULONG_PTR spinCount;
};

void InitializeCriticalSection(CRITICAL_SECTION *cs);
void InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *cs, DWORD spinCount);
void DeleteCriticalSection(CRITICAL_SECTION *cs);
void EnterCriticalSection(CRITICAL_SECTION *cs);
BOOL TryEnterCriticalSection(CRITICAL_SECTION *cs);
void LeaveCriticalSection(CRITICAL_SECTION *cs);

DWORD TlsAlloc();
LPVOID TlsGetValue(DWORD dwTlsIndex);
BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);
BOOL TlsFree(DWORD dwTlsIndex);

DWORD GetLastError();
DWORD GetCurrentThreadId();
HANDLE GetCurrentThread();
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD ResumeThread(HANDLE hThread);
BOOL SetThreadPriority(HANDLE hThread, int nPriority);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);
void Sleep(DWORD dwMilliseconds);
HANDLE CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
#ifdef UNICODE
#define CreateEvent CreateEventW
#else
#define CreateEvent CreateEventA
#endif
BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);
DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#ifdef UNICODE
#define CreateFile CreateFileW
#else
#define CreateFile CreateFileA
#endif
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL CloseHandle(HANDLE hObject);

#include <chrono>
#include <ctime>

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
    lpFrequency->QuadPart = 1000000000;
    return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER *lpCount)
{
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    lpCount->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();
    return TRUE;
}

inline void GetSystemTime(SYSTEMTIME *lpSystemTime)
{
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm gmt;
    gmtime_r(&tt, &gmt);
    lpSystemTime->wYear = gmt.tm_year + 1900;
    lpSystemTime->wMonth = gmt.tm_mon + 1;
    lpSystemTime->wDayOfWeek = gmt.tm_wday;
    lpSystemTime->wDay = gmt.tm_mday;
    lpSystemTime->wHour = gmt.tm_hour;
    lpSystemTime->wMinute = gmt.tm_min;
    lpSystemTime->wSecond = gmt.tm_sec;
    lpSystemTime->wMilliseconds = 0;
}

inline BOOL FileTimeToSystemTime(const FILETIME *lpFileTime, SYSTEMTIME *lpSystemTime)
{
    uint64_t ticks = ((uint64_t)lpFileTime->dwHighDateTime << 32) | lpFileTime->dwLowDateTime;
    // FILETIME is 100ns intervals since 1601-01-01; time_t is seconds since 1970-01-01.
    const uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
    time_t tt = (time_t)((ticks - EPOCH_DIFF_100NS) / 10000000ULL);
    struct tm gmt;
    gmtime_r(&tt, &gmt);
    lpSystemTime->wYear = gmt.tm_year + 1900;
    lpSystemTime->wMonth = gmt.tm_mon + 1;
    lpSystemTime->wDayOfWeek = gmt.tm_wday;
    lpSystemTime->wDay = gmt.tm_mday;
    lpSystemTime->wHour = gmt.tm_hour;
    lpSystemTime->wMinute = gmt.tm_min;
    lpSystemTime->wSecond = gmt.tm_sec;
    lpSystemTime->wMilliseconds = 0;
    return TRUE;
}

#define INVALID_SET_FILE_POINTER ((DWORD) - 1)

inline HANDLE GetModuleHandle(const char *lpModuleName)
{
    return nullptr;
}

inline void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
    lpBuffer->dwLength = sizeof(MEMORYSTATUS);
    lpBuffer->dwMemoryLoad = 0;
    lpBuffer->dwTotalPhys = 2ULL * 1024 * 1024 * 1024;
    lpBuffer->dwAvailPhys = 1ULL * 1024 * 1024 * 1024;
    lpBuffer->dwTotalPageFile = 2ULL * 1024 * 1024 * 1024;
    lpBuffer->dwAvailPageFile = 1ULL * 1024 * 1024 * 1024;
    lpBuffer->dwTotalVirtual = 2ULL * 1024 * 1024 * 1024;
    lpBuffer->dwAvailVirtual = 1ULL * 1024 * 1024 * 1024;
    lpBuffer->dwAvailExtendedVirtual = 0;
}
