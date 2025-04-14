function void* Job_Malloc(size_t Size) {
    size_t Offset = DEFAULT_ALIGNMENT - 1 + sizeof(void*);
    void* P1 = Allocator_Allocate_Memory(Default_Allocator_Get(), Size+Offset);
    if(!P1) return NULL;

    void** P2 = (void**)(((size_t)(P1) + Offset) & ~(DEFAULT_ALIGNMENT - 1));
    P2[-1] = P1;
         
    return P2;
}

function inline void Job_Free(void* Memory) {
    if(Memory) {
        void** P2 = (void**)Memory;
		Allocator_Free_Memory(Default_Allocator_Get(), P2[-1]);
    }
}

#define Job_Validate_Ptr(Ptr) ((((size_t)Ptr) & (DEFAULT_ALIGNMENT-1)) == 0)
#define Job_Byte_Offset(ptr, offset, type) (type*)(((uint8_t*)(ptr))+offset)

function inline job_id Job_Make_ID(job* Job) {
	job_id Result = {
		.Job = Job,
		.Generation = Atomic_Load_U64(&Job->Generation)
	};
    return Result;
}

function inline job_id Job_ID_Empty() {
	job_id Result = { 0 };
	return Result;
}

function inline b32 Job_Is_Null(job_id JobID) {
	return JobID.Job == NULL || JobID.Generation == 0;
}

function void Job_Allocate_Data(job* Job, void* Bytes, size_t ByteSize) {
    void* Data;
    if(ByteSize <= sizeof(Job->JobUserData)) {
        Data = Job->JobUserData;
        Job->JobInfoFlags &= ~JOB_INFO_FLAG_HEAP_ALLOCATED_BIT;
    } else {
        void* SlowData = Job_Malloc(ByteSize);
        Assert(SlowData);

        /*Copy pointer address to user data*/
        Memory_Copy(Job->JobUserData, &SlowData, sizeof(void*));
        Data = SlowData;
        Job->JobInfoFlags |= JOB_INFO_FLAG_HEAP_ALLOCATED_BIT;
    }
    Memory_Copy(Data, Bytes, ByteSize);
}

#define Job_Get_User_Data_Heap(user_data) (void*)(*(size_t*)(user_data))

function void Job_Free_Data(job* Job) {
    if(Job->JobInfoFlags & JOB_INFO_FLAG_HEAP_ALLOCATED_BIT) {
        void* Ptr = Job_Get_User_Data_Heap(Job->JobUserData);
        Job_Free(Ptr);
        Job->JobInfoFlags &= ~JOB_INFO_FLAG_HEAP_ALLOCATED_BIT;
    }
}

function void Job_Storage_Init(job_storage* JobStorage, u32* FreeJobIndices, job* Jobs, u32 JobCount) {
    Async_Stack_Index32_Init_Raw(&JobStorage->FreeJobIndices, FreeJobIndices, JobCount);
    Assert(Job_Validate_Ptr(Jobs));
    JobStorage->Jobs = Jobs;

    u32 i;
    for(i = 0; i < JobCount; i++) {
        /*Add all the entries to the freelist since every job is free to begin with. 
		This can be done synchronously without worrying about the stack key and aba problem */
        Async_Stack_Index32_Push_Sync(&JobStorage->FreeJobIndices, i);

        /*Set index and generation for the job. Generation of 0 is not valid*/
        JobStorage->Jobs[i].Index = i;
		*(u64*)(&JobStorage->Jobs[i].Generation) = 1;
        Assert((((size_t)&JobStorage->Jobs[i].PendingJobs) % 4) == 0);
        Assert((((size_t)&JobStorage->Jobs[i].Generation) % 4) == 0);
    }
}

function job* Job_Storage_Get(job_id JobID) {
	if (!JobID.Job) return NULL;

	job* Job = JobID.Job;
    if(Atomic_Load_U64(&Job->Generation) == JobID.Generation) {
        return Job;
    }
    return NULL;
}

function job* Job_Storage_Alloc(job_storage* JobStorage) {
    /*Need to find a free job index atomically*/
    u32 FreeIndex = Async_Stack_Index32_Pop(&JobStorage->FreeJobIndices);
    if(FreeIndex == ASYNC_STACK_INDEX32_INVALID) {
        /*No more jobs avaiable*/
        Assert(false);
        return NULL;
    }

    job* Job = JobStorage->Jobs+FreeIndex;
    Assert(Job->Index == FreeIndex); /*Invalid indices!*/
    return Job;  
}

