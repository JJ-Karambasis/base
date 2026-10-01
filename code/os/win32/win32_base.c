#include "../../base.h"

#include "win32_base.h"
#include "../../third_party/rpmalloc/rpmalloc.h"

Dynamic_Array_Implement_Type(string, String);
Array_Implement(string, String);

global win32_base* G_Win32;

function win32_base* Win32_Get() {
    return G_Win32;
}

function string Win32_Get_Error_Message(allocator* Allocator, DWORD Error) {
    if (Error == 0) {
        return String_Lit("No error code was set");
    }

    wchar_t* Buffer = NULL;
    DWORD Size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&Buffer,
        0,
        NULL);

    if (Size == 0 || !Buffer) {
        return String_Lit("Unknown error");
    }

    while (Size > 0 && (Buffer[Size - 1] == L'\n' || Buffer[Size - 1] == L'\r' || Buffer[Size - 1] == L' ')) {
        Size--;
    }

    string Result = String_From_WString(Allocator, Make_WString(Buffer, Size));
    LocalFree(Buffer);
    return Result;
}

function void Win32_Log_Last_Error(string Operation, string Path) {
    DWORD Error = GetLastError();
    arena* Scratch = Scratch_Get();
    string Message = Win32_Get_Error_Message((allocator*)Scratch, Error);

    if (Path.Size > 0) {
        Debug_Log("%.*s failed for '%.*s': %.*s (error %lu)",
                  Operation.Size, Operation.Ptr,
                  Path.Size, Path.Ptr,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
    } else {
        Debug_Log("%.*s failed: %.*s (error %lu)",
                  Operation.Size, Operation.Ptr,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
    }

    Scratch_Release();
}

function void Win32_Log_Last_Error_For_Size(string Operation, u64 Size) {
    DWORD Error = GetLastError();
    arena* Scratch = Scratch_Get();
    string Message = Win32_Get_Error_Message((allocator*)Scratch, Error);
    Debug_Log("%.*s failed for %llu bytes: %.*s (error %lu)",
              Operation.Size, Operation.Ptr,
              (unsigned long long)Size,
              Message.Size, Message.Ptr,
              (unsigned long)Error);
    Scratch_Release();
}

function string Win32_Get_File_Path_From_Handle(arena* Scratch, HANDLE Handle) {
    if (!Handle || Handle == INVALID_HANDLE_VALUE) {
        return String_Empty();
    }

    DWORD Size = GetFinalPathNameByHandleW(Handle, NULL, 0, VOLUME_NAME_DOS);
    if (Size == 0) {
        return String_Empty();
    }

    wchar_t* Buffer = (wchar_t*)Arena_Push(Scratch, sizeof(wchar_t) * (Size + 1));
    if (GetFinalPathNameByHandleW(Handle, Buffer, Size + 1, VOLUME_NAME_DOS) == 0) {
        return String_Empty();
    }

    return String_From_WString((allocator*)Scratch, WString_Null_Term(Buffer));
}

function void Win32_Log_Last_Error_For_Handle(string Operation, HANDLE Handle) {
    DWORD Error = GetLastError();
    arena* Scratch = Scratch_Get();
    string Message = Win32_Get_Error_Message((allocator*)Scratch, Error);
    string Path = Win32_Get_File_Path_From_Handle(Scratch, Handle);

    if (Path.Size > 0) {
        Debug_Log("%.*s failed for '%.*s': %.*s (error %lu)",
                  Operation.Size, Operation.Ptr,
                  Path.Size, Path.Ptr,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
    } else {
        Debug_Log("%.*s failed: %.*s (error %lu)",
                  Operation.Size, Operation.Ptr,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
    }

    Scratch_Release();
}

function OS_RESERVE_MEMORY_DEFINE(Win32_Reserve_Memory) {
    void* Result = VirtualAlloc(NULL, ReserveSize, MEM_RESERVE, PAGE_READWRITE);
    if (Result) {
        os_base* Base = (os_base*)Win32_Get();
        Atomic_Add_U64(&Base->ReservedAmount, ReserveSize);
        Atomic_Increment_U64(&Base->ReservedCount);
    } else {
        Win32_Log_Last_Error_For_Size(String_Lit("VirtualAlloc reserve"), ReserveSize);
    }
    return Result;
}

function OS_COMMIT_MEMORY_DEFINE(Win32_Commit_Memory) {
    void* Result = VirtualAlloc(BaseAddress, CommitSize, MEM_COMMIT, PAGE_READWRITE);
    if (Result) {
        os_base* Base = (os_base*)Win32_Get();
        Atomic_Add_U64(&Base->CommittedAmount, CommitSize);
        Atomic_Increment_U64(&Base->CommittedCount);
    } else {
        Win32_Log_Last_Error_For_Size(String_Lit("VirtualAlloc commit"), CommitSize);
    }
    return Result;
}

function OS_DECOMMIT_MEMORY_DEFINE(Win32_Decommit_Memory) {
    if (BaseAddress) {
        os_base* Base = (os_base*)Win32_Get();
        Atomic_Sub_U64(&Base->CommittedAmount, DecommitSize);
        Atomic_Decrement_U64(&Base->CommittedCount);
        VirtualFree(BaseAddress, DecommitSize, MEM_DECOMMIT);
    }
}

function OS_RELEASE_MEMORY_DEFINE(Win32_Release_Memory) {
    if (BaseAddress) {
        os_base* Base = (os_base*)Win32_Get();
        
        MEMORY_BASIC_INFORMATION MemoryInfo;
        u8* CurrentAddress = (u8*)BaseAddress;
        u8* EndAddress = CurrentAddress + ReleaseSize;
        
        while (CurrentAddress < EndAddress) {
            VirtualQuery(CurrentAddress, &MemoryInfo, sizeof(MemoryInfo));
            if (MemoryInfo.State == MEM_COMMIT) {
                Atomic_Sub_U64(&Base->CommittedAmount, MemoryInfo.RegionSize);
            }
            
            CurrentAddress = (u8*)Offset_Pointer(MemoryInfo.BaseAddress, MemoryInfo.RegionSize);
        }
        
        
        Atomic_Sub_U64(&Base->ReservedAmount, ReleaseSize);
        Atomic_Decrement_U64(&Base->ReservedCount);
        VirtualFree(BaseAddress, 0, MEM_RELEASE);
    }
}

function OS_QUERY_PERFORMANCE_DEFINE(Win32_Query_Performance_Counter) {
    u64 Result;
    QueryPerformanceCounter((LARGE_INTEGER*)&Result);
    return Result;
}

function OS_QUERY_PERFORMANCE_DEFINE(Win32_Query_Performance_Frequency) {
    u64 Result;
    QueryPerformanceFrequency((LARGE_INTEGER *)&Result);
    return Result;
}

function OS_OPEN_FILE_DEFINE(Win32_Open_File) {
    DWORD DesiredAccess = 0;
    DWORD CreationType = 0;
    
    if (Attributes == (OS_FILE_ATTRIBUTE_READ|OS_FILE_ATTRIBUTE_WRITE)) {
        DesiredAccess = GENERIC_READ | GENERIC_WRITE;
        CreationType = OPEN_ALWAYS;
    } else if (Attributes == OS_FILE_ATTRIBUTE_READ) {
        DesiredAccess = GENERIC_READ;
        CreationType = OPEN_EXISTING;
    } else if (Attributes == OS_FILE_ATTRIBUTE_WRITE) {
        DesiredAccess = GENERIC_WRITE;
        CreationType = CREATE_ALWAYS;
    } else {
        Assert(!"Invalid file attributes!");
        return NULL;
    }
    
    os_file* Result = NULL;
    
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Path);
    HANDLE Handle = CreateFileW(PathW.Ptr, DesiredAccess, 0, NULL, CreationType, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if(Handle != INVALID_HANDLE_VALUE) {
        
        win32_base* Win32 = Win32_Get();
        
        EnterCriticalSection(&Win32->ResourceLock);
        Result = Win32->FreeFiles;
        if (Result) SLL_Pop_Front(Win32->FreeFiles);
        else Result = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_file);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Memory_Clear(Result, sizeof(os_file));
        Result->Handle = Handle;
        
        Atomic_Increment_U64(&Win32->Base.AllocatedFileCount);
    } else {
        Win32_Log_Last_Error(String_Lit("CreateFileW"), Path);
    }
    
    Scratch_Release();
    
    return Result;
}

function OS_GET_FILE_SIZE_DEFINE(Win32_Get_File_Size) {
    Assert(File);
    if (!File) return 0;
    
    LARGE_INTEGER Result;
    if (!GetFileSizeEx(File->Handle, &Result)) {
        Win32_Log_Last_Error_For_Handle(String_Lit("GetFileSizeEx"), File->Handle);
        return 0;
    }
    return Result.QuadPart;
}

function OS_READ_FILE_DEFINE(Win32_Read_File) {
    Assert(File);
    if (!File) return false;
    
    u32 ReadSizeTrunc = (u32)ReadSize;
    DWORD BytesRead;
    
    if (!ReadFile(File->Handle, Data, ReadSizeTrunc, &BytesRead, NULL)) {
        Win32_Log_Last_Error_For_Handle(String_Lit("ReadFile"), File->Handle);
        return false;
    }
    
    if ((u64)BytesRead != ReadSize) {
        arena* Scratch = Scratch_Get();
        string Path = Win32_Get_File_Path_From_Handle(Scratch, File->Handle);
        Debug_Log("ReadFile incomplete read for '%.*s': expected %llu bytes, got %lu",
                    Path.Size, Path.Ptr,
                    (unsigned long long)ReadSize,
                    (unsigned long)BytesRead);
        Scratch_Release();
        return false;
    }
    
    return true;
}

function OS_WRITE_FILE_DEFINE(Win32_Write_File) {
    Assert(File);
    if (!File) return false;
    
    u32 WriteSizeTrunc = (u32)WriteSize;
    DWORD BytesWritten;
    
    if (!WriteFile(File->Handle, Data, WriteSizeTrunc, &BytesWritten, NULL)) {
        Win32_Log_Last_Error_For_Handle(String_Lit("WriteFile"), File->Handle);
        return false;
    }
    
    if ((u64)BytesWritten != WriteSize) {
        arena* Scratch = Scratch_Get();
        string Path = Win32_Get_File_Path_From_Handle(Scratch, File->Handle);
        Debug_Log("WriteFile incomplete write for '%.*s': expected %llu bytes, wrote %lu",
                    Path.Size, Path.Ptr,
                    (unsigned long long)WriteSize,
                    (unsigned long)BytesWritten);
        Scratch_Release();
        return false;
    }
    
    return true;
}

function OS_SET_FILE_POINTER_DEFINE(Win32_Set_File_Pointer) {
    Assert(File);
    if(!File) return;
    LARGE_INTEGER Offset; 
    Offset.QuadPart = Pointer;
    if (!SetFilePointerEx(File->Handle, Offset, NULL, FILE_BEGIN)) {
        Win32_Log_Last_Error_For_Handle(String_Lit("SetFilePointerEx"), File->Handle);
    }
}

function OS_GET_FILE_POINTER_DEFINE(Win32_Get_File_Pointer) {
    Assert(File);
    if(!File) return (u64)-1;
    LARGE_INTEGER Offset;
    Offset.QuadPart = 0;
    
    LARGE_INTEGER Result;
    if (!SetFilePointerEx(File->Handle, Offset, &Result, FILE_CURRENT)) {
        Win32_Log_Last_Error_For_Handle(String_Lit("SetFilePointerEx"), File->Handle);
        return (u64)-1;
    }
    return Result.QuadPart;
}

function OS_CLOSE_FILE_DEFINE(Win32_Close_File) {
    Assert(File);
    if (!File) return;
    
    win32_base* Win32 = Win32_Get();
    
    CloseHandle(File->Handle);
    
    EnterCriticalSection(&Win32->ResourceLock);
    SLL_Push_Front(Win32->FreeFiles, File);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Atomic_Decrement_U64(&Win32->Base.AllocatedFileCount);
}

function void Win32_Get_All_Files_Recursive(allocator* Allocator, dynamic_string_array* Array, string Directory, b32 Recursive) {
    arena* Scratch = Scratch_Get();
    
    if (!String_Ends_With_Char(Directory, '\\') && !String_Ends_With_Char(Directory, '/')) {
        Directory = String_Concat((allocator*)Scratch, Directory, String_Lit("\\"));
    }
    
    string DirectoryWithWildcard = String_Concat((allocator*)Scratch, Directory, String_Lit("*"));
    
    
    WIN32_FIND_DATAW FindData;
    wstring DirectoryW = WString_From_String((allocator*)Scratch, DirectoryWithWildcard);
    HANDLE Handle = FindFirstFileW(DirectoryW.Ptr, &FindData);
    while (Handle != INVALID_HANDLE_VALUE) {
        string FileOrDirectoryName = String_From_WString((allocator*)Scratch, WString_Null_Term(FindData.cFileName));
        if (!String_Equals(FileOrDirectoryName, String_Lit(".")) && !String_Equals(FileOrDirectoryName, String_Lit(".."))) {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                string DirectoryName = FileOrDirectoryName;
                if (Recursive) {
                    string DirectoryPath = String_Directory_Concat((allocator*)Scratch, Directory, DirectoryName);
                    Win32_Get_All_Files_Recursive(Allocator, Array, DirectoryPath, true);
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
        
        if (!FindNextFileW(Handle, &FindData)) {
            break;
        }
    }
    
    Scratch_Release();
}

function OS_GET_ALL_FILES_DEFINE(Win32_Get_All_Files) {
    dynamic_string_array Array = Dynamic_String_Array_Init(Allocator);
    Win32_Get_All_Files_Recursive(Allocator, &Array, Path, Recursive);
    return String_Array_Init(Array.Ptr, Array.Count);
}

function OS_IS_PATH_DEFINE(Win32_Is_File_Path) {
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Path);
    DWORD Attributes = GetFileAttributesW(PathW.Ptr);
    Scratch_Release();
    return ((Attributes != INVALID_FILE_ATTRIBUTES) && 
            !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
}

function OS_IS_PATH_DEFINE(Win32_Is_Directory_Path) {
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Path);
    DWORD Attributes = GetFileAttributesW(PathW.Ptr);
    Scratch_Release();
    return ((Attributes != INVALID_FILE_ATTRIBUTES) && 
            (Attributes & FILE_ATTRIBUTE_DIRECTORY));
}

function OS_MAKE_DIRECTORY_DEFINE(Win32_Make_Directory) {
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Directory);
    BOOL Result = CreateDirectoryW(PathW.Ptr, NULL);
    if (!Result) {
        if(GetLastError() != ERROR_ALREADY_EXISTS) {
            Win32_Log_Last_Error(String_Lit("CreateDirectoryW"), Directory);
        }
    }
    Scratch_Release();
    return Result;
}

function OS_DELETE_FILE_DEFINE(Win32_Delete_File) {
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Path);
    BOOL Result = DeleteFileW(PathW.Ptr);
    if (!Result) {
        Win32_Log_Last_Error(String_Lit("DeleteFileW"), Path);
    }
    Scratch_Release();
    return Result;
}

/* Recursive helper for Win32_Delete_Directory. Mirrors the FindFirstFileW /
   FindNextFileW pattern used by Win32_Get_All_Files_Recursive: enumerate
   every entry, recurse into subdirectories, then remove this directory.
   Win32 RemoveDirectoryW only succeeds on empty directories, so we must
   drain children before removing the parent. */
function b32 Win32_Delete_Directory_Recursive(string Directory) {
    arena* Scratch = Scratch_Get();

    if (!String_Ends_With_Char(Directory, '\\') && !String_Ends_With_Char(Directory, '/')) {
        Directory = String_Concat((allocator*)Scratch, Directory, String_Lit("\\"));
    }

    string DirectoryWithWildcard = String_Concat((allocator*)Scratch, Directory, String_Lit("*"));

    b32 Result = true;

    WIN32_FIND_DATAW FindData;
    wstring DirectoryW = WString_From_String((allocator*)Scratch, DirectoryWithWildcard);
    HANDLE Handle = FindFirstFileW(DirectoryW.Ptr, &FindData);
    if (Handle == INVALID_HANDLE_VALUE) {
        Win32_Log_Last_Error(String_Lit("FindFirstFileW"), Directory);
        Result = false;
    } else {
        do {
            string FileOrDirectoryName = String_From_WString((allocator*)Scratch, WString_Null_Term(FindData.cFileName));
            if (String_Equals(FileOrDirectoryName, String_Lit(".")) || String_Equals(FileOrDirectoryName, String_Lit(".."))) {
                continue;
            }

            string ChildPath = String_Concat((allocator*)Scratch, Directory, FileOrDirectoryName);
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!Win32_Delete_Directory_Recursive(ChildPath)) {
                    Result = false;
                }
            } else {
                /* Read-only files refuse DeleteFileW; clear the attribute
                   first so the wipe doesn't bail on something the user
                   forgot they marked read-only at some point. */
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    wstring ChildPathW = WString_From_String((allocator*)Scratch, ChildPath);
                    SetFileAttributesW(ChildPathW.Ptr, FindData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                }
                wstring ChildPathW = WString_From_String((allocator*)Scratch, ChildPath);
                if (!DeleteFileW(ChildPathW.Ptr)) {
                    Win32_Log_Last_Error(String_Lit("DeleteFileW"), ChildPath);
                    Result = false;
                }
            }
        } while (FindNextFileW(Handle, &FindData));
        FindClose(Handle);
    }

    wstring DirOnlyW = WString_From_String((allocator*)Scratch, Directory);
    if (!RemoveDirectoryW(DirOnlyW.Ptr)) {
        Win32_Log_Last_Error(String_Lit("RemoveDirectoryW"), Directory);
        Result = false;
    }

    Scratch_Release();
    return Result;
}

