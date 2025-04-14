typedef struct {
    u8 Size;
    u8 Data[JOB_FAST_USERDATA_SIZE-1];
} job_system_job_user_data_small_data;

typedef struct {
    u32 Size;
    u32 Data[JOB_FAST_USERDATA_SIZE*10];
} job_system_job_user_data_large_data;

JOB_CALLBACK_DEFINE(Job_System_Small_User_Data_Test_Callback) {
    job_system_job_user_data_small_data* JobData = (job_system_job_user_data_small_data*)UserData;
    Assert(JobData->Size == JOB_FAST_USERDATA_SIZE);
    u8 i;
    for(i = 0; i < JOB_FAST_USERDATA_SIZE-1; i++) {
        Assert(JobData->Data[i] == i);
    }
}

JOB_CALLBACK_DEFINE(Job_System_Large_User_Data_Test_Callback) {
    job_system_job_user_data_large_data* JobData = (job_system_job_user_data_large_data*)UserData;
    Assert(JobData->Size == JOB_FAST_USERDATA_SIZE*10);
    u32 i;
    for(i = 0; i < JOB_FAST_USERDATA_SIZE*10; i++) {
        Assert(JobData->Data[i] == i);
    }
}

UTEST(Job_System, UserData) {
    const u32 Iterations = 100000;

    job_system* JobSystem = Job_System_Create((Iterations*2)+1, (u32)OS_Processor_Thread_Count(), 0);
    job_id RootJob = Job_System_Alloc_Empty_Job(JobSystem, JOB_FLAG_FREE_WHEN_DONE_BIT);

    u32 i;
    for(i = 0; i < Iterations; i++) {
        /*Make sure the job properly copies the data from the stack*/
        job_system_job_user_data_small_data SmallData = {0};
        SmallData.Size = JOB_FAST_USERDATA_SIZE;
        
        u8 k;
        for(k = 0; k < JOB_FAST_USERDATA_SIZE-1; k++) {
            SmallData.Data[k] = k;
        }

        job_system_job_user_data_large_data LargeData = {0};
        LargeData.Size = JOB_FAST_USERDATA_SIZE*10;

        u32 j;
        for(j = 0; j < JOB_FAST_USERDATA_SIZE*10; j++) {
            LargeData.Data[j] = j;
        }

        job_data SmallJobData;
        SmallJobData.JobCallback = Job_System_Small_User_Data_Test_Callback;
        SmallJobData.Data = &SmallData;
        SmallJobData.DataByteSize = sizeof(SmallData);
        job_id JobID = Job_System_Alloc_Job(JobSystem, SmallJobData, 
											RootJob, JOB_FLAG_QUEUE_IMMEDIATELY_BIT|JOB_FLAG_FREE_WHEN_DONE_BIT);
        
        job_data LargeJobData;
        LargeJobData.JobCallback = Job_System_Large_User_Data_Test_Callback;
        LargeJobData.Data = &LargeData;
        LargeJobData.DataByteSize = sizeof(LargeData);
        JobID = Job_System_Alloc_Job(JobSystem, LargeJobData, 
									 RootJob, JOB_FLAG_QUEUE_IMMEDIATELY_BIT|JOB_FLAG_FREE_WHEN_DONE_BIT);
        ASSERT_TRUE(!Job_Is_Null(JobID));
    }

    Job_System_Add_Job(JobSystem, RootJob);
    Job_System_Wait_For_Job(JobSystem, RootJob);
    Job_System_Delete(JobSystem);
}

typedef struct {
    u32  Count;
    u32* CounterTable;
} job_system_thread_data;

typedef struct {
    job_system_thread_data* ThreadData;
    u32                     Index;
} job_system_job_data;

JOB_CALLBACK_DEFINE(AK_Job_System_Test_Callback) {
    job_system_job_data* JobData = (job_system_job_data*)UserData;
    Assert(JobData->Index < JobData->ThreadData->Count); /*overflow occurred*/
    JobData->ThreadData->CounterTable[JobData->Index]++;
}