function uint32_t Job_Storage_Capacity(job_storage* JobStorage) {
    return JobStorage->FreeJobIndices.Capacity;
}

function void Job_Storage_Free(job_storage* JobStorage, job_id JobID) {
    job* Job = JobID.Job;
    Assert(!Atomic_Load_B32(&Job->IsProcessing));
    Job_Free_Data(Job);
    u64 Generation = JobID.Generation;
    u64 NextGenerationIndex = Generation+1;
    if(NextGenerationIndex == 0) NextGenerationIndex = 1;
    Atomic_Store_U64(&Job->Generation, NextGenerationIndex);
    Async_Stack_Index32_Push(&JobStorage->FreeJobIndices, Job->Index);
}

function void Job_Init_Dependencies(job_dependencies* Dependencies, u32* FreeIndices, job_dependency* DependenciesPtr, u32 DependencyCount) {
    
    Async_Stack_Index32_Init_Raw(&Dependencies->FreeJobDependencies, FreeIndices, DependencyCount);
    Assert(Job_Validate_Ptr(DependenciesPtr));
    Dependencies->JobDependencies = DependenciesPtr;
    Dependencies->MaxDependencyCount = DependencyCount;
    
    u32 i;
    for(i = 0; i < DependencyCount; i++) {
        /*Add all the entries to the freelist since every job is free to begin with. 
        This can be done synchronously without worrying about the stack key and aba problem */
        Async_Stack_Index32_Push_Sync(&Dependencies->FreeJobDependencies, i);
        Dependencies->JobDependencies[i].Index = i;
    }
}

function void Job_Add_Dependency(job_dependencies* Dependencies, job* Job, job* DependencyJob) {
    u32 FreeDependencyIndex = Async_Stack_Index32_Pop(&Dependencies->FreeJobDependencies);
    if(FreeDependencyIndex == ASYNC_STACK_INDEX32_INVALID) {
        Assert(false);
        return;
    }

    job_dependency* Dependency = Dependencies->JobDependencies + FreeDependencyIndex;
    Assert(Dependency->Index == FreeDependencyIndex);
    
    Dependency->Job = DependencyJob;
    Atomic_Increment_U32(&DependencyJob->PendingDependencies);

    for(;;) {
        job_dependency* OldTop = (job_dependency*)Atomic_Load_Ptr(&Job->DependencyList);
        Dependency->Next = OldTop;
        if(Atomic_Compare_Exchange_Ptr(&Job->DependencyList, OldTop, Dependency) == OldTop) {
            return;
        }
    }
}

function job_dependency_iter Job_Dependency_Begin_Iter(job* Job) {
    job_dependency_iter Iter = {0};
    Iter.Dependency = (job_dependency*)Atomic_Load_Ptr(&Job->DependencyList);
    return Iter;
}

function bool Job_Next_Dependency(job_dependency_iter* Iter, job_dependencies* Dependencies) {
    if(!Iter->Dependency) {
        return false;
    }

    job_dependency* DependencyToDelete = Iter->Dependency;
    Iter->Dependency = Iter->Dependency->Next;
    job* JobDependency = DependencyToDelete->Job;
    if(Atomic_Decrement_U32(&JobDependency->PendingDependencies) == 0) {
        Iter->ProcessJob = true;
        Iter->Job = JobDependency;
    }
    DependencyToDelete->Next = NULL;
    Async_Stack_Index32_Push(&Dependencies->FreeJobDependencies, DependencyToDelete->Index);
    return true;
}

function job_system_queue* Job_System_Create_Queue(job_system* JobSystem, atomic_ptr* TopQueuePtr) {
    size_t AllocationSize = sizeof(job_system_queue)+(sizeof(job*)*Job_Storage_Capacity(&JobSystem->JobStorage));
    job_system_queue* JobQueue = (job_system_queue*)Job_Malloc(AllocationSize);
    Assert(JobQueue);
    if(!JobQueue) return NULL;
	Memory_Clear(JobQueue, AllocationSize);
    JobQueue->Queue = (job**)(JobQueue+1);

    /*Append to link list atomically*/
    for(;;) {
        job_system_queue* OldTop = (job_system_queue*)Atomic_Load_Ptr(TopQueuePtr);
        JobQueue->Next = OldTop;
        if(Atomic_Compare_Exchange_Ptr(TopQueuePtr, OldTop, JobQueue) == OldTop) {
            return JobQueue;
        }
    }
}

