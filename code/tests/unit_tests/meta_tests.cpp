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
		"        META_ENUM_ENTRY(${META_ENTRY_NAME});\n"
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
		"        META_ENUM_ENTRY(${META_ENTRY_NAME});\n"
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