UTEST(Job_System, MainTest) {
    const u32 ThreadCount = (u32)OS_Processor_Thread_Count();
    const u32 MaxJobCount = (1000000);

    job_system_thread_data ThreadData = {0};
    ThreadData.Count = MaxJobCount;
    ThreadData.CounterTable = Allocator_Allocate_Array(Default_Allocator_Get(), MaxJobCount, u32);
    Memory_Clear(ThreadData.CounterTable, sizeof(uint32_t)*MaxJobCount);

    job_system* JobSystem = Job_System_Create(MaxJobCount+1, ThreadCount, 0);
    ASSERT_TRUE(JobSystem);

    u64 StartTime = OS_Query_Performance_Counter();

    job_id RootJob = Job_System_Alloc_Empty_Job(JobSystem, JOB_FLAG_FREE_WHEN_DONE_BIT);
    u32 i;
    for(i = 0; i < MaxJobCount; i++) {
        job_system_job_data JobData;
        JobData.ThreadData = &ThreadData;
        JobData.Index = i;

        job_data JobCallbackData;
        JobCallbackData.JobCallback = AK_Job_System_Test_Callback;
        JobCallbackData.Data = &JobData;
        JobCallbackData.DataByteSize = sizeof(job_system_job_data);

        job_id JobID = Job_System_Alloc_Job(JobSystem, JobCallbackData, 
											RootJob, JOB_FLAG_QUEUE_IMMEDIATELY_BIT|JOB_FLAG_FREE_WHEN_DONE_BIT);
        ASSERT_TRUE(!Job_Is_Null(JobID));
    }

    Job_System_Add_Job(JobSystem, RootJob);
    Job_System_Wait_For_Job(JobSystem, RootJob);
    Debug_Log("Time: %fms", ((f64)(OS_Query_Performance_Counter()-StartTime)/(f64)OS_Query_Performance_Frequency())*1000.0);
    Job_System_Delete(JobSystem);

    for(i = 0; i < MaxJobCount; i++) {
        /*Each index should've only been operated on once*/
        ASSERT_EQ(ThreadData.CounterTable[i], 1);
    }

    Allocator_Free_Memory(Default_Allocator_Get(), ThreadData.CounterTable);
}

typedef struct {
    b32 A;
    b32 B;
    b32 C;
    b32 D;
    b32 E;
    b32 F;
} job_system_dependency_test_data;

typedef struct job_system_dependency_data_wrapper {
    job_system_dependency_test_data* DependencyData;
} job_system_dependency_data_wrapper;

JOB_CALLBACK_DEFINE(Async_Job_Dependency_A) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(!DependencyData->A);
    Assert(!DependencyData->C);
    Assert(!DependencyData->F);
    DependencyData->A = true;
}

JOB_CALLBACK_DEFINE(Async_Job_Dependency_B) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(!DependencyData->B);
    Assert(!DependencyData->C);
    Assert(!DependencyData->F);
    DependencyData->B = true;
}

JOB_CALLBACK_DEFINE(Async_Job_Dependency_C) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(!DependencyData->C);
    Assert(!DependencyData->F);
    DependencyData->C = true;
}

JOB_CALLBACK_DEFINE(Async_Job_Dependency_D) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(!DependencyData->D);
    Assert(!DependencyData->E);
    Assert(!DependencyData->F);
    DependencyData->D = true;
}

JOB_CALLBACK_DEFINE(Async_Job_Dependency_E) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(!DependencyData->E);
    Assert(!DependencyData->F);
    DependencyData->E = true;
}

JOB_CALLBACK_DEFINE(Async_Job_Dependency_F) {
    job_system_dependency_test_data* DependencyData = ((job_system_dependency_data_wrapper*)UserData)->DependencyData;
    Assert(DependencyData->A);
    Assert(DependencyData->B);
    Assert(DependencyData->D);
    Assert(DependencyData->E);
    Assert(!DependencyData->F);
    DependencyData->F = true;
}

