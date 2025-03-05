#include <pthread.h>
#include <sys/mman.h>
#import <Foundation/Foundation.h>
#include <sys/stat.h>
#include <dirent.h>

#include <base/base.h>
#include "osx_base.h"

#include <base/base.c>

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

function OS_READ_ENTIRE_FILE_DEFINE(OSX_Read_Entire_File) {
    arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }
    Assert(String_Is_Null_Term(Path));

    int FileHandle = open(Path.Ptr, O_RDONLY, 0);
    Scratch_Release();

    if(FileHandle == -1) {
        return Buffer_Empty();
    }

    size_t FileSize = lseek(FileHandle, 0, SEEK_END);
    lseek(FileHandle, 0, SEEK_SET);
    
    Assert(FileSize <= 0xFFFFFFFF);
    buffer Result = Buffer_Alloc(Allocator, FileSize);
    read(FileHandle, Result.Ptr, FileSize);
    close(FileHandle);
    return Result;
}

function OS_WRITE_ENTIRE_FILE_DEFINE(OSX_Write_Entire_File) {
	arena* Scratch = Scratch_Get();
    if(!String_Is_Null_Term(Path)) {
        Path = String_Copy((allocator*)Scratch, Path);
    }
    Assert(String_Is_Null_Term(Path));
    int FileHandle = open(Path.Ptr, O_WRONLY|O_CREAT, 0666);
	Scratch_Release();

    if (FileHandle == -1) {
		return false;
	}

	bool Result = write(FileHandle, Data.Ptr, Data.Size) == Data.Size;
	close(FileHandle);
	return Result;
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
            }
        }

        closedir(Dir);
    }

	Scratch_Release();
}

function OS_GET_ALL_FILES_DEFINE(OSX_Get_All_Files) {
	arena* Scratch = Scratch_Get();
    if(String_Is_Null_Term(Path)) {
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

global os_base_vtable OSX_Base_VTable = {
	.ReserveMemoryFunc = OSX_Reserve_Memory,
	.CommitMemoryFunc = OSX_Commit_Memory,
	.DecommitMemoryFunc = OSX_Decommit_Memory,
	.ReleaseMemoryFunc = OSX_Release_Memory,
	
	.QueryPerformanceCounterFunc = OSX_Query_Performance_Counter,
	.QueryPerformanceFrequencyFunc = OSX_Query_Performance_Frequency,

	.ReadEntireFileFunc = OSX_Read_Entire_File,
	.WriteEntireFileFunc = OSX_Write_Entire_File,
	.GetAllFilesFunc = OSX_Get_All_Files,
	.IsDirectoryPathFunc = OSX_Is_Directory_Path,
	.IsFilePathFunc = OSX_Is_File_Path,
	.MakeDirectoryFunc = OSX_Make_Directory,

	.TLSCreateFunc = OSX_TLS_Create,
	.TLSDeleteFunc = OSX_TLS_Delete,
	.TLSGetFunc = OSX_TLS_Get,
	.TLSSetFunc = OSX_TLS_Set,

	.MutexCreateFunc = OSX_Mutex_Create,
	.MutexDeleteFunc = OSX_Mutex_Delete,
	.MutexLockFunc = OSX_Mutex_Lock,
	.MutexUnlockFunc = OSX_Mutex_Unlock
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

        return String_Copy(Allocator, String_Null_Term(Base));
    }
}

void OS_Base_Init(base* Base) {
    static osx_base OSX;

    Base_Set(Base);

    OSX.Base.VTable = &OSX_Base_VTable;
    OSX.Base.PageSize = (size_t)sysconf(_SC_PAGESIZE);
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