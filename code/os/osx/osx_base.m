#include <pthread.h>
#include <sys/mman.h>
#import <Foundation/Foundation.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <semaphore.h>

#include "../../base.h"
#include "osx_base.h"

#include "../../base.c"

global osx_base* G_OSX;
function osx_base* OSX_Get() {
	return G_OSX;
}

function OS_RESERVE_MEMORY_DEFINE(OSX_Reserve_Memory) {
    void* Result = mmap(NULL, ReserveSize, PROT_NONE, (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
    if(Result) {
        msync(Result, ReserveSize, (MS_SYNC|MS_INVALIDATE));
    }
    return Result;
}

function OS_COMMIT_MEMORY_DEFINE(OSX_Commit_Memory) {
    if(BaseAddress) {
        void* Result = mmap(BaseAddress, CommitSize, (PROT_READ|PROT_WRITE), (MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS), -1, 0);
        msync(BaseAddress, CommitSize, (MS_SYNC|MS_INVALIDATE));
        return Result;
    }
    return NULL;
}

function OS_DECOMMIT_MEMORY_DEFINE(OSX_Decommit_Memory) {
    if(BaseAddress) {
        mmap(BaseAddress, DecommitSize, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        msync(BaseAddress, DecommitSize, MS_SYNC|MS_INVALIDATE);
    }
}

function OS_RELEASE_MEMORY_DEFINE(OSX_Release_Memory) {
    if(BaseAddress) {
        msync(BaseAddress, ReleaseSize, MS_SYNC);
        munmap(BaseAddress, ReleaseSize);
    }
}

static const u64 NS_PER_SECOND = 1000000000LL;
function OS_QUERY_PERFORMANCE_DEFINE(OSX_Query_Performance_Counter) {
    struct timespec Now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &Now);

    u64 Result = Now.tv_sec;
    Result *= NS_PER_SECOND;
    Result += Now.tv_nsec;

    return Result;
}

function OS_QUERY_PERFORMANCE_DEFINE(OSX_Query_Performance_Frequency) {
    return NS_PER_SECOND;
}

function OS_OPEN_FILE_DEFINE(OSX_Open_File) {
    int Flags = 0;
    int Permissions = 0;

    if (Attributes == (OS_FILE_ATTRIBUTE_READ|OS_FILE_ATTRIBUTE_WRITE)) {
		Flags = O_RDWR;
        Permissions = 0666;
	} else if (Attributes == OS_FILE_ATTRIBUTE_READ) {
        Flags = O_RDONLY; 
	} else if (Attributes == OS_FILE_ATTRIBUTE_WRITE) {
        Flags = O_WRONLY|O_CREAT;
        Permissions = 0666;
	} else {
		Assert(!"Invalid file attributes!");
		return NULL;
	}
    
    arena* Scratch = Scratch_Get();

    //Copy path to make sure it null terminated
    Path = String_Copy((allocator*)Scratch, Path);
    int FileHandle = open(Path.Ptr, Flags, Permissions);
    Scratch_Release();

    if(FileHandle == -1) {
        Debug_Log("open failed!");
        return NULL;
    }

    osx_base* OSX = OSX_Get();

    pthread_mutex_lock(&OSX->ResourceLock);
    os_file* Result = OSX->FreeFiles;
    if(Result) SLL_Pop_Front(OSX->FreeFiles);
    else Result = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_file);
    pthread_mutex_unlock(&OSX->ResourceLock);

    Memory_Clear(Result, sizeof(os_file));
    Result->Handle = FileHandle;

    return Result;
}

function OS_GET_FILE_SIZE_DEFINE(OSX_Get_File_Size) {
	Assert(File);
	if (!File) return 0;

    struct stat Stat;
    fstat(File->Handle, &Stat);
    
    return (u64)Stat.st_size;
}

function OS_READ_FILE_DEFINE(OSX_Read_File) {
    Assert(File);
    if(!File) return false;

    size_t BytesRead = read(File->Handle, Data, ReadSize);
    if(BytesRead != ReadSize) {
        Debug_Log("read failed!");
        return false;
    }
    
    return true;
}

function OS_WRITE_FILE_DEFINE(OSX_Write_File) {
	Assert(File);
    if(!File) return false;

    size_t BytesWritten = write(File->Handle, Data, WriteSize);
    if(BytesWritten != WriteSize) {
        Debug_Log("write failed!");
        return false;
    }

    return true;
}

function OS_CLOSE_FILE_DEFINE(OSX_Close_File) {
    Assert(File);
    if(!File) return;

    osx_base* OSX = OSX_Get();

    close(File->Handle);

    pthread_mutex_lock(&OSX->ResourceLock);
    SLL_Push_Front(OSX->FreeFiles, File);
    pthread_mutex_unlock(&OSX->ResourceLock);
}

function void OSX_Get_All_Files_Recursive(allocator* Allocator, dynamic_string_array* Array, string Directory, b32 Recursive) {
	arena* Scratch = Scratch_Get();
	if (!String_Ends_With_Char(Directory, '/')) {
		Directory = String_Concat((allocator*)Scratch, Directory, String_Lit("/"));
	}
    Assert(String_Is_Null_Term(Directory));

    DIR* Dir = opendir(Directory.Ptr);
    if(Dir != NULL) {

        struct dirent* DirEntry;
        while((DirEntry = readdir(Dir)) != NULL) {
            string FileOrDirectoryName = String_Null_Term(DirEntry->d_name);
            if (!String_Equals(FileOrDirectoryName, String_Lit(".")) && !String_Equals(FileOrDirectoryName, String_Lit(".."))) {
                if(DirEntry->d_type == DT_DIR) {
                    if(Recursive) {
                        string DirectoryName = FileOrDirectoryName;
                        string Strings[] = { Directory, DirectoryName, String_Lit("/") };
                        string DirectoryPath = String_Combine((allocator*)Scratch, Strings, Array_Count(Strings));
                        OSX_Get_All_Files_Recursive(Allocator, Array, DirectoryPath, true);
                    }
                } else {
                    string FileName = FileOrDirectoryName;
                    string FilePath = String_Concat(Allocator, Directory, FileName);
                    Dynamic_String_Array_Add(Array, FilePath);
                }
            }
        }

        closedir(Dir);
    }

	Scratch_Release();
}

function OS_GET_ALL_FILES_DEFINE(OSX_Get_All_Files) {
	arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }
    dynamic_string_array Array = Dynamic_String_Array_Init(Allocator);
	OSX_Get_All_Files_Recursive(Allocator, &Array, Path, Recursive);
	Scratch_Release();
    return String_Array_Init(Array.Ptr, Array.Count);
}

