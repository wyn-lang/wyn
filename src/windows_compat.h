#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#ifdef _WIN32
// Windows compatibility layer for POSIX functions

#include <windows.h>
#include <direct.h>
#include <io.h>
#include <process.h>

// Map POSIX functions to Windows equivalents
#define getcwd _getcwd
#define chdir _chdir
#define mkdir(path) _mkdir(path)
#define rmdir _rmdir
#define unlink _unlink
#define access _access
#define getpid _getpid

// POSIX constants
#define F_OK 0
#define W_OK 2
#define R_OK 4

// pthread compatibility (minimal)
typedef HANDLE pthread_t;
typedef void* pthread_attr_t;

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
                                 void *(*start_routine)(void*), void *arg) {
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
    return (*thread == NULL) ? -1 : 0;
}

static inline int pthread_join(pthread_t thread, void **retval) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

#else
// Unix/POSIX systems
#include <unistd.h>
#include <pthread.h>
#endif

#endif // WINDOWS_COMPAT_H
