#ifndef OS_BASE_H
#define OS_BASE_H

#define OS_RESERVE_MEMORY_DEFINE(name) void* name(size_t ReserveSize)
#define OS_COMMIT_MEMORY_DEFINE(name) void* name(void* BaseAddress, size_t CommitSize)
#define OS_DECOMMIT_MEMORY_DEFINE(name) void name(void* BaseAddress, size_t DecommitSize)
#define OS_RELEASE_MEMORY_DEFINE(name) void name(void* BaseAddress, size_t ReleaseSize)

typedef OS_RESERVE_MEMORY_DEFINE(os_reserve_memory_func);
typedef OS_COMMIT_MEMORY_DEFINE(os_commit_memory_func);
typedef OS_DECOMMIT_MEMORY_DEFINE(os_decommit_memory_func);
typedef OS_RELEASE_MEMORY_DEFINE(os_release_memory_func);

#define OS_QUERY_PERFORMANCE_DEFINE(name) u64 name()
typedef OS_QUERY_PERFORMANCE_DEFINE(os_query_performance_func);

#define OS_OPEN_FILE_DEFINE(name) os_file* name(string Path, os_file_attribute_flags Attributes)
#define OS_GET_FILE_SIZE_DEFINE(name) u64 name(os_file* File)
#define OS_READ_FILE_DEFINE(name) b32 name(os_file* File, void* Data, size_t ReadSize)
#define OS_WRITE_FILE_DEFINE(name) b32 name(os_file* File, const void* Data, size_t WriteSize)
#define OS_CLOSE_FILE_DEFINE(name) void name(os_file* File)

#define OS_GET_ALL_FILES_DEFINE(name) string_array name(allocator* Allocator, string Path, b32 Recursive)
#define OS_IS_PATH_DEFINE(name) b32 name(string Path)
#define OS_MAKE_DIRECTORY_DEFINE(name) b32 name(string Directory)
#define OS_COPY_FILE_DEFINE(name) b32 name(string SrcFilePath, string DstFilePath)

enum {
	OS_FILE_ATTRIBUTE_NONE,
	OS_FILE_ATTRIBUTE_READ = (1 << 0),
	OS_FILE_ATTRIBUTE_WRITE = (1 << 1)
};
typedef u32 os_file_attribute_flags;

typedef struct os_file os_file;
typedef OS_OPEN_FILE_DEFINE(os_open_file_func);
typedef OS_GET_FILE_SIZE_DEFINE(os_get_file_size_func);
typedef OS_READ_FILE_DEFINE(os_read_file_func);
typedef OS_WRITE_FILE_DEFINE(os_write_file_func);
typedef OS_CLOSE_FILE_DEFINE(os_close_file_func);

typedef OS_GET_ALL_FILES_DEFINE(os_get_all_files_func);
typedef OS_IS_PATH_DEFINE(os_is_path_func);
typedef OS_MAKE_DIRECTORY_DEFINE(os_make_directory_func);
typedef OS_COPY_FILE_DEFINE(os_copy_file_func);

typedef struct os_tls os_tls;
#define OS_TLS_CREATE_DEFINE(name) os_tls* name()
#define OS_TLS_DELETE_DEFINE(name) void name(os_tls* TLS)
#define OS_TLS_GET_DEFINE(name) void* name(os_tls* TLS)
#define OS_TLS_SET_DEFINE(name) void name(os_tls* TLS, void* Data)

typedef OS_TLS_CREATE_DEFINE(os_tls_create_func);
typedef OS_TLS_DELETE_DEFINE(os_tls_delete_func);
typedef OS_TLS_GET_DEFINE(os_tls_get_func);
typedef OS_TLS_SET_DEFINE(os_tls_set_func);

typedef struct os_thread os_thread;
#define OS_THREAD_CALLBACK_DEFINE(name) void name(os_thread* Thread, void* UserData)
typedef OS_THREAD_CALLBACK_DEFINE(os_thread_callback_func);