function OS_DELETE_DIRECTORY_DEFINE(Win32_Delete_Directory) {
    if (Recursive) {
        return Win32_Delete_Directory_Recursive(Directory);
    }

    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Directory);
    BOOL Result = RemoveDirectoryW(PathW.Ptr);
    if (!Result) {
        Win32_Log_Last_Error(String_Lit("RemoveDirectoryW"), Directory);
    }
    Scratch_Release();
    return Result;
}

function OS_COPY_FILE_DEFINE(Win32_Copy_File) {
    arena* Scratch = Scratch_Get();
    wstring SrcFileW = WString_From_String((allocator*)Scratch, SrcFilePath);
    wstring DstFileW = WString_From_String((allocator*)Scratch, DstFilePath);
    BOOL Result = CopyFileW(SrcFileW.Ptr, DstFileW.Ptr, FALSE);
    if (!Result) {
        DWORD Error = GetLastError();
        string Message = Win32_Get_Error_Message((allocator*)Scratch, Error);
        Debug_Log("CopyFileW failed copying '%.*s' to '%.*s': %.*s (error %lu)",
                  SrcFilePath.Size, SrcFilePath.Ptr,
                  DstFilePath.Size, DstFilePath.Ptr,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
    }
    Scratch_Release();
    return Result;
}

function OS_SANITIZE_PATH_DEFINE(Win32_Sanitize_Path) {
    if (String_Is_Empty(Path)) {
        return String_Empty();
    }
    arena* Scratch = Scratch_Get();
    wstring PathW = WString_From_String((allocator*)Scratch, Path);
    DWORD Size = GetFullPathNameW(PathW.Ptr, 0, NULL, NULL);
    if (!Size) {
        Win32_Log_Last_Error(String_Lit("GetFullPathNameW"), Path);
        Scratch_Release();
        return Path;
    }
    wchar_t* Buffer = (wchar_t*)Arena_Push(Scratch, sizeof(wchar_t) * Size);
    DWORD FinalSize = GetFullPathNameW(PathW.Ptr, Size, Buffer, NULL);
    if (FinalSize == 0 || FinalSize >= Size) {
        Win32_Log_Last_Error(String_Lit("GetFullPathNameW"), Path);
        Scratch_Release();
        return Path;
    }
    string Result = String_From_WString(Allocator, Make_WString(Buffer, FinalSize));
    Scratch_Release();
    return Result;
}

function OS_TLS_CREATE_DEFINE(Win32_TLS_Create) {
    win32_base* Win32 = Win32_Get();
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_tls* TLS = Win32->FreeTLS;
    if (TLS) SLL_Pop_Front(Win32->FreeTLS);
    else TLS = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_tls);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(TLS, sizeof(os_tls));
    TLS->Index = TlsAlloc();
    if (TLS->Index == TLS_OUT_OF_INDEXES) {
        Win32_Log_Last_Error(String_Lit("TlsAlloc"), String_Empty());
    }
    
    Atomic_Increment_U64(&Win32->Base.AllocatedTLSCount);
    
    return TLS;
}