function job_system_queue* Job_System_Get_Local_Queue(job_system* JobSystem) {
    job_system_queue* JobQueue = (job_system_queue*)OS_TLS_Get(JobSystem->TLS);
    if(!JobQueue) {
        u64 ThreadID = OS_Get_Current_Thread_ID();

        /*If we do not find a queue in our TLS we linear search for it by thread id*/

        job_system_queue* Queue;
        for(Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->NonThreadRunnerQueueTopPtr);
            Queue; Queue = Queue->Next) {
            if(Queue->ThreadID == ThreadID) {
                JobQueue = Queue;
                break;
            }
        }

        for(Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->ThreadRunnerQueueTopPtr);
            Queue; Queue = Queue->Next) {
            if(Queue->ThreadID == ThreadID) {
                JobQueue = Queue;
                break;
            }
        }

        if(JobQueue) OS_TLS_Set(JobSystem->TLS, JobQueue);
    }
    return JobQueue;
}

function job_system_queue* Job_System_Get_Or_Create_Local_Queue(job_system* JobSystem) {
    job_system_queue* JobQueue = Job_System_Get_Local_Queue(JobSystem);
    if(!JobQueue) {
        JobQueue = Job_System_Create_Queue(JobSystem, &JobSystem->NonThreadRunnerQueueTopPtr);
        JobQueue->ThreadID = OS_Get_Current_Thread_ID();
        OS_TLS_Set(JobSystem->TLS, JobQueue);
    }
    return JobQueue;
}

function job* Job_System_Steal_Job(job_system* JobSystem, job_system_queue* JobQueue) {
    s32 Top = Atomic_Load_S32(&JobQueue->TopIndex);
    s32 Bottom = Atomic_Load_S32(&JobQueue->BottomIndex);

    job* Result = NULL;
    if(Top < Bottom) {
        Result = JobQueue->Queue[Top % Job_Storage_Capacity(&JobSystem->JobStorage)];
        if(Atomic_Compare_Exchange_S32(&JobQueue->TopIndex, Top, Top+1) != Top) {
            Result = NULL;
        }
    } 
    return Result;
}

function job* Job_System_Pop_Job(job_system* JobSystem, job_system_queue* JobQueue) {
    s32 Bottom = Atomic_Load_S32(&JobQueue->BottomIndex)-1;

    Atomic_Store_S32(&JobQueue->BottomIndex, Bottom);
    /*We need to make sure Top is read before bottom thus this is a StoreLoad situation
		and needs an explicit memory barrier to handle this case */
    
    s32 Top = Atomic_Load_S32(&JobQueue->TopIndex);

    job* Result = NULL;
    if(Top <= Bottom) {
        Result = JobQueue->Queue[Bottom % Job_Storage_Capacity(&JobSystem->JobStorage)];
        if(Top == Bottom) {
            if(Atomic_Compare_Exchange_S32(&JobQueue->TopIndex, Top, Top+1) != Top) {
                Result = NULL;
            }
            Atomic_Store_S32(&JobQueue->BottomIndex, Bottom+1);
        }
    } else {
        Atomic_Store_S32(&JobQueue->BottomIndex, Bottom+1);
    }
    return Result;
}

function void Job_System_Push_Job(job_system* JobSystem, job_system_queue* JobQueue, job* Job) {
    s32 Bottom = Atomic_Load_S32(&JobQueue->BottomIndex);
    s32 Top = Atomic_Load_S32(&JobQueue->TopIndex);
    if(((s32)Job_Storage_Capacity(&JobSystem->JobStorage)-1) < (Bottom-Top)) {
        Assert(false);
    }
    JobQueue->Queue[Bottom % Job_Storage_Capacity(&JobSystem->JobStorage)] = Job;
    Atomic_Store_S32(&JobQueue->BottomIndex, Bottom+1);
}

function size_t Job_System_Get_Queue_Size(job_system_queue* JobQueue) {
    s32 Bottom = Atomic_Load_S32(&JobQueue->BottomIndex); 
    s32 Top = Atomic_Load_S32(&JobQueue->TopIndex); 
    return (size_t)(Bottom >= Top ? Bottom-Top : 0);
}

