function void Vk_Instance_Funcs_Load(VkInstance Instance) {
	{
		vkDestroyInstance=(PFN_vkDestroyInstance)vkGetInstanceProcAddr(Instance,"vkDestroyInstance");
	}
	{
		vkEnumeratePhysicalDevices=(PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(Instance,"vkEnumeratePhysicalDevices");
	}
	{
		vkGetPhysicalDeviceProperties=(PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceProperties");
	}
	{
		vkEnumerateDeviceExtensionProperties=(PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(Instance,"vkEnumerateDeviceExtensionProperties");
	}
	{
		vkGetPhysicalDeviceQueueFamilyProperties=(PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceQueueFamilyProperties");
	}
	{
		vkCreateDevice=(PFN_vkCreateDevice)vkGetInstanceProcAddr(Instance,"vkCreateDevice");
	}
	{
		vkGetPhysicalDeviceMemoryProperties=(PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceMemoryProperties");
	}
	{
		vkGetDeviceProcAddr=(PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(Instance,"vkGetDeviceProcAddr");
	}
}
function void Vk_Khr_Get_Physical_Device_Properties2_Funcs_Load(VkInstance Instance) {
	{
		vkGetPhysicalDeviceFeatures2KHR=(PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceFeatures2KHR");
	}
}
function void Vk_Device_Funcs_Load(VkDevice Device) {
	{
		vkGetDeviceQueue=(PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(Device,"vkGetDeviceQueue");
	}
	{
		vkDestroyDevice=(PFN_vkDestroyDevice)vkGetDeviceProcAddr(Device,"vkDestroyDevice");
	}
	{
		vkCreateCommandPool=(PFN_vkCreateCommandPool)vkGetDeviceProcAddr(Device,"vkCreateCommandPool");
	}
	{
		vkAllocateCommandBuffers=(PFN_vkAllocateCommandBuffers)vkGetDeviceProcAddr(Device,"vkAllocateCommandBuffers");
	}
	{
		vkCreateFence=(PFN_vkCreateFence)vkGetDeviceProcAddr(Device,"vkCreateFence");
	}
	{
		vkCreateSemaphore=(PFN_vkCreateSemaphore)vkGetDeviceProcAddr(Device,"vkCreateSemaphore");
	}
	{
		vkResetCommandPool=(PFN_vkResetCommandPool)vkGetDeviceProcAddr(Device,"vkResetCommandPool");
	}
	{
		vkBeginCommandBuffer=(PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(Device,"vkBeginCommandBuffer");
	}
	{
		vkCmdPipelineBarrier=(PFN_vkCmdPipelineBarrier)vkGetDeviceProcAddr(Device,"vkCmdPipelineBarrier");
	}
	{
		vkCmdExecuteCommands=(PFN_vkCmdExecuteCommands)vkGetDeviceProcAddr(Device,"vkCmdExecuteCommands");
	}
	{
		vkEndCommandBuffer=(PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(Device,"vkEndCommandBuffer");
	}
	{
		vkQueueSubmit=(PFN_vkQueueSubmit)vkGetDeviceProcAddr(Device,"vkQueueSubmit");
	}
	{
		vkWaitForFences=(PFN_vkWaitForFences)vkGetDeviceProcAddr(Device,"vkWaitForFences");
	}
	{
		vkGetFenceStatus=(PFN_vkGetFenceStatus)vkGetDeviceProcAddr(Device,"vkGetFenceStatus");
	}
	{
		vkResetFences=(PFN_vkResetFences)vkGetDeviceProcAddr(Device,"vkResetFences");
	}
	{
		vkCreateImageView=(PFN_vkCreateImageView)vkGetDeviceProcAddr(Device,"vkCreateImageView");
	}
	{
		vkAllocateMemory=(PFN_vkAllocateMemory)vkGetDeviceProcAddr(Device,"vkAllocateMemory");
	}
	{
		vkFreeMemory=(PFN_vkFreeMemory)vkGetDeviceProcAddr(Device,"vkFreeMemory");
	}
	{
		vkMapMemory=(PFN_vkMapMemory)vkGetDeviceProcAddr(Device,"vkMapMemory");
	}
	{
		vkUnmapMemory=(PFN_vkUnmapMemory)vkGetDeviceProcAddr(Device,"vkUnmapMemory");
	}
	{
		vkFlushMappedMemoryRanges=(PFN_vkFlushMappedMemoryRanges)vkGetDeviceProcAddr(Device,"vkFlushMappedMemoryRanges");
	}
	{
		vkInvalidateMappedMemoryRanges=(PFN_vkInvalidateMappedMemoryRanges)vkGetDeviceProcAddr(Device,"vkInvalidateMappedMemoryRanges");
	}
	{
		vkBindBufferMemory=(PFN_vkBindBufferMemory)vkGetDeviceProcAddr(Device,"vkBindBufferMemory");
	}
	{
		vkBindImageMemory=(PFN_vkBindImageMemory)vkGetDeviceProcAddr(Device,"vkBindImageMemory");
	}
	{
		vkGetBufferMemoryRequirements=(PFN_vkGetBufferMemoryRequirements)vkGetDeviceProcAddr(Device,"vkGetBufferMemoryRequirements");
	}
	{
		vkGetImageMemoryRequirements=(PFN_vkGetImageMemoryRequirements)vkGetDeviceProcAddr(Device,"vkGetImageMemoryRequirements");
	}
	{
		vkCreateBuffer=(PFN_vkCreateBuffer)vkGetDeviceProcAddr(Device,"vkCreateBuffer");
	}
	{
		vkDestroyBuffer=(PFN_vkDestroyBuffer)vkGetDeviceProcAddr(Device,"vkDestroyBuffer");
	}
	{
		vkCreateImage=(PFN_vkCreateImage)vkGetDeviceProcAddr(Device,"vkCreateImage");
	}
	{
		vkDestroyImage=(PFN_vkDestroyImage)vkGetDeviceProcAddr(Device,"vkDestroyImage");
	}
	{
		vkCmdCopyBuffer=(PFN_vkCmdCopyBuffer)vkGetDeviceProcAddr(Device,"vkCmdCopyBuffer");
	}
	{
		vkCmdCopyBufferToImage=(PFN_vkCmdCopyBufferToImage)vkGetDeviceProcAddr(Device,"vkCmdCopyBufferToImage");
	}
	{
		vkCreateSampler=(PFN_vkCreateSampler)vkGetDeviceProcAddr(Device,"vkCreateSampler");
	}
	{
		vkCreateDescriptorSetLayout=(PFN_vkCreateDescriptorSetLayout)vkGetDeviceProcAddr(Device,"vkCreateDescriptorSetLayout");
	}
	{
		vkCreateDescriptorPool=(PFN_vkCreateDescriptorPool)vkGetDeviceProcAddr(Device,"vkCreateDescriptorPool");
	}
	{
		vkAllocateDescriptorSets=(PFN_vkAllocateDescriptorSets)vkGetDeviceProcAddr(Device,"vkAllocateDescriptorSets");
	}
	{
		vkUpdateDescriptorSets=(PFN_vkUpdateDescriptorSets)vkGetDeviceProcAddr(Device,"vkUpdateDescriptorSets");
	}
	{
		vkCreateShaderModule=(PFN_vkCreateShaderModule)vkGetDeviceProcAddr(Device,"vkCreateShaderModule");
	}
	{
		vkCreatePipelineLayout=(PFN_vkCreatePipelineLayout)vkGetDeviceProcAddr(Device,"vkCreatePipelineLayout");
	}
	{
		vkCreateGraphicsPipelines=(PFN_vkCreateGraphicsPipelines)vkGetDeviceProcAddr(Device,"vkCreateGraphicsPipelines");
	}
	{
		vkDestroyPipelineLayout=(PFN_vkDestroyPipelineLayout)vkGetDeviceProcAddr(Device,"vkDestroyPipelineLayout");
	}
	{
		vkDestroyShaderModule=(PFN_vkDestroyShaderModule)vkGetDeviceProcAddr(Device,"vkDestroyShaderModule");
	}
	{
		vkCmdBindPipeline=(PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(Device,"vkCmdBindPipeline");
	}
	{
		vkCmdBindDescriptorSets=(PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(Device,"vkCmdBindDescriptorSets");
	}
	{
		vkCmdBindVertexBuffers=(PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(Device,"vkCmdBindVertexBuffers");
	}
	{
		vkCmdBindIndexBuffer=(PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(Device,"vkCmdBindIndexBuffer");
	}
	{
		vkCmdDrawIndexed=(PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(Device,"vkCmdDrawIndexed");
	}
	{
		vkCmdPushConstants=(PFN_vkCmdPushConstants)vkGetDeviceProcAddr(Device,"vkCmdPushConstants");
	}
	{
		vkCmdSetViewport=(PFN_vkCmdSetViewport)vkGetDeviceProcAddr(Device,"vkCmdSetViewport");
	}
	{
		vkCmdSetScissor=(PFN_vkCmdSetScissor)vkGetDeviceProcAddr(Device,"vkCmdSetScissor");
	}
	{
		vkCreateComputePipelines=(PFN_vkCreateComputePipelines)vkGetDeviceProcAddr(Device,"vkCreateComputePipelines");
	}
	{
		vkCmdDispatch=(PFN_vkCmdDispatch)vkGetDeviceProcAddr(Device,"vkCmdDispatch");
	}
	{
		vkDestroyDescriptorSetLayout=(PFN_vkDestroyDescriptorSetLayout)vkGetDeviceProcAddr(Device,"vkDestroyDescriptorSetLayout");
	}
	{
		vkCmdCopyImageToBuffer=(PFN_vkCmdCopyImageToBuffer)vkGetDeviceProcAddr(Device,"vkCmdCopyImageToBuffer");
	}
	{
		vkDestroyImageView=(PFN_vkDestroyImageView)vkGetDeviceProcAddr(Device,"vkDestroyImageView");
	}
}
function void Vk_Ext_Debug_Utils_Funcs_Load(VkInstance Instance) {
	{
		vkCreateDebugUtilsMessengerEXT=(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance,"vkCreateDebugUtilsMessengerEXT");
	}
	{
		vkDestroyDebugUtilsMessengerEXT=(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance,"vkDestroyDebugUtilsMessengerEXT");
	}
}
function void Vk_Khr_Surface_Funcs_Load(VkInstance Instance) {
	{
		vkDestroySurfaceKHR=(PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(Instance,"vkDestroySurfaceKHR");
	}
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR=(PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	}
	{
		vkGetPhysicalDeviceSurfaceFormatsKHR=(PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceSurfaceFormatsKHR");
	}
	{
		vkGetPhysicalDeviceSurfacePresentModesKHR=(PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceSurfacePresentModesKHR");
	}
	{
		vkGetPhysicalDeviceSurfaceSupportKHR=(PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(Instance,"vkGetPhysicalDeviceSurfaceSupportKHR");
	}
}
function void Vk_Khr_Swapchain_Funcs_Load(VkDevice Device) {
	{
		vkAcquireNextImageKHR=(PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(Device,"vkAcquireNextImageKHR");
	}
	{
		vkCreateSwapchainKHR=(PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(Device,"vkCreateSwapchainKHR");
	}
	{
		vkDestroySwapchainKHR=(PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(Device,"vkDestroySwapchainKHR");
	}
	{
		vkGetSwapchainImagesKHR=(PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(Device,"vkGetSwapchainImagesKHR");
	}
	{
		vkQueuePresentKHR=(PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(Device,"vkQueuePresentKHR");
	}
}
function void Vk_Khr_Synchronization2_Funcs_Load(VkDevice Device) {
	{
		vkCmdPipelineBarrier2KHR=(PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(Device,"vkCmdPipelineBarrier2KHR");
	}
}
function void Vk_Khr_Dynamic_Rendering_Funcs_Load(VkDevice Device) {
	{
		vkCmdBeginRenderingKHR=(PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(Device,"vkCmdBeginRenderingKHR");
	}
	{
		vkCmdEndRenderingKHR=(PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(Device,"vkCmdEndRenderingKHR");
	}
}
function void Vk_Khr_Push_Descriptor_Funcs_Load(VkDevice Device) {
	{
		vkCmdPushDescriptorSetKHR=(PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(Device,"vkCmdPushDescriptorSetKHR");
	}
}
function void VK_Load_Library_Funcs(os_library* Library) {
	{
		vkGetInstanceProcAddr=(PFN_vkGetInstanceProcAddr)OS_Library_Get_Function(Library,"vkGetInstanceProcAddr");
	}
}
function void VK_Load_Global_Funcs() {
	{
		vkEnumerateInstanceExtensionProperties=(PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(NULL,"vkEnumerateInstanceExtensionProperties");
	}
	{
		vkEnumerateInstanceLayerProperties=(PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr(NULL,"vkEnumerateInstanceLayerProperties");
	}
	{
		vkCreateInstance=(PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL,"vkCreateInstance");
	}
}
function void VK_Texture_Pool_Init(vk_texture_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Texture_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->TexturePool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_TEXTURE;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->TexturePool.Entries[ID.Index],sizeof(vk_texture));
	return Result;
}
function void VK_Texture_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,TEXTURE));
		GDI_Pool_Free(&Pool->TexturePool.Pool,Handle.ID);
	}
}
function vk_texture* VK_Texture_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->TexturePool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,TEXTURE));
	return Pool->TexturePool.Entries + Handle.ID.Index;
}
function void VK_Texture_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_texture* Object=VK_Texture_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_texture_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_texture_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->TextureDeleteQueue.First,ThreadContext->TextureDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_texture));
		VK_Texture_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Texture_View_Pool_Init(vk_texture_view_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Texture_View_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->Texture_ViewPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_TEXTURE_VIEW;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->Texture_ViewPool.Entries[ID.Index],sizeof(vk_texture_view));
	return Result;
}
function void VK_Texture_View_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,TEXTURE_VIEW));
		GDI_Pool_Free(&Pool->Texture_ViewPool.Pool,Handle.ID);
	}
}
function vk_texture_view* VK_Texture_View_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->Texture_ViewPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,TEXTURE_VIEW));
	return Pool->Texture_ViewPool.Entries + Handle.ID.Index;
}
function void VK_Texture_View_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_texture_view* Object=VK_Texture_View_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_texture_view_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_texture_view_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->Texture_ViewDeleteQueue.First,ThreadContext->Texture_ViewDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_texture_view));
		VK_Texture_View_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Buffer_Pool_Init(vk_buffer_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Buffer_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->BufferPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_BUFFER;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->BufferPool.Entries[ID.Index],sizeof(vk_buffer));
	return Result;
}
function void VK_Buffer_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,BUFFER));
		GDI_Pool_Free(&Pool->BufferPool.Pool,Handle.ID);
	}
}
function vk_buffer* VK_Buffer_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->BufferPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,BUFFER));
	return Pool->BufferPool.Entries + Handle.ID.Index;
}
function void VK_Buffer_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_buffer* Object=VK_Buffer_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_buffer_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_buffer_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->BufferDeleteQueue.First,ThreadContext->BufferDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_buffer));
		VK_Buffer_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Sampler_Pool_Init(vk_sampler_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Sampler_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->SamplerPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_SAMPLER;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->SamplerPool.Entries[ID.Index],sizeof(vk_sampler));
	return Result;
}
function void VK_Sampler_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,SAMPLER));
		GDI_Pool_Free(&Pool->SamplerPool.Pool,Handle.ID);
	}
}
function vk_sampler* VK_Sampler_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->SamplerPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,SAMPLER));
	return Pool->SamplerPool.Entries + Handle.ID.Index;
}
function void VK_Sampler_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_sampler* Object=VK_Sampler_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_sampler_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_sampler_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->SamplerDeleteQueue.First,ThreadContext->SamplerDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_sampler));
		VK_Sampler_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Bind_Group_Layout_Pool_Init(vk_bind_group_layout_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Bind_Group_Layout_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->Bind_Group_LayoutPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_BIND_GROUP_LAYOUT;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->Bind_Group_LayoutPool.Entries[ID.Index],sizeof(vk_bind_group_layout));
	return Result;
}
function void VK_Bind_Group_Layout_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,BIND_GROUP_LAYOUT));
		GDI_Pool_Free(&Pool->Bind_Group_LayoutPool.Pool,Handle.ID);
	}
}
function vk_bind_group_layout* VK_Bind_Group_Layout_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->Bind_Group_LayoutPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,BIND_GROUP_LAYOUT));
	return Pool->Bind_Group_LayoutPool.Entries + Handle.ID.Index;
}
function void VK_Bind_Group_Layout_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_bind_group_layout* Object=VK_Bind_Group_Layout_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_bind_group_layout_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_bind_group_layout_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->Bind_Group_LayoutDeleteQueue.First,ThreadContext->Bind_Group_LayoutDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_bind_group_layout));
		VK_Bind_Group_Layout_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Bind_Group_Pool_Init(vk_bind_group_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Bind_Group_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->Bind_GroupPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_BIND_GROUP;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->Bind_GroupPool.Entries[ID.Index],sizeof(vk_bind_group));
	return Result;
}
function void VK_Bind_Group_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,BIND_GROUP));
		GDI_Pool_Free(&Pool->Bind_GroupPool.Pool,Handle.ID);
	}
}
function vk_bind_group* VK_Bind_Group_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->Bind_GroupPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,BIND_GROUP));
	return Pool->Bind_GroupPool.Entries + Handle.ID.Index;
}
function void VK_Bind_Group_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_bind_group* Object=VK_Bind_Group_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_bind_group_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_bind_group_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->Bind_GroupDeleteQueue.First,ThreadContext->Bind_GroupDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_bind_group));
		VK_Bind_Group_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Shader_Pool_Init(vk_shader_pool* Pool, arena* Arena) {
	u16* IndicesPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),u16);
	gdi_id* IDsPtr=Arena_Push_Array(Arena,Array_Count(Pool->Entries),gdi_id);
	GDI_Pool_Init_Raw(&Pool->Pool,IndicesPtr,IDsPtr,Array_Count(Pool->Entries));
}
function gdi_handle VK_Shader_Pool_Allocate(vk_resource_pool* Pool) {
	gdi_id ID=GDI_Pool_Allocate(&Pool->ShaderPool.Pool);
	if(GDI_ID_Is_Null(ID))return GDI_Null_Handle();
	gdi_handle Result;
#ifdef DEBUG_BUILD
	Result.Type=GDI_OBJECT_TYPE_SHADER;
#endif
	Result.ID=ID;
	Memory_Clear(&Pool->ShaderPool.Entries[ID.Index],sizeof(vk_shader));
	return Result;
}
function void VK_Shader_Pool_Free(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Is_Null(Handle)){
		Assert(GDI_Is_Type(Handle,SHADER));
		GDI_Pool_Free(&Pool->ShaderPool.Pool,Handle.ID);
	}
}
function vk_shader* VK_Shader_Pool_Get(vk_resource_pool* Pool, gdi_handle Handle) {
	if(!GDI_Pool_Is_Allocated(&Pool->ShaderPool.Pool,Handle.ID))return NULL;
	Assert(GDI_Is_Type(Handle,SHADER));
	return Pool->ShaderPool.Entries + Handle.ID.Index;
}
function void VK_Shader_Add_To_Delete_Queue(vk_gdi* GDI, gdi_handle Handle) {
	vk_shader* Object=VK_Shader_Pool_Get(&GDI->ResourcePool,Handle);
	if(Object){
		OS_RW_Mutex_Read_Lock(GDI->DeleteLock);
		vk_delete_thread_context* ThreadContext=VK_Get_Delete_Thread_Context(GDI,GDI->CurrentDeleteQueue);
		vk_shader_delete_queue_entry* Entry=Arena_Push_Struct(ThreadContext->Arena,vk_shader_delete_queue_entry);
		Entry->Entry=*Object;
		SLL_Push_Back(ThreadContext->ShaderDeleteQueue.First,ThreadContext->ShaderDeleteQueue.Last,Entry);
		OS_RW_Mutex_Read_Unlock(GDI->DeleteLock);
		Memory_Clear(Object,sizeof(vk_shader));
		VK_Shader_Pool_Free(&GDI->ResourcePool,Handle);
	}
}
function void VK_Resource_Pool_Init(vk_resource_pool* Pool, arena* Arena) {
	{
		VK_Texture_Pool_Init(&Pool->TexturePool,Arena);
	}
	{
		VK_Texture_View_Pool_Init(&Pool->Texture_ViewPool,Arena);
	}
	{
		VK_Buffer_Pool_Init(&Pool->BufferPool,Arena);
	}
	{
		VK_Sampler_Pool_Init(&Pool->SamplerPool,Arena);
	}
	{
		VK_Bind_Group_Layout_Pool_Init(&Pool->Bind_Group_LayoutPool,Arena);
	}
	{
		VK_Bind_Group_Pool_Init(&Pool->Bind_GroupPool,Arena);
	}
	{
		VK_Shader_Pool_Init(&Pool->ShaderPool,Arena);
	}
}
function gdi_format VK_Get_GDI_Format(VkFormat Format) {
	switch(Format){
		case VK_FORMAT_UNDEFINED:return GDI_FORMAT_NONE;
		case VK_FORMAT_R8_UNORM:return GDI_FORMAT_R8_UNORM;
		case VK_FORMAT_R8G8_UNORM:return GDI_FORMAT_R8G8_UNORM;
		case VK_FORMAT_R8G8B8_UNORM:return GDI_FORMAT_R8G8B8_UNORM;
		case VK_FORMAT_R8G8B8A8_UNORM:return GDI_FORMAT_R8G8B8A8_UNORM;
		case VK_FORMAT_B8G8R8A8_UNORM:return GDI_FORMAT_B8G8R8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SRGB:return GDI_FORMAT_R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB:return GDI_FORMAT_B8G8R8A8_SRGB;
		case VK_FORMAT_R32_SFLOAT:return GDI_FORMAT_R32_FLOAT;
		case VK_FORMAT_R32G32_SFLOAT:return GDI_FORMAT_R32G32_FLOAT;
		case VK_FORMAT_R32G32B32_SFLOAT:return GDI_FORMAT_R32G32B32_FLOAT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:return GDI_FORMAT_R32G32B32A32_FLOAT;
		case VK_FORMAT_D32_SFLOAT:return GDI_FORMAT_D32_FLOAT;
		Invalid_Default_Case;
	}
	return GDI_FORMAT_NONE;
}
function VkFilter VK_Get_Filter(gdi_filter Filter) {
	static const VkFilter Filters[]={
		VK_FILTER_NEAREST,VK_FILTER_LINEAR,}
	;
	Assert((size_t)Filter < Array_Count(Filters));
	return Filters[Filter];
}
function VkSamplerMipmapMode VK_Get_Mipmap_Mode(gdi_filter Filter) {
	static const VkSamplerMipmapMode MipmapMode[]={
		VK_SAMPLER_MIPMAP_MODE_NEAREST,VK_SAMPLER_MIPMAP_MODE_LINEAR,}
	;
	Assert((size_t)Filter < Array_Count(MipmapMode));
	return MipmapMode[Filter];
}
function VkFormat VK_Get_Format(gdi_format Entry) {
	static const VkFormat Entries[]={
		(VkFormat)VK_FORMAT_UNDEFINED,(VkFormat)VK_FORMAT_R8_UNORM,(VkFormat)VK_FORMAT_R8G8_UNORM,(VkFormat)VK_FORMAT_R8G8B8_UNORM,(VkFormat)VK_FORMAT_R8G8B8A8_UNORM,(VkFormat)VK_FORMAT_B8G8R8A8_UNORM,(VkFormat)VK_FORMAT_R8G8B8A8_SRGB,(VkFormat)VK_FORMAT_B8G8R8A8_SRGB,(VkFormat)VK_FORMAT_R32_SFLOAT,(VkFormat)VK_FORMAT_R32G32_SFLOAT,(VkFormat)VK_FORMAT_R32G32B32_SFLOAT,(VkFormat)VK_FORMAT_R32G32B32A32_SFLOAT,(VkFormat)VK_FORMAT_D32_SFLOAT,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkSamplerAddressMode VK_Get_Address_Mode(gdi_address_mode Entry) {
	static const VkSamplerAddressMode Entries[]={
		(VkSamplerAddressMode)VK_SAMPLER_ADDRESS_MODE_REPEAT,(VkSamplerAddressMode)VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkDescriptorType VK_Get_Descriptor_Type(gdi_bind_group_type Entry) {
	static const VkDescriptorType Entries[]={
		(VkDescriptorType)-1,(VkDescriptorType)VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,(VkDescriptorType)VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,(VkDescriptorType)VK_DESCRIPTOR_TYPE_SAMPLER,(VkDescriptorType)VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,(VkDescriptorType)VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkCompareOp VK_Get_Compare_Func(gdi_compare_func Entry) {
	static const VkCompareOp Entries[]={
		(VkCompareOp)VK_COMPARE_OP_NEVER,(VkCompareOp)VK_COMPARE_OP_LESS,(VkCompareOp)VK_COMPARE_OP_LESS_OR_EQUAL,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkIndexType VK_Get_Idx_Type(gdi_idx_format Entry) {
	static const VkIndexType Entries[]={
		(VkIndexType)-1,(VkIndexType)VK_INDEX_TYPE_UINT16,(VkIndexType)VK_INDEX_TYPE_UINT32,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkPrimitiveTopology VK_Get_Primitive(gdi_primitive Entry) {
	static const VkPrimitiveTopology Entries[]={
		(VkPrimitiveTopology)VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,(VkPrimitiveTopology)VK_PRIMITIVE_TOPOLOGY_LINE_LIST,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function VkBlendFactor VK_Get_Blend(gdi_blend Entry) {
	static const VkBlendFactor Entries[]={
		(VkBlendFactor)VK_BLEND_FACTOR_ZERO,(VkBlendFactor)VK_BLEND_FACTOR_ONE,(VkBlendFactor)VK_BLEND_FACTOR_SRC_COLOR,(VkBlendFactor)VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,(VkBlendFactor)VK_BLEND_FACTOR_DST_COLOR,(VkBlendFactor)VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,(VkBlendFactor)VK_BLEND_FACTOR_SRC_ALPHA,(VkBlendFactor)VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,(VkBlendFactor)VK_BLEND_FACTOR_DST_ALPHA,(VkBlendFactor)VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,}
	;
	Assert((size_t)Entry < Array_Count(Entries));
	return Entries[Entry];
}
function void VK_Delete_Queued_Thread_Resources(vk_gdi* GDI, vk_delete_thread_context* DeleteContext) {
	{
	}
	{
	}
	{
		for(vk_texture_delete_queue_entry* QueueEntry=DeleteContext->TextureDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Texture_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->TextureDeleteQueue,sizeof(vk_texture_delete_queue));
	}
	{
		for(vk_texture_view_delete_queue_entry* QueueEntry=DeleteContext->Texture_ViewDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Texture_View_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->Texture_ViewDeleteQueue,sizeof(vk_texture_view_delete_queue));
	}
	{
		for(vk_buffer_delete_queue_entry* QueueEntry=DeleteContext->BufferDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Buffer_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->BufferDeleteQueue,sizeof(vk_buffer_delete_queue));
	}
	{
		for(vk_sampler_delete_queue_entry* QueueEntry=DeleteContext->SamplerDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Sampler_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->SamplerDeleteQueue,sizeof(vk_sampler_delete_queue));
	}
	{
		for(vk_bind_group_layout_delete_queue_entry* QueueEntry=DeleteContext->Bind_Group_LayoutDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Bind_Group_Layout_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->Bind_Group_LayoutDeleteQueue,sizeof(vk_bind_group_layout_delete_queue));
	}
	{
		for(vk_bind_group_delete_queue_entry* QueueEntry=DeleteContext->Bind_GroupDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Bind_Group_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->Bind_GroupDeleteQueue,sizeof(vk_bind_group_delete_queue));
	}
	{
		for(vk_shader_delete_queue_entry* QueueEntry=DeleteContext->ShaderDeleteQueue.First;
		QueueEntry;
		QueueEntry=QueueEntry->Next){
			VK_Delete_Shader_Resources(GDI,&QueueEntry->Entry);
		}
		Memory_Clear(&DeleteContext->ShaderDeleteQueue,sizeof(vk_shader_delete_queue));
	}
	{
	}
	Arena_Set_Marker(DeleteContext->Arena,DeleteContext->ArenaMarker);
}
