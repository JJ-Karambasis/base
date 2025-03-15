#ifndef OSX_BASE_H
#define OSX_BASE_H

struct os_tls {
    pthread_key_t Key;
    os_tls* Next;
};

struct os_mutex {
    pthread_mutex_t Lock;
    os_mutex* Next;
};

struct os_rw_mutex {
    pthread_rwlock_t Lock;
    os_rw_mutex* Next;
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

typedef struct {
    os_base Base;

    arena* ResourceArena;
    pthread_mutex_t ResourceLock;
    os_tls* FreeTLS;
    os_mutex* FreeMutex;
    os_rw_mutex* FreeRWMutex;
    os_hot_reload* FreeHotReload;
    os_library* FreeLibrary;
} osx_base;

#endif