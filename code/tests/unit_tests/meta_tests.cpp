function b32 Meta_Test_Parse(arena* Arena, string Content, meta_parser_process_info* Info, meta_parser* Parser) {
	Meta_Parser_Init_Globals(Arena);
	G_ErrorStream = SStream_Writer_Begin((allocator*)Arena);
	Memory_Clear(Parser, sizeof(meta_parser));

	b32 Result = Meta_Parser_Parse(Parser, String_Lit("test.meta"), Content, Arena, Info);
	if (!Result) {
		string ErrorText = SStream_Writer_Join(&G_ErrorStream, (allocator*)Arena, String_Lit("\n"));
		printf("%.*s\n", (int)ErrorText.Size, ErrorText.Ptr);
		return false;
	}
	return true;
}

function meta_struct_type* Meta_Test_Find_Struct(meta_parser* Parser, string Name) {
	meta_struct_type* Struct = NULL;
	if (!Hashmap_Find(&Parser->StructMap, &Name, &Struct)) {
		return NULL;
	}
	return Struct;
}

UTEST(Meta_Parser, ForLoopCopiesPreviousStructMembers) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_STRUCT(point) {\n"
		"    META_VARIABLE(f32, X);\n"
		"    META_VARIABLE(f32, Y);\n"
		"}\n"
		"META_STRUCT(widget) {\n"
		"    META_FOR(point, flags: NoBraces) {\n"
		"        META_VARIABLE(${META_ENTRY_TYPE}, ${META_ENTRY_NAME});\n"
		"    }\n"
		"    META_VARIABLE(u32, Extra);\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_struct_type* Point = Meta_Test_Find_Struct(&Parser, String_Lit("point"));
	ASSERT_TRUE(Point != NULL);
	ASSERT_EQ(Point->EntryCount, (size_t)2);

	meta_struct_type* Widget = Meta_Test_Find_Struct(&Parser, String_Lit("widget"));
	ASSERT_TRUE(Widget != NULL);
	ASSERT_EQ(Widget->EntryCount, (size_t)3);

	meta_variable_entry* Entry = Widget->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("f32")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("X")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("f32")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Y")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("u32")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Extra")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, ForLoopCopiesSourceStructMembers) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	meta_variable_entry Height = { 0 };
	Height.Type = String_Lit("s32");
	Height.Name = String_Lit("Height");

	meta_variable_entry Width = { 0 };
	Width.Type = String_Lit("s32");
	Width.Name = String_Lit("Width");
	Width.Next = &Height;

	meta_struct_type Size = { 0 };
	Size.Name = String_Lit("size");
	Size.FirstEntry = &Width;
	Size.LastEntry = &Height;
	Size.EntryCount = 2;

	meta_struct_type* Structs[] = { &Size };
	meta_parser_process_info Info = { 0 };
	Info.Structs = Structs;
	Info.StructCount = 1;

	string Content = String_Lit(
		"META_STRUCT(copy) {\n"
		"    META_FOR(size, flags: NoBraces) {\n"
		"        META_VARIABLE(${META_ENTRY_TYPE}, ${META_ENTRY_NAME});\n"
		"    }\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, &Info, &Parser));

	meta_struct_type* Copy = Meta_Test_Find_Struct(&Parser, String_Lit("copy"));
	ASSERT_TRUE(Copy != NULL);
	ASSERT_EQ(Copy->EntryCount, (size_t)2);

	meta_variable_entry* Entry = Copy->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("s32")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Width")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("s32")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Height")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, ForLoopCopiesPreviousEnumEntries) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_ENUM(color) {\n"
		"    META_ENUM_ENTRY(Red);\n"
		"    META_ENUM_ENTRY(Green);\n"
		"}\n"
		"META_ENUM(copy) {\n"
		"    META_FOR(color, flags: NoBraces) {\n"
		"        META_ENUM_ENTRY(${META_ENTRY_SHORT_NAME});\n"
		"    }\n"
		"    META_ENUM_ENTRY(Blue);\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_enum_type* Copy = NULL;
	string CopyName = String_Lit("copy");
	ASSERT_TRUE(Hashmap_Find(&Parser.EnumMap, &CopyName, &Copy));
	ASSERT_EQ(Copy->EntryCount, (size_t)3);

	meta_enum_entry* Entry = Copy->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Red")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Green")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Blue")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, ForLoopCopiesSourceEnumEntries) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	meta_enum_entry Green = { 0 };
	Green.Name = String_Lit("Green");

	meta_enum_entry Red = { 0 };
	Red.Name = String_Lit("Red");
	Red.Next = &Green;

	meta_enum_type Color = { 0 };
	Color.Name = String_Lit("color");
	Color.Type = String_Lit("u32");
	Color.FirstEntry = &Red;
	Color.LastEntry = &Green;
	Color.EntryCount = 2;

	meta_enum_type* Enums[] = { &Color };
	meta_parser_process_info Info = { 0 };
	Info.Enums = Enums;
	Info.EnumCount = 1;

	string Content = String_Lit(
		"META_ENUM(copy) {\n"
		"    META_FOR(color, flags: NoBraces) {\n"
		"        META_ENUM_ENTRY(${META_ENTRY_SHORT_NAME});\n"
		"    }\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, &Info, &Parser));

	meta_enum_type* Copy = NULL;
	string CopyName = String_Lit("copy");
	ASSERT_TRUE(Hashmap_Find(&Parser.EnumMap, &CopyName, &Copy));
	ASSERT_EQ(Copy->EntryCount, (size_t)2);

	meta_enum_entry* Entry = Copy->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Red")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Green")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