function OS_IS_PATH_DEFINE(OSX_Is_File_Path) {
    arena* Scratch = Scratch_Get();
    if(String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }

    struct stat Stats;
    int StatResult = stat(Path.Ptr, &Stats);
    Scratch_Release();
    
    if(StatResult != 0) {
        return false;
    }

    return !S_ISDIR(Stats.st_mode);
}

function OS_IS_PATH_DEFINE(OSX_Is_Directory_Path) {
    arena* Scratch = Scratch_Get();
    if(String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }

    struct stat Stats;
    int StatResult = stat(Path.Ptr, &Stats);
    Scratch_Release();
    
    if(StatResult != 0) {
        return false;
    }

    return S_ISDIR(Stats.st_mode);
}

function OS_MAKE_DIRECTORY_DEFINE(OSX_Make_Directory) {
    arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Directory)) {
        Directory = String_Copy((allocator*)Scratch, Directory);
    }
    Assert(String_Is_Null_Term(Directory));

    b32 Result = mkdir(Directory.Ptr, 0755) != 0;
    Scratch_Release();

    return Result;
}

function OS_TLS_CREATE_DEFINE(OSX_TLS_Create) {
    osx_base* OSX = OSX_Get();

    pthread_key_t Key;
    if(pthread_key_create(&Key, NULL) != 0) {
        return NULL;
    }

    pthread_mutex_lock(&OSX->ResourceLock);
    os_tls* TLS = OSX->FreeTLS;
    if(TLS) SLL_Pop_Front(OSX->FreeTLS);
    else TLS = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_tls);
    pthread_mutex_unlock(&OSX->ResourceLock);

    Memory_Clear(TLS, sizeof(os_tls));
    TLS->Key = Key;

    return TLS;
}