UTEST(Job_System, DependencyTest) {
    const uint32_t Iterations = 100000;

    job_system* JobSystem = Job_System_Create((6*Iterations)+1, (u32)OS_Processor_Thread_Count(), (5*Iterations));

    job_system_dependency_test_data* DependencyDataArray = (job_system_dependency_test_data*)Allocator_Allocate_Memory(Default_Allocator_Get(), Iterations*sizeof(job_system_dependency_test_data));
    Memory_Clear(DependencyDataArray, Iterations*sizeof(job_system_dependency_test_data));

    job_id RootJob = Job_System_Alloc_Empty_Job(JobSystem, 0);

    u32 i;
    for(i = 0; i < Iterations; i++) {
        job_system_dependency_data_wrapper Wrapper;
        Wrapper.DependencyData = &DependencyDataArray[i];

        job_data JobDataA;
        JobDataA.JobCallback = Async_Job_Dependency_A;
        JobDataA.Data = &Wrapper;
        JobDataA.DataByteSize = sizeof(Wrapper);

        job_data JobDataB;
        JobDataB.JobCallback = Async_Job_Dependency_B;
        JobDataB.Data = &Wrapper;
        JobDataB.DataByteSize = sizeof(Wrapper);

        job_data JobDataC;
        JobDataC.JobCallback = Async_Job_Dependency_C;
        JobDataC.Data = &Wrapper;
        JobDataC.DataByteSize = sizeof(Wrapper);

        job_data JobDataD;
        JobDataD.JobCallback = Async_Job_Dependency_D;
        JobDataD.Data = &Wrapper;
        JobDataD.DataByteSize = sizeof(Wrapper);

        job_data JobDataE;
        JobDataE.JobCallback = Async_Job_Dependency_E;
        JobDataE.Data = &Wrapper;
        JobDataE.DataByteSize = sizeof(Wrapper);

        job_data JobDataF;
        JobDataF.JobCallback = Async_Job_Dependency_F;
        JobDataF.Data = &Wrapper;
        JobDataF.DataByteSize = sizeof(Wrapper);

        job_id JobA = Job_System_Alloc_Job(JobSystem, JobDataA, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        job_id JobB = Job_System_Alloc_Job(JobSystem, JobDataB, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        job_id JobC = Job_System_Alloc_Job(JobSystem, JobDataC, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        job_id JobD = Job_System_Alloc_Job(JobSystem, JobDataD, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        job_id JobE = Job_System_Alloc_Job(JobSystem, JobDataE, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        job_id JobF = Job_System_Alloc_Job(JobSystem, JobDataF, RootJob, JOB_FLAG_FREE_WHEN_DONE_BIT);
        
        Job_System_Add_Dependency(JobSystem, JobA, JobC);
        Job_System_Add_Dependency(JobSystem, JobB, JobC);
        Job_System_Add_Dependency(JobSystem, JobD, JobE);
        Job_System_Add_Dependency(JobSystem, JobC, JobF);
        Job_System_Add_Dependency(JobSystem, JobE, JobF);
        Job_System_Add_Job(JobSystem, JobA);
        Job_System_Add_Job(JobSystem, JobB);
        Job_System_Add_Job(JobSystem, JobD);
    }
    Job_System_Add_Job(JobSystem, RootJob);
    Job_System_Wait_For_Job(JobSystem, RootJob);
    Job_System_Free_Job(JobSystem, RootJob);

    for(i = 0; i < Iterations; i++) {
        job_system_dependency_test_data* DependencyData = DependencyDataArray+i;
        ASSERT_TRUE(DependencyData->A);
        ASSERT_TRUE(DependencyData->B);
        ASSERT_TRUE(DependencyData->C);
        ASSERT_TRUE(DependencyData->D);
        ASSERT_TRUE(DependencyData->E);
        ASSERT_TRUE(DependencyData->F);
    }

    Allocator_Free_Memory(Default_Allocator_Get(), DependencyDataArray);
    Job_System_Delete(JobSystem);
}