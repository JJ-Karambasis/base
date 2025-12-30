#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../../base.h"

#ifdef OS_OSX
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/mach_vm.h>
#else
#include <semaphore.h>
#endif

#include "posix_base.h"
#include "../../third_party/rpmalloc/rpmalloc.h"

Array_Implement(string, String);
Dynamic_Array_Implement_Type(string, String);

global posix_base* G_Posix;
function posix_base* Posix_Get() {
	return G_Posix;
}

function OS_RESERVE_MEMORY_DEFINE(Posix_Reserve_Memory) {
    void* Result = mmap(NULL, ReserveSize, PROT_NONE, (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
    if(Result) {
        os_base* Base = (os_base*)Posix_Get();

        msync(Result, ReserveSize, (MS_SYNC|MS_INVALIDATE));
        Atomic_Add_U64(&Base->ReservedAmount, ReserveSize);
		Atomic_Increment_U64(&Base->ReservedCount);
    }
    return Result;
}

function OS_COMMIT_MEMORY_DEFINE(Posix_Commit_Memory) {
    if(BaseAddress) {
        os_base* Base = (os_base*)Posix_Get();

        void* Result = mmap(BaseAddress, CommitSize, (PROT_READ|PROT_WRITE), (MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS), -1, 0);
        msync(BaseAddress, CommitSize, (MS_SYNC|MS_INVALIDATE));
        return Result;
    }
    return NULL;
}

function OS_DECOMMIT_MEMORY_DEFINE(Posix_Decommit_Memory) {
    if(BaseAddress) {
        os_base* Base = (os_base*)Posix_Get();

        mmap(BaseAddress, DecommitSize, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        msync(BaseAddress, DecommitSize, MS_SYNC|MS_INVALIDATE);
    }
}

function OS_RELEASE_MEMORY_DEFINE(Posix_Release_Memory) {
    if(BaseAddress) {
        os_base* Base = (os_base*)Posix_Get();

        Atomic_Sub_U64(&Base->ReservedAmount, ReleaseSize);
        Atomic_Decrement_U64(&Base->ReservedCount);

        msync(BaseAddress, ReleaseSize, MS_SYNC);
        munmap(BaseAddress, ReleaseSize);
    }
}

static const u64 NS_PER_SECOND = 1000000000LL;
function OS_QUERY_PERFORMANCE_DEFINE(Posix_Query_Performance_Counter) {
    struct timespec Now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &Now);

    u64 Result = Now.tv_sec;
    Result *= NS_PER_SECOND;
    Result += Now.tv_nsec;

    return Result;
}

function OS_QUERY_PERFORMANCE_DEFINE(Posix_Query_Performance_Frequency) {
    return NS_PER_SECOND;
}

function OS_OPEN_FILE_DEFINE(Posix_Open_File) {
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

    posix_base* Posix = Posix_Get();

    pthread_mutex_lock(&Posix->ResourceLock);
    os_file* Result = Posix->FreeFiles;
    if(Result) SLL_Pop_Front(Posix->FreeFiles);
    else Result = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_file);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Memory_Clear(Result, sizeof(os_file));
    Result->Handle = FileHandle;

    Atomic_Increment_U64(&Posix->Base.AllocatedFileCount);

    return Result;
}

function OS_GET_FILE_SIZE_DEFINE(Posix_Get_File_Size) {
	Assert(File);
	if (!File) return 0;

    struct stat Stat;
    fstat(File->Handle, &Stat);
    
    return (u64)Stat.st_size;
}

function OS_READ_FILE_DEFINE(Posix_Read_File) {
    Assert(File);
    if(!File) return false;

    size_t BytesRead = read(File->Handle, Data, ReadSize);
    if(BytesRead != ReadSize) {
        Debug_Log("read failed!");
        return false;
    }
    
    return true;
}

function OS_WRITE_FILE_DEFINE(Posix_Write_File) {
	Assert(File);
    if(!File) return false;

    size_t BytesWritten = write(File->Handle, Data, WriteSize);
    if(BytesWritten != WriteSize) {
        Debug_Log("write failed!");
        return false;
    }

    return true;
}

function OS_SET_FILE_POINTER_DEFINE(Posix_Set_File_Pointer) {
    Assert(File);
    if(!File) return;
    lseek(File->Handle, Pointer, SEEK_SET);
}

function OS_GET_FILE_POINTER_DEFINE(Posix_Get_File_Pointer) {
    Assert(File);
    if(!File) return (u64)-1;
    u64 Result = lseek(File->Handle, 0, SEEK_CUR);
    return Result;
}

function OS_CLOSE_FILE_DEFINE(Posix_Close_File) {
    Assert(File);
    if(!File) return;

    posix_base* Posix = Posix_Get();

    close(File->Handle);

    pthread_mutex_lock(&Posix->ResourceLock);
    SLL_Push_Front(Posix->FreeFiles, File);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Atomic_Decrement_U64(&Posix->Base.AllocatedFileCount);
}

function void Posix_Get_All_Files_Recursive(allocator* Allocator, dynamic_string_array* Array, string Directory, b32 Recursive) {
	arena* Scratch = Scratch_Get();
	if (!String_Ends_With_Char(Directory, '/')) {
		Directory = String_Concat((allocator*)Scratch, Directory, String_Lit("/"));
	}

    //Copy the directory to ensure it is null terminated
    Directory = String_Copy((allocator*)Scratch, Directory);

    DIR* Dir = opendir(Directory.Ptr);
    if(Dir != NULL) {

        struct dirent* DirEntry;
        while((DirEntry = readdir(Dir)) != NULL) {
            string FileOrDirectoryName = String_Null_Term(DirEntry->d_name);
            if (!String_Equals(FileOrDirectoryName, String_Lit(".")) && !String_Equals(FileOrDirectoryName, String_Lit(".."))) {
                if(DirEntry->d_type == DT_DIR) {
                    string DirectoryName = FileOrDirectoryName;
                    if (Recursive) {
                        string DirectoryPath = String_Directory_Concat((allocator*)Scratch, Directory, DirectoryName);
                        Posix_Get_All_Files_Recursive(Allocator, Array, DirectoryPath, true);
                    } else {
                        string DirectoryPath = String_Directory_Concat(Allocator, Directory, DirectoryName);
                        Dynamic_String_Array_Add(Array, DirectoryPath);
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

function OS_GET_ALL_FILES_DEFINE(Posix_Get_All_Files) {
	arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }
    dynamic_string_array Array = Dynamic_String_Array_Init(Allocator);
	Posix_Get_All_Files_Recursive(Allocator, &Array, Path, Recursive);
	Scratch_Release();
    return String_Array_Init(Array.Ptr, Array.Count);
}

function OS_IS_PATH_DEFINE(Posix_Is_File_Path) {
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

function OS_IS_PATH_DEFINE(Posix_Is_Directory_Path) {
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

function OS_MAKE_DIRECTORY_DEFINE(Posix_Make_Directory) {
    arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Directory)) {
        Directory = String_Copy((allocator*)Scratch, Directory);
    }
    Assert(String_Is_Null_Term(Directory));

    b32 Result = mkdir(Directory.Ptr, 0755) != 0;
    Scratch_Release();

    return Result;
}

function OS_COPY_FILE_DEFINE(Posix_Copy_File) {    
    os_file* SrcFile = Posix_Open_File(SrcFilePath, OS_FILE_ATTRIBUTE_READ);
    os_file* DstFile = Posix_Open_File(DstFilePath, OS_FILE_ATTRIBUTE_WRITE);

    b32 Result = false;
    if(SrcFile && DstFile) {
        //Right now we do not support copies larger than
        //4GB
        u64 FileSize = Posix_Get_File_Size(SrcFile);
        if(FileSize < 0xFFFFFFFF) {
            arena* Scratch = Scratch_Get();
            void* CopyData = Arena_Push(Scratch, FileSize);
            if(Posix_Read_File(SrcFile, CopyData, (u32)FileSize)) {
                Result = Posix_Write_File(DstFile, CopyData, (u32)FileSize);
            }

            Scratch_Release();
        }
    }

    if(SrcFile) Posix_Close_File(SrcFile);
    if(DstFile) Posix_Close_File(DstFile);

    return Result;
}

function OS_TLS_CREATE_DEFINE(Posix_TLS_Create) {
    posix_base* Posix = Posix_Get();

    pthread_key_t Key;
    if(pthread_key_create(&Key, NULL) != 0) {
        return NULL;
    }

    pthread_mutex_lock(&Posix->ResourceLock);
    os_tls* TLS = Posix->FreeTLS;
    if(TLS) SLL_Pop_Front(Posix->FreeTLS);
    else TLS = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_tls);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Memory_Clear(TLS, sizeof(os_tls));
    TLS->Key = Key;

    Atomic_Increment_U64(&Posix->Base.AllocatedTLSCount);

    return TLS;
}

function OS_TLS_DELETE_DEFINE(Posix_TLS_Delete) {
    posix_base* Posix = Posix_Get();

    pthread_key_delete(TLS->Key);

    pthread_mutex_lock(&Posix->ResourceLock);
    SLL_Push_Front(Posix->FreeTLS, TLS);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Atomic_Decrement_U64(&Posix->Base.AllocatedTLSCount);
}

function OS_TLS_GET_DEFINE(Posix_TLS_Get) {
    return pthread_getspecific(TLS->Key);
}

function OS_TLS_SET_DEFINE(Posix_TLS_Set) {
    pthread_setspecific(TLS->Key, Data);
}

function void* Posix_Thread_Callback(void* Parameter) {
	os_thread* Thread = (os_thread*)Parameter;
	Thread->Callback(Thread, Thread->UserData);
	return 0;
}

function OS_THREAD_CREATE_DEFINE(Posix_Thread_Create) {
    posix_base* Posix = Posix_Get();

	pthread_mutex_lock(&Posix->ResourceLock);
	os_thread* Thread = Posix->FreeThreads;
	if (Thread) SLL_Pop_Front(Posix->FreeThreads);
	else Thread = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_thread);
	pthread_mutex_unlock(&Posix->ResourceLock);

	Memory_Clear(Thread, sizeof(os_thread));
	Thread->Callback = Callback;
	Thread->UserData = UserData;
    if(pthread_create(&Thread->Thread, NULL, Posix_Thread_Callback, Thread) != 0) {
        pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeThreads, Thread);
		pthread_mutex_unlock(&Posix->ResourceLock);
		Thread = NULL;
    } else {
        Atomic_Increment_U64(&Posix->Base.AllocatedThreadCount);
    }

	return Thread;
}

function OS_THREAD_JOIN_DEFINE(Posix_Thread_Join) {
	if (Thread) {
        posix_base* Posix = Posix_Get();
        pthread_join(Thread->Thread, NULL);

		pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeThreads, Thread);
		pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedThreadCount);
	}
}

function OS_THREAD_GET_ID_DEFINE(Posix_Thread_Get_ID) {
	return Thread ? (u64)Thread->Thread : 0;
}

function OS_GET_CURRENT_THREAD_ID_DEFINE(Posix_Get_Current_Thread_ID) {
    return (u64)pthread_self();
}

function OS_MUTEX_CREATE_DEFINE(Posix_Mutex_Create) {

    posix_base* Posix = Posix_Get();

    pthread_mutex_lock(&Posix->ResourceLock);
    os_mutex* Mutex = Posix->FreeMutex;
    if(Mutex) SLL_Pop_Front(Posix->FreeMutex);
    else Mutex = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_mutex);
    pthread_mutex_unlock(&Posix->ResourceLock);
    
    Memory_Clear(Mutex, sizeof(os_mutex));

    pthread_mutexattr_t Attribute;
    pthread_mutexattr_init(&Attribute);
    pthread_mutexattr_settype(&Attribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&Mutex->Lock, &Attribute);
    pthread_mutexattr_destroy(&Attribute);

    Atomic_Increment_U64(&Posix->Base.AllocatedMutexCount);

    return Mutex;
}

