#ifndef JOB_H
#define JOB_H

#define JOB_FAST_USERDATA_SIZE 76

typedef struct job job;
typedef struct {
	job* Job;
	u64  Generation;
} job_id;

typedef struct ak_job_system ak_job_system;

enum {
    JOB_FLAG_NONE,
    JOB_FLAG_QUEUE_IMMEDIATELY_BIT = (1 << 0),
    JOB_FLAG_FREE_WHEN_DONE_BIT = (1 << 1)
};
typedef u32 job_flags;

enum {
    JOB_INFO_FLAG_NONE,
    JOB_INFO_FLAG_HEAP_ALLOCATED_BIT = (1 << 0),
    JOB_INFO_FLAG_FREE_WHEN_DONE_BIT = (1 << 1)
};
typedef u32 job_info_flags;

typedef struct job_system job_system;
#define JOB_CALLBACK_DEFINE(name) void name(job_system* JobSystem, job_id CallbackJobID, void* UserData)
typedef JOB_CALLBACK_DEFINE(job_callback_func);

#define JOB_INVALID_INDEX ((u32)-1)
struct job {
	atomic_u64 		   Generation;	
    job*      		   ParentJob;
    atomic_ptr 	   	   DependencyList;
	job_callback_func* JobCallback;
    u32        		   Index;
    atomic_u32 	   	   PendingDependencies;
    atomic_u32 	   	   PendingJobs;
    atomic_b32 	   	   IsProcessing;
    job_info_flags     JobInfoFlags;
    uint8_t       	   JobUserData[JOB_FAST_USERDATA_SIZE];
};

typedef struct {
    job_callback_func* JobCallback;
    void*              Data;
    size_t             DataByteSize;
} job_data;

Static_Assert(sizeof(job) == 128);


typedef struct job_dependency job_dependency;
struct job_dependency {
    u32             Index;
    job*            Job;
    job_dependency* Next;
};

typedef struct {
    async_stack_index32 FreeJobDependencies;
    job_dependency*     JobDependencies;
    u32                 MaxDependencyCount;
} job_dependencies;

typedef struct {
    b32             ProcessJob;
    job*            Job;
    job_dependency* Dependency;
} job_dependency_iter;

typedef struct {
    async_stack_index32 FreeJobIndices;
    job*                Jobs; /*Array of max job count*/
} job_storage;

typedef struct {
    job_system* JobSystem;
    os_thread*  Thread;
} job_system_thread;

typedef struct job_system_queue job_system_queue;
struct job_system_queue {
    atomic_s32        BottomIndex;
    atomic_s32        TopIndex;
    job**             Queue;
    u64               ThreadID;
    job_system_queue* Next;
};

struct job_system {
    /*System information*/
    os_tls* TLS;

    /*Job information*/
    job_storage      JobStorage;
    job_dependencies Dependencies;
    os_semaphore*    JobSemaphore;

    /*Threads and queues*/
    atomic_ptr         NonThreadRunnerQueueTopPtr; /*Link list of ak__job_system_queue queues that are not thread runners*/    
    atomic_ptr         ThreadRunnerQueueTopPtr;  /*Link list of ak__job_system_queue queues that are thread runners*/
    u32                ThreadCount;
    job_system_thread* Threads;
    atomic_b32         IsRunning;
};

function inline job_id Job_ID_Empty() {
	job_id Result = { 0 };
	return Result;
}

function inline b32 Job_Is_Null(job_id JobID) {
	return JobID.Job == NULL || JobID.Generation == 0;
}

export_function job_system* Job_System_Create(u32 MaxJobCount, u32 ThreadCount, u32 MaxDependencyCount);
export_function void        Job_System_Delete(job_system* JobSystem);
export_function job_id      Job_System_Alloc_Job(job_system* JobSystem, job_data JobData, job_id ParentID, job_flags Flags);
export_function job_id      Job_System_Alloc_Empty_Job(job_system* JobSystem, job_flags Flags);
export_function void        Job_System_Free_Job(job_system* JobSystem, job_id JobID);
export_function void        Job_System_Add_Job(job_system* JobSystem, job_id JobID);
export_function void        Job_System_Wait_For_Job(job_system* JobSystem, job_id JobID);
export_function void        Job_System_Add_Dependency(job_system* JobSystem, job_id Job, job_id DependencyJob);

#endif