function OS_TLS_DELETE_DEFINE(Win32_TLS_Delete) {
    win32_base* Win32 = Win32_Get();
    
    TlsFree(TLS->Index);
    
    EnterCriticalSection(&Win32->ResourceLock);
    SLL_Push_Front(Win32->FreeTLS, TLS);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Atomic_Decrement_U64(&Win32->Base.AllocatedTLSCount);
}

function OS_TLS_GET_DEFINE(Win32_TLS_Get) {
    return TlsGetValue(TLS->Index);
}

function OS_TLS_SET_DEFINE(Win32_TLS_Set) {
    if (!TlsSetValue(TLS->Index, Data)) {
        Win32_Log_Last_Error(String_Lit("TlsSetValue"), String_Empty());
    }
}

function DWORD Win32_Thread_Callback(LPVOID Parameter) {
    os_thread* Thread = (os_thread*)Parameter;
    rpmalloc_thread_initialize();
    Thread->Callback(Thread, Thread->UserData);
    Thread_Context_Remove();
    rpmalloc_thread_finalize();
    return 0;
}

function OS_THREAD_CREATE_DEFINE(Win32_Thread_Create) {
    win32_base* Win32 = Win32_Get();
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_thread* Thread = Win32->FreeThreads;
    if (Thread) SLL_Pop_Front(Win32->FreeThreads);
    else Thread = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_thread);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Thread, sizeof(os_thread));
    Thread->Callback = Callback;
    Thread->UserData = UserData;
    Thread->Handle = CreateThread(NULL, 0, Win32_Thread_Callback, Thread, 0, &Thread->ThreadID);
    if (Thread->Handle == NULL) {
        Win32_Log_Last_Error(String_Lit("CreateThread"), DebugName);
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeThreads, Thread);
        LeaveCriticalSection(&Win32->ResourceLock);
        Thread = NULL;
    } else {
        if (!String_Is_Empty(DebugName)) {
            //Make sure its null terminated
            arena* Scratch = Scratch_Get();
            DebugName = String_Copy((allocator*)Scratch, DebugName);
            
            THREADNAME_INFO ThreadInfo = {
                .dwType = 0x1000,
                .szName = DebugName.Ptr,
                .dwThreadID = Thread->ThreadID
            };
            
            __try {
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(ThreadInfo) / sizeof(ULONG_PTR), (ULONG_PTR*)&ThreadInfo);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            
            
            Scratch_Release();
        }
        
        Atomic_Increment_U64(&Win32->Base.AllocatedThreadCount);
    }
    
    return Thread;
}

