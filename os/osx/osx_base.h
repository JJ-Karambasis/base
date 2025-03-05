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

typedef struct {
    os_base Base;

    arena* ResourceArena;
    pthread_mutex_t ResourceLock;
    os_tls* FreeTLS;
    os_mutex* FreeMutex;
} osx_base;

#endif