function meta_function_type* Meta_Test_Find_Function(meta_parser* Parser, string Name) {
	meta_function_type* Function = NULL;
	if (!Hashmap_Find(&Parser->FunctionMap, &Name, &Function)) {
		return NULL;
	}
	return Function;
}

UTEST(Meta_Parser, FunctionBodyPreservesWhitespace) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_FUNCTION(Add, s32 A, s32 B) -> s32 {\n"
		"\ts32 Result = A + B;\n"
		"\treturn Result;\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_file_source Source = Meta_Generate_Source(Arena, &Parser);
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("s32 Result = A + B;")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("\treturn Result;")));

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, EmptyForIterationOmitsBraces) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_STRUCT(item) {\n"
		"    META_VARIABLE(s32, Keep) Tags(Emit);\n"
		"    META_VARIABLE(s32, Skip);\n"
		"    META_VARIABLE(s32, KeepTwo) Tags(Emit);\n"
		"}\n"
		"META_FUNCTION(Emit_Items) -> void {\n"
		"\tMETA_FOR(item) {\n"
		"\t\tMETA_IF(META_CONTAINS_TAG(Emit)) {\n"
		"\t\t\tUse_${META_ENTRY_NAME}();\n"
		"\t\t}\n"
		"\t}\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_file_source Source = Meta_Generate_Source(Arena, &Parser);
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("Use_Keep();")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("Use_KeepTwo();")));
	ASSERT_FALSE(String_Contains(Source.Source, String_Lit("Use_Skip();")));

	// Empty Skip iteration must not leave a bare {} block between the two emits
	size_t First = String_Find_First(Source.Source, String_Lit("Use_Keep();"));
	size_t Second = String_Find_First(Source.Source, String_Lit("Use_KeepTwo();"));
	ASSERT_TRUE(First != STRING_INVALID_INDEX);
	ASSERT_TRUE(Second != STRING_INVALID_INDEX);
	ASSERT_TRUE(Second > First);
	string Between = String_Substr(Source.Source, First, Second);
	ASSERT_FALSE(String_Contains(Between, String_Lit("{}")));

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, AllEmptyForIterationsRemoved) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_STRUCT(item) {\n"
		"    META_VARIABLE(s32, A);\n"
		"    META_VARIABLE(s32, B);\n"
		"}\n"
		"META_FUNCTION(Emit_None) -> void {\n"
		"\tint Marker = 1;\n"
		"\tMETA_FOR(item) {\n"
		"\t\tMETA_IF(META_CONTAINS_TAG(Never)) {\n"
		"\t\t\tUse_${META_ENTRY_NAME}();\n"
		"\t\t}\n"
		"\t}\n"
		"\tint After = 2;\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_function_type* Function = Meta_Test_Find_Function(&Parser, String_Lit("Emit_None"));
	ASSERT_TRUE(Function != NULL);

	meta_file_source Source = Meta_Generate_Source(Arena, &Parser);
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("int Marker = 1;")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("int After = 2;")));
	ASSERT_FALSE(String_Contains(Source.Source, String_Lit("Use_")));
	ASSERT_FALSE(String_Contains(Source.Source, String_Lit("{}")));

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, NoBracesFlagStillUnwrapsNonEmpty) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_STRUCT(item) {\n"
		"    META_VARIABLE(s32, A);\n"
		"    META_VARIABLE(s32, B);\n"
		"}\n"
		"META_FUNCTION(Emit_Unwrapped) -> void {\n"
		"\tMETA_FOR(item, flags: NoBraces) {\n"
		"\t\tUse_${META_ENTRY_NAME}();\n"
		"\t}\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_file_source Source = Meta_Generate_Source(Arena, &Parser);
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("Use_A();")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("Use_B();")));

	// With NoBraces, statements should not be wrapped in per-iteration braces.
	// Match a close-then-open brace pair between the two uses (would appear if wrapped).
	size_t First = String_Find_First(Source.Source, String_Lit("Use_A();"));
	size_t Second = String_Find_First(Source.Source, String_Lit("Use_B();"));
	ASSERT_TRUE(First != STRING_INVALID_INDEX);
	ASSERT_TRUE(Second != STRING_INVALID_INDEX);
	ASSERT_TRUE(Second > First);
	string Between = String_Substr(Source.Source, First, Second);
	ASSERT_FALSE(String_Contains(Between, String_Lit("}")));
	ASSERT_FALSE(String_Contains(Between, String_Lit("{")));

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, NoBracesNewLineEmitsArrayRows) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_STRUCT(item) {\n"
		"    META_VARIABLE(s32, A);\n"
		"    META_VARIABLE(s32, B);\n"
		"}\n"
		"META_FUNCTION(Emit_Rows) -> void {\n"
		"\tstatic const s32 Values[] = {\n"
		"\t\tMETA_FOR(item, flags: NoBraces|NewLine) {\n"
		"\t\t\t${META_ENTRY_NAME},\n"
		"\t\t}\n"
		"\t};\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_file_source Source = Meta_Generate_Source(Arena, &Parser);
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("static const s32 Values[] = {")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("\t\tA,")));
	ASSERT_TRUE(String_Contains(Source.Source, String_Lit("\t\tB,")));
	ASSERT_FALSE(String_Contains(Source.Source, String_Lit("A,B")));

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, TemplateVariableTypes) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	meta_struct_type Attachment = { 0 };
	Attachment.Name = String_Lit("attachment");

	meta_struct_type AttachmentMesh = { 0 };
	AttachmentMesh.Name = String_Lit("attachment_mesh");
	AttachmentMesh.ParentStruct = &Attachment;

	meta_struct_type* Structs[] = { &Attachment, &AttachmentMesh };
	meta_parser_process_info Info = { 0 };
	Info.Structs = Structs;
	Info.StructCount = 2;

	string Content = String_Lit(
		"META_STRUCT(attachment_manager) {\n"
		"    META_FOR(META_GET_STRUCTS_WITH_PARENT(attachment), flags: NoBraces) {\n"
		"        META_VARIABLE(entity_pool<${META_ENTRY_TYPE}>, ${META_ENTRY_TYPE:pascal}Pool);\n"
		"    }\n"
		"    META_VARIABLE(hashmap<string, u32>, Map);\n"
		"    META_VARIABLE(entity_pool<hashmap<string, u32>>, NestedPool);\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, &Info, &Parser));

	meta_struct_type* Manager = Meta_Test_Find_Struct(&Parser, String_Lit("attachment_manager"));
	ASSERT_TRUE(Manager != NULL);
	ASSERT_EQ(Manager->EntryCount, (size_t)3);

	meta_variable_entry* Entry = Manager->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("entity_pool<attachment_mesh>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Attachment_MeshPool")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("hashmap<string,u32>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Map")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("entity_pool<hashmap<string,u32>>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("NestedPool")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

UTEST(Meta_Parser, MacroTemplateTypeArgument) {
	arena* Arena = Arena_Create(String_Lit("Meta Test"));

	string Content = String_Lit(
		"META_MACRO(MAKE_POOL, type, name) {\n"
		"    META_VARIABLE(${type}, ${name});\n"
		"}\n"
		"META_STRUCT(holder) {\n"
		"    MAKE_POOL(hashmap<string, u32>, Map);\n"
		"}\n"
	);

	meta_parser Parser = { 0 };
	ASSERT_TRUE(Meta_Test_Parse(Arena, Content, NULL, &Parser));

	meta_struct_type* Holder = Meta_Test_Find_Struct(&Parser, String_Lit("holder"));
	ASSERT_TRUE(Holder != NULL);
	ASSERT_EQ(Holder->EntryCount, (size_t)1);

	meta_variable_entry* Entry = Holder->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("hashmap<string,u32>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Map")));

	Arena_Delete(Arena);
}

