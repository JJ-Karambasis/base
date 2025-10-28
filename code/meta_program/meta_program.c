#include "../base.h"
#include "meta_errors.c"
#include "source_parser.h"
#include "meta_parser.h"
#include "source_parser.c"
#include "meta_parser.c"
#include "meta_generator.h"
#include "meta_generator.c"

Dynamic_Array_Implement_Type(string, String);

int main(int ArgCount, const char** Args) {
	Base_Init();
	arena* Arena = Arena_Create();
	Source_Parser_Init_Globals(Arena);
	Meta_Parser_Init_Globals(Arena);
	G_ErrorStream = SStream_Writer_Begin((allocator*)Arena);
	
	//Hashmaps to check if directories or files are excluded
	hashmap DirectoriesToExclude = Hashmap_Init((allocator*)Arena, sizeof(b8), sizeof(string), Hash_String, Compare_String);
	hashmap FilesToExclude = Hashmap_Init((allocator*)Arena, sizeof(b8), sizeof(string), Hash_String, Compare_String);
	for (int i = 1; i < ArgCount; i++) {
		string Arg = String_Null_Term(Args[i]);
		if (String_Equals(Arg, String_Lit("-D"))) {
			i++;
			while (i < ArgCount) {
				string Parameter = String_Null_Term(Args[i]);
				
				if (String_Starts_With_Char(Parameter, '-')) {
					i--;
					break;
				}

				if (!OS_Is_Directory_Path(Parameter)) {
					Debug_Log("Invalid parameter '-D %.*s'. Not a valid directory path", Parameter.Size, Parameter.Ptr);
					return 1;
				}

				b8 Value = true;
				string DirectoryPath = String_Directory_Concat((allocator*)Arena, Parameter, String_Empty());
				Hashmap_Add(&DirectoriesToExclude, &DirectoryPath, &Value);
				i++;
			}
		}

		if (String_Equals(Arg, String_Lit("-F"))) {
			i++;
			while (i < ArgCount) {
				string Parameter = String_Null_Term(Args[i]);
				
				if (String_Starts_With_Char(Parameter, '-')) {
					i--;
					break;
				}

				if (!OS_Is_File_Path(Parameter)) {
					Debug_Log("Invalid parameter '-F %.*s'. Not a valid directory path", Parameter.Size, Parameter.Ptr);
					return 1;
				}

				b8 Value = true;
				Hashmap_Add(&FilesToExclude, &Parameter, &Value);
				i++;
			}
		}
	}

	//Hashmap to prevent duplicate file entries
	hashmap FileMap = Hashmap_Init((allocator*)Arena, sizeof(b8), sizeof(string), Hash_String, Compare_String);

	for (int i = 1; i < ArgCount; i++) {
		string Arg = String_Null_Term(Args[i]);
		if (String_Equals(Arg, String_Lit("-d"))) {
			i++;
			while (i < ArgCount) {
				string Parameter = String_Null_Term(Args[i]);
				
				if (String_Starts_With_Char(Parameter, '-')) {
					i--;
					break;
				}

				if (!OS_Is_Directory_Path(Parameter)) {
					Debug_Log("Invalid parameter '-d %.*s'. Not a valid directory path", Parameter.Size, Parameter.Ptr);
					return 1;
				}

				string DirectoryPath = String_Directory_Concat((allocator*)Arena, Parameter, String_Empty());

				dynamic_string_array FileStack = Dynamic_String_Array_Init((allocator*)Arena);
				Dynamic_String_Array_Add(&FileStack, DirectoryPath);

				while (!Dynamic_String_Array_Is_Empty(&FileStack)) {
					string DirectoryOrFilePath = Dynamic_String_Array_Pop(&FileStack);
					if (OS_Is_Directory_Path(DirectoryOrFilePath)) {
						if (!Hashmap_Find_Ptr(&DirectoriesToExclude, &DirectoryOrFilePath)) {
							string_array Files = OS_Get_All_Files((allocator*)Arena, DirectoryOrFilePath, false);
							Dynamic_String_Array_Add_Range(&FileStack, Files.Ptr, Files.Count);
						}
					} else if (OS_Is_File_Path(DirectoryOrFilePath)) {
						if (!Hashmap_Find_Ptr(&FileMap, &DirectoryOrFilePath) && !Hashmap_Find_Ptr(&FilesToExclude, &DirectoryOrFilePath)) {
							b8 Value = true;
							Hashmap_Add(&FileMap, &DirectoryOrFilePath, &Value);
						}						
					}
				}

				i++;
			}
		}

		if (String_Equals(Arg, String_Lit("-f"))) {
			i++;
			while (i < ArgCount) {
				string Parameter = String_Null_Term(Args[i]);
				
				if (String_Starts_With_Char(Parameter, '-')) {
					i--;
					break;
				}

				if (!OS_Is_File_Path(Parameter)) {
					Debug_Log("Invalid parameter '-f %.*s'. Not a valid file path", Parameter.Size, Parameter.Ptr);
					return 1;
				}

				if (!Hashmap_Find_Ptr(&FileMap, &Parameter) && !Hashmap_Find_Ptr(&FilesToExclude, &Parameter)) {
					b8 Value = true;
					Hashmap_Add(&FileMap, &Parameter, &Value);
				}

				i++;
			}
		}
	}

	dynamic_string_array MetaFiles = Dynamic_String_Array_Init((allocator*)Arena);
	dynamic_string_array SourceFiles = Dynamic_String_Array_Init((allocator*)Arena);

	for (size_t i = 0; i < FileMap.ItemCount; i++) {
		string File;
		if (Hashmap_Get_Key(&FileMap, i, &File)) {
			string Ext = String_Get_Filename_Ext(File);
			if (String_Equals(Ext, String_Lit("meta"))) {
				Dynamic_String_Array_Add(&MetaFiles, File);
			} else if (String_Equals(Ext, String_Lit("h")) || String_Equals(Ext, String_Lit("c")) || String_Equals(Ext, String_Lit("cpp"))) {
				Dynamic_String_Array_Add(&SourceFiles, File);
			} else {
				Debug_Log("Skipping file %.*s", File.Size, File.Ptr);
			}
		}
	}

	hashmap StructTypeMap = Hashmap_Init((allocator*)Arena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);
	hashmap EnumTypeMap = Hashmap_Init((allocator*)Arena, sizeof(meta_enum_type*), sizeof(string), Hash_String, Compare_String);
	hashmap UnionTypeMap = Hashmap_Init((allocator*)Arena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);

	for (size_t i = 0; i < SourceFiles.Count; i++) {
		arena* Scratch = Scratch_Get();
		string File = SourceFiles.Ptr[i];
		buffer Buffer = Read_Entire_File((allocator*)Scratch, File);
		if (!Buffer_Is_Empty(Buffer)) {
			string FileContent = String_From_Buffer(Buffer);
			
			source_parser Parser = { 0 };
			if (Source_Parser_Parse(&Parser, File, FileContent, Arena)) {
				b32 AddStructs = true;
				b32 AddEnums = true;
				b32 AddUnions = true;

				for (size_t i = 0; i < Parser.StructMap.ItemCount; i++) {
					string StructName;
					source_struct_type* Struct;
					Hashmap_Get_Key_Value(&Parser.StructMap, i, &StructName, &Struct);

					if (Hashmap_Find(&StructTypeMap, &StructName, &Struct)) {
						Report_Error(File, Struct->Token->LineNumber, "Duplicate struct '%.*s' for meta parser", Struct->Struct->Name.Size, Struct->Struct->Name.Ptr);
						AddStructs = false;
					}
				}

				for (size_t i = 0; i < Parser.UnionMap.ItemCount; i++) {
					string UnionName;
					source_struct_type* Union;
					Hashmap_Get_Key_Value(&Parser.UnionMap, i, &UnionName, &Union);

					if (Hashmap_Find(&UnionTypeMap, &UnionName, &Union)) {
						Report_Error(File, Union->Token->LineNumber, "Duplicate union '%.*s' for meta parser", Union->Struct->Name.Size, Union->Struct->Name.Ptr);
						AddUnions = false;
					}
				}

				for (size_t i = 0; i < Parser.EnumMap.ItemCount; i++) {
					string EnumName;
					source_enum_type* Enum;
					Hashmap_Get_Key_Value(&Parser.EnumMap, i, &EnumName, &Enum);

					if (Hashmap_Find(&EnumTypeMap, &EnumName, &Enum)) {
						Report_Error(File, Enum->Token->LineNumber, "Duplicate enum '%.*s' for meta parser", Enum->Enum->Name.Size, Enum->Enum->Name.Ptr);
						AddEnums = false;
					}
				}

				if (AddStructs) {
					for (size_t i = 0; i < Parser.StructMap.ItemCount; i++) {
						string StructName;
						source_struct_type* Struct;
						Hashmap_Get_Key_Value(&Parser.StructMap, i, &StructName, &Struct);
						Hashmap_Add(&StructTypeMap, &StructName, &Struct->Struct);
					}
				}

				if (AddUnions) {
					for (size_t i = 0; i < Parser.UnionMap.ItemCount; i++) {
						string UnionName;
						source_struct_type* Union;
						Hashmap_Get_Key_Value(&Parser.UnionMap, i, &UnionName, &Union);
						Hashmap_Add(&UnionTypeMap, &UnionName, &Union->Struct);
					}
				}

				if (AddEnums) {
					for (size_t i = 0; i < Parser.EnumMap.ItemCount; i++) {
						string EnumName;
						source_enum_type* Enum;
						Hashmap_Get_Key_Value(&Parser.EnumMap, i, &EnumName, &Enum);
						Hashmap_Add(&EnumTypeMap, &EnumName, &Enum->Enum);
					}
				}
			}
		}

		Scratch_Release();
	}

	meta_parser_process_info ProcessInfo = {
		.Structs = (meta_struct_type**)StructTypeMap.Values,
		.StructCount = StructTypeMap.ItemCount,
		.Unions = (meta_struct_type**)UnionTypeMap.Values,
		.UnionCount = UnionTypeMap.ItemCount,
		.Enums = (meta_enum_type**)EnumTypeMap.Values,
		.EnumCount = EnumTypeMap.ItemCount
	};

	for (size_t i = 0; i < MetaFiles.Count; i++) {
		arena* Scratch = Scratch_Get();
		string File = MetaFiles.Ptr[i];
		buffer Buffer = Read_Entire_File((allocator*)Scratch, File);
		if (!Buffer_Is_Empty(Buffer)) {
			string FileContent = String_From_Buffer(Buffer);
			meta_parser Parser = { 0 };			
			if (Meta_Parser_Parse(&Parser, File, FileContent, Scratch, &ProcessInfo)) {
				meta_file_source SourceCode = Meta_Generate_Source(Scratch, &Parser);

				string FileDirectory = String_Get_Directory_Path(File);
				string Filename = String_Get_Filename_Without_Ext(File);
				string FullPath = String_Directory_Concat((allocator*)Scratch, FileDirectory, String_Lit("meta"));

				if (!OS_Is_Directory_Path(FullPath)) {
					OS_Make_Directory(FullPath);
				}

				string HeaderFileName = String_Concat((allocator*)Scratch, Filename, String_Lit("_meta.h"));
				string SourceFileName = String_Concat((allocator*)Scratch, Filename, String_Lit("_meta.c"));

				string HeaderFilePath = String_Directory_Concat((allocator*)Scratch, FullPath, HeaderFileName);
				string SourceFilePath = String_Directory_Concat((allocator*)Scratch, FullPath, SourceFileName);

				if (!String_Is_Empty(SourceCode.Header)) {
					Write_Entire_File(HeaderFilePath, Buffer_From_String(SourceCode.Header));
				}

				if (!String_Is_Empty(SourceCode.Source)) {
					Write_Entire_File(SourceFilePath, Buffer_From_String(SourceCode.Source));
				}
			}
		}
		Scratch_Release();
	}

	Output_Errors();

	return 0;
}