function OS_THREAD_JOIN_DEFINE(Win32_Thread_Join) {
    if (Thread) {
        win32_base* Win32 = Win32_Get();
        if (WaitForSingleObject(Thread->Handle, INFINITE) == WAIT_FAILED) {
            Win32_Log_Last_Error(String_Lit("WaitForSingleObject"), String_Empty());
        }
        CloseHandle(Thread->Handle);
        
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeThreads, Thread);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedThreadCount);
    }
}

function OS_THREAD_GET_ID_DEFINE(Win32_Thread_Get_ID) {
    return Thread ? (u64)Thread->ThreadID : 0;
}

function OS_GET_CURRENT_THREAD_ID_DEFINE(Win32_Get_Current_Thread_ID) {
    return (u64)GetCurrentThreadId();
}

function OS_MUTEX_CREATE_DEFINE(Win32_Mutex_Create) {
    win32_base* Win32 = Win32_Get();
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_mutex* Mutex = Win32->FreeMutex;
    if (Mutex) SLL_Pop_Front(Win32->FreeMutex);
    else Mutex = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_mutex);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Mutex, sizeof(os_mutex));
    InitializeCriticalSection(&Mutex->CriticalSection);
    
    Atomic_Increment_U64(&Win32->Base.AllocatedMutexCount);
    
    return Mutex;
}