function OS_MUTEX_DELETE_DEFINE(Posix_Mutex_Delete) {
    if(Mutex) {
        posix_base* Posix = Posix_Get();

        pthread_mutex_destroy(&Mutex->Lock);

        pthread_mutex_lock(&Posix->ResourceLock);
        SLL_Push_Front(Posix->FreeMutex, Mutex);
        pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedMutexCount);
    }
}

function OS_MUTEX_LOCK_DEFINE(Posix_Mutex_Lock) {
    if(Mutex) {
        pthread_mutex_lock(&Mutex->Lock);
    }
}

function OS_MUTEX_LOCK_DEFINE(Posix_Mutex_Unlock) {
    if(Mutex) {
        pthread_mutex_unlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_CREATE_DEFINE(Posix_RW_Mutex_Create) {
    posix_base* Posix = Posix_Get();

    pthread_mutex_lock(&Posix->ResourceLock);
    os_rw_mutex* Mutex = Posix->FreeRWMutex;
    if(Mutex) SLL_Pop_Front(Posix->FreeRWMutex);
    else Mutex = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_rw_mutex);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Memory_Clear(Mutex, sizeof(os_rw_mutex));
    pthread_rwlock_init(&Mutex->Lock, NULL);

    Atomic_Increment_U64(&Posix->Base.AllocatedRWMutexCount);

    return Mutex;
}

function OS_RW_MUTEX_DELETE_DEFINE(Posix_RW_Mutex_Delete) {
    if(Mutex) {
        posix_base* Posix = Posix_Get();

        pthread_rwlock_destroy(&Mutex->Lock);

        pthread_mutex_lock(&Posix->ResourceLock);
        SLL_Push_Front(Posix->FreeRWMutex, Mutex);
        pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedRWMutexCount);
    }
}

