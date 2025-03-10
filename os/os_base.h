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

#define OS_READ_ENTIRE_FILE_DEFINE(name) buffer name(allocator* Allocator, string Path)
#define OS_WRITE_ENTIRE_FILE_DEFINE(name) b32 name(string Path, buffer Data)
#define OS_GET_ALL_FILES_DEFINE(name) string_array name(allocator* Allocator, string Path, b32 Recursive)
#define OS_IS_PATH_DEFINE(name) b32 name(string Path)
#define OS_MAKE_DIRECTORY_DEFINE(name) b32 name(string Directory)

typedef OS_READ_ENTIRE_FILE_DEFINE(os_read_entire_file_func);
typedef OS_WRITE_ENTIRE_FILE_DEFINE(os_write_entire_file_func);
typedef OS_GET_ALL_FILES_DEFINE(os_get_all_files_func);
typedef OS_IS_PATH_DEFINE(os_is_path_func);
typedef OS_MAKE_DIRECTORY_DEFINE(os_make_directory_func);

typedef struct os_tls os_tls;
#define OS_TLS_CREATE_DEFINE(name) os_tls* name()
#define OS_TLS_DELETE_DEFINE(name) void name(os_tls* TLS)
#define OS_TLS_GET_DEFINE(name) void* name(os_tls* TLS)
#define OS_TLS_SET_DEFINE(name) void name(os_tls* TLS, void* Data)

typedef OS_TLS_CREATE_DEFINE(os_tls_create_func);
typedef OS_TLS_DELETE_DEFINE(os_tls_delete_func);
typedef OS_TLS_GET_DEFINE(os_tls_get_func);
typedef OS_TLS_SET_DEFINE(os_tls_set_func);

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

typedef struct {
	os_reserve_memory_func* ReserveMemoryFunc;
	os_commit_memory_func* CommitMemoryFunc;
	os_decommit_memory_func* DecommitMemoryFunc;
	os_release_memory_func* ReleaseMemoryFunc;

	os_query_performance_func* QueryPerformanceCounterFunc;
	os_query_performance_func* QueryPerformanceFrequencyFunc;

	os_read_entire_file_func*  ReadEntireFileFunc;
	os_write_entire_file_func* WriteEntireFileFunc;
	os_get_all_files_func*     GetAllFilesFunc;
	os_is_path_func*           IsDirectoryPathFunc;
	os_is_path_func*           IsFilePathFunc;
	os_make_directory_func*    MakeDirectoryFunc;

	os_tls_create_func* TLSCreateFunc;
	os_tls_delete_func* TLSDeleteFunc;
	os_tls_get_func*    TLSGetFunc;
	os_tls_set_func*    TLSSetFunc;

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

	os_hot_reload_create_func* 		 HotReloadCreateFunc;
	os_hot_reload_delete_func* 		 HotReloadDeleteFunc;
	os_hot_reload_has_reloaded_func* HotReloadHasReloadedFunc;

	os_library_create_func* 	  LibraryCreateFunc;
	os_library_delete_func* 	  LibraryDeleteFunc;
	os_library_get_function_func* LibraryGetFunctionFunc;
} os_base_vtable;

typedef struct {
	os_base_vtable* VTable;
	size_t PageSize;
	string ProgramPath;
} os_base;

#define OS_Program_Path() (Base_Get()->OSBase->ProgramPath)

#define OS_Page_Size() (Base_Get()->OSBase->PageSize)
#define OS_Reserve_Memory(reserve_size) Base_Get()->OSBase->VTable->ReserveMemoryFunc(reserve_size)
#define OS_Commit_Memory(base_address, commit_size) Base_Get()->OSBase->VTable->CommitMemoryFunc(base_address, commit_size)
#define OS_Decommit_Memory(base_address, decommit_size) Base_Get()->OSBase->VTable->DecommitMemoryFunc(base_address, decommit_size)
#define OS_Release_Memory(base_address, release_size) Base_Get()->OSBase->VTable->ReleaseMemoryFunc(base_address, release_size)

#define OS_Query_Performance_Counter() Base_Get()->OSBase->VTable->QueryPerformanceCounterFunc()
#define OS_Query_Performance_Frequency() Base_Get()->OSBase->VTable->QueryPerformanceFrequencyFunc()

#define OS_Read_Entire_File(allocator, path) Base_Get()->OSBase->VTable->ReadEntireFileFunc(allocator, path)
#define OS_Write_Entire_File(path, buffer) Base_Get()->OSBase->VTable->WriteEntireFileFunc(path, buffer)
#define OS_Get_All_Files(allocator, path, recursive) Base_Get()->OSBase->VTable->GetAllFilesFunc(allocator, path, recursive)
#define OS_Is_Directory_Path(path) Base_Get()->OSBase->VTable->IsDirectoryPathFunc(path)
#define OS_Is_File_Path(path) Base_Get()->OSBase->VTable->IsFilePathFunc(path)
#define OS_Make_Directory(path) Base_Get()->OSBase->VTable->MakeDirectoryFunc(path)

#define OS_TLS_Create() Base_Get()->OSBase->VTable->TLSCreateFunc()
#define OS_TLS_Delete(tls) Base_Get()->OSBase->VTable->TLSDeleteFunc(tls)
#define OS_TLS_Get(tls) Base_Get()->OSBase->VTable->TLSGetFunc(tls)
#define OS_TLS_Set(tls, data) Base_Get()->OSBase->VTable->TLSSetFunc(tls, data)

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

#define OS_Hot_Reload_Create(path) Base_Get()->OSBase->VTable->HotReloadCreateFunc(path)
#define OS_Hot_Reload_Delete(reload) Base_Get()->OSBase->VTable->HotReloadDeleteFunc(reload)
#define OS_Hot_Reload_Has_Reloaded(reload) Base_Get()->OSBase->VTable->HotReloadHasReloadedFunc(reload)

#define OS_Library_Create(path) Base_Get()->OSBase->VTable->LibraryCreateFunc(path)
#define OS_Library_Delete(library) Base_Get()->OSBase->VTable->LibraryDeleteFunc(library)
#define OS_Library_Get_Function(library, name) Base_Get()->OSBase->VTable->LibraryGetFunctionFunc(library, name)

#define OS_Read_Entire_File_Str(allocator, path) String_From_Buffer(OS_Read_Entire_File(allocator, path))

#endif