function OS_MUTEX_DELETE_DEFINE(Win32_Mutex_Delete) {
    win32_base* Win32 = Win32_Get();
    
    DeleteCriticalSection(&Mutex->CriticalSection);
    
    EnterCriticalSection(&Win32->ResourceLock);
    SLL_Push_Front(Win32->FreeMutex, Mutex);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Atomic_Decrement_U64(&Win32->Base.AllocatedMutexCount);
}

function OS_MUTEX_LOCK_DEFINE(Win32_Mutex_Lock) {
    EnterCriticalSection(&Mutex->CriticalSection);
}

function OS_MUTEX_LOCK_DEFINE(Win32_Mutex_Unlock) {
    LeaveCriticalSection(&Mutex->CriticalSection);
}

function OS_RW_MUTEX_CREATE_DEFINE(Win32_RW_Mutex_Create) {
    win32_base* Win32 = Win32_Get();
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_rw_mutex* Mutex = Win32->FreeRWMutex;
    if (Mutex) SLL_Pop_Front(Win32->FreeRWMutex);
    else Mutex = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_rw_mutex);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Mutex, sizeof(os_rw_mutex));
    InitializeSRWLock(&Mutex->SRWLock);
    
    Atomic_Increment_U64(&Win32->Base.AllocatedRWMutexCount);
    
    return Mutex;
}

function OS_RW_MUTEX_DELETE_DEFINE(Win32_RW_Mutex_Delete) {
    win32_base* Win32 = Win32_Get();
    EnterCriticalSection(&Win32->ResourceLock);
    SLL_Push_Front(Win32->FreeRWMutex, Mutex);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Atomic_Decrement_U64(&Win32->Base.AllocatedRWMutexCount);
}

function OS_RW_MUTEX_TRY_LOCK_DEFINE(Win32_RW_Mutex_Try_Read_Lock) {
    if(!Mutex) return false;
    return TryAcquireSRWLockShared(&Mutex->SRWLock);
}

function OS_RW_MUTEX_LOCK_DEFINE(Win32_RW_Mutex_Read_Lock) {
    AcquireSRWLockShared(&Mutex->SRWLock);
}

function OS_RW_MUTEX_LOCK_DEFINE(Win32_RW_Mutex_Read_Unlock) {
    ReleaseSRWLockShared(&Mutex->SRWLock);
}

function OS_RW_MUTEX_LOCK_DEFINE(Win32_RW_Mutex_Write_Lock) {
    AcquireSRWLockExclusive(&Mutex->SRWLock);
}

function OS_RW_MUTEX_LOCK_DEFINE(Win32_RW_Mutex_Write_Unlock) {
    ReleaseSRWLockExclusive(&Mutex->SRWLock);
}

function OS_SEMAPHORE_CREATE_DEFINE(Win32_Semaphore_Create) {
    HANDLE Handle = CreateSemaphoreA(NULL, (LONG)InitialCount, LONG_MAX, NULL);
    if (Handle == NULL) {
        Win32_Log_Last_Error(String_Lit("CreateSemaphoreA"), String_Empty());
        return NULL;
    }
    
    win32_base* Win32 = Win32_Get();
    EnterCriticalSection(&Win32->ResourceLock);
    os_semaphore* Semaphore = Win32->FreeSemaphores;
    if (Semaphore) SLL_Pop_Front(Win32->FreeSemaphores);
    else Semaphore = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_semaphore);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Semaphore, sizeof(os_semaphore));
    Semaphore->Handle = Handle;
    
    Atomic_Increment_U64(&Win32->Base.AllocatedSemaphoresCount);
    
    return Semaphore;
}