function OS_RW_MUTEX_TRY_LOCK_DEFINE(Posix_RW_Mutex_Try_Read_Lock) {
    if(!Mutex) return false;
    return pthread_rwlock_tryrdlock(&Mutex->Lock) == 0;
}

function OS_RW_MUTEX_LOCK_DEFINE(Posix_RW_Mutex_Read_Lock) {
    if(Mutex) {
        pthread_rwlock_rdlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(Posix_RW_Mutex_Read_Unlock) {
    if(Mutex) {
        pthread_rwlock_unlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(Posix_RW_Mutex_Write_Lock) {
    if(Mutex) {
        pthread_rwlock_wrlock(&Mutex->Lock);
    }
}

function OS_RW_MUTEX_LOCK_DEFINE(Posix_RW_Mutex_Write_Unlock) {
    if(Mutex) {
        pthread_rwlock_unlock(&Mutex->Lock);
    }
}

#if defined(OS_OSX)
function OS_SEMAPHORE_CREATE_DEFINE(Posix_Semaphore_Create) {
    semaphore_t Handle;
    if(semaphore_create(mach_task_self(), &Handle, SYNC_POLICY_FIFO, InitialCount) != KERN_SUCCESS) {
        return NULL;
    }

    posix_base* Posix = Posix_Get();
	pthread_mutex_lock(&Posix->ResourceLock);
	os_semaphore* Semaphore = Posix->FreeSemaphores;
	if (Semaphore) SLL_Pop_Front(Posix->FreeSemaphores);
	else Semaphore = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_semaphore);
	pthread_mutex_unlock(&Posix->ResourceLock);

	Memory_Clear(Semaphore, sizeof(os_semaphore));
	Semaphore->Handle = Handle;

    Atomic_Increment_U64(&Posix->Base.AllocatedSemaphoresCount);

	return Semaphore;
}

function OS_SEMAPHORE_DELETE_DEFINE(Posix_Semaphore_Delete) {
	if (Semaphore) {
        semaphore_destroy(mach_task_self(), Semaphore->Handle);
	
        posix_base* Posix = Posix_Get();
		pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeSemaphores, Semaphore);
		pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedSemaphoresCount);
	}
}

function OS_SEMAPHORE_INCREMENT_DEFINE(Posix_Semaphore_Increment) {
	if (Semaphore) {
		semaphore_signal(Semaphore->Handle);
	}
}

function OS_SEMAPHORE_DECREMENT_DEFINE(Posix_Semaphore_Decrement) {
	if (Semaphore) {
		semaphore_wait(Semaphore->Handle);
	}
}

function OS_SEMAPHORE_ADD_DEFINE(Posix_Semaphore_Add) {
	if (Semaphore) {
        for(s32 i = 0; i < Count; i++) {
		    semaphore_signal(Semaphore->Handle);
        }
	}
}
#elif defined(OS_LINUX)

function OS_SEMAPHORE_CREATE_DEFINE(Posix_Semaphore_Create) {
    sem_t Handle;
    if(sem_init(&Handle, 0, InitialCount) != 0) {
        return NULL;
    }

    posix_base* Posix = Posix_Get();
	pthread_mutex_lock(&Posix->ResourceLock);
	os_semaphore* Semaphore = Posix->FreeSemaphores;
	if (Semaphore) SLL_Pop_Front(Posix->FreeSemaphores);
	else Semaphore = Arena_Push_Struct_No_Clear(Posix->ResourceArena, os_semaphore);
	pthread_mutex_unlock(&Posix->ResourceLock);

	Memory_Clear(Semaphore, sizeof(os_semaphore));
	Semaphore->Handle = Handle;

    Atomic_Increment_U64(&Posix->Base.AllocatedSemaphoresCount);

	return Semaphore;
}

function OS_SEMAPHORE_DELETE_DEFINE(Posix_Semaphore_Delete) {
	if (Semaphore) {
        sem_destroy(&Semaphore->Handle);
        posix_base* Posix = Posix_Get();
		pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeSemaphores, Semaphore);
		pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedSemaphoresCount);
	}
}