#define OS_THREAD_CREATE_DEFINE(name) os_thread* name(os_thread_callback_func* Callback, void* UserData)
#define OS_THREAD_JOIN_DEFINE(name) void name(os_thread* Thread)
#define OS_THREAD_GET_ID_DEFINE(name) u64 name(os_thread* Thread)
#define OS_GET_CURRENT_THREAD_ID_DEFINE(name) u64 name()

typedef OS_THREAD_CREATE_DEFINE(os_thread_create_func);
typedef OS_THREAD_JOIN_DEFINE(os_thread_join_func);
typedef OS_THREAD_GET_ID_DEFINE(os_thread_get_id_func);
typedef OS_GET_CURRENT_THREAD_ID_DEFINE(os_get_current_thread_id_func);

typedef struct os_mutex os_mutex;
#define OS_MUTEX_CREATE_DEFINE(name) os_mutex* name()
#define OS_MUTEX_DELETE_DEFINE(name) void name(os_mutex* Mutex)
#define OS_MUTEX_LOCK_DEFINE(name) void name(os_mutex* Mutex)

typedef OS_MUTEX_CREATE_DEFINE(os_mutex_create_func);
typedef OS_MUTEX_DELETE_DEFINE(os_mutex_delete_func);
typedef OS_MUTEX_LOCK_DEFINE(os_mutex_lock_func);

typedef struct os_rw_mutex os_rw_mutex;
#define OS_RW_MUTEX_CREATE_DEFINE(name) os_rw_mutex* name()
#define OS_RW_MUTEX_DELETE_DEFINE(name) void name(os_rw_mutex* Mutex)
#define OS_RW_MUTEX_LOCK_DEFINE(name) void name(os_rw_mutex* Mutex)

typedef OS_RW_MUTEX_CREATE_DEFINE(os_rw_mutex_create_func);
typedef OS_RW_MUTEX_DELETE_DEFINE(os_rw_mutex_delete_func);
typedef OS_RW_MUTEX_LOCK_DEFINE(os_rw_mutex_lock_func);

typedef struct os_semaphore os_semaphore;
#define OS_SEMAPHORE_CREATE_DEFINE(name) os_semaphore* name(u32 InitialCount)
#define OS_SEMAPHORE_DELETE_DEFINE(name) void name(os_semaphore* Semaphore)
#define OS_SEMAPHORE_INCREMENT_DEFINE(name) void name(os_semaphore* Semaphore)
#define OS_SEMAPHORE_DECREMENT_DEFINE(name) void name(os_semaphore* Semaphore)
#define OS_SEMAPHORE_ADD_DEFINE(name) void name(os_semaphore* Semaphore, s32 Count)

typedef OS_SEMAPHORE_CREATE_DEFINE(os_semaphore_create_func);
typedef OS_SEMAPHORE_DELETE_DEFINE(os_semaphore_delete_func);
typedef OS_SEMAPHORE_INCREMENT_DEFINE(os_semaphore_increment_func);
typedef OS_SEMAPHORE_DECREMENT_DEFINE(os_semaphore_decrement_func);
typedef OS_SEMAPHORE_ADD_DEFINE(os_semaphore_add_func);

typedef struct os_event os_event;
#define OS_EVENT_CREATE_DEFINE(name) os_event* name()
#define OS_EVENT_DELETE_DEFINE(name) void name(os_event* Event)
#define OS_EVENT_RESET_DEFINE(name) void name(os_event* Event)
#define OS_EVENT_WAIT_DEFINE(name) void name(os_event* Event)
#define OS_EVENT_SIGNAL_DEFINE(name) void name(os_event* Event)

typedef OS_EVENT_CREATE_DEFINE(os_event_create_func);
typedef OS_EVENT_DELETE_DEFINE(os_event_delete_func);
typedef OS_EVENT_RESET_DEFINE(os_event_reset_func);
typedef OS_EVENT_WAIT_DEFINE(os_event_wait_func);
typedef OS_EVENT_SIGNAL_DEFINE(os_event_signal_func);