function job_system_queue* Job_System_Get_Largest_Queue(job_system* JobSystem, job_system_queue* CurrentQueue) {
    size_t BestSize = 0;
    job_system_queue* BestQueue = NULL;

    job_system_queue* Queue;
    for(Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->NonThreadRunnerQueueTopPtr);
        Queue; Queue = Queue->Next) {
        if(Queue != CurrentQueue) {
            size_t QueueSize = Job_System_Get_Queue_Size(Queue); 
            if(QueueSize > BestSize) {
                BestSize = QueueSize;
                BestQueue = Queue;
            }
        }
    }

    if(BestQueue) return BestQueue;

    for(Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->ThreadRunnerQueueTopPtr);
        Queue; Queue = Queue->Next) {
        if(Queue != CurrentQueue) {
            size_t QueueSize = Job_System_Get_Queue_Size(Queue); 
            if(QueueSize > BestSize) {
                BestSize = QueueSize;
                BestQueue = Queue;
            }
        }
    }

    return BestQueue;
}


function void Job_System_Requeue_Job(job_system* JobSystem, job_system_queue* JobQueue, job* Job) {    
    Job_System_Push_Job(JobSystem, JobQueue, Job);
    OS_Semaphore_Increment(JobSystem->JobSemaphore);
}

function void Job_System_Queue_Add_Job(job_system* JobSystem, job_system_queue* JobQueue, job* Job) {
    Atomic_Increment_U32(&Job->PendingJobs);
    if(Job->ParentJob) {
        /*Only increment the parent once, and it should not be processing yet*/
        Atomic_Increment_U32(&Job->ParentJob->PendingJobs);
    }

    if(Atomic_Compare_Exchange_B32(&Job->IsProcessing, false, true) == false) {
        Job_System_Requeue_Job(JobSystem, JobQueue, Job);
    } else {
        Assert(false); /*Cannot submit an already submitted job!*/
    }
}

function void Job_System_Finish_Job(job_system* JobSystem, job_system_queue* JobQueue, job* Job) {
    /*Pending jobs will increment everytime we submit a job. Parents will also get incremented
	when its child is submitted, however parents must be submitted later. To prevent the
	parent*/
    b32 IsProcessing = Atomic_Load_B32(&Job->IsProcessing);
    if(Atomic_Decrement_U32(&Job->PendingJobs) == 0 && IsProcessing) {
        job_dependency_iter DependencyIter = Job_Dependency_Begin_Iter(Job);
        while(Job_Next_Dependency(&DependencyIter, &JobSystem->Dependencies)) {
            if(DependencyIter.ProcessJob) {
                Job_System_Queue_Add_Job(JobSystem, JobQueue, DependencyIter.Job);
            }
        }

        if(Job->ParentJob) {
            Job_System_Finish_Job(JobSystem, JobQueue, Job->ParentJob);
        }

        Atomic_Store_Ptr(&Job->DependencyList, NULL);
        job_id JobID = Job_Make_ID(Job);

        Atomic_Store_B32(&Job->IsProcessing, false);
        if(Job->JobInfoFlags & JOB_INFO_FLAG_FREE_WHEN_DONE_BIT)
            Job_Storage_Free(&JobSystem->JobStorage, JobID);
    }
}

function void Job_System_Process_Job(job_system* JobSystem, job_system_queue* JobQueue, job* Job) {    
    job_id JobID = Job_Make_ID(Job);

    if(Job->JobCallback) {
        void* UserData = 
            ((Job->JobInfoFlags & JOB_INFO_FLAG_HEAP_ALLOCATED_BIT) ? 
                Job_Get_User_Data_Heap(Job->JobUserData) : 
                (void*)Job->JobUserData);
        job_callback_func* JobCallback = Job->JobCallback;
        JobCallback(JobSystem, JobID, UserData);
    }
    
    Job_System_Finish_Job(JobSystem, JobQueue, Job);
}


function job* Job_System_Get_Next_Job(job_system* JobSystem, job_system_queue* JobQueue) {
    job* Job = Job_System_Pop_Job(JobSystem, JobQueue);
    if(!Job) {
        job_system_queue* StolenJobQueue = Job_System_Get_Largest_Queue(JobSystem, JobQueue);
        if(StolenJobQueue) {
            Job = Job_System_Steal_Job(JobSystem, StolenJobQueue);
        }
    }
    return Job;   
}

function bool Job_System_Process_Next_Job(job_system* JobSystem, job_system_queue* JobQueue) {
    job* Job = Job_System_Get_Next_Job(JobSystem, JobQueue);
    if(Job) {
        Job_System_Process_Job(JobSystem, JobQueue, Job);
        return true;
    }
    return false;
}