function OS_SEMAPHORE_DELETE_DEFINE(Win32_Semaphore_Delete) {
    if (Semaphore && Semaphore->Handle) {
        CloseHandle(Semaphore->Handle);
        
        win32_base* Win32 = Win32_Get();
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeSemaphores, Semaphore);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedSemaphoresCount);
    }
}

function OS_SEMAPHORE_INCREMENT_DEFINE(Win32_Semaphore_Increment) {
    if (Semaphore && Semaphore->Handle) {
        if (!ReleaseSemaphore(Semaphore->Handle, 1, NULL)) {
            Win32_Log_Last_Error(String_Lit("ReleaseSemaphore"), String_Empty());
        }
    }
}

function OS_SEMAPHORE_DECREMENT_DEFINE(Win32_Semaphore_Decrement) {
    if (Semaphore && Semaphore->Handle) {
        if (WaitForSingleObject(Semaphore->Handle, INFINITE) == WAIT_FAILED) {
            Win32_Log_Last_Error(String_Lit("WaitForSingleObject"), String_Empty());
        }
    }
}

function OS_SEMAPHORE_ADD_DEFINE(Win32_Semaphore_Add) {
    if (Semaphore && Semaphore->Handle) {
        if (!ReleaseSemaphore(Semaphore->Handle, Count, NULL)) {
            Win32_Log_Last_Error(String_Lit("ReleaseSemaphore"), String_Empty());
        }
    }
}

function OS_EVENT_CREATE_DEFINE(Win32_Event_Create) {
    HANDLE Handle = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (Handle == NULL) {
        Win32_Log_Last_Error(String_Lit("CreateEventA"), String_Empty());
        return NULL;
    }
    
    win32_base* Win32 = Win32_Get();
    EnterCriticalSection(&Win32->ResourceLock);
    os_event* Event = Win32->FreeEvents;
    if (Event) SLL_Pop_Front(Win32->FreeEvents);
    else Event = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_event);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Event, sizeof(os_event));
    Event->Handle = Handle;
    
    Atomic_Increment_U64(&Win32->Base.AllocatedEventsCount);
    
    return Event;
}

function OS_EVENT_DELETE_DEFINE(Win32_Event_Delete) {
    if (Event && Event->Handle != NULL) {
        CloseHandle(Event->Handle);
        
        win32_base* Win32 = Win32_Get();
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeEvents, Event);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedEventsCount);
    }
}

function OS_EVENT_WAIT_DEFINE(Win32_Event_Wait) {
    if (WaitForSingleObject(Event->Handle, INFINITE) == WAIT_FAILED) {
        Win32_Log_Last_Error(String_Lit("WaitForSingleObject"), String_Empty());
    }
}

function OS_EVENT_SIGNAL_DEFINE(Win32_Event_Signal) {
    if (!SetEvent(Event->Handle)) {
        Win32_Log_Last_Error(String_Lit("SetEvent"), String_Empty());
    }
}

function OS_EVENT_RESET_DEFINE(Win32_Event_Reset) {
    if (!ResetEvent(Event->Handle)) {
        Win32_Log_Last_Error(String_Lit("ResetEvent"), String_Empty());
    }
}

function OS_HOT_RELOAD_CREATE_DEFINE(Win32_Hot_Reload_Create) {
    wstring FilePathW = WString_From_String(Default_Allocator_Get(), FilePath);
    
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
    if (!GetFileAttributesExW(FilePathW.Ptr, GetFileExInfoStandard, &FileAttributes)) {
        Win32_Log_Last_Error(String_Lit("GetFileAttributesExW"), FilePath);
        Allocator_Free_Memory(Default_Allocator_Get(), (void*)FilePathW.Ptr);
        return NULL;
    }
    
    win32_base* Win32 = Win32_Get();
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_hot_reload* HotReload = Win32->FreeHotReload;
    if (HotReload) SLL_Pop_Front(Win32->FreeHotReload);
    else HotReload = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_hot_reload);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(HotReload, sizeof(os_hot_reload));
    HotReload->LastWriteTime = FileAttributes.ftLastWriteTime;
    HotReload->FilePath = FilePathW;
    
    Atomic_Increment_U64(&Win32->Base.AllocatedHotReloadCount);
    
    return HotReload;
}

function OS_HOT_RELOAD_DELETE_DEFINE(Win32_Hot_Reload_Delete) {
    if (HotReload) {
        Allocator_Free_Memory(Default_Allocator_Get(), (void*)HotReload->FilePath.Ptr);
        win32_base* Win32 = Win32_Get();
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeHotReload, HotReload);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedHotReloadCount);
    }
}

function OS_HOT_RELOAD_HAS_RELOADED_DEFINE(Win32_Hot_Reload_Has_Reloaded) {
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
    GetFileAttributesExW(HotReload->FilePath.Ptr, GetFileExInfoStandard, &FileAttributes);
    
    if (CompareFileTime(&FileAttributes.ftLastWriteTime, &HotReload->LastWriteTime) > 0) {
        HotReload->LastWriteTime = FileAttributes.ftLastWriteTime;
        return true;
    }
    
    return false;
}

