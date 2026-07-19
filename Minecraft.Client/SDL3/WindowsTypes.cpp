#include "WindowsTypes.h"
#include <chrono>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>

// TLS implementation using pthreads
static pthread_key_t s_tlsKeys[64];
static bool s_tlsKeysInit[64] = {};

static bool EnsureTlsKey(DWORD index)
{
    if (index >= 64)
    {
        return false;
    }
    if (!s_tlsKeysInit[index])
    {
        pthread_key_create(&s_tlsKeys[index], NULL);
        s_tlsKeysInit[index] = true;
    }
    return true;
}

static pthread_mutex_t *GetOrCreateMutex(CRITICAL_SECTION *cs)
{
    if (cs->debug == NULL)
    {
        pthread_mutex_t *mutex = new pthread_mutex_t;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        // Win32 critical sections are recursive (same thread can re-Enter without
        // deadlocking itself) - the engine relies on that in a few places.
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        cs->debug = mutex;
    }
    return (pthread_mutex_t *)cs->debug;
}

void InitializeCriticalSection(CRITICAL_SECTION *cs)
{
    memset(cs, 0, sizeof(*cs));
    GetOrCreateMutex(cs);
}

void InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *cs, DWORD spinCount)
{
    memset(cs, 0, sizeof(*cs));
    cs->spinCount = spinCount;
    GetOrCreateMutex(cs);
}
void DeleteCriticalSection(CRITICAL_SECTION *cs)
{
    if (cs->debug != NULL)
    {
        pthread_mutex_destroy((pthread_mutex_t *)cs->debug);
        delete (pthread_mutex_t *)cs->debug;
        cs->debug = NULL;
    }
}

void EnterCriticalSection(CRITICAL_SECTION *cs)
{
    pthread_mutex_lock(GetOrCreateMutex(cs));
}

BOOL TryEnterCriticalSection(CRITICAL_SECTION *cs)
{
    return pthread_mutex_trylock(GetOrCreateMutex(cs)) == 0 ? TRUE : FALSE;
}

void LeaveCriticalSection(CRITICAL_SECTION *cs)
{
    pthread_mutex_unlock(GetOrCreateMutex(cs));
}

DWORD TlsAlloc()
{
    static DWORD nextKey = 1;
    DWORD key = nextKey++;
    EnsureTlsKey(key);
    return key;
}

LPVOID TlsGetValue(DWORD dwTlsIndex)
{
    if (!EnsureTlsKey(dwTlsIndex))
    {
        return NULL;
    }
    return pthread_getspecific(s_tlsKeys[dwTlsIndex]);
}

BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
    if (!EnsureTlsKey(dwTlsIndex))
    {
        return FALSE;
    }
    return pthread_setspecific(s_tlsKeys[dwTlsIndex], lpTlsValue) == 0 ? TRUE : FALSE;
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
    return (DWORD)(uintptr_t)pthread_self();
}

HANDLE GetCurrentThread()
{
    return (HANDLE)(uintptr_t)1;
}

// HANDLE objects for threads/events are heap-allocated Win32Object*s tagged by
// kind, so WaitForSingleObject/WaitForMultipleObjects/CloseHandle can dispatch
// without callers needing to know which kind they're holding (matching how
// Win32 HANDLE works).
enum class Win32ObjectKind
{
    Thread,
    Event,
    File
};

struct Win32Object
{
    Win32ObjectKind kind;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    // File fields
    std::fstream *fileStream;

    // Thread fields
    pthread_t thread;
    LPTHREAD_START_ROUTINE startAddress;
    LPVOID parameter;
    DWORD exitCode;
    bool started;  // pthread_create has been called
    bool runnable; // false while CREATE_SUSPENDED and not yet Resumed
    bool finished;
    bool joined;

    // Event fields
    bool manualReset;
    bool signaled;
};