typedef struct os_hot_reload os_hot_reload;
#define OS_HOT_RELOAD_CREATE_DEFINE(name) os_hot_reload* name(string FilePath)
#define OS_HOT_RELOAD_DELETE_DEFINE(name) void name(os_hot_reload* HotReload)
#define OS_HOT_RELOAD_HAS_RELOADED_DEFINE(name) b32 name(os_hot_reload* HotReload)

typedef OS_HOT_RELOAD_CREATE_DEFINE(os_hot_reload_create_func);
typedef OS_HOT_RELOAD_DELETE_DEFINE(os_hot_reload_delete_func);
typedef OS_HOT_RELOAD_HAS_RELOADED_DEFINE(os_hot_reload_has_reloaded_func);

typedef struct os_library os_library;
#define OS_LIBRARY_CREATE_DEFINE(name) os_library* name(string LibraryPath)
#define OS_LIBRARY_DELETE_DEFINE(name) void name(os_library* Library)
#define OS_LIBRARY_GET_FUNCTION_DEFINE(name) void* name(os_library* Library, const char* FunctionName)

typedef OS_LIBRARY_CREATE_DEFINE(os_library_create_func);
typedef OS_LIBRARY_DELETE_DEFINE(os_library_delete_func);
typedef OS_LIBRARY_GET_FUNCTION_DEFINE(os_library_get_function_func);

#define OS_GET_ENTROPY_DEFINE(name) void name(void* Buffer, size_t Size)
typedef OS_GET_ENTROPY_DEFINE(os_get_entropy_func);

#define OS_SLEEP_DEFINE(name) void name(u64 Nanoseconds)
typedef OS_SLEEP_DEFINE(os_sleep_func);

typedef struct {
	os_reserve_memory_func* ReserveMemoryFunc;
	os_commit_memory_func* CommitMemoryFunc;
	os_decommit_memory_func* DecommitMemoryFunc;
	os_release_memory_func* ReleaseMemoryFunc;

	os_query_performance_func* QueryPerformanceCounterFunc;
	os_query_performance_func* QueryPerformanceFrequencyFunc;

	os_open_file_func* OpenFileFunc;
	os_get_file_size_func* GetFileSizeFunc;
	os_read_file_func* ReadFileFunc;
	os_write_file_func* WriteFileFunc;
	os_close_file_func* CloseFileFunc;

	os_get_all_files_func*  GetAllFilesFunc;
	os_is_path_func*        IsDirectoryPathFunc;
	os_is_path_func*        IsFilePathFunc;
	os_make_directory_func* MakeDirectoryFunc;
	os_copy_file_func*      CopyFileFunc;

	os_tls_create_func* TLSCreateFunc;
	os_tls_delete_func* TLSDeleteFunc;
	os_tls_get_func*    TLSGetFunc;
	os_tls_set_func*    TLSSetFunc;

	os_thread_create_func* ThreadCreateFunc;
	os_thread_join_func* ThreadJoinFunc;
	os_thread_get_id_func* ThreadGetIdFunc;
	os_get_current_thread_id_func* GetCurrentThreadIdFunc;

	os_mutex_create_func* MutexCreateFunc;
	os_mutex_delete_func* MutexDeleteFunc;
	os_mutex_lock_func* MutexLockFunc;
	os_mutex_lock_func* MutexUnlockFunc;

	os_rw_mutex_create_func* RWMutexCreateFunc;
	os_rw_mutex_delete_func* RWMutexDeleteFunc;
	os_rw_mutex_lock_func* RWMutexReadLockFunc;
	os_rw_mutex_lock_func* RWMutexReadUnlockFunc;
	os_rw_mutex_lock_func* RWMutexWriteLockFunc;
	os_rw_mutex_lock_func* RWMutexWriteUnlockFunc;

	os_semaphore_create_func* SemaphoreCreateFunc;
	os_semaphore_delete_func* SemaphoreDeleteFunc;
	os_semaphore_increment_func* SemaphoreIncrementFunc;
	os_semaphore_decrement_func* SemaphoreDecrementFunc;
	os_semaphore_add_func* SemaphoreAddFunc;

	os_event_create_func* EventCreateFunc;
	os_event_delete_func* EventDeleteFunc;
	os_event_reset_func* EventResetFunc;
	os_event_wait_func* EventWaitFunc;
	os_event_signal_func* EventSignalFunc;

	os_hot_reload_create_func* 		 HotReloadCreateFunc;
	os_hot_reload_delete_func* 		 HotReloadDeleteFunc;
	os_hot_reload_has_reloaded_func* HotReloadHasReloadedFunc;

	os_library_create_func* 	  LibraryCreateFunc;
	os_library_delete_func* 	  LibraryDeleteFunc;
	os_library_get_function_func* LibraryGetFunctionFunc;

	os_get_entropy_func* GetEntropyFunc;
	os_sleep_func* SleepFunc;
} os_base_vtable;