function OS_LIBRARY_CREATE_DEFINE(Win32_Library_Create) {
    arena* Scratch = Scratch_Get();
    wstring LibraryPathW = WString_From_String((allocator*)Scratch, LibraryPath);
    HMODULE Library = LoadLibraryW(LibraryPathW.Ptr);
    if (!Library) {
        Win32_Log_Last_Error(String_Lit("LoadLibraryW"), LibraryPath);
        Scratch_Release();
        return NULL;
    }
    Scratch_Release();
    
    win32_base* Win32 = Win32_Get();
    EnterCriticalSection(&Win32->ResourceLock);
    os_library* Result = Win32->FreeLibrary;
    if (Result) SLL_Pop_Front(Win32->FreeLibrary);
    else Result = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_library);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Result, sizeof(os_library));
    Result->Library = Library;
    
    Atomic_Increment_U64(&Win32->Base.AllocatedLibraryCount);
    
    return Result;
}

function OS_LIBRARY_DELETE_DEFINE(Win32_Library_Delete) {
    if (Library && Library->Library) {
        FreeLibrary(Library->Library);
        
        win32_base* Win32 = Win32_Get();
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeLibrary, Library);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedLibraryCount);
    }
}

function OS_LIBRARY_GET_FUNCTION_DEFINE(Win32_Library_Get_Function) {
    void* Result = (void*)GetProcAddress(Library->Library, FunctionName);
    if (!Result) {
        DWORD Error = GetLastError();
        arena* Scratch = Scratch_Get();
        string Message = Win32_Get_Error_Message((allocator*)Scratch, Error);
        Debug_Log("GetProcAddress failed for '%s': %.*s (error %lu)",
                  FunctionName,
                  Message.Size, Message.Ptr,
                  (unsigned long)Error);
        Scratch_Release();
    }
    return Result;
}

function OS_CONDITION_VARIABLE_CREATE_DEFINE(Win32_Condition_Variable_Create) {
    win32_base* Win32 = Win32_Get();
    
    CONDITION_VARIABLE Handle;
    InitializeConditionVariable(&Handle);
    
    EnterCriticalSection(&Win32->ResourceLock);
    os_condition_variable* Result = Win32->FreeConditionVariables;
    if (Result) SLL_Pop_Front(Win32->FreeConditionVariables);
    else Result = Arena_Push_Struct_No_Clear(Win32->Base.ResourceArena, os_condition_variable);
    LeaveCriticalSection(&Win32->ResourceLock);
    
    Memory_Clear(Result, sizeof(os_condition_variable));
    Result->Handle = Handle;
    
    Atomic_Increment_U64(&Win32->Base.AllocatedConditionVariableCount);
    
    return Result;
}

function OS_CONDITION_VARIABLE_DELETE_DEFINE(Win32_Condition_Variable_Delete) {
    if (Variable) {
        win32_base* Win32 = Win32_Get();
        EnterCriticalSection(&Win32->ResourceLock);
        SLL_Push_Front(Win32->FreeConditionVariables, Variable);
        LeaveCriticalSection(&Win32->ResourceLock);
        
        Atomic_Decrement_U64(&Win32->Base.AllocatedConditionVariableCount);
    }
}

function OS_CONDITION_VARIABLE_WAIT_DEFINE(Win32_Condition_Variable_Wait) {
    if (Variable && Mutex) {
        if (!SleepConditionVariableCS(&Variable->Handle, &Mutex->CriticalSection, INFINITE)) {
            Win32_Log_Last_Error(String_Lit("SleepConditionVariableCS"), String_Empty());
        }
    }
}

function OS_CONDITION_VARIABLE_WAKE_DEFINE(Win32_Condition_Variable_Wake) {
    if (Variable) {
        WakeConditionVariable(&Variable->Handle);
    }
}

function OS_CONDITION_VARIABLE_WAKE_ALL_DEFINE(Win32_Condition_Variable_Wake_All) {
    if (Variable) {
        WakeAllConditionVariable(&Variable->Handle);
    }
}

function OS_GET_ENTROPY_DEFINE(Win32_Get_Entropy) {
    NTSTATUS Status = BCryptGenRandom(NULL, (PUCHAR)Buffer, (ULONG)Size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(Status)) {
        Debug_Log("BCryptGenRandom failed for %llu bytes: status 0x%08lx",
                  (unsigned long long)Size,
                  (unsigned long)Status);
    }
}

function OS_SLEEP_DEFINE(Win32_Sleep) {
    //todo: Higher resolution sleep with waitable timers
    if (Nanoseconds > 0) {
        DWORD Milliseconds = Max((DWORD)(Nanoseconds / 1000000), 1);
        Sleep(Milliseconds);
    }
}