function void Job_System_Run(job_system* JobSystem, job_system_thread* JobThread, job_system_queue* JobQueue) {
    while(Atomic_Load_B32(&JobSystem->IsRunning)) {
        if(!Job_System_Process_Next_Job(JobSystem, JobQueue)) {
            OS_Semaphore_Decrement(JobSystem->JobSemaphore);
        }
    }

    /*Cleanup remaining jobs*/
    while(Job_System_Process_Next_Job(JobSystem, JobQueue)) {}
}

function void Job_System_Main(job_system_thread* JobThread) {
    job_system* JobSystem = JobThread->JobSystem;
    job_system_queue* JobQueue = Job_System_Create_Queue(JobSystem, &JobSystem->ThreadRunnerQueueTopPtr);
    JobQueue->ThreadID = OS_Get_Current_Thread_ID();
    OS_TLS_Set(JobSystem->TLS, JobQueue);
    Job_System_Run(JobSystem, JobThread, JobQueue);
}

function OS_THREAD_CALLBACK_DEFINE(Job_System_Thread_Callback) {
    job_system_thread* JobThread = (job_system_thread*)UserData;
    Job_System_Main(JobThread);
}

function job_system* Job_System_Create(u32 MaxJobCount, u32 ThreadCount, u32 MaxDependencyCount) {

    /*Get the allocation size. Want to avoid many small allocations so batch them together*/
    /*Data entries should also be 16 byte aligned, so if any entry is not aligned
	we will add some extra padding*/
    
    size_t JobSystemSize = Align_Pow2(sizeof(job_system), DEFAULT_ALIGNMENT);
    size_t JobFreeIndicesSize = Align_Pow2(MaxJobCount*sizeof(u32), DEFAULT_ALIGNMENT);
    size_t JobSize = Align_Pow2(MaxJobCount*sizeof(job), DEFAULT_ALIGNMENT);
    size_t ThreadSize = Align_Pow2(ThreadCount*sizeof(job_system_thread), DEFAULT_ALIGNMENT);
    size_t DependencyFreeIndicesSize = Align_Pow2(MaxDependencyCount*sizeof(u32), DEFAULT_ALIGNMENT);
    size_t DependencySize = Align_Pow2(MaxDependencyCount*sizeof(job_dependencies), DEFAULT_ALIGNMENT);

    size_t AllocationSize = JobSystemSize; /*Space for the job system*/ 
    AllocationSize += JobFreeIndicesSize; /*Space for the job free indices*/
    AllocationSize += JobSize; /*Space for the jobs*/
    AllocationSize += ThreadSize; /*Space for the threads*/
    AllocationSize += DependencyFreeIndicesSize; /*Space for the job free dependency indices*/
    AllocationSize += DependencySize; /*Space for the dependencies*/

    /*Allocate and clear all the memory*/
    job_system* JobSystem = (job_system*)Job_Malloc(AllocationSize);
    Assert(JobSystem);

    if(!JobSystem) return NULL;
    Assert(Job_Validate_Ptr(JobSystem));

	Memory_Clear(JobSystem, AllocationSize);

    /*System information*/
    JobSystem->TLS = OS_TLS_Create();
    JobSystem->JobSemaphore = OS_Semaphore_Create(0);

    /*Job information*/
    u32* FreeJobIndices = Job_Byte_Offset(JobSystem, JobSystemSize, u32);
    job* Jobs = Job_Byte_Offset(FreeJobIndices, JobFreeIndicesSize, job);
    Job_Storage_Init(&JobSystem->JobStorage, FreeJobIndices, Jobs, MaxJobCount);

    JobSystem->ThreadCount = ThreadCount;
    JobSystem->Threads = Job_Byte_Offset(Jobs, JobSize, job_system_thread);
    Assert(Job_Validate_Ptr(JobSystem->Threads));

    if(MaxDependencyCount) {
        u32* FreeDependencyIndices = Job_Byte_Offset(JobSystem->Threads, ThreadSize, u32);
        job_dependency* JobDependencies = Job_Byte_Offset(FreeDependencyIndices, DependencyFreeIndicesSize, job_dependency); 
        Job_Init_Dependencies(&JobSystem->Dependencies, FreeDependencyIndices, JobDependencies, MaxDependencyCount);
    }

    Atomic_Store_B32(&JobSystem->IsRunning, true);

    u32 i;
    for(i = 0; i < JobSystem->ThreadCount; i++) {
        job_system_thread* Thread = JobSystem->Threads + i;
        Thread->JobSystem = JobSystem;
		Thread->Thread = OS_Thread_Create(Job_System_Thread_Callback, Thread);
		if(!Thread->Thread) {
            /*todo: cleanup resources*/
            return NULL;
        }
    }

    return JobSystem;
}