static void *ThreadTrampoline(void *param)
{
    Win32Object *obj = (Win32Object *)param;

    pthread_mutex_lock(&obj->mutex);
    while (!obj->runnable)
    {
        pthread_cond_wait(&obj->cond, &obj->mutex);
    }
    pthread_mutex_unlock(&obj->mutex);

    DWORD result = obj->startAddress(obj->parameter);

    pthread_mutex_lock(&obj->mutex);
    obj->exitCode = result;
    obj->finished = true;
    pthread_cond_broadcast(&obj->cond);
    pthread_mutex_unlock(&obj->mutex);

    return NULL;
}

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
    Win32Object *obj = new Win32Object();
    obj->kind = Win32ObjectKind::Thread;
    pthread_mutex_init(&obj->mutex, NULL);
    pthread_cond_init(&obj->cond, NULL);
    obj->startAddress = lpStartAddress;
    obj->parameter = lpParameter;
    obj->exitCode = STILL_ACTIVE;
    obj->started = false;
    obj->runnable = (dwCreationFlags & CREATE_SUSPENDED) == 0;
    obj->finished = false;
    obj->joined = false;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (dwStackSize > 0)
    {
        // pthreads enforces a platform minimum internally, so this is safe
        // even for C4JThread's smallest requested stack sizes.
        pthread_attr_setstacksize(&attr, dwStackSize);
    }

    int rc = pthread_create(&obj->thread, &attr, ThreadTrampoline, obj);
    pthread_attr_destroy(&attr);

    if (rc != 0)
    {
        pthread_mutex_destroy(&obj->mutex);
        pthread_cond_destroy(&obj->cond);
        delete obj;
        return NULL;
    }

    obj->started = true;
    if (lpThreadId != NULL)
    {
        *lpThreadId = (DWORD)(uintptr_t)obj->thread;
    }
    return (HANDLE)obj;
}

DWORD ResumeThread(HANDLE hThread)
{
    Win32Object *obj = (Win32Object *)hThread;
    if (obj == NULL)
    {
        return (DWORD)-1;
    }

    pthread_mutex_lock(&obj->mutex);
    bool wasSuspended = !obj->runnable;
    obj->runnable = true;
    pthread_cond_broadcast(&obj->cond);
    pthread_mutex_unlock(&obj->mutex);

    return wasSuspended ? 1 : 0;
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{
    return TRUE;
}

static DWORD WaitOnThread(Win32Object *obj, DWORD dwMilliseconds)
{
    pthread_mutex_lock(&obj->mutex);
    if (dwMilliseconds == INFINITE)
    {
        while (!obj->finished)
        {
            pthread_cond_wait(&obj->cond, &obj->mutex);
        }
    }
    else
    {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += dwMilliseconds / 1000;
        deadline.tv_nsec += (dwMilliseconds % 1000) * 1000000;
        if (deadline.tv_nsec >= 1000000000)
        {
            deadline.tv_sec += 1;
            deadline.tv_nsec -= 1000000000;
        }
        while (!obj->finished)
        {
            if (pthread_cond_timedwait(&obj->cond, &obj->mutex, &deadline) != 0)
            {
                break;
            }
        }
    }
    bool finished = obj->finished;
    if (finished && !obj->joined)
    {
        obj->joined = true;
        pthread_mutex_unlock(&obj->mutex);
        pthread_join(obj->thread, NULL);
        pthread_mutex_lock(&obj->mutex);
    }
    pthread_mutex_unlock(&obj->mutex);
    return finished ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}

static DWORD WaitOnEvent(Win32Object *obj, DWORD dwMilliseconds)
{
    pthread_mutex_lock(&obj->mutex);
    DWORD result = WAIT_OBJECT_0;
    if (dwMilliseconds == INFINITE)
    {
        while (!obj->signaled)
        {
            pthread_cond_wait(&obj->cond, &obj->mutex);
        }
    }
    else
    {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += dwMilliseconds / 1000;
        deadline.tv_nsec += (dwMilliseconds % 1000) * 1000000;
        if (deadline.tv_nsec >= 1000000000)
        {
            deadline.tv_sec += 1;
            deadline.tv_nsec -= 1000000000;
        }
        while (!obj->signaled)
        {
            if (pthread_cond_timedwait(&obj->cond, &obj->mutex, &deadline) != 0)
            {
                result = obj->signaled ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
                break;
            }
        }
    }
    if (obj->signaled && !obj->manualReset)
    {
        obj->signaled = false; // auto-reset: exactly one waiter gets released
    }
    pthread_mutex_unlock(&obj->mutex);
    return result;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    Win32Object *obj = (Win32Object *)hHandle;
    if (obj == NULL)
    {
        return WAIT_OBJECT_0;
    }
    return obj->kind == Win32ObjectKind::Thread ? WaitOnThread(obj, dwMilliseconds) : WaitOnEvent(obj, dwMilliseconds);
}

BOOL GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode)
{
    Win32Object *obj = (Win32Object *)hThread;
    if (obj == NULL)
    {
        *lpExitCode = STILL_ACTIVE;
        return TRUE;
    }
    pthread_mutex_lock(&obj->mutex);
    *lpExitCode = obj->finished ? obj->exitCode : STILL_ACTIVE;
    pthread_mutex_unlock(&obj->mutex);
    return TRUE;
}