function b32 Source_Test_Parse(arena* Arena, string Content, source_parser* Parser) {
	Source_Parser_Init_Globals(Arena);
	G_ErrorStream = SStream_Writer_Begin((allocator*)Arena);
	Memory_Clear(Parser, sizeof(source_parser));

	b32 Result = Source_Parser_Parse(Parser, String_Lit("test.h"), Content, Arena);
	if (!Result) {
		string ErrorText = SStream_Writer_Join(&G_ErrorStream, (allocator*)Arena, String_Lit("\n"));
		printf("%.*s\n", (int)ErrorText.Size, ErrorText.Ptr);
		return false;
	}
	return true;
}

UTEST(Source_Parser, TemplateFieldTypes) {
	arena* Arena = Arena_Create(String_Lit("Source Test"));

	string Content = String_Lit(
		"Meta()\n"
		"struct template_fields {\n"
		"    array<material> Materials;\n"
		"    hashmap<string, u32> Map;\n"
		"    entity_pool<hashmap<string, u32>> NestedPool;\n"
		"};\n"
	);

	source_parser Parser = { 0 };
	ASSERT_TRUE(Source_Test_Parse(Arena, Content, &Parser));

	string StructName = String_Lit("template_fields");
	source_struct_type* Struct = NULL;
	ASSERT_TRUE(Hashmap_Find(&Parser.StructMap, &StructName, &Struct));
	ASSERT_TRUE(Struct != NULL);
	ASSERT_TRUE(Struct->Struct != NULL);
	ASSERT_EQ(Struct->Struct->EntryCount, (size_t)3);

	meta_variable_entry* Entry = Struct->Struct->FirstEntry;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("array<material>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Materials")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("hashmap<string,u32>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("Map")));

	Entry = Entry->Next;
	ASSERT_TRUE(Entry != NULL);
	ASSERT_TRUE(String_Equals(Entry->Type, String_Lit("entity_pool<hashmap<string,u32>>")));
	ASSERT_TRUE(String_Equals(Entry->Name, String_Lit("NestedPool")));
	ASSERT_TRUE(Entry->Next == NULL);

	Arena_Delete(Arena);
}

UTEST(Source_Parser, StructAndEnumTags) {
	arena* Arena = Arena_Create(String_Lit("Source Test"));

	string Content = String_Lit(
		"Meta()\n"
		"Tags(count: 5, tagName)\n"
		"struct tagged_struct {\n"
		"    s32 Value;\n"
		"};\n"
		"Meta()\n"
		"Tags(serialize, kind: mesh)\n"
		"enum tagged_enum {\n"
		"    TAGGED_ENUM_A,\n"
		"    TAGGED_ENUM_B,\n"
		"};\n"
	);

	source_parser Parser = { 0 };
	ASSERT_TRUE(Source_Test_Parse(Arena, Content, &Parser));

	string StructName = String_Lit("tagged_struct");
	source_struct_type* Struct = NULL;
	ASSERT_TRUE(Hashmap_Find(&Parser.StructMap, &StructName, &Struct));
	ASSERT_TRUE(Struct != NULL);
	ASSERT_TRUE(Struct->Struct != NULL);
	ASSERT_EQ(Struct->Struct->Tags.Count, (size_t)2);

	meta_tag* Tag = Struct->Struct->Tags.First;
	ASSERT_TRUE(Tag != NULL);
	ASSERT_TRUE(String_Equals(Tag->Name, String_Lit("count")));
	ASSERT_EQ(Tag->Values.Count, (size_t)1);
	ASSERT_TRUE(String_Equals(Tag->Values.First->Value, String_Lit("5")));

	Tag = Tag->Next;
	ASSERT_TRUE(Tag != NULL);
	ASSERT_TRUE(String_Equals(Tag->Name, String_Lit("tagName")));
	ASSERT_EQ(Tag->Values.Count, (size_t)0);
	ASSERT_TRUE(Tag->Next == NULL);

	string EnumName = String_Lit("tagged_enum");
	source_enum_type* Enum = NULL;
	ASSERT_TRUE(Hashmap_Find(&Parser.EnumMap, &EnumName, &Enum));
	ASSERT_TRUE(Enum != NULL);
	ASSERT_TRUE(Enum->Enum != NULL);
	ASSERT_EQ(Enum->Enum->Tags.Count, (size_t)2);

	Tag = Enum->Enum->Tags.First;
	ASSERT_TRUE(Tag != NULL);
	ASSERT_TRUE(String_Equals(Tag->Name, String_Lit("serialize")));
	ASSERT_EQ(Tag->Values.Count, (size_t)0);

	Tag = Tag->Next;
	ASSERT_TRUE(Tag != NULL);
	ASSERT_TRUE(String_Equals(Tag->Name, String_Lit("kind")));
	ASSERT_EQ(Tag->Values.Count, (size_t)1);
	ASSERT_TRUE(String_Equals(Tag->Values.First->Value, String_Lit("mesh")));
	ASSERT_TRUE(Tag->Next == NULL);

	Arena_Delete(Arena);
}