function void Job_System_Delete(job_system* JobSystem) {
    /*Turn off the system*/    
    Atomic_Store_B32(&JobSystem->IsRunning, false);

    /*Set all the threads status to IsRunning false before we increment the semaphore*/
    OS_Semaphore_Add(JobSystem->JobSemaphore, (s32)JobSystem->ThreadCount);

    /*Wait for the threads to finish and delete them*/
    u32 ThreadIndex;
    for(ThreadIndex = 0; ThreadIndex < JobSystem->ThreadCount; ThreadIndex++) {
        job_system_thread* Thread = JobSystem->Threads + ThreadIndex;
        OS_Thread_Join(Thread->Thread);
    }

    /*Finally delete all the queue memory*/
    job_system_queue* Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->NonThreadRunnerQueueTopPtr);
    while(Queue) {
        job_system_queue* QueueToDelete = Queue;
        Queue = Queue->Next;
        Job_Free(QueueToDelete);
    }
    Queue = (job_system_queue*)Atomic_Load_Ptr(&JobSystem->ThreadRunnerQueueTopPtr);
    while(Queue) {
        job_system_queue* QueueToDelete = Queue;
        Queue = Queue->Next;
        Job_Free(QueueToDelete);
    }
    OS_Semaphore_Delete(JobSystem->JobSemaphore);
    OS_TLS_Delete(JobSystem->TLS);
    Job_Free(JobSystem);
}

function job_id Job_System_Alloc_Job(job_system* JobSystem, job_data JobData, job_id ParentID, job_flags Flags) {
    job* Job = Job_Storage_Alloc(&JobSystem->JobStorage);

    Job->JobCallback = JobData.JobCallback;
    Job_Allocate_Data(Job, JobData.Data, JobData.DataByteSize);
    Job->ParentJob = Job_Storage_Get(ParentID);

    if(Flags & JOB_FLAG_FREE_WHEN_DONE_BIT) {
        Job->JobInfoFlags |= JOB_INFO_FLAG_FREE_WHEN_DONE_BIT;
    } else {
        Job->JobInfoFlags &= ~JOB_INFO_FLAG_FREE_WHEN_DONE_BIT;
    }


    Assert(Atomic_Load_U32(&Job->PendingDependencies) == 0);
    Assert(Atomic_Load_Ptr(&Job->DependencyList) == NULL);
    /*Pending jobs should be 0 for the job*/    
    Assert(Atomic_Load_U32(&Job->PendingJobs) == 0);
    Assert(!Atomic_Load_B32(&Job->IsProcessing));

    job_id JobID = Job_Make_ID(Job);
    if(Flags & JOB_FLAG_QUEUE_IMMEDIATELY_BIT) {
        Job_System_Add_Job(JobSystem, JobID);
    }
    return JobID;
}

function void Job_System_Free_Job(job_system* JobSystem, job_id JobID) {
    Job_Storage_Free(&JobSystem->JobStorage, JobID);
}

function job_id Job_System_Alloc_Empty_Job(job_system* JobSystem, job_flags Flags) {
    job_data JobData = {0};
    return Job_System_Alloc_Job(JobSystem, JobData, Job_ID_Empty(), Flags);
}

function void Job_System_Add_Job(job_system* JobSystem, job_id JobID) {
    job_system_queue* JobQueue = Job_System_Get_Or_Create_Local_Queue(JobSystem);
    job* Job = Job_Storage_Get(JobID);
    if(Job) {
        Job_System_Queue_Add_Job(JobSystem, JobQueue, Job);
    }
}

function void Job_System_Wait_For_Job(job_system* JobSystem, job_id JobID) {
    job* Job = Job_Storage_Get(JobID);
    while(Job && Atomic_Load_B32(&Job->IsProcessing)) {
        job_system_queue* JobQueue = Job_System_Get_Or_Create_Local_Queue(JobSystem);
        Job_System_Process_Next_Job(JobSystem, JobQueue);
    }
}

function void Job_System_Add_Dependency(job_system* JobSystem, job_id JobID, job_id DependencyJobID) {
    job* Job = Job_Storage_Get(JobID);
    job* DependencyJob = Job_Storage_Get(DependencyJobID);
    if(!Job || !DependencyJob) {
        Assert(false);
        return;
    }

    Job_Add_Dependency(&JobSystem->Dependencies, Job, DependencyJob);
}