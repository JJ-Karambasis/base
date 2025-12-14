#include "../../base.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include "win32_base.h"
#include "../../third_party/rpmalloc/rpmalloc.h"

Dynamic_Array_Implement_Type(string, String);
Array_Implement(string, String);

global win32_base* G_Win32;

function win32_base* Win32_Get() {
	return G_Win32;
}

function OS_RESERVE_MEMORY_DEFINE(Win32_Reserve_Memory) {
	void* Result = VirtualAlloc(NULL, ReserveSize, MEM_RESERVE, PAGE_READWRITE);
	if (Result) {
		os_base* Base = (os_base*)Win32_Get();
		Atomic_Add_U64(&Base->ReservedAmount, ReserveSize);
		Atomic_Increment_U64(&Base->ReservedCount);
	}
	return Result;
}

function OS_COMMIT_MEMORY_DEFINE(Win32_Commit_Memory) {
	void* Result = VirtualAlloc(BaseAddress, CommitSize, MEM_COMMIT, PAGE_READWRITE);
	if (Result) {
		os_base* Base = (os_base*)Win32_Get();
		Atomic_Add_U64(&Base->CommittedAmount, CommitSize);
		Atomic_Increment_U64(&Base->CommittedCount);
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

			CurrentAddress = Offset_Pointer(MemoryInfo.BaseAddress, MemoryInfo.RegionSize);
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
		//todo: Diagnostic and error logging 
		Debug_Log("CreateFileW failed!");
	}

	Scratch_Release();

	return Result;
}

function OS_GET_FILE_SIZE_DEFINE(Win32_Get_File_Size) {
	Assert(File);
	if (!File) return 0;

	LARGE_INTEGER Result;
	GetFileSizeEx(File->Handle, &Result);
	return Result.QuadPart;
}

function OS_READ_FILE_DEFINE(Win32_Read_File) {
	Assert(File);
	if (!File) return false;

	u32 ReadSizeTrunc = (u32)ReadSize;
	DWORD BytesRead;

	if (!ReadFile(File->Handle, Data, ReadSizeTrunc, &BytesRead, NULL)) {
		Debug_Log("ReadFile failed!");
		return false;
	}

	if ((u64)BytesRead != ReadSize) {
		//todo: Diagnostic and error logging
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
		Debug_Log("WriteFile failed!");
		return false;
	}

	if ((u64)BytesWritten != WriteSize) {
		//todo: Diagnostic and error logging
		return false;
	}

	return true;
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
	Scratch_Release();
	return Result;
}

function OS_COPY_FILE_DEFINE(Win32_Copy_File) {
	arena* Scratch = Scratch_Get();
	wstring SrcFileW = WString_From_String((allocator*)Scratch, SrcFilePath);
	wstring DstFileW = WString_From_String((allocator*)Scratch, DstFilePath);
	BOOL Result = CopyFileW(SrcFileW.Ptr, DstFileW.Ptr, FALSE);
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
	TlsSetValue(TLS->Index, Data);
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
		WaitForSingleObject(Thread->Handle, INFINITE);
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
	if (Handle == NULL) return NULL;

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
		ReleaseSemaphore(Semaphore->Handle, 1, NULL);
	}
}

function OS_SEMAPHORE_DECREMENT_DEFINE(Win32_Semaphore_Decrement) {
	if (Semaphore && Semaphore->Handle) {
		WaitForSingleObject(Semaphore->Handle, INFINITE);
	}
}

function OS_SEMAPHORE_ADD_DEFINE(Win32_Semaphore_Add) {
	if (Semaphore && Semaphore->Handle) {
		ReleaseSemaphore(Semaphore->Handle, Count, NULL);
	}
}

function OS_EVENT_CREATE_DEFINE(Win32_Event_Create) {
	HANDLE Handle = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (Handle == NULL) return NULL;

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
	WaitForSingleObject(Event->Handle, INFINITE);
}

function OS_EVENT_SIGNAL_DEFINE(Win32_Event_Signal) {
	SetEvent(Event->Handle);
}

function OS_EVENT_RESET_DEFINE(Win32_Event_Reset) {
	ResetEvent(Event->Handle);
}

function OS_HOT_RELOAD_CREATE_DEFINE(Win32_Hot_Reload_Create) {
	wstring FilePathW = WString_From_String(Default_Allocator_Get(), FilePath);
	
	WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
	if (!GetFileAttributesExW(FilePathW.Ptr, GetFileExInfoStandard, &FileAttributes)) {
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
	Scratch_Release();

	if (!Library) {
		return NULL;
	}

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
	return (void*)GetProcAddress(Library->Library, FunctionName);
}

function OS_GET_ENTROPY_DEFINE(Win32_Get_Entropy) {
	BCryptGenRandom(NULL, (PUCHAR)Buffer,(ULONG)Size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
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
	.CloseFileFunc = Win32_Close_File,

	.GetAllFilesFunc = Win32_Get_All_Files,
	.IsDirectoryPathFunc = Win32_Is_Directory_Path,
	.IsFilePathFunc = Win32_Is_File_Path,
	.MakeDirectoryFunc = Win32_Make_Directory,
	.CopyFileFunc = Win32_Copy_File,

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

void OS_Base_Init(base* Base) {
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

void OS_Base_Shutdown(base* Base) {
	win32_base* Win32 = (win32_base*)Base->OSBase;

	DeleteCriticalSection(&Win32->ResourceLock);
}

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

#ifdef __cplusplus
}
#endif 