function OS_SEMAPHORE_INCREMENT_DEFINE(Posix_Semaphore_Increment) {
	if (Semaphore) {
		sem_post(&Semaphore->Handle);
	}
}

function OS_SEMAPHORE_DECREMENT_DEFINE(Posix_Semaphore_Decrement) {
	if (Semaphore) {
		sem_wait(&Semaphore->Handle);
	}
}

function OS_SEMAPHORE_ADD_DEFINE(Posix_Semaphore_Add) {
	if (Semaphore) {
        for(s32 i = 0; i < Count; i++) {
		    sem_post(&Semaphore->Handle);
        }
	}
}

#else
#error "Not Implemented"
#endif

function OS_EVENT_CREATE_DEFINE(Posix_Event_Create) {
    pthread_mutex_t Mutex;
	pthread_cond_t Cond;
	
	pthread_mutex_init(&Mutex, NULL);
    pthread_cond_init(&Cond, NULL);

    posix_base* Posix = Posix_Get();
	pthread_mutex_lock(&Posix->ResourceLock);
	os_event* Event = Posix->FreeEvents;
	if (Event) SLL_Pop_Front(Posix->FreeEvents);
	else Event = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_event);
	pthread_mutex_unlock(&Posix->ResourceLock);

	Memory_Clear(Event, sizeof(os_event));
	Event->Mutex = Mutex;
    Event->Cond = Cond;

    Atomic_Increment_U64(&Posix->Base.AllocatedEventsCount);

	return Event;
}

