function b32 Meta_Generator_Whitespace_Has_Newline(string Whitespace) {
	for (size_t i = 0; i < Whitespace.Size; i++) {
		if (Whitespace.Ptr[i] == '\n') return true;
	}
	return false;
}

function void Meta_Generator_Emit_Tabs(sstream_writer* Writer, int TabCount) {
	for (int i = 0; i < TabCount; i++) {
		SStream_Writer_Add(Writer, String_Lit("\t"));
	}
}

// Preserve same-line spacing as written. For newline breaks, keep blank lines
// from the source whitespace but rebase indent to the current scope depth.
function void Meta_Generator_Emit_Leading_Whitespace(sstream_writer* Writer, string Whitespace, int TabCount) {
	if (String_Is_Empty(Whitespace)) return;

	if (!Meta_Generator_Whitespace_Has_Newline(Whitespace)) {
		SStream_Writer_Add(Writer, Whitespace);
		return;
	}

	for (size_t i = 0; i < Whitespace.Size; i++) {
		if (Whitespace.Ptr[i] == '\n') {
			SStream_Writer_Add(Writer, String_Lit("\n"));
		}
	}
	Meta_Generator_Emit_Tabs(Writer, TabCount);
}

function void Meta_Generator_Emit_Newline_At_Scope(sstream_writer* Writer, int TabCount) {
	SStream_Writer_Add(Writer, String_Lit("\n"));
	Meta_Generator_Emit_Tabs(Writer, TabCount);
}

function void Meta_Generator_Emit_Function_Signature(sstream_writer* Writer, meta_function_type* Function) {
	Assert(!(Function->IsExport && Function->IsInline));

	string Linkage;
	if (Function->IsExport) {
		Linkage = String_Lit("export_function");
	} else if (Function->IsInline) {
		Linkage = String_Lit("function inline");
	} else {
		Linkage = String_Lit("function");
	}

	SStream_Writer_Add(Writer, Linkage);
	SStream_Writer_Add_Format(Writer, " %.*s %.*s(", Function->ReturnType.Size, Function->ReturnType.Ptr, Function->Name.Size, Function->Name.Ptr);
	for (meta_parameter* Parameter = Function->FirstParameter; Parameter; Parameter = Parameter->Next) {
		if (Parameter == Function->LastParameter) {
			SStream_Writer_Add_Format(Writer, "%.*s %.*s", Parameter->Type.Size, Parameter->Type.Ptr,
									  Parameter->Name.Size, Parameter->Name.Ptr);
		} else {
			SStream_Writer_Add_Format(Writer, "%.*s %.*s, ", Parameter->Type.Size, Parameter->Type.Ptr,
									  Parameter->Name.Size, Parameter->Name.Ptr);
		}
	}
	SStream_Writer_Add(Writer, String_Lit(")"));
}

function void Meta_Generator_Emit_Function_Body(sstream_writer* Writer, meta_function_type* Function) {
	SStream_Writer_Add(Writer, String_Lit(" {"));

	int TabCount = 1;
	meta_token_list CodeTokenList = Function->CodeTokens;
	size_t CodeTokenIndex = 0;
	for (meta_token* CodeToken = CodeTokenList.First; CodeToken && CodeTokenIndex < CodeTokenList.Count; CodeToken = CodeToken->Next, CodeTokenIndex++) {
		if (CodeToken->Type == '\n') {
			Meta_Generator_Emit_Newline_At_Scope(Writer, TabCount);
			continue;
		}

		if (CodeToken->Type == '}') {
			Assert(TabCount > 0);
			TabCount--;
		}

		if (CodeToken->Type == META_TOKEN_TYPE_PREPROCESSOR) {
			Meta_Generator_Emit_Leading_Whitespace(Writer, CodeToken->LeadingWhitespace, TabCount);
			SStream_Writer_Add_Format(Writer, "%.*s", CodeToken->Identifier.Size, CodeToken->Identifier.Ptr);

			//todo: Fix this. Its some ugly hacky shit. I don't like the
			//preprocessor token type either. So probably remove that
			if (!String_Equals(CodeToken->Identifier, String_Lit("#endif"))) {
				CodeToken = CodeToken->Next;
				CodeTokenIndex++;
				SStream_Writer_Add_Format(Writer, " %.*s", CodeToken->Identifier.Size, CodeToken->Identifier.Ptr);
			}
		} else {
			Meta_Generator_Emit_Leading_Whitespace(Writer, CodeToken->LeadingWhitespace, TabCount);
			SStream_Writer_Add_Format(Writer, "%.*s", CodeToken->Identifier.Size, CodeToken->Identifier.Ptr);
		}

		if (CodeToken->Type == '{') {
			TabCount++;
		}
	}

	SStream_Writer_Add(Writer, String_Lit("\n}\n"));
}