global os_base_vtable Win32_Base_VTable = {
    .ReserveMemoryFunc = Win32_Reserve_Memory,
    .CommitMemoryFunc = Win32_Commit_Memory,
    .DecommitMemoryFunc = Win32_Decommit_Memory,
    .ReleaseMemoryFunc = Win32_Release_Memory,
    
    .QueryPerformanceCounterFunc = Win32_Query_Performance_Counter,
    .QueryPerformanceFrequencyFunc = Win32_Query_Performance_Frequency,
    
    .OpenFileFunc = Win32_Open_File,
    .GetFileSizeFunc = Win32_Get_File_Size,
    .ReadFileFunc = Win32_Read_File,
    .WriteFileFunc = Win32_Write_File,
    .SetFilePointerFunc = Win32_Set_File_Pointer,
    .GetFilePointerFunc = Win32_Get_File_Pointer,
    .CloseFileFunc = Win32_Close_File,
    
    .GetAllFilesFunc = Win32_Get_All_Files,
    .IsDirectoryPathFunc = Win32_Is_Directory_Path,
    .IsFilePathFunc = Win32_Is_File_Path,
    .MakeDirectoryFunc = Win32_Make_Directory,
    .DeleteFileFunc = Win32_Delete_File,
    .DeleteDirectoryFunc = Win32_Delete_Directory,
    .CopyFileFunc = Win32_Copy_File,
    .SanitizePathFunc = Win32_Sanitize_Path,
    
    .TLSCreateFunc = Win32_TLS_Create,
    .TLSDeleteFunc = Win32_TLS_Delete,
    .TLSGetFunc = Win32_TLS_Get,
    .TLSSetFunc = Win32_TLS_Set,
    
    .ThreadCreateFunc = Win32_Thread_Create,
    .ThreadJoinFunc = Win32_Thread_Join,
    .ThreadGetIdFunc = Win32_Thread_Get_ID,
    .GetCurrentThreadIdFunc = Win32_Get_Current_Thread_ID,
    
    .MutexCreateFunc = Win32_Mutex_Create,
    .MutexDeleteFunc = Win32_Mutex_Delete,
    .MutexLockFunc = Win32_Mutex_Lock,
    .MutexUnlockFunc = Win32_Mutex_Unlock,
    
    .RWMutexCreateFunc = Win32_RW_Mutex_Create,
    .RWMutexDeleteFunc = Win32_RW_Mutex_Delete,
    .RWMutexTryReadLockFunc = Win32_RW_Mutex_Try_Read_Lock,
    .RWMutexReadLockFunc = Win32_RW_Mutex_Read_Lock,
    .RWMutexReadUnlockFunc = Win32_RW_Mutex_Read_Unlock,
    .RWMutexWriteLockFunc = Win32_RW_Mutex_Write_Lock,
    .RWMutexWriteUnlockFunc = Win32_RW_Mutex_Write_Unlock,
    
    .SemaphoreCreateFunc = Win32_Semaphore_Create,
    .SemaphoreDeleteFunc = Win32_Semaphore_Delete,
    .SemaphoreIncrementFunc = Win32_Semaphore_Increment,
    .SemaphoreDecrementFunc = Win32_Semaphore_Decrement,
    .SemaphoreAddFunc = Win32_Semaphore_Add,
    
    .EventCreateFunc = Win32_Event_Create,
    .EventDeleteFunc = Win32_Event_Delete,
    .EventResetFunc = Win32_Event_Reset,
    .EventWaitFunc = Win32_Event_Wait,
    .EventSignalFunc = Win32_Event_Signal,
    
    .HotReloadCreateFunc = Win32_Hot_Reload_Create,
    .HotReloadDeleteFunc = Win32_Hot_Reload_Delete,
    .HotReloadHasReloadedFunc = Win32_Hot_Reload_Has_Reloaded,
    
    .LibraryCreateFunc = Win32_Library_Create,
    .LibraryDeleteFunc = Win32_Library_Delete,
    .LibraryGetFunctionFunc = Win32_Library_Get_Function,
    
    .ConditionVariableCreateFunc = Win32_Condition_Variable_Create,
    .ConditionVariableDeleteFunc = Win32_Condition_Variable_Delete,
    .ConditionVariableWaitFunc = Win32_Condition_Variable_Wait,
    .ConditionVariableWakeFunc = Win32_Condition_Variable_Wake,
    .ConditionVariableWakeAllFunc = Win32_Condition_Variable_Wake_All,
    
    .GetEntropyFunc = Win32_Get_Entropy,
    .SleepFunc = Win32_Sleep
};

function string Win32_Get_Executable_Path(allocator* Allocator) {
    DWORD MemorySize = 1024;
    for (int Iterations = 0; Iterations < 32; Iterations++) {
        arena* Scratch = Scratch_Get();
        wchar_t* Buffer = (wchar_t*)Arena_Push(Scratch, sizeof(wchar_t)*MemorySize);
        DWORD Size = GetModuleFileNameW(NULL, Buffer, MemorySize);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            string String = String_From_WString(Allocator, Make_WString(Buffer, Size));
            Scratch_Release();
            return String;
        }
        
        Scratch_Release();
        MemorySize *= 2;
    }
    
    Win32_Log_Last_Error(String_Lit("GetModuleFileNameW"), String_Empty());
    return String_Empty();
}

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

export_function void OS_Base_Init(base* Base) {
    static win32_base Win32;
    
    Base_Set(Base);
    
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    
    Win32.Base.VTable = &Win32_Base_VTable;
    Win32.Base.PageSize = SystemInfo.dwAllocationGranularity;
    Win32.Base.ProcessorThreadCount = SystemInfo.dwNumberOfProcessors;
    G_Win32 = &Win32;
    
    Base->OSBase = (os_base*)&Win32;
    
    rpmalloc_config_t Config = {
        .unmap_on_finalize = true
    };
    
    rpmalloc_initialize_config(&Memory_VTable, &Config);
    
    InitializeCriticalSection(&Win32.ResourceLock);
    InitializeSRWLock(&Win32.ArenaLock.SRWLock);
    Base->ArenaLock = &Win32.ArenaLock;
    Win32.Base.ResourceArena = Arena_Create(String_Lit("OS Base Resources"));
    
    Base->ThreadContextTLS = OS_TLS_Create();
    Base->ThreadContextLock = OS_Mutex_Create();
    
    arena* Scratch = Scratch_Get();
    string ExecutablePath = Win32_Get_Executable_Path((allocator*)Scratch);
    Win32.Base.ProgramPath = String_Copy((allocator*)Win32.Base.ResourceArena, String_Get_Directory_Path(ExecutablePath));
    Scratch_Release();
}

export_function void OS_Base_Shutdown(base* Base) {
    win32_base* Win32 = (win32_base*)Base->OSBase;
    
    DeleteCriticalSection(&Win32->ResourceLock);
}

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")