typedef struct {
	os_base_vtable* VTable;
	size_t PageSize;
	size_t ProcessorThreadCount;
	string ProgramPath;
} os_base;

#define OS_Program_Path() (Base_Get()->OSBase->ProgramPath)
#define OS_Processor_Thread_Count() ((u32)(Base_Get()->OSBase->ProcessorThreadCount))

#define OS_Page_Size() (Base_Get()->OSBase->PageSize)
#define OS_Reserve_Memory(reserve_size) Base_Get()->OSBase->VTable->ReserveMemoryFunc(reserve_size)
#define OS_Commit_Memory(base_address, commit_size) Base_Get()->OSBase->VTable->CommitMemoryFunc(base_address, commit_size)
#define OS_Decommit_Memory(base_address, decommit_size) Base_Get()->OSBase->VTable->DecommitMemoryFunc(base_address, decommit_size)
#define OS_Release_Memory(base_address, release_size) Base_Get()->OSBase->VTable->ReleaseMemoryFunc(base_address, release_size)

#define OS_Query_Performance_Counter() Base_Get()->OSBase->VTable->QueryPerformanceCounterFunc()
#define OS_Query_Performance_Frequency() Base_Get()->OSBase->VTable->QueryPerformanceFrequencyFunc()

#define OS_Open_File(path, attribs) Base_Get()->OSBase->VTable->OpenFileFunc(path, attribs)
#define OS_Get_File_Size(file) Base_Get()->OSBase->VTable->GetFileSizeFunc(file)
#define OS_Read_File(file, data, size) Base_Get()->OSBase->VTable->ReadFileFunc(file, data, size)
#define OS_Write_File(file, data, size) Base_Get()->OSBase->VTable->WriteFileFunc(file, data, size)
#define OS_Close_File(file) Base_Get()->OSBase->VTable->CloseFileFunc(file)

#define OS_Get_All_Files(allocator, path, recursive) Base_Get()->OSBase->VTable->GetAllFilesFunc(allocator, path, recursive)
#define OS_Is_Directory_Path(path) Base_Get()->OSBase->VTable->IsDirectoryPathFunc(path)
#define OS_Is_File_Path(path) Base_Get()->OSBase->VTable->IsFilePathFunc(path)
#define OS_Make_Directory(path) Base_Get()->OSBase->VTable->MakeDirectoryFunc(path)
#define OS_Copy_File(src_file, dst_file) Base_Get()->OSBase->VTable->CopyFileFunc(src_file, dst_file)

#define OS_TLS_Create() Base_Get()->OSBase->VTable->TLSCreateFunc()
#define OS_TLS_Delete(tls) Base_Get()->OSBase->VTable->TLSDeleteFunc(tls)
#define OS_TLS_Get(tls) Base_Get()->OSBase->VTable->TLSGetFunc(tls)
#define OS_TLS_Set(tls, data) Base_Get()->OSBase->VTable->TLSSetFunc(tls, data)