function meta_file_source Meta_Generate_Source(arena* SourceArena, meta_parser* Parser) {
	arena* Scratch = Scratch_Get();

	sstream_writer HeaderWriter = SStream_Writer_Begin((allocator*)Scratch);
	for (size_t i = 0; i < Parser->GlobalMap.ItemCount; i++) {
		meta_struct_type* Struct;
		Hashmap_Get_Value(&Parser->GlobalMap, i, &Struct);
		for (meta_variable_entry* Variable = Struct->FirstEntry; Variable; Variable = Variable->Next) {
			SStream_Writer_Add_Format(&HeaderWriter, "global %.*s %.*s;\n", Variable->Type.Size, Variable->Type.Ptr,
									  Variable->Name.Size, Variable->Name.Ptr);
		}
	}

	for (size_t i = 0; i < Parser->StructMap.ItemCount; i++) {
		meta_struct_type* Struct;
		Hashmap_Get_Value(&Parser->StructMap, i, &Struct);

		SStream_Writer_Add_Format(&HeaderWriter, "typedef struct %.*s %.*s;\n", Struct->Name.Size, Struct->Name.Ptr, Struct->Name.Size, Struct->Name.Ptr);
		SStream_Writer_Add_Format(&HeaderWriter, "struct %.*s {\n", Struct->Name.Size, Struct->Name.Ptr);
		for (meta_variable_entry* Variable = Struct->FirstEntry; Variable; Variable = Variable->Next) {
			SStream_Writer_Add_Format(&HeaderWriter, "\t%.*s %.*s;\n", Variable->Type.Size, Variable->Type.Ptr,
									  Variable->Name.Size, Variable->Name.Ptr);
		}
		SStream_Writer_Add(&HeaderWriter, String_Lit("};\n"));
	}

	for (size_t i = 0; i < Parser->EnumMap.ItemCount; i++) {
		meta_enum_type* Enum;
		Hashmap_Get_Value(&Parser->EnumMap, i, &Enum);
		SStream_Writer_Add(&HeaderWriter, String_Lit("enum {\n"));
		
		for (meta_enum_entry* Entry = Enum->FirstEntry; Entry; Entry = Entry->Next) {
			string Name = Meta_Get_Enum_Value_Name(Scratch, Enum, Entry);

			SStream_Writer_Add_Format(&HeaderWriter, "\t%.*s", Name.Size, Name.Ptr);
			if (!String_Is_Empty(Entry->OptionalValue)) {
				SStream_Writer_Add_Format(&HeaderWriter, " = %.*s", Entry->OptionalValue.Size, Entry->OptionalValue.Ptr);
			}
			if (Entry != Enum->LastEntry) {
				SStream_Writer_Add(&HeaderWriter, String_Lit(","));
			}
			SStream_Writer_Add(&HeaderWriter, String_Lit("\n"));
		}

		SStream_Writer_Add(&HeaderWriter, String_Lit("};\n"));
		SStream_Writer_Add_Format(&HeaderWriter, "typedef %.*s %.*s;\n", Enum->Type.Size, Enum->Type.Ptr, Enum->Name.Size, Enum->Name.Ptr);
	}

	for (size_t i = 0; i < Parser->FunctionMap.ItemCount; i++) {
		meta_function_type* Function;
		Hashmap_Get_Value(&Parser->FunctionMap, i, &Function);

		Meta_Generator_Emit_Function_Signature(&HeaderWriter, Function);
		if (Function->IsInline) {
			Meta_Generator_Emit_Function_Body(&HeaderWriter, Function);
		} else {
			SStream_Writer_Add(&HeaderWriter, String_Lit(";\n"));
		}
	}

	sstream_writer SourceWriter = SStream_Writer_Begin((allocator*)Scratch);
	for (size_t i = 0; i < Parser->FunctionMap.ItemCount; i++) {
		meta_function_type* Function;
		Hashmap_Get_Value(&Parser->FunctionMap, i, &Function);

		if (Function->IsInline) {
			continue;
		}

		Meta_Generator_Emit_Function_Signature(&SourceWriter, Function);
		Meta_Generator_Emit_Function_Body(&SourceWriter, Function);
	}

	string Header = SStream_Writer_Join(&HeaderWriter, (allocator*)SourceArena, String_Empty());
	string Source = SStream_Writer_Join(&SourceWriter, (allocator*)SourceArena, String_Empty());

	Scratch_Release();

	meta_file_source Result = { 0 };
	Result.Header = Header;
	Result.Source = Source;
	return Result;
}