function OS_TLS_DELETE_DEFINE(OSX_TLS_Delete) {
    osx_base* OSX = OSX_Get();

    pthread_key_delete(TLS->Key);

    pthread_mutex_lock(&OSX->ResourceLock);
    SLL_Push_Front(OSX->FreeTLS, TLS);
    pthread_mutex_unlock(&OSX->ResourceLock);
}

function OS_TLS_GET_DEFINE(OSX_TLS_Get) {
    osx_base* OSX = OSX_Get();
    return pthread_getspecific(TLS->Key);
}

function OS_TLS_SET_DEFINE(OSX_TLS_Set) {
    osx_base* OSX = OSX_Get();
    pthread_setspecific(TLS->Key, Data);
}

function void* OSX_Thread_Callback(void* Parameter) {
	os_thread* Thread = (os_thread*)Parameter;
	Thread->Callback(Thread, Thread->UserData);
	return 0;
}

function OS_THREAD_CREATE_DEFINE(OSX_Thread_Create) {
    osx_base* OSX = OSX_Get();

	pthread_mutex_lock(&OSX->ResourceLock);
	os_thread* Thread = OSX->FreeThreads;
	if (Thread) SLL_Pop_Front(OSX->FreeThreads);
	else Thread = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_thread);
	pthread_mutex_unlock(&OSX->ResourceLock);

	Memory_Clear(Thread, sizeof(os_thread));
	Thread->Callback = Callback;
	Thread->UserData = UserData;
    if(pthread_create(&Thread->Thread, NULL, OSX_Thread_Callback, Thread) != 0) {
        pthread_mutex_lock(&OSX->ResourceLock);
		SLL_Push_Front(OSX->FreeThreads, Thread);
		pthread_mutex_unlock(&OSX->ResourceLock);
		Thread = NULL;
    }

	return Thread;
}

function OS_THREAD_JOIN_DEFINE(OSX_Thread_Join) {
	if (Thread) {
		osx_base* OSX = OSX_Get();
        pthread_join(Thread->Thread, NULL);

		pthread_mutex_lock(&OSX->ResourceLock);
		SLL_Push_Front(OSX->FreeThreads, Thread);
		pthread_mutex_unlock(&OSX->ResourceLock);
	}
}

function OS_THREAD_GET_ID_DEFINE(OSX_Thread_Get_ID) {
	return Thread ? (u64)Thread->Thread : 0;
}

function OS_GET_CURRENT_THREAD_ID_DEFINE(OSX_Get_Current_Thread_ID) {
    return (u64)pthread_self();
}

function OS_MUTEX_CREATE_DEFINE(OSX_Mutex_Create) {

    osx_base* OSX = OSX_Get();

    pthread_mutex_lock(&OSX->ResourceLock);
    os_mutex* Mutex = OSX->FreeMutex;
    if(Mutex) SLL_Pop_Front(OSX->FreeMutex);
    else Mutex = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_mutex);
    pthread_mutex_unlock(&OSX->ResourceLock);
    
    Memory_Clear(Mutex, sizeof(os_mutex));

    pthread_mutex_init(&Mutex->Lock, NULL);

    return Mutex;
}

function OS_MUTEX_DELETE_DEFINE(OSX_Mutex_Delete) {
    if(Mutex) {
        osx_base* OSX = OSX_Get();

        pthread_mutex_destroy(&Mutex->Lock);

        pthread_mutex_lock(&OSX->ResourceLock);
        SLL_Push_Front(OSX->FreeMutex, Mutex);
        pthread_mutex_unlock(&OSX->ResourceLock);
    }
}

function OS_MUTEX_LOCK_DEFINE(OSX_Mutex_Lock) {
    if(Mutex) {
        pthread_mutex_lock(&Mutex->Lock);
    }
}

