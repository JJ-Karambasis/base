global sstream_writer G_ErrorStream;
function void Report_Error(string FilePath, size_t Line, const char* Format, ...) {
	arena* Scratch = Scratch_Get();

	va_list List;
	va_start(List, Format);
	string String = String_FormatV((allocator*)Scratch, Format, List);
	va_end(List);

	SStream_Writer_Add_Format(&G_ErrorStream, "%.*s(%u): error: %.*s", FilePath.Size, FilePath.Ptr, Line, String.Size, String.Ptr);

	Scratch_Release();
}

function void Output_Errors() {
	arena* Scratch = Scratch_Get();
	string ErrorText = SStream_Writer_Join(&G_ErrorStream, (allocator*)Scratch, String_Lit("\n"));
	Debug_Log("%.*s", ErrorText.Size, ErrorText.Ptr);
	Scratch_Release();
}