function OS_EVENT_DELETE_DEFINE(Posix_Event_Delete) {
	if (Event) {
        pthread_mutex_destroy(&Event->Mutex);
        pthread_cond_destroy(&Event->Cond);

        posix_base* Posix = Posix_Get();
		pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeEvents, Event);
		pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedEventsCount);
	}
}

function OS_EVENT_WAIT_DEFINE(Posix_Event_Wait) {
    if(Event) {
        pthread_mutex_lock(&Event->Mutex);
    
        // If already signaled, return immediately
        if (Event->IsSignaled) {
            pthread_mutex_unlock(&Event->Mutex);
            return;
        }
        
        pthread_cond_wait(&Event->Cond, &Event->Mutex);
        pthread_mutex_unlock(&Event->Mutex);
    }
}

function OS_EVENT_SIGNAL_DEFINE(Posix_Event_Signal) {
	if(Event) {
        pthread_mutex_lock(&Event->Mutex);
        Event->IsSignaled = true;
        pthread_cond_broadcast(&Event->Cond);  // Wake all waiting threads
        pthread_mutex_unlock(&Event->Mutex);
    }
}

function OS_EVENT_RESET_DEFINE(Posix_Event_Reset) {
	if(Event) {
        pthread_mutex_lock(&Event->Mutex);
        Event->IsSignaled = false;
        pthread_mutex_unlock(&Event->Mutex);
    }
}

function OS_HOT_RELOAD_CREATE_DEFINE(Posix_Hot_Reload_Create) {
    FilePath = String_Copy(Default_Allocator_Get(), FilePath);

    struct stat Stats;
    int StatResult = stat(FilePath.Ptr, &Stats);    
    if(StatResult != 0) {
        Allocator_Free_Memory(Default_Allocator_Get(), (void*)FilePath.Ptr);
        return false;
    }

    posix_base* Posix = Posix_Get();
    pthread_mutex_lock(&Posix->ResourceLock);
    os_hot_reload* HotReload = Posix->FreeHotReload;
    if(HotReload) SLL_Pop_Front(Posix->FreeHotReload);
    else HotReload = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_hot_reload);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Memory_Clear(HotReload, sizeof(os_hot_reload));

    HotReload->FilePath = FilePath;
    HotReload->LastWriteTime = Stats.st_mtime;

    Atomic_Increment_U64(&Posix->Base.AllocatedHotReloadCount);

	return HotReload;
}

function OS_HOT_RELOAD_DELETE_DEFINE(Posix_Hot_Reload_Delete) {
	if (HotReload) {
		Allocator_Free_Memory(Default_Allocator_Get(), (void*)HotReload->FilePath.Ptr);
        posix_base* Posix = Posix_Get();
		pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeHotReload, HotReload);
		pthread_mutex_unlock(&Posix->ResourceLock);
	
        Atomic_Decrement_U64(&Posix->Base.AllocatedHotReloadCount);
    }
}

