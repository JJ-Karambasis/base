#ifndef WIN32_BASE_H
#define WIN32_BASE_H

struct os_tls {
	DWORD Index;
	os_tls* Next;
};

struct os_thread {
	HANDLE Handle;
	DWORD ThreadID;
	os_thread_callback_func* Callback;
	void* UserData;
	os_thread* Next;
};

struct os_mutex {
	CRITICAL_SECTION CriticalSection;
	os_mutex* Next;
};

struct os_rw_mutex {
	SRWLOCK SRWLock;
	os_rw_mutex* Next;
};

struct os_semaphore {
	HANDLE Handle;
	os_semaphore* Next;
};

struct os_hot_reload {
	wstring 	   FilePath;
	FILETIME 	   LastWriteTime;
	os_hot_reload* Next;
};

struct os_library {
	HMODULE Library;
	os_library* Next;
};

typedef struct {
	os_base Base;

	arena* ResourceArena;
	CRITICAL_SECTION ResourceLock;
	os_tls* FreeTLS;
	os_thread* FreeThreads;
	os_mutex* FreeMutex;
	os_rw_mutex* FreeRWMutex;
	os_semaphore* FreeSemaphores;
	os_hot_reload* FreeHotReload;
	os_library* FreeLibrary;
} win32_base;

#endif