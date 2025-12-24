#ifndef POSIX_BASE_H
#define POSIX_BASE_H

struct os_file {
    int Handle;
    os_file* Next;
};

struct os_tls {
    pthread_key_t Key;
    os_tls* Next;
};

struct os_thread {
    pthread_t Thread;
    os_thread_callback_func* Callback;
    void* UserData;
    os_thread* Next;
};

struct os_mutex {
    pthread_mutex_t Lock;
    os_mutex* Next;
};

struct os_rw_mutex {
    pthread_rwlock_t Lock;
    os_rw_mutex* Next;
};

#ifdef OS_OSX
struct os_semaphore {
    semaphore_t Handle;
    os_semaphore* Next;
};
#elif defined(OS_LINUX)
struct os_semaphore{
    sem_t Handle;
    os_semaphore* Next;
};
#else
#error "Not Implemented"
#endif

struct os_event {
    pthread_mutex_t Mutex;
    pthread_cond_t Cond;
    b32 IsSignaled;
    os_event* Next;
};

struct os_hot_reload {
	string 	       FilePath;
	time_t 	       LastWriteTime;
	os_hot_reload* Next;
};

struct os_library {
    void* Library;
    os_library* Next;
};

struct os_condition_variable {
    pthread_cond_t Handle;
    os_condition_variable* Next;
};

typedef struct {
    os_base Base;

    pthread_mutex_t ResourceLock;
    os_file* FreeFiles;
    os_tls* FreeTLS;
    os_thread* FreeThreads;
    os_mutex* FreeMutex;
    os_rw_mutex* FreeRWMutex;
    os_semaphore* FreeSemaphores;
    os_event* FreeEvents;
    os_hot_reload* FreeHotReload;
    os_library* FreeLibrary;
    os_condition_variable* FreeConditionVariables;


    os_rw_mutex ArenaLock;
} posix_base;

#endif