void Sleep(DWORD dwMilliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}
static HANDLE CreateEventCommon(BOOL bManualReset, BOOL bInitialState)
{
    Win32Object *obj = new Win32Object();
    obj->kind = Win32ObjectKind::Event;
    pthread_mutex_init(&obj->mutex, NULL);
    pthread_cond_init(&obj->cond, NULL);
    obj->manualReset = bManualReset != FALSE;
    obj->signaled = bInitialState != FALSE;
    return (HANDLE)obj;
}

HANDLE CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName)
{
    return CreateEventCommon(bManualReset, bInitialState);
}

HANDLE CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
{
    return CreateEventCommon(bManualReset, bInitialState);
}

BOOL SetEvent(HANDLE hEvent)
{
    Win32Object *obj = (Win32Object *)hEvent;
    if (obj == NULL)
    {
        return FALSE;
    }
    pthread_mutex_lock(&obj->mutex);
    obj->signaled = true;
    pthread_cond_broadcast(&obj->cond);
    pthread_mutex_unlock(&obj->mutex);
    return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
    Win32Object *obj = (Win32Object *)hEvent;
    if (obj == NULL)
    {
        return FALSE;
    }
    pthread_mutex_lock(&obj->mutex);
    obj->signaled = false;
    pthread_mutex_unlock(&obj->mutex);
    return TRUE;
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
    // Only the bWaitAll=TRUE/INFINITE case is exercised anywhere in this
    // codebase (ServerStoppedWait/ServerReadyWait use single-handle waits
    // elsewhere) - implement that case for real and fall back to a single
    // poll-then-return for anything fancier rather than guessing at exact
    // Win32 multi-wait semantics nothing here needs yet.
    if (bWaitAll)
    {
        for (DWORD i = 0; i < nCount; i++)
        {
            DWORD result = WaitForSingleObject(lpHandles[i], dwMilliseconds);
            if (result != WAIT_OBJECT_0)
            {
                return result;
            }
        }
        return WAIT_OBJECT_0;
    }

    for (DWORD i = 0; i < nCount; i++)
    {
        if (WaitForSingleObject(lpHandles[i], 0) == WAIT_OBJECT_0)
        {
            return WAIT_OBJECT_0 + i;
        }
    }
    return WAIT_TIMEOUT;
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
    Win32Object *obj = (Win32Object *)hFile;
    if (obj == NULL || obj->kind != Win32ObjectKind::File)
    {
        return INVALID_SET_FILE_POINTER;
    }

    std::ios::seekdir dir = std::ios::beg;
    if (dwMoveMethod == FILE_CURRENT)
    {
        dir = std::ios::cur;
    }
    else if (dwMoveMethod == FILE_END)
    {
        dir = std::ios::end;
    }

    std::fstream *fs = obj->fileStream;
    fs->clear(); // shed any lingering eof/fail from a prior read
    // Perform seekg (which must succeed for read access)
    fs->seekg(lDistanceToMove, dir);
    if (fs->fail())
    {
        return INVALID_SET_FILE_POINTER;
    }
    // Try seekp (which might fail on input-only files, in which case we clear the failbit)
    fs->seekp(lDistanceToMove, dir);
    if (fs->fail())
    {
        fs->clear();
    }
    return (DWORD)((uint64_t)fs->tellg() & 0xFFFFFFFF);
}

BOOL SetEndOfFile(HANDLE hFile)
{
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
    return TRUE;
}

// Opens a std::fstream honouring the Win32 access mode / creation disposition,
// and wraps it in a File-kind Win32Object so ReadFile/WriteFile/SetFilePointer/
// CloseHandle can operate on the returned HANDLE. Returns INVALID_HANDLE_VALUE
// if the file can't be opened as requested (e.g. OPEN_EXISTING on a missing
// file), matching Win32 CreateFile semantics closely enough for the callers
// here (FileInputStream/FileOutputStream).
static HANDLE OpenFileHandle(const std::string &path, DWORD dwDesiredAccess, DWORD dwCreationDisposition)
{
    const bool wantRead = (dwDesiredAccess & GENERIC_READ) != 0;
    const bool wantWrite = (dwDesiredAccess & GENERIC_WRITE) != 0;

    std::ios::openmode mode = std::ios::binary;
    if (wantRead)
    {
        mode |= std::ios::in;
    }
    if (wantWrite)
    {
        mode |= std::ios::out;
    }

    if (dwCreationDisposition == CREATE_ALWAYS)
    {
        // Always start from an empty file. trunc needs out.
        mode |= std::ios::out | std::ios::trunc;
    }

    std::fstream *fs = new std::fstream();
    fs->open(path.c_str(), mode);

    if (!fs->is_open() && dwCreationDisposition == OPEN_ALWAYS && wantWrite)
    {
        // OPEN_ALWAYS: create it if it didn't exist, then reopen for I/O.
        std::fstream create;
        create.open(path.c_str(), std::ios::out | std::ios::binary);
        create.close();
        fs->clear();
        fs->open(path.c_str(), mode);
    }

    if (!fs->is_open())
    {
        delete fs;
        return INVALID_HANDLE_VALUE;
    }

    Win32Object *obj = new Win32Object();
    obj->kind = Win32ObjectKind::File;
    obj->fileStream = fs;
    return (HANDLE)obj;
}

HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    // Narrow the wide path (and normalise Win32 backslashes to '/').
    std::string path;
    for (const wchar_t *p = lpFileName; p && *p; ++p)
    {
        path.push_back(*p == L'\\' ? '/' : (char)*p);
    }
    return OpenFileHandle(path, dwDesiredAccess, dwCreationDisposition);
}

HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    if (lpFileName == NULL)
    {
        return INVALID_HANDLE_VALUE;
    }
    std::string path(lpFileName);
    for (char &c : path)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return OpenFileHandle(path, dwDesiredAccess, dwCreationDisposition);
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    Win32Object *obj = (Win32Object *)hFile;
    if (obj == NULL || obj->kind != Win32ObjectKind::File)
    {
        if (lpNumberOfBytesRead)
        {
            *lpNumberOfBytesRead = 0;
        }
        return FALSE;
    }

    std::fstream *fs = obj->fileStream;
    fs->read((char *)lpBuffer, nNumberOfBytesToRead);
    std::streamsize got = fs->gcount();

    // A short read at end-of-file is success with fewer bytes (the caller
    // detects EOF via *lpNumberOfBytesRead == 0), not an error. Clear the
    // eof/fail bits so later seeks/reads on the same handle still work.
    const bool hardError = fs->bad();
    if (!fs->good())
    {
        fs->clear();
    }

    if (lpNumberOfBytesRead)
    {
        *lpNumberOfBytesRead = (DWORD)got;
    }
    return hardError ? FALSE : TRUE;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    Win32Object *obj = (Win32Object *)hFile;
    if (obj == NULL || obj->kind != Win32ObjectKind::File)
    {
        if (lpNumberOfBytesWritten)
        {
            *lpNumberOfBytesWritten = 0;
        }
        return FALSE;
    }

    std::fstream *fs = obj->fileStream;
    fs->write((const char *)lpBuffer, nNumberOfBytesToWrite);
    if (lpNumberOfBytesWritten)
    {
        *lpNumberOfBytesWritten = fs->good() ? nNumberOfBytesToWrite : 0;
    }
    return fs->good() ? TRUE : FALSE;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    Win32Object *obj = (Win32Object *)hFile;
    if (obj == NULL || obj->kind != Win32ObjectKind::File)
    {
        return 0;
    }

    std::fstream *fs = obj->fileStream;
    std::streampos cur = fs->tellg();
    fs->seekg(0, std::ios::end);
    std::streampos end = fs->tellg();
    fs->seekg(cur);

    if (lpFileSizeHigh)
    {
        *lpFileSizeHigh = (DWORD)((uint64_t)end >> 32);
    }
    return (DWORD)((uint64_t)end & 0xFFFFFFFF);
}

BOOL CloseHandle(HANDLE hObject)
{
    Win32Object *obj = (Win32Object *)hObject;
    if (obj == NULL || obj == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }

    if (obj->kind == Win32ObjectKind::File)
    {
        // File handles never allocate the pthread mutex/cond (only threads and
        // events do), so close and free just the stream here.
        delete obj->fileStream;
        delete obj;
        return TRUE;
    }

    if (obj->kind == Win32ObjectKind::Thread && obj->started && !obj->joined)
    {
        pthread_detach(obj->thread);
    }
    pthread_mutex_destroy(&obj->mutex);
    pthread_cond_destroy(&obj->cond);
    delete obj;
    return TRUE;
}