#define OS_Thread_Create(callback, user_data) Base_Get()->OSBase->VTable->ThreadCreateFunc(callback, user_data)
#define OS_Thread_Join(thread) Base_Get()->OSBase->VTable->ThreadJoinFunc(thread)
#define OS_Thread_Get_ID(thread) Base_Get()->OSBase->VTable->ThreadGetIdFunc(thread)
#define OS_Get_Current_Thread_ID() Base_Get()->OSBase->VTable->GetCurrentThreadIdFunc()

#define OS_Mutex_Create() Base_Get()->OSBase->VTable->MutexCreateFunc()
#define OS_Mutex_Delete(mutex) Base_Get()->OSBase->VTable->MutexDeleteFunc(mutex)
#define OS_Mutex_Lock(mutex) Base_Get()->OSBase->VTable->MutexLockFunc(mutex)
#define OS_Mutex_Unlock(mutex) Base_Get()->OSBase->VTable->MutexUnlockFunc(mutex)

#define OS_RW_Mutex_Create() Base_Get()->OSBase->VTable->RWMutexCreateFunc()
#define OS_RW_Mutex_Delete(mutex) Base_Get()->OSBase->VTable->RWMutexDeleteFunc(mutex)
#define OS_RW_Mutex_Read_Lock(mutex) Base_Get()->OSBase->VTable->RWMutexReadLockFunc(mutex)
#define OS_RW_Mutex_Read_Unlock(mutex) Base_Get()->OSBase->VTable->RWMutexReadUnlockFunc(mutex)
#define OS_RW_Mutex_Write_Lock(mutex) Base_Get()->OSBase->VTable->RWMutexWriteLockFunc(mutex)
#define OS_RW_Mutex_Write_Unlock(mutex) Base_Get()->OSBase->VTable->RWMutexWriteUnlockFunc(mutex)

#define OS_Semaphore_Create(initial_count) Base_Get()->OSBase->VTable->SemaphoreCreateFunc(initial_count)
#define OS_Semaphore_Delete(semaphore) Base_Get()->OSBase->VTable->SemaphoreDeleteFunc(semaphore)
#define OS_Semaphore_Increment(semaphore) Base_Get()->OSBase->VTable->SemaphoreIncrementFunc(semaphore)
#define OS_Semaphore_Decrement(semaphore) Base_Get()->OSBase->VTable->SemaphoreDecrementFunc(semaphore)
#define OS_Semaphore_Add(semaphore, count) Base_Get()->OSBase->VTable->SemaphoreAddFunc(semaphore, count)

#define OS_Event_Create() Base_Get()->OSBase->VTable->EventCreateFunc()
#define OS_Event_Delete(event) Base_Get()->OSBase->VTable->EventDeleteFunc(event)
#define OS_Event_Reset(event) Base_Get()->OSBase->VTable->EventResetFunc(event)
#define OS_Event_Wait(event) Base_Get()->OSBase->VTable->EventWaitFunc(event)
#define OS_Event_Signal(event) Base_Get()->OSBase->VTable->EventSignalFunc(event)

#define OS_Hot_Reload_Create(path) Base_Get()->OSBase->VTable->HotReloadCreateFunc(path)
#define OS_Hot_Reload_Delete(reload) Base_Get()->OSBase->VTable->HotReloadDeleteFunc(reload)
#define OS_Hot_Reload_Has_Reloaded(reload) Base_Get()->OSBase->VTable->HotReloadHasReloadedFunc(reload)

#define OS_Library_Create(path) Base_Get()->OSBase->VTable->LibraryCreateFunc(path)
#define OS_Library_Delete(library) Base_Get()->OSBase->VTable->LibraryDeleteFunc(library)
#define OS_Library_Get_Function(library, name) Base_Get()->OSBase->VTable->LibraryGetFunctionFunc(library, name)

#define OS_Get_Entropy(buffer, size) Base_Get()->OSBase->VTable->GetEntropyFunc(buffer, size)
#define OS_Sleep(nanoseconds) Base_Get()->OSBase->VTable->SleepFunc(nanoseconds)

#endif