function OS_HOT_RELOAD_HAS_RELOADED_DEFINE(Posix_Hot_Reload_Has_Reloaded) {

    struct stat Stats;
    stat(HotReload->FilePath.Ptr, &Stats);    

	if (difftime(Stats.st_mtime, HotReload->LastWriteTime) > 0) {
		HotReload->LastWriteTime = Stats.st_mtime;
		return true;
	}

	return false;
}

function OS_LIBRARY_CREATE_DEFINE(Posix_Library_Create) {
    void* Library = dlopen(LibraryPath.Ptr, RTLD_NOW|RTLD_GLOBAL);
    if(!Library) {
        return NULL;
    }

    posix_base* Posix = Posix_Get();

    pthread_mutex_lock(&Posix->ResourceLock);
    os_library* Result = Posix->FreeLibrary;
    if(Result) SLL_Pop_Front(Posix->FreeLibrary);
    else Result = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_library);
    pthread_mutex_unlock(&Posix->ResourceLock);

    Memory_Clear(Result, sizeof(os_library));
    Result->Library = Library;

    Atomic_Increment_U64(&Posix->Base.AllocatedLibraryCount);

    return Result;
}

function OS_LIBRARY_DELETE_DEFINE(Posix_Library_Delete) {
    if(Library) {
        int Status = dlclose(Library->Library);
        Assert(Status == 0);

        posix_base* Posix = Posix_Get();
        pthread_mutex_lock(&Posix->ResourceLock);
        SLL_Push_Front(Posix->FreeLibrary, Library);
        pthread_mutex_unlock(&Posix->ResourceLock);

        Atomic_Decrement_U64(&Posix->Base.AllocatedLibraryCount);
    }
}

function OS_LIBRARY_GET_FUNCTION_DEFINE(Posix_Library_Get_Function) {
    if(!Library) return NULL;
    return dlsym(Library->Library, FunctionName);
}

function OS_CONDITION_VARIABLE_CREATE_DEFINE(Posix_Condition_Variable_Create) {
    posix_base* Posix = Posix_Get();

	pthread_cond_t Handle;
	pthread_cond_init(&Handle, NULL);

	pthread_mutex_lock(&Posix->ResourceLock);
	os_condition_variable* Result = Posix->FreeConditionVariables;
	if (Result) SLL_Pop_Front(Posix->FreeConditionVariables);
	else Result = Arena_Push_Struct_No_Clear(Posix->Base.ResourceArena, os_condition_variable);
	pthread_mutex_unlock(&Posix->ResourceLock);

	Memory_Clear(Result, sizeof(os_condition_variable));
	Result->Handle = Handle;

	Atomic_Increment_U64(&Posix->Base.AllocatedConditionVariableCount);

	return Result;
}

function OS_CONDITION_VARIABLE_DELETE_DEFINE(Posix_Condition_Variable_Delete) {
	if (Variable) {
        posix_base* Posix = Posix_Get();

        pthread_cond_destroy(&Variable->Handle);

	    pthread_mutex_lock(&Posix->ResourceLock);
		SLL_Push_Front(Posix->FreeConditionVariables, Variable);
	    pthread_mutex_unlock(&Posix->ResourceLock);

		Atomic_Decrement_U64(&Posix->Base.AllocatedConditionVariableCount);
	}
}

function OS_CONDITION_VARIABLE_WAIT_DEFINE(Posix_Condition_Variable_Wait) {
	if (Variable && Mutex) {
        pthread_cond_wait(&Variable->Handle, &Mutex->Lock);
	}
}

function OS_CONDITION_VARIABLE_WAKE_ALL_DEFINE(Posix_Condition_Variable_Wake) {
	if (Variable) {
        pthread_cond_signal(&Variable->Handle);
	}
}

function OS_CONDITION_VARIABLE_WAKE_DEFINE(Posix_Condition_Variable_Wake_All) {
	if (Variable) {
        pthread_cond_broadcast(&Variable->Handle);
	}
}

function OS_GET_ENTROPY_DEFINE(Posix_Get_Entropy) {
#ifdef OS_OSX    
    arc4random_buf(Buffer, Size);
#elif defined(OS_LINUX)
    getrandom(Buffer, Size, 0);
#else
#error "Not Implemented!"
#endif
}

function OS_SLEEP_DEFINE(Posix_Sleep) {
	if (Nanoseconds > 0) {
        struct timespec req, rem;
    
        // Convert nanoseconds to seconds and nanoseconds
        req.tv_sec = Nanoseconds / 1000000000ULL;
        req.tv_nsec = Nanoseconds % 1000000000ULL;
    
        // Handle interruptions by signals (EINTR)
        while (nanosleep(&req, &rem) == -1) {
            if (errno == EINTR) {
                // Sleep was interrupted, continue with remaining time
                req = rem;
            } else {
                // Some other error occurred, break out
                break;
            }
        }
	}
}

global os_base_vtable Posix_Base_VTable = {
	.ReserveMemoryFunc = Posix_Reserve_Memory,
	.CommitMemoryFunc = Posix_Commit_Memory,
	.DecommitMemoryFunc = Posix_Decommit_Memory,
	.ReleaseMemoryFunc = Posix_Release_Memory,
	
	.QueryPerformanceCounterFunc = Posix_Query_Performance_Counter,
	.QueryPerformanceFrequencyFunc = Posix_Query_Performance_Frequency,

	.OpenFileFunc = Posix_Open_File,
	.GetFileSizeFunc = Posix_Get_File_Size,
	.ReadFileFunc = Posix_Read_File,
	.WriteFileFunc = Posix_Write_File,
    .SetFilePointerFunc = Posix_Set_File_Pointer,
    .GetFilePointerFunc = Posix_Get_File_Pointer,
	.CloseFileFunc = Posix_Close_File,

	.GetAllFilesFunc = Posix_Get_All_Files,
	.IsDirectoryPathFunc = Posix_Is_Directory_Path,
	.IsFilePathFunc = Posix_Is_File_Path,
	.MakeDirectoryFunc = Posix_Make_Directory,
    .CopyFileFunc = Posix_Copy_File,

	.TLSCreateFunc = Posix_TLS_Create,
	.TLSDeleteFunc = Posix_TLS_Delete,
	.TLSGetFunc = Posix_TLS_Get,
	.TLSSetFunc = Posix_TLS_Set,

    .ThreadCreateFunc = Posix_Thread_Create,
    .ThreadJoinFunc = Posix_Thread_Join,
    .ThreadGetIdFunc = Posix_Thread_Get_ID,
    .GetCurrentThreadIdFunc = Posix_Get_Current_Thread_ID,

	.MutexCreateFunc = Posix_Mutex_Create,
	.MutexDeleteFunc = Posix_Mutex_Delete,
	.MutexLockFunc = Posix_Mutex_Lock,
	.MutexUnlockFunc = Posix_Mutex_Unlock,

    .RWMutexCreateFunc = Posix_RW_Mutex_Create,
	.RWMutexDeleteFunc = Posix_RW_Mutex_Delete,
    .RWMutexTryReadLockFunc = Posix_RW_Mutex_Try_Read_Lock,
	.RWMutexReadLockFunc = Posix_RW_Mutex_Read_Lock,
	.RWMutexReadUnlockFunc = Posix_RW_Mutex_Read_Unlock,
	.RWMutexWriteLockFunc = Posix_RW_Mutex_Write_Lock,
	.RWMutexWriteUnlockFunc = Posix_RW_Mutex_Write_Unlock,

    .SemaphoreCreateFunc = Posix_Semaphore_Create,
    .SemaphoreDeleteFunc = Posix_Semaphore_Delete,
    .SemaphoreIncrementFunc = Posix_Semaphore_Increment,
    .SemaphoreDecrementFunc = Posix_Semaphore_Decrement,
    .SemaphoreAddFunc = Posix_Semaphore_Add,

    .EventCreateFunc = Posix_Event_Create,
    .EventDeleteFunc = Posix_Event_Delete,
    .EventResetFunc = Posix_Event_Reset,
    .EventSignalFunc = Posix_Event_Signal,
    .EventWaitFunc = Posix_Event_Wait,

	.HotReloadCreateFunc = Posix_Hot_Reload_Create,
	.HotReloadDeleteFunc = Posix_Hot_Reload_Delete,
	.HotReloadHasReloadedFunc = Posix_Hot_Reload_Has_Reloaded,
	
	.LibraryCreateFunc = Posix_Library_Create,
	.LibraryDeleteFunc = Posix_Library_Delete,
	.LibraryGetFunctionFunc = Posix_Library_Get_Function,
    
    .ConditionVariableCreateFunc = Posix_Condition_Variable_Create,
    .ConditionVariableDeleteFunc = Posix_Condition_Variable_Delete,
    .ConditionVariableWaitFunc = Posix_Condition_Variable_Wait,
    .ConditionVariableWakeAllFunc = Posix_Condition_Variable_Wake_All,
    .ConditionVariableWakeFunc = Posix_Condition_Variable_Wake,

    .GetEntropyFunc = Posix_Get_Entropy,
    .SleepFunc = Posix_Sleep
};