function OS_MUTEX_LOCK_DEFINE(OSX_Mutex_Unlock) {
    if(Mutex) {
        pthread_mutex_unlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_CREATE_DEFINE(OSX_RW_Mutex_Create) {
    osx_base* OSX = OSX_Get();

    pthread_mutex_lock(&OSX->ResourceLock);
    os_rw_mutex* Mutex = OSX->FreeRWMutex;
    if(Mutex) SLL_Pop_Front(OSX->FreeRWMutex);
    else Mutex = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_rw_mutex);
    pthread_mutex_unlock(&OSX->ResourceLock);

    Memory_Clear(Mutex, sizeof(os_rw_mutex));
    pthread_rwlock_init(&Mutex->Lock, NULL);

    return Mutex;
}

function OS_RW_MUTEX_DELETE_DEFINE(OSX_RW_Mutex_Delete) {
    if(Mutex) {
        osx_base* OSX = OSX_Get();

        pthread_rwlock_destroy(&Mutex->Lock);

        pthread_mutex_lock(&OSX->ResourceLock);
        SLL_Push_Front(OSX->FreeRWMutex, Mutex);
        pthread_mutex_unlock(&OSX->ResourceLock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(OSX_RW_Mutex_Read_Lock) {
    if(Mutex) {
        pthread_rwlock_rdlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(OSX_RW_Mutex_Read_Unlock) {
    if(Mutex) {
        pthread_rwlock_unlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(OSX_RW_Mutex_Write_Lock) {
    if(Mutex) {
        pthread_rwlock_wrlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(OSX_RW_Mutex_Write_Unlock) {
    if(Mutex) {
        pthread_rwlock_unlock(&Mutex->Lock);
    }
}

function OS_SEMAPHORE_CREATE_DEFINE(OSX_Semaphore_Create) {
    semaphore_t Handle;
    if(semaphore_create(mach_task_self(), &Handle, SYNC_POLICY_FIFO, 0) != KERN_SUCCESS) {
        return NULL;
    }

	osx_base* OSX = OSX_Get();
	pthread_mutex_lock(&OSX->ResourceLock);
	os_semaphore* Semaphore = OSX->FreeSemaphores;
	if (Semaphore) SLL_Pop_Front(OSX->FreeSemaphores);
	else Semaphore = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_semaphore);
	pthread_mutex_unlock(&OSX->ResourceLock);

	Memory_Clear(Semaphore, sizeof(os_semaphore));
	Semaphore->Handle = Handle;

	return Semaphore;
}

function OS_SEMAPHORE_DELETE_DEFINE(OSX_Semaphore_Delete) {
	if (Semaphore) {
        semaphore_destroy(mach_task_self(), Semaphore->Handle);
	
	    osx_base* OSX = OSX_Get();
		pthread_mutex_lock(&OSX->ResourceLock);
		SLL_Push_Front(OSX->FreeSemaphores, Semaphore);
		pthread_mutex_unlock(&OSX->ResourceLock);
	}
}

function OS_SEMAPHORE_INCREMENT_DEFINE(OSX_Semaphore_Increment) {
	if (Semaphore) {
		semaphore_signal(Semaphore->Handle);
	}
}

function OS_SEMAPHORE_DECREMENT_DEFINE(OSX_Semaphore_Decrement) {
	if (Semaphore) {
		semaphore_wait(Semaphore->Handle);
	}
}

function OS_SEMAPHORE_ADD_DEFINE(OSX_Semaphore_Add) {
	if (Semaphore) {
        for(s32 i = 0; i < Count; i++) {
		    semaphore_signal(Semaphore->Handle);
        }
	}
}

function OS_HOT_RELOAD_CREATE_DEFINE(OSX_Hot_Reload_Create) {
    FilePath = String_Copy(Default_Allocator_Get(), FilePath);

    struct stat Stats;
    int StatResult = stat(FilePath.Ptr, &Stats);    
    if(StatResult != 0) {
        Allocator_Free_Memory(Default_Allocator_Get(), (void*)FilePath.Ptr);
        return false;
    }

    osx_base* OSX = OSX_Get();
    pthread_mutex_lock(&OSX->ResourceLock);
    os_hot_reload* HotReload = OSX->FreeHotReload;
    if(HotReload) SLL_Pop_Front(OSX->FreeHotReload);
    else HotReload = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_hot_reload);
    pthread_mutex_unlock(&OSX->ResourceLock);

    Memory_Clear(HotReload, sizeof(os_hot_reload));

    HotReload->FilePath = FilePath;
    HotReload->LastWriteTime = Stats.st_mtime;
    
	return HotReload;
}

function OS_HOT_RELOAD_DELETE_DEFINE(OSX_Hot_Reload_Delete) {
	if (HotReload) {
		Allocator_Free_Memory(Default_Allocator_Get(), (void*)HotReload->FilePath.Ptr);
		osx_base* OSX = OSX_Get();
		pthread_mutex_lock(&OSX->ResourceLock);
		SLL_Push_Front(OSX->FreeHotReload, HotReload);
		pthread_mutex_unlock(&OSX->ResourceLock);
	}
}

function OS_HOT_RELOAD_HAS_RELOADED_DEFINE(OSX_Hot_Reload_Has_Reloaded) {

    struct stat Stats;
    stat(HotReload->FilePath.Ptr, &Stats);    

	if (difftime(Stats.st_mtime, HotReload->LastWriteTime) > 0) {
		HotReload->LastWriteTime = Stats.st_mtime;
		return true;
	}

	return false;
}

function OS_LIBRARY_CREATE_DEFINE(OSX_Library_Create) {
    void* Library = dlopen(LibraryPath.Ptr, RTLD_NOW);
    if(!Library) {
        return NULL;
    }

    osx_base* OSX = OSX_Get();

    pthread_mutex_lock(&OSX->ResourceLock);
    os_library* Result = OSX->FreeLibrary;
    if(Result) SLL_Pop_Front(OSX->FreeLibrary);
    else Result = Arena_Push_Struct_No_Clear(OSX->ResourceArena, os_library);
    pthread_mutex_unlock(&OSX->ResourceLock);

    Memory_Clear(Result, sizeof(os_library));
    Result->Library = Library;

    return Result;
}

function OS_LIBRARY_DELETE_DEFINE(OSX_Library_Delete) {
    if(Library) {
        dlclose(Library->Library);

        osx_base* OSX = OSX_Get();
        pthread_mutex_lock(&OSX->ResourceLock);
        SLL_Push_Front(OSX->FreeLibrary, Library);
        pthread_mutex_unlock(&OSX->ResourceLock);
    }
}

function OS_LIBRARY_GET_FUNCTION_DEFINE(OSX_Library_Get_Function) {
    if(!Library) return NULL;
    return dlsym(Library->Library, FunctionName);
}

function OS_GET_ENTROPY_DEFINE(OSX_Get_Entropy) {
    arc4random_buf(Buffer, Size);
}

global os_base_vtable OSX_Base_VTable = {
	.ReserveMemoryFunc = OSX_Reserve_Memory,
	.CommitMemoryFunc = OSX_Commit_Memory,
	.DecommitMemoryFunc = OSX_Decommit_Memory,
	.ReleaseMemoryFunc = OSX_Release_Memory,
	
	.QueryPerformanceCounterFunc = OSX_Query_Performance_Counter,
	.QueryPerformanceFrequencyFunc = OSX_Query_Performance_Frequency,

	.OpenFileFunc = OSX_Open_File,
	.GetFileSizeFunc = OSX_Get_File_Size,
	.ReadFileFunc = OSX_Read_File,
	.WriteFileFunc = OSX_Write_File,
	.CloseFileFunc = OSX_Close_File,

	.GetAllFilesFunc = OSX_Get_All_Files,
	.IsDirectoryPathFunc = OSX_Is_Directory_Path,
	.IsFilePathFunc = OSX_Is_File_Path,
	.MakeDirectoryFunc = OSX_Make_Directory,

	.TLSCreateFunc = OSX_TLS_Create,
	.TLSDeleteFunc = OSX_TLS_Delete,
	.TLSGetFunc = OSX_TLS_Get,
	.TLSSetFunc = OSX_TLS_Set,

    .ThreadCreateFunc = OSX_Thread_Create,
    .ThreadJoinFunc = OSX_Thread_Join,
    .ThreadGetIdFunc = OSX_Thread_Get_ID,
    .GetCurrentThreadIdFunc = OSX_Get_Current_Thread_ID,

	.MutexCreateFunc = OSX_Mutex_Create,
	.MutexDeleteFunc = OSX_Mutex_Delete,
	.MutexLockFunc = OSX_Mutex_Lock,
	.MutexUnlockFunc = OSX_Mutex_Unlock,

    .RWMutexCreateFunc = OSX_RW_Mutex_Create,
	.RWMutexDeleteFunc = OSX_RW_Mutex_Delete,
	.RWMutexReadLockFunc = OSX_RW_Mutex_Read_Lock,
	.RWMutexReadUnlockFunc = OSX_RW_Mutex_Read_Unlock,
	.RWMutexWriteLockFunc = OSX_RW_Mutex_Write_Lock,
	.RWMutexWriteUnlockFunc = OSX_RW_Mutex_Write_Unlock,

    .SemaphoreCreateFunc = OSX_Semaphore_Create,
    .SemaphoreDeleteFunc = OSX_Semaphore_Delete,
    .SemaphoreIncrementFunc = OSX_Semaphore_Increment,
    .SemaphoreDecrementFunc = OSX_Semaphore_Decrement,
    .SemaphoreAddFunc = OSX_Semaphore_Add,

	.HotReloadCreateFunc = OSX_Hot_Reload_Create,
	.HotReloadDeleteFunc = OSX_Hot_Reload_Delete,
	.HotReloadHasReloadedFunc = OSX_Hot_Reload_Has_Reloaded,
	
	.LibraryCreateFunc = OSX_Library_Create,
	.LibraryDeleteFunc = OSX_Library_Delete,
	.LibraryGetFunctionFunc = OSX_Library_Get_Function,

    .GetEntropyFunc = OSX_Get_Entropy
};

function string OSX_Get_Executable_Path(allocator* Allocator) {
    @autoreleasepool {
        NSBundle* Bundle = [NSBundle mainBundle];
        const char* BaseType = [[[Bundle infoDictionary] objectForKey:@"SDL_FILESYSTEM_BASE_DIR_TYPE"] UTF8String];
        const char* Base = NULL;

        if (BaseType == NULL) {
            BaseType = "resource";
        }

        if (strcasecmp(BaseType, "bundle") == 0) {
            Base = [[Bundle bundlePath] fileSystemRepresentation];
        } else if (strcasecmp(BaseType, "parent") == 0) {
            Base = [[[Bundle bundlePath] stringByDeletingLastPathComponent] fileSystemRepresentation];
        } else {
            // this returns the exedir for non-bundled  and the resourceDir for bundled apps
            Base = [[Bundle resourcePath] fileSystemRepresentation];
        }

        return String_Concat(Allocator, String_Null_Term(Base), String_Lit("/"));
    }
}

void OS_Base_Init(base* Base) {
    static osx_base OSX;

    Base_Set(Base);

    OSX.Base.VTable = &OSX_Base_VTable;
    OSX.Base.PageSize = (size_t)sysconf(_SC_PAGESIZE);
    OSX.Base.ProcessorThreadCount = (size_t)sysconf(_SC_NPROCESSORS_ONLN);
    G_OSX = &OSX;

    Base->OSBase = (os_base*)&OSX;

    OSX.ResourceArena = Arena_Create();
    pthread_mutex_init(&OSX.ResourceLock, NULL);

    Base->ThreadContextTLS = OS_TLS_Create();

    arena* Scratch = Scratch_Get();
    string ExecutablePath = OSX_Get_Executable_Path((allocator*)Scratch);
    OSX.Base.ProgramPath = String_Copy((allocator*)OSX.ResourceArena, String_Get_Directory_Path(ExecutablePath));

    Scratch_Release();
}