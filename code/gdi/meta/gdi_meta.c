function gdi_format GDI_Get_SRGB_Format(gdi_format Format) {
	static const gdi_format Formats[]={
		GDI_FORMAT_NONE,GDI_FORMAT_R8_UNORM,GDI_FORMAT_R8G8_UNORM,GDI_FORMAT_R8G8B8_UNORM,GDI_FORMAT_R8G8B8A8_SRGB,GDI_FORMAT_B8G8R8A8_SRGB,GDI_FORMAT_R8G8B8A8_SRGB,GDI_FORMAT_B8G8R8A8_SRGB,GDI_FORMAT_R32_FLOAT,GDI_FORMAT_R32G32_FLOAT,GDI_FORMAT_R32G32B32_FLOAT,GDI_FORMAT_R32G32B32A32_FLOAT,GDI_FORMAT_D32_FLOAT,}
	;
	Assert(Format < Array_Count(Formats));
	return Formats[Format];
}
function b32 GDI_Is_Depth_Format(gdi_format Format) {
	static const b32 IsDepthFormat[]={
		false,false,false,false,false,false,false,false,false,false,false,false,true,}
	;
	Assert(Format < Array_Count(IsDepthFormat));
	return IsDepthFormat[Format];
}
function size_t GDI_Get_Format_Size(gdi_format Format) {
	static const size_t Sizes[]={
		0,1,2,3,4,4,4,4,4,8,12,16,4,}
	;
	Assert(Format < Array_Count(Sizes));
	return Sizes[Format];
}
function b32 GDI_Is_Bind_Group_Type_Writable(gdi_bind_group_type Type) {
	static const b32 IsWritable[]={
		false,false,false,false,true,true,}
	;
	Assert(Type < Array_Count(IsWritable));
	return IsWritable[Type];
}
