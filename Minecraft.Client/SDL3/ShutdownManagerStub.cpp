#include "../PS3/PS3Extras/ShutdownManager.h"
#include "../stdafx.h"

// TODO: placeholder. ShutdownManager has no SDL3-specific header (other
// platforms hardcode "PS3/PS3Extras/ShutdownManager.h" as an #include even
// on non-PS3 builds, since the declarations are platform-agnostic aside from
// a couple of __PS3__-guarded members), but no implementation exists outside
// PS3Extras. These are no-op stand-ins - real shutdown coordination for
// worker threads (render/server/level update) isn't implemented yet.

void ShutdownManager::Initialise()
{
}

void ShutdownManager::StartShutdown()
{
}

void ShutdownManager::MainThreadHandleShutdown()
{
}

void ShutdownManager::HasStarted(EThreadId)
{
}

void ShutdownManager::HasStarted(EThreadId, C4JThread::EventArray *)
{
}

bool ShutdownManager::ShouldRun(EThreadId)
{
    return true;
}

void ShutdownManager::HasFinished(EThreadId)
{
}