#if defined(OS_OSX)
function string Posix_Get_Executable_Path(allocator* Allocator) {
    uint32_t Size = 0;
    
    string Result = String_Empty();
    if (_NSGetExecutablePath(NULL, &Size) == -1) {
        arena* Scratch = Scratch_Get();
        char* Buffer = Arena_Push(Scratch, Size+1);
        Buffer[Size] = 0;
        
        if (_NSGetExecutablePath(Buffer, &Size) == 0) {
            char* Resolved = Arena_Push(Scratch, Size+1);
            realpath(Buffer, Resolved);
            Result = String_Copy(Allocator, String_Null_Term(Resolved));
        }

        Scratch_Release();
    }
    return Result;    
}
#elif defined(OS_LINUX)
function string Posix_Get_Executable_Path(allocator* Allocator) {
	size_t Size = 1024;
	for (int Iterations = 0; Iterations < 32; Iterations++) {
		arena* Scratch = Scratch_Get();
        char* Path = (char*)Arena_Push(Scratch, Size*sizeof(char));
        size_t Length = readlink("/proc/self/exe", Path, Size);
        if(Length == (size_t)-1) {
            break;
        }

        if(Length < Size-1) {
            string String = String_Copy(Allocator, Make_String(Path, Length));
            Scratch_Release();
            return String;
        }

		Scratch_Release();
		Size *= 2;
	}

	return String_Empty();
}
#else
#error "Not Implemented"
#endif

function void* RPMalloc_Memory_Map(size_t Size, size_t Alignment, size_t* Offset, size_t* MappedSize) {
	size_t ReserveSize = Size + Alignment;
	void* Memory = OS_Reserve_Memory(ReserveSize);
	if (Memory) {
		if (Alignment) {
			size_t Padding = ((uintptr_t)Memory & (uintptr_t)(Alignment - 1));
			if (Padding) Padding = Alignment - Padding;
			Memory = Offset_Pointer(Memory, Padding);
			*Offset = Padding;
		}
		*MappedSize = ReserveSize;
	}

	return Memory;
}

function void RPMalloc_Memory_Commit(void* Address, size_t Size) {
	OS_Commit_Memory(Address, Size);
}

function void RPMalloc_Memory_Decommit(void* Address, size_t Size) {
	OS_Decommit_Memory(Address, Size);
}

function void RPMalloc_Memory_Unmap(void* Address, size_t Offset, size_t MappedSize) {
	Address = Offset_Pointer(Address, -(intptr_t)Offset);
	OS_Release_Memory(Address, MappedSize);
}

global rpmalloc_interface_t Memory_VTable = {
	.memory_map = RPMalloc_Memory_Map,
	.memory_commit = RPMalloc_Memory_Commit,
	.memory_decommit = RPMalloc_Memory_Decommit,
	.memory_unmap = RPMalloc_Memory_Unmap
};

void OS_Base_Init(base* Base) {
    static posix_base Posix;

    Base_Set(Base);

    Posix.Base.VTable = &Posix_Base_VTable;
    Posix.Base.PageSize = (size_t)sysconf(_SC_PAGESIZE);
    Posix.Base.ProcessorThreadCount = (size_t)sysconf(_SC_NPROCESSORS_ONLN);
    G_Posix = &Posix;

    Base->OSBase = (os_base*)&Posix;

    rpmalloc_config_t Config = {
		.unmap_on_finalize = true
	};

	rpmalloc_initialize_config(&Memory_VTable, &Config);

	pthread_mutex_init(&Posix.ResourceLock, 0);
    pthread_rwlock_init(&Posix.ArenaLock.Lock, NULL);
	Base->ArenaLock = &Posix.ArenaLock;
    Posix.Base.ResourceArena = Arena_Create(String_Lit("OS Base Resources"));

    Base->ThreadContextTLS = OS_TLS_Create();
    Base->ThreadContextLock = OS_Mutex_Create();

    arena* Scratch = Scratch_Get();
    string ExecutablePath = Posix_Get_Executable_Path((allocator*)Scratch);
    Posix.Base.ProgramPath = String_Copy((allocator*)Posix.Base.ResourceArena, String_Get_Directory_Path(ExecutablePath));

    Scratch_Release();
}

void OS_Base_Shutdown(base* Base) {
	posix_base* Posix = (posix_base*)Base->OSBase;
    pthread_mutex_destroy(&Posix->ResourceLock);
}