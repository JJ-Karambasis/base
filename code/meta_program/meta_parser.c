global hashmap G_MetaKeywordMap;
global hashmap G_MetaReplacementAttributeMap;
global hashmap G_MetaForLoopReplacementsMetadataMap;
global hashmap G_MetaForLoopFlagsMetadataMap;
global hashmap G_MetaPredicatesMap;
global hashmap G_MetaRoutinesMap;

function void Meta_Parser_Init_Globals(arena* Arena) {
	G_MetaKeywordMap = Hashmap_Init((allocator*)Arena, sizeof(meta_token_type), sizeof(string), Hash_String, Compare_String);
	G_MetaReplacementAttributeMap = Hashmap_Init((allocator*)Arena, sizeof(meta_token_flag), sizeof(string), Hash_String, Compare_String);
	G_MetaForLoopReplacementsMetadataMap = Hashmap_Init((allocator*)Arena, sizeof(meta_for_loop_replacement_metadata*), sizeof(string), Hash_String, Compare_String);
	G_MetaForLoopFlagsMetadataMap = Hashmap_Init((allocator*)Arena, sizeof(meta_for_loop_flags_metadata*), sizeof(string), Hash_String, Compare_String);
	G_MetaRoutinesMap = Hashmap_Init((allocator*)Arena, sizeof(meta_routine), sizeof(string), Hash_String, Compare_String);
	G_MetaPredicatesMap = Hashmap_Init((allocator*)Arena, sizeof(meta_predicate), sizeof(string), Hash_String, Compare_String);

	//Keywords
	string Keywords[] = {
		String_Lit("META_MACRO"),
		String_Lit("META_STRUCT"),
		String_Lit("META_VARIABLE"),
		String_Lit("META_FUNCTION"),
		String_Lit("->"),
		String_Lit("META_FOR"),
		String_Lit("META_ENTRY_NAME"),
		String_Lit("META_ENTRY_TYPE"),
		String_Lit("META_GLOBALS"),
		String_Lit("META_IF"),
		String_Lit("META_ENUM"),
		String_Lit("META_ENUM_ENTRY"),
		String_Lit("META_ELSE")
	};
	meta_token_type KeywordsType[] = {
		META_TOKEN_TYPE_MACRO,
		META_TOKEN_TYPE_STRUCT,
		META_TOKEN_TYPE_VARIABLE_ENTRY,
		META_TOKEN_TYPE_FUNCTION,
		META_TOKEN_TYPE_RETURN_ARROW,
		META_TOKEN_TYPE_FOR,
		META_TOKEN_TYPE_VARIABLE_ENTRY_NAME,
		META_TOKEN_TYPE_VARIABLE_ENTRY_TYPE,
		META_TOKEN_TYPE_GLOBAL,
		META_TOKEN_TYPE_IF,
		META_TOKEN_TYPE_ENUM,
		META_TOKEN_TYPE_ENUM_ENTRY,
		META_TOKEN_TYPE_ELSE
	};
	Static_Assert(Array_Count(Keywords) == Array_Count(KeywordsType));
	for (size_t i = 0; i < Array_Count(Keywords); i++) {
		Hashmap_Add(&G_MetaKeywordMap, &Keywords[i], &KeywordsType[i]);
	}

	//Replacement attributes
	string ReplacementAttributes[] = {
		String_Lit("pascal"),
		String_Lit("upper"),
		String_Lit("lower")
	};
	meta_token_flag ReplacementFlags[] = {
		META_TOKEN_FLAG_PASCAL,
		META_TOKEN_FLAG_UPPER,
		META_TOKEN_FLAG_LOWER
	};
	Static_Assert(Array_Count(ReplacementAttributes) == Array_Count(ReplacementFlags));
	for (size_t i = 0; i < Array_Count(ReplacementAttributes); i++) {
		Hashmap_Add(&G_MetaReplacementAttributeMap, &ReplacementAttributes[i], &ReplacementFlags[i]);
	}

	//For loop flags
	string ForLoopFlagsStr[] = {
		String_Lit("NoBraces")
	};
	meta_for_loop_flags_metadata ForLoopFlagsMetadata[] = {
		{ META_FOR_LOOP_NO_BRACE_FLAG, META_FOR_LOOP_SUPPORT_ALL }
	};
	Static_Assert(Array_Count(ForLoopFlagsMetadata) == Array_Count(ForLoopFlagsStr));
	for (size_t i = 0; i < Array_Count(ForLoopFlagsMetadata); i++) {
		meta_for_loop_flags_metadata* MetadataPtr = Arena_Push_Struct(Arena, meta_for_loop_flags_metadata);
		*MetadataPtr = ForLoopFlagsMetadata[i];

		Hashmap_Add(&G_MetaForLoopFlagsMetadataMap, &ForLoopFlagsStr[i], &MetadataPtr);
	}

	//For loop Replacement 
	string ForLoopReplacementsStr[] = {
		String_Lit("META_ENTRY_NAME"),
		String_Lit("META_ENTRY_TYPE"),
		String_Lit("META_ENTRY_SHORT_NAME"),
		String_Lit("META_ENTRY_SHORT_NAME_SUBSTR_CHAR"),
		String_Lit("META_GET_TAG_VALUE"),
		String_Lit("META_ENTRY_VARIABLE_TYPE")
	};
	meta_for_loop_replacement_metadata ForLoopReplacementsMetadata[] = {
		{ META_FOR_LOOP_REPLACEMENT_ENTRY_NAME, META_FOR_LOOP_SUPPORT_ALL },
		{ META_FOR_LOOP_REPLACEMENT_ENTRY_TYPE, META_FOR_LOOP_SUPPORT_STRUCT },
		{ META_FOR_LOOP_REPLACEMENT_SHORT_ENTRY_NAME, META_FOR_LOOP_SUPPORT_ENUM },
		{ META_FOR_LOOP_REPLACEMENT_SHORT_NAME_SUBSTR_CHAR, META_FOR_LOOP_SUPPORT_ENUM },
		{ META_FOR_LOOP_REPLACEMENT_GET_TAG_VALUE, META_FOR_LOOP_SUPPORT_ALL },
		{ META_FOR_LOOP_REPLACEMENT_VARIABLE_TYPE, META_FOR_LOOP_SUPPORT_STRUCT }
	};
	Static_Assert(Array_Count(ForLoopReplacementsMetadata) == Array_Count(ForLoopReplacementsStr));
	for (size_t i = 0; i < Array_Count(ForLoopReplacementsMetadata); i++) {
		meta_for_loop_replacement_metadata* MetadataPtr = Arena_Push_Struct(Arena, meta_for_loop_replacement_metadata);
		*MetadataPtr = ForLoopReplacementsMetadata[i];
		Hashmap_Add(&G_MetaForLoopReplacementsMetadataMap, &ForLoopReplacementsStr[i], &MetadataPtr);
	}

	//For loop routines
	string RoutinesStr[] = {
		String_Lit("META_GET_FUNCTIONS_WITH_TAG")
	};
	meta_routine Routines[] = {
		META_GET_FUNCTIONS_WITH_TAG
	};
	Static_Assert(Array_Count(Routines) == Array_Count(RoutinesStr));
	Static_Assert(Array_Count(Routines) == META_ROUTINE_COUNT);
	for (size_t i = 0; i < Array_Count(Routines); i++) {
		Hashmap_Add(&G_MetaRoutinesMap, &RoutinesStr[i], &Routines[i]);
	}

	//If statement predicates
	string PredicatesStr[] = {
		String_Lit("META_CONTAINS_TAG"),
		String_Lit("META_NOT_CONTAINS_TAG"),
		String_Lit("META_IS_STRUCT"),
		String_Lit("META_IS_UNION"),
		String_Lit("META_IS_ENUM"),
		String_Lit("META_IS_ARRAY")
	};
	meta_predicate Predicates[] = {
		META_CONTAINS_TAG_PREDICATE,
		META_NOT_CONTAINS_TAG_PREDICATE,
		META_IS_STRUCT_PREDICATE,
		META_IS_UNION_PREDICATE,
		META_IS_ENUM_PREDICATE,
		META_IS_ARRAY_PREDICATE
	};
	Static_Assert(Array_Count(Predicates) == Array_Count(PredicatesStr));
	Static_Assert(Array_Count(Predicates) == META_PREDICATE_COUNT);
	for (size_t i = 0; i < Array_Count(Predicates); i++) {
		Hashmap_Add(&G_MetaPredicatesMap, &PredicatesStr[i], &Predicates[i]);
	}
}

function string Meta_Get_Enum_Value_Name(arena* Arena, meta_enum_type* Enum, meta_enum_entry* Entry) {
	if (Enum->IsTrueName) return Entry->Name;

	arena* Scratch = Scratch_Get();
	string EnumName = String_Upper_Case((allocator*)Scratch, Enum->Name);
	string Name = String_Upper_Case((allocator*)Scratch, Entry->Name);
	string Result = String_Format((allocator*)Arena, "%.*s_%.*s", EnumName.Size, EnumName.Ptr, Name.Size, Name.Ptr);
	Scratch_Release();
	return Result;
}

function inline b32 Meta_Parser_Is_Routine(string RoutineName, meta_routine* OutRoutine) {
	return Hashmap_Find(&G_MetaRoutinesMap, &RoutineName, OutRoutine);
}

function inline b32 Meta_Parser_Is_Struct(meta_parser* Parser, string StructName, meta_struct_type** OutStruct) {
	return Hashmap_Find(&Parser->StructMap, &StructName, OutStruct) || Hashmap_Find(&Parser->StructInfoMap, &StructName, OutStruct);
}

function inline b32 Meta_Parser_Is_Union(meta_parser* Parser, string UnionName, meta_struct_type** OutUnion) {
	return Hashmap_Find(&Parser->UnionInfoMap, &UnionName, OutUnion);
}

function inline b32 Meta_Parser_Is_Global(meta_parser* Parser, string StructName, meta_struct_type** OutStruct) {
	return Hashmap_Find(&Parser->GlobalMap, &StructName, OutStruct);
}

function inline b32 Meta_Parser_Is_Enum(meta_parser* Parser, string EnumName, meta_enum_type** OutEnum) {
	return Hashmap_Find(&Parser->EnumMap, &EnumName, OutEnum) || Hashmap_Find(&Parser->EnumInfoMap, &EnumName, OutEnum);
}

function string Meta_Get_Enum_Value_Short_Name(arena* Arena, meta_enum_type* Enum, meta_enum_entry* Entry) {
	string Name = Enum->Name;
	if (Enum->IsTrueName) {
		size_t StartIndex = Enum->Name.Size;
		if (Entry->Name.Ptr[StartIndex] == '_') {
			StartIndex++;
		}
		Name = String_Substr(Entry->Name, StartIndex, Entry->Name.Size);
	}
	string Result = String_Pascal_Case((allocator*)Arena, Name);
	return Result;
}

function b32 Meta_Contains_Tag(meta_tag_list* Tags, string TagString) {
	for (meta_tag* Tag = Tags->First; Tag; Tag = Tag->Next) {
		if (String_Equals(TagString, Tag->Name)) {
			return true;
		}
	}
	return false;
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Contains_Tag_Predicate) {
	for (string_entry* Parameter = Parameters.First; Parameter; Parameter = Parameter->Next) {
		if (Meta_Contains_Tag(&Entry->Tags, Parameter->String)) {
			return true;
		}
	}
	return false;
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Not_Contains_Tag_Predicate) {
	return !Meta_Parser_Contains_Tag_Predicate(Parser, Entry, Parameters);
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Contains_Is_Struct_Predicate) {
	meta_struct_type* StructType;
	return Meta_Parser_Is_Struct(Parser, Parameters.First->String, &StructType);
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Contains_Is_Union_Predicate) {
	meta_struct_type* StructType;
	return Meta_Parser_Is_Union(Parser, Parameters.First->String, &StructType);
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Contains_Is_Enum_Predicate) {
	meta_enum_type* EnumType;
	return Meta_Parser_Is_Enum(Parser, Parameters.First->String, &EnumType);
}

function META_PARSER_PREDICATE_DEFINE(Meta_Parser_Contains_Is_Array_Predicate) {
	return Entry->IsArray;
}

function meta_parser_predicate_func* Meta_Parser_Get_Predicate(string PredicateName) {
	meta_predicate Predicate = (meta_predicate)-1;
	if (!Hashmap_Find(&G_MetaPredicatesMap, &PredicateName, &Predicate)) {
		return NULL;
	}

	local meta_parser_predicate_func* MetaPredicates[] = {
		Meta_Parser_Contains_Tag_Predicate,
		Meta_Parser_Not_Contains_Tag_Predicate,
		Meta_Parser_Contains_Is_Struct_Predicate,
		Meta_Parser_Contains_Is_Union_Predicate,
		Meta_Parser_Contains_Is_Enum_Predicate,
		Meta_Parser_Contains_Is_Array_Predicate
	};
	Static_Assert(Array_Count(MetaPredicates) == META_PREDICATE_COUNT);
	Assert(Predicate < META_PREDICATE_COUNT);
	return MetaPredicates[Predicate];
}

function meta_token* Meta_Allocate_Token(meta_tokenizer* Tokenizer, u32 Type) {
	meta_token* Token = Tokenizer->FreeTokens;
	if (Token) SLL_Pop_Front(Tokenizer->FreeTokens);
	else Token = Arena_Push_Struct_No_Clear(Tokenizer->Arena, meta_token);
	Memory_Clear(Token, sizeof(meta_token));
	
	Token->Type = (meta_token_type)Type;
	if (Type < 256) {
		Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, Make_String((const char*)&Type, 1));
	}
	return Token;
}

function void Meta_Add_Token(meta_tokenizer* Tokenizer, meta_token_type Type, size_t c0, size_t c1, size_t LineNumber) {
	meta_token* Token = Meta_Allocate_Token(Tokenizer, Type);
	Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, String_Substr(Tokenizer->Content, c0, c1));
	Token->c0 = c0;
	Token->LineNumber = LineNumber;
	DLL_Push_Back(Tokenizer->Tokens.First, Tokenizer->Tokens.Last, Token);
	Tokenizer->Tokens.Count++;
}

function inline void Meta_Add_Token_Char(meta_tokenizer* Tokenizer, sstream_char Char) {
	Meta_Add_Token(Tokenizer, (meta_token_type)Char.Char, Char.Index, Char.Index + 1, Char.LineIndex);
}

function void Meta_Tokenizer_Begin_Identifier(meta_tokenizer* Tokenizer) {
	sstream_char Char = SStream_Reader_Consume_Char(&Tokenizer->Stream);
	if (!Tokenizer->BeginIdentifier) {
		Tokenizer->StartIdentifierChar = Char;
		Tokenizer->BeginIdentifier = true;
	}
}

function void Meta_Tokenizer_End_Identifier(meta_tokenizer* Tokenizer) {
	if (Tokenizer->BeginIdentifier) {
		size_t Index = SStream_Reader_Get_Index(&Tokenizer->Stream);
		Meta_Add_Token(Tokenizer, META_TOKEN_TYPE_IDENTIFIER, Tokenizer->StartIdentifierChar.Index, Index, Tokenizer->StartIdentifierChar.LineIndex);
		Tokenizer->BeginIdentifier = false;
	}
}

function void Meta_Tokens_Replace(meta_token_list* ReplaceList, meta_token_list* InsertLink) {
	InsertLink->First->Prev = ReplaceList->First->Prev;
	InsertLink->Last->Next = ReplaceList->Last->Next;

	if (ReplaceList->First->Prev) {
		ReplaceList->First->Prev->Next = InsertLink->First;
	}

	if (ReplaceList->Last->Next) {
		ReplaceList->Last->Next->Prev = InsertLink->Last;
	}

	ReplaceList->First->Prev = NULL;
	ReplaceList->First->Next = NULL;
}

function void Meta_Tokens_Free(meta_tokenizer* Tokenizer, meta_token_list* FreeList) {
	meta_token* TokenToFree = FreeList->First;
	
	while (TokenToFree) {
		meta_token* TokenToDelete = TokenToFree;
		TokenToFree = TokenToFree->Next;

		TokenToDelete->Next = NULL;
		TokenToDelete->Prev = NULL;
		SLL_Push_Front(Tokenizer->FreeTokens, TokenToDelete);
		if (FreeList->Last->Next == TokenToFree) {
			break;
		}
	}
}

function inline b32 Meta_Check_Token(meta_token* Token, u32 Type) {
	return Token && Token->Type == (meta_token_type)Type;
}

function meta_token* Meta_Add_Replacement_Token(meta_tokenizer* Tokenizer, meta_token* Token) {
	Assert(Token->Type == '$' && Token->Next->Type == '{');
	meta_token* ReplacementToken = Meta_Allocate_Token(Tokenizer, META_TOKEN_TYPE_REPLACEMENT);
	ReplacementToken->LineNumber = Token->LineNumber;
	
	//Check if the character before the replacement token is apart of the same 
	//string 

	size_t StartIndex = Token->c0;
	Assert(Token->c0 && Token->Prev);
	if (!Is_Whitespace(Tokenizer->Content.Ptr[StartIndex-1]) && Token->Prev->Type == META_TOKEN_TYPE_IDENTIFIER) {
		ReplacementToken->Flags |= META_TOKEN_FLAG_HAS_PREV_IDENTIFIER;
	}
	
	meta_token* StartReplacementToken = Token;
	meta_token* EndReplacementToken = NULL;
	meta_token* TokenIter = StartReplacementToken->Next->Next;

	b32 StartedIdentifier = false;
	b32 GotAttributes = false;
	b32 GotParameters = false;
	b32 GotStackUpwardOffset = false;
	while (!EndReplacementToken && TokenIter) {
		switch (TokenIter->Type) {
			case META_TOKEN_TYPE_IDENTIFIER: {
				ReplacementToken->Identifier = String_Copy((allocator*)Tokenizer->Arena, TokenIter->Identifier);
				StartedIdentifier = true;
			} break;

			case '(': {
				if (!StartedIdentifier) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}

				if (GotAttributes || GotParameters || GotStackUpwardOffset) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}
				string_list Parameters = {0};

				TokenIter = TokenIter->Next;
				while (TokenIter && TokenIter->Type != ')') {
					if (!Meta_Check_Token(TokenIter, META_TOKEN_TYPE_IDENTIFIER)) {
						Report_Error(Tokenizer->FilePath, Token->LineNumber, "Expected identifier after '%.*s'", Token->Prev->Identifier.Size, Token->Prev->Identifier.Ptr);
						return NULL;
					}
					
					string_entry* Entry = Arena_Push_Struct(Tokenizer->Arena, string_entry);
					Entry->String = String_Copy((allocator*)Tokenizer->Arena, TokenIter->Identifier);
					SLL_Push_Back(Parameters.First, Parameters.Last, Entry);
					Parameters.Count++;

					TokenIter = TokenIter->Next;
					if (Meta_Check_Token(TokenIter, ')')) {
						continue;
					}

					if (!Meta_Check_Token(TokenIter, ',')) {
						Report_Error(Tokenizer->FilePath, Token->LineNumber, "Expected ',' after '%.*s'", Token->Prev->Identifier.Size, Token->Prev->Identifier.Ptr);
						return NULL;
					}
				}

				if (!Meta_Check_Token(TokenIter, ')')) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Expected ')' after '%.*s'", Token->Prev->Identifier.Size, Token->Prev->Identifier.Ptr);
					return NULL;
				}

				ReplacementToken->Parameters = Parameters;
				GotParameters = true;
			} break;

			case ':': {
				if (!StartedIdentifier || GotAttributes) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}

				if (!TokenIter->Next) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token expected an attribute");
					return NULL;
				}

				TokenIter = TokenIter->Next;
				if (TokenIter->Type != META_TOKEN_TYPE_IDENTIFIER) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token expected an attribute identifier");
					return NULL;
				}

				string AttributeTokenStr = TokenIter->Identifier;

				meta_token_flag AttributeFlag;
				if (Hashmap_Find(&G_MetaReplacementAttributeMap, &AttributeTokenStr, &AttributeFlag)) {
					ReplacementToken->Flags |= AttributeFlag;
					GotAttributes = true;
				} else {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token has an invalid replacement attribute '%.*s'", AttributeTokenStr.Size, AttributeTokenStr.Ptr);
					return NULL;
				}
			} break;

			case '-': {
				if (!StartedIdentifier || GotStackUpwardOffset || GotAttributes) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}

				if (!TokenIter->Next) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token expected a upward stack offset value");
					return NULL;
				}

				TokenIter = TokenIter->Next;
				if (TokenIter->Type != META_TOKEN_TYPE_IDENTIFIER) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token expected a upward stack offset value identifier");
					return NULL;
				}

				s64 StackUpwardOffset;
				if (!Try_Parse_Integer(TokenIter->Identifier, &StackUpwardOffset)) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Replacement token expected an integer value for the upward stack offset");
					return NULL;
				}

				ReplacementToken->StackUpwardOffset = StackUpwardOffset;
				GotStackUpwardOffset = true;
			} break;

			case '}': {
				if (!StartedIdentifier) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}

				EndReplacementToken = TokenIter;
			} break;

			default: {
				if (!StartedIdentifier) {
					Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
					return NULL;
				}
			} break;
		}

		TokenIter = TokenIter->Next;
	}

	if (!EndReplacementToken) {
		Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid replacement token");
		return NULL;
	}
	size_t EndIndex = EndReplacementToken->c0 + EndReplacementToken->Identifier.Size;

	//Check if the character after the replacement token is apart of the same 
	//string 
	if (EndReplacementToken->Next && EndReplacementToken->Next->Type == META_TOKEN_TYPE_IDENTIFIER &&
		!Is_Whitespace(Tokenizer->Content.Ptr[EndIndex])) {
		ReplacementToken->Flags |= META_TOKEN_FLAG_HAS_NEXT_IDENTIFIER;
	}

	meta_token_list InsertList = { ReplacementToken, ReplacementToken };
	meta_token_list ReplaceList = { StartReplacementToken, EndReplacementToken };
	Meta_Tokens_Replace(&ReplaceList, &InsertList);
	Meta_Tokens_Free(Tokenizer, &ReplaceList);

	return ReplacementToken;
}

function b32 Meta_Parser_Generate_Tokens(meta_tokenizer* Tokenizer, arena* Arena, string Content) {
	Tokenizer->Arena = Arena;
	Tokenizer->Content = Content;
	Tokenizer->Stream = SStream_Reader_Begin(Content);

	sstream_reader* Stream = &Tokenizer->Stream;
	while (SStream_Reader_Is_Valid(Stream)) {
		sstream_char Char = SStream_Reader_Peek_Char(Stream);
		switch (Char.Char) {
			case '(':
			case ')':
			case '{':
			case '}':
			case ',':
			case ':':
			case ';':
			case '$':
			case '=':
			case '-':
			case '>':
			{
				Meta_Tokenizer_End_Identifier(Tokenizer);
				Meta_Add_Token_Char(Tokenizer, SStream_Reader_Consume_Char(Stream));
			} break;

			case '/': {
				Meta_Tokenizer_End_Identifier(Tokenizer);
				SStream_Reader_Consume_Char(Stream);

				b32 IsComment = false;
				if (SStream_Reader_Is_Valid(Stream)) {
					sstream_char NextChar = SStream_Reader_Peek_Char(Stream);
					if (NextChar.Char == '/' && NextChar.LineIndex == Char.LineIndex) {
						//Single line comment
						IsComment = true;
						SStream_Reader_Skip_Line(Stream);
					}

					if (NextChar.Char == '*') {
						//Multi line comment

						IsComment = true;
						b32 FinishedComment = false;
						while (SStream_Reader_Is_Valid(Stream) && !FinishedComment) {
							sstream_char CommentChar = SStream_Reader_Consume_Char(Stream);
							if (CommentChar.Char == '*') {
								sstream_char EndChar = SStream_Reader_Peek_Char(Stream);
								if (EndChar.Char == '/') {
									SStream_Reader_Consume_Char(Stream);
									FinishedComment = true;
								}
							}
						}

						if (!FinishedComment) {
							Report_Error(Tokenizer->FilePath, Char.LineIndex, "Unexpected end of multi line comment");
							return false;
						}
					}
				}
					
				if (!IsComment) {
					Meta_Add_Token_Char(Tokenizer, Char);
				}

			} break;

			default: {
				if (Is_Whitespace(Char.Char)) {
					Meta_Tokenizer_End_Identifier(Tokenizer);
					SStream_Reader_Eat_Whitespace(Stream);
				} else {
					Meta_Tokenizer_Begin_Identifier(Tokenizer);
				}
			} break;
		}
	}
	Meta_Tokenizer_End_Identifier(Tokenizer);

	for (meta_token* Token = Tokenizer->Tokens.First; Token; Token = Token->Next) {
		if (Token->Type == META_TOKEN_TYPE_IDENTIFIER) {
			meta_token_type Type;
			if (Hashmap_Find(&G_MetaKeywordMap, &Token->Identifier, &Type)) {
				Token->Type = Type;
			} else {
				if (Token->Identifier.Ptr[0] == '#') {
					Token->Type = META_TOKEN_TYPE_PREPROCESSOR;
				}
			}
		}

		if (Token->Type == '-') {
			if (Token->Next && Token->Next->Type == '>') {
				meta_token* ReplacementToken = Meta_Allocate_Token(Tokenizer, META_TOKEN_TYPE_RETURN_ARROW);
				ReplacementToken->Identifier = String_Lit("->");
				ReplacementToken->LineNumber = Token->LineNumber;
				ReplacementToken->c0 = Token->c0;

				meta_token_list InsertList = { ReplacementToken, ReplacementToken };
				meta_token_list ReplaceList = { Token, Token->Next };
				Meta_Tokens_Replace(&ReplaceList, &InsertList);
				Meta_Tokens_Free(Tokenizer, &ReplaceList);
				Token = ReplacementToken;
			}
		}

		if (Token->Type == '$') {
			if (Token->Next && Token->Next->Type == '{') {
				Token = Meta_Add_Replacement_Token(Tokenizer, Token);
				if (!Token) {
					return false;
				}
			}
		}
	}

	return true;
}

function meta_token_iter Meta_Begin_Simple_Token_Iter(meta_token* Token) {
	meta_token_iter Result = { 0 };
	Result.Token = Token;
	Result.PrevToken = (Token != NULL) ? Token->Prev : NULL;
	return Result;
}

function meta_token_iter Meta_Begin_Token_Iter(meta_token_list* List) {
	meta_token_iter Result = { 0 };
	Result.Token = List->First;
	Result.PrevToken = List->First->Prev ? List->First->Prev : NULL;
	Result.Count = List->Count;
	return Result;
}

function b32 Meta_Token_Iter_Move_Next(meta_token_iter* Iter) {
	if (!Iter->Token) return false;

	Iter->PrevToken = Iter->Token;
	Iter->Token = Iter->Token->Next;
	b32 Result = Iter->Index < Iter->Count;
	if (Result) {
		Iter->Index++;
	}
	return Result;
}

function b32 Meta_Token_Iter_Is_Valid(meta_token_iter* Iter) {
	if (!Iter->Token) return false;
	return Iter->Index < Iter->Count;
}


function b32 Meta_Token_Iter_Skip_To(meta_token_iter* Iter, meta_token* TargetToken) {
	while (Meta_Token_Iter_Is_Valid(Iter) && Iter->Token != TargetToken) {
		Meta_Token_Iter_Move_Next(Iter);
	}
	return Meta_Token_Iter_Is_Valid(Iter);
}

function meta_token* Meta_Parse_Code_Tokens(meta_parser* Parser, meta_token* Token, meta_token_list* CodeTokens) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token);
	
	if (!Meta_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '{' after ')'");
		return NULL;
	}

	Memory_Clear(CodeTokens, sizeof(meta_token_list));
	Meta_Token_Iter_Move_Next(&TokenIter);

	size_t BracketStackIndex = 0;
	while (TokenIter.Token && (TokenIter.Token->Type != '}' || BracketStackIndex)) {
		if (TokenIter.Token->Type == '{') {
			BracketStackIndex++;
		}

		if (TokenIter.Token->Type == '}') {
			if (BracketStackIndex == 0) continue;
			BracketStackIndex--;
		}

		if (!CodeTokens->Count) {
			CodeTokens->First = TokenIter.Token;
		}

		CodeTokens->Count++;

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, '}') || BracketStackIndex) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of for loop. Missing '}' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	if (CodeTokens->Count) {
		CodeTokens->Last = TokenIter.Token->Prev;
	}

	return TokenIter.Token;
}

function meta_token* Meta_Parse_Macro(meta_parser* Parser, meta_token* MacroToken) {
	Assert(MacroToken->Type == META_TOKEN_TYPE_MACRO);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(MacroToken->Next);

	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Macro must begin with an open parenthesis '('");
		return NULL;
	}
	
	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Macro must start with a valid macro name.");
		return NULL;
	}

	meta_token* MacroNameToken = TokenIter.Token;
	string MacroName = MacroNameToken->Identifier;
	
	meta_macro* Macro = NULL;
	if (Hashmap_Find(&Parser->MacroMap, &MacroName, &Macro)) {
		Report_Error(Parser->Tokenizer->FilePath, MacroNameToken->LineNumber, "Duplicate macro names '%.*s'", MacroName.Size, MacroName.Ptr);
		return NULL;
	}

	Assert(!Macro);
	Macro = Arena_Push_Struct(Parser->Arena, meta_macro);
	Macro->Name = String_Copy((allocator*)Parser->Arena, MacroName);
	Hashmap_Add(&Parser->MacroMap, &Macro->Name, &Macro);

	Meta_Token_Iter_Move_Next(&TokenIter);

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after macro name");
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		while (TokenIter.Token && TokenIter.Token->Type != ')') {
			if (!Meta_Check_Token(TokenIter.Token , META_TOKEN_TYPE_IDENTIFIER)) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after ','");
				return NULL;
			}

			meta_parameter* Parameter = Arena_Push_Struct(Parser->Arena, meta_parameter);
			Parameter->Name = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);

			SLL_Push_Back(Macro->FirstParameter, Macro->LastParameter, Parameter);
			Macro->ParameterCount++;

			Meta_Token_Iter_Move_Next(&TokenIter);
			if (Meta_Check_Token(TokenIter.Token, ')')) {
				continue;
			}

			if (!Meta_Check_Token(TokenIter.Token, ',')) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
				return NULL;
			}

			Meta_Token_Iter_Move_Next(&TokenIter);
		}

	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);

	meta_token* EndToken = Meta_Parse_Code_Tokens(Parser, TokenIter.Token, &Macro->CodeTokens);
	return EndToken;
}

function meta_token* Meta_Get_Macro_Parameter_Values(meta_parser* Parser, meta_macro* Macro, meta_token* Token) {
	if (!Meta_Check_Token(Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, Token->LineNumber, "Macro expansion must begin with an open parenthesis '('");
		return NULL;
	}
	
	Hashmap_Clear(&Parser->MacroParamMap);

	meta_parameter* Parameter = Macro->FirstParameter;
	size_t ParameterCount = 0;

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	while (TokenIter.Token && TokenIter.Token->Type != ')') {
		string MacroParameterName = Parameter->Name;

		//To parse the parameter values of the macro, we need to loop over all the tokens until we see a 
		//comma, or until we see a closing parenthesis that matches the first parenthesis.
		size_t ParenStack = 0;

		meta_token_list ValueTokens = { 0 };
		ValueTokens.First = TokenIter.Token;
		ValueTokens.Count++;

		Meta_Token_Iter_Move_Next(&TokenIter);

		while (TokenIter.Token) {	
			if (TokenIter.Token->Type == '(') {
				ParenStack++;
			}

			if (TokenIter.Token->Type == ')') {
				if (ParenStack == 0) {
					break;
				}
				ParenStack--;
			}

			if (TokenIter.Token->Type == ',') {
				if (ParenStack == 0) {
					break;
				}
			}

			ValueTokens.Count++;
			Meta_Token_Iter_Move_Next(&TokenIter);
		}

		if (!TokenIter.Token || ParenStack) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of macro");
			return NULL;
		}

		//Remove the last entry so we don't add the comma or parenthesis
		ValueTokens.Last = TokenIter.PrevToken;

		Hashmap_Add(&Parser->MacroParamMap, &MacroParameterName, &ValueTokens);
		ParameterCount++;

		if (Meta_Check_Token(TokenIter.Token, ')')) {
			continue;
		}

		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		Parameter = Parameter->Next;
	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of macro. Expected ')' to end the macro");
		return NULL;
	}

	if (ParameterCount != Macro->ParameterCount) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Macro parameter mismatch. Expected %d, actual %d", Macro->ParameterCount, ParameterCount);
		return NULL;
	}

	return TokenIter.Token;
}

function string Meta_Get_Replacement_Identifier(arena* Arena, meta_token_flag Flags, string Identifier) {
	if (Flags & META_TOKEN_FLAG_PASCAL) {
		Identifier = String_Pascal_Case((allocator*)Arena, Identifier);
	}

	if (Flags & META_TOKEN_FLAG_UPPER) {
		Identifier = String_Upper_Case((allocator*)Arena, Identifier);
	}

	if (Flags & META_TOKEN_FLAG_LOWER) {
		Identifier = String_Lower_Case((allocator*)Arena, Identifier);
	}

	return Identifier;
}

function string Meta_Parser_Get_Tag_Name(meta_tag_list Tags, s64 Index) {
	s64 i = 0;
	for (meta_tag* Tag = Tags.First; Tag; Tag = Tag->Next) {
		if (i == Index) return Tag->Name;
		i++;
	}
	return String_Empty();
}

function string Meta_Parser_Get_Tag_Value(meta_tag_list Tags, string Name) {
	string Result = String_Empty();
	for (meta_tag* Tag = Tags.First; Tag; Tag = Tag->Next) {
		if (String_Equals(Tag->Name, Name)) {
			return Tag->Value;
		}
	}
	return Result;
}

function string Meta_Parser_Get_Struct_Type(meta_struct_type* Struct, size_t Index) {
	string Result = String_Empty();
	if (Struct->EntryCount > Index) {
		meta_variable_entry* Entry = Struct->FirstEntry;
		for (size_t i = 0; i < Struct->EntryCount; i++) {
			if (i == Index) {
				Result = Entry->Type;
				break;
			}
			Entry = Entry->Next;
		}
	}
	return Result;
}

function b32 Meta_Get_Replacement_Value(meta_parser* Parser, meta_token* Token, meta_for_loop_replacement Replacement, meta_for_loop_entry* Entry, string* OutResult) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	string ReplacementValue = String_Empty();
	
	switch (Replacement) {
		case META_FOR_LOOP_REPLACEMENT_ENTRY_NAME: {
			if (String_Is_Empty(Entry->Name)) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Variable does not have a valid name");
				return false;
			}

			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, Entry->Name);
		} break;

		case META_FOR_LOOP_REPLACEMENT_ENTRY_TYPE: {
			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, Entry->Type);
		} break;

		case META_FOR_LOOP_REPLACEMENT_SHORT_ENTRY_NAME: {
			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, Entry->ShortName);
		} break;

		case META_FOR_LOOP_REPLACEMENT_GET_TAG_VALUE: {
			if (Token->Parameters.Count != 1) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid amount of parameters for META_GET_TAG_VALUE. Expected '1', got '%d'", Token->Parameters.Count);
				return false;
			}

			string Parameter = Token->Parameters.First->String;

			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, 
										   Meta_Parser_Get_Tag_Value(Entry->Tags, Parameter));
			if (String_Is_Empty(ReplacementValue)) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Could not find a tag with the name '%.*s'", Parameter.Size, Parameter.Ptr);
				return false;
			}
		} break;

		case META_FOR_LOOP_REPLACEMENT_SHORT_NAME_SUBSTR_CHAR: {
			if (Token->Parameters.Count != 1) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid amount of parameters for META_ENTRY_SHORT_NAME_SUBSTR_CHAR. Expected '1', got '%d'", Token->Parameters.Count);
				return false;
			}

			string Parameter = Token->Parameters.First->String;
			if (Parameter.Size != 1) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid parameter entry for META_ENTRY_SHORT_NAME_SUBSTR_CHAR. Expected char, got a string '%.*s' instead", Parameter.Size, Parameter.Size);
				return false;
			}

			string ShortName = Entry->ShortName;
			size_t CharEntry = String_Find_First_Char(ShortName, Parameter.Ptr[0]);
			if (CharEntry == STRING_INVALID_INDEX) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Could not find '_' in short name entry '%.*s'", Parameter.Ptr[0], ShortName.Size, ShortName.Ptr);
				return false;
			}

			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, 
										   String_Substr(Entry->ShortName, 0, CharEntry));
		} break;

		case META_FOR_LOOP_REPLACEMENT_VARIABLE_TYPE: {
			if (Token->Parameters.Count != 1) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid amount of parameters for META_ENTRY_VARIABLE_TYPE. Expected '1', got '%d'", Token->Parameters.Count);
				return false;
			}

			f64 Value;
			if (!Try_Parse_Number(Token->Parameters.First->String, &Value) && Value > 0) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid parameter entry for META_ENTRY_VARIABLE_TYPE. Expected number, got a string '%.*s' instead", Token->Parameters.First->String.Size, Token->Parameters.First->String.Ptr);
				return false;
			}

			meta_struct_type* Struct;
			if (!Meta_Parser_Is_Struct(Parser, Entry->Type, &Struct)) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Type '%.*s' must be a struct for META_ENTRY_VARIABLE_TYPE", Entry->Type.Size, Entry->Type.Ptr);
				return false;
			}

			string Type = Meta_Parser_Get_Struct_Type(Struct, (size_t)Value);
			if (String_Is_Empty(Type)) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Struct does not have a '%d' entry.", (size_t)Value);
				return false;
			}
			
			ReplacementValue = String_Copy((allocator*)Tokenizer->Arena, Type);
		} break;

		Invalid_Default_Case;
	}

	*OutResult = Meta_Get_Replacement_Identifier(Tokenizer->Arena, Token->Flags, ReplacementValue);

	return true;
}

function b32 Meta_Replace_Next_For_Token(meta_parser* Parser, meta_token_iter* TokenIter, meta_token_list* NewTokens, meta_for_loop_context_stack* ForLoopStack) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	if (TokenIter->Token->Type == META_TOKEN_TYPE_REPLACEMENT) {
		string ReplacementName = TokenIter->Token->Identifier;

		meta_for_loop_replacement_metadata* ReplacementTypeMeta;
		if (!Hashmap_Find(&G_MetaForLoopReplacementsMetadataMap, &ReplacementName, &ReplacementTypeMeta)) {
			Report_Error(Tokenizer->FilePath, TokenIter->Token->LineNumber, "Invalid replacement type '%.*s'", ReplacementName.Size, ReplacementName.Ptr);
			return false;
		}

		if (ForLoopStack->Count == (size_t)-1) {
			//todo: Error message sucks
			Report_Error(Tokenizer->FilePath, TokenIter->Token->LineNumber, "Invalid for loop replacement");
			return false;
		}

		size_t ContextIndex = ForLoopStack->Count - 1;

		size_t StackUpwardOffset = TokenIter->Token->StackUpwardOffset;
		if (((s64)ContextIndex - (s64)StackUpwardOffset) < 0) {
			Report_Error(Tokenizer->FilePath, TokenIter->Token->LineNumber, "Invalid replacement stack offset value. Offset is '%d' and value cannot exceed more tha '%d'", StackUpwardOffset, ContextIndex);
			return false;
		}

		meta_for_loop_context* Context = &ForLoopStack->Stack[ContextIndex - StackUpwardOffset];
		if (!(ReplacementTypeMeta->SupportFlag & Context->SupportFlag)) {
			Report_Error(Tokenizer->FilePath, TokenIter->Token->LineNumber, "Invalid %.*s replacement type flag '%.*s'", ForLoopStack->CurrentContext->TypeStr.Size, ForLoopStack->CurrentContext->TypeStr.Ptr, ReplacementName.Size, ReplacementName.Ptr);
			return false;
		}

		string ReplacementValue;
		if (!Meta_Get_Replacement_Value(Parser, TokenIter->Token, ReplacementTypeMeta->Replacement, Context->CurrentEntry, &ReplacementValue)) {
			return false;
		}

		if (TokenIter->Token->Flags & META_TOKEN_FLAG_HAS_PREV_IDENTIFIER) {
			meta_token* Last = NewTokens->Last;
			Assert(Last->Type == META_TOKEN_TYPE_IDENTIFIER);
			Last->Identifier = String_Concat((allocator*)Tokenizer->Arena, Last->Identifier, ReplacementValue);
		} else {
			meta_token* FirstNewToken = Meta_Allocate_Token(Tokenizer, META_TOKEN_TYPE_IDENTIFIER);
			FirstNewToken->LineNumber = TokenIter->Token->LineNumber;
			FirstNewToken->Identifier = ReplacementValue;
			FirstNewToken->Flags = TokenIter->Token->Flags;

			DLL_Push_Back(NewTokens->First, NewTokens->Last, FirstNewToken);
			NewTokens->Count++;
		}

		if (TokenIter->Token->Flags & META_TOKEN_FLAG_HAS_NEXT_IDENTIFIER) {
			meta_token* LastToken = NewTokens->Last;
			Assert(LastToken->Type == META_TOKEN_TYPE_IDENTIFIER);

			meta_token* NextToken = TokenIter->Token->Next;
			Assert(NextToken->Type == META_TOKEN_TYPE_IDENTIFIER);
			LastToken->Identifier = String_Concat((allocator*)Tokenizer->Arena, LastToken->Identifier, NextToken->Identifier);

			//We want to skip the next token since we've concat it with the previous token
			Meta_Token_Iter_Move_Next(TokenIter);
		}
	} else {
		meta_token* NewToken = Meta_Allocate_Token(Tokenizer, TokenIter->Token->Type);
		NewToken->LineNumber = TokenIter->Token->LineNumber;
		NewToken->Identifier = TokenIter->Token->Identifier;
		NewToken->Flags = TokenIter->Token->Flags;

		DLL_Push_Back(NewTokens->First, NewTokens->Last, NewToken);
		NewTokens->Count++;
	}

	return true;
}

function meta_token* Meta_Parse_For_Params(meta_parser* Parser, meta_token* Token, meta_for_loop_context_stack* Stack, meta_token_list* NewTokens) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	
	meta_token_list TypeTokens = { 0 };

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token);

	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after for loop declaration");
		return NULL;
	}

	TypeTokens.First = TokenIter.Token;
	TypeTokens.Count++;

	Meta_Token_Iter_Move_Next(&TokenIter);

	size_t ParamStackIndex = 0;
	while (TokenIter.Token && (TokenIter.Token->Type != ')' || ParamStackIndex)) {
		if (TokenIter.Token->Type == '(') {
			ParamStackIndex++;
		}

		if (TokenIter.Token->Type == ')') {
			ParamStackIndex--;
		}

		TypeTokens.Count++;
		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, ')') || ParamStackIndex) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of META_FOR parameters");
		return NULL;
	}
	TypeTokens.Last = TokenIter.Token;
	TypeTokens.Count++;

	meta_token_iter TypeTokenIter = Meta_Begin_Token_Iter(&TypeTokens);
	while (Meta_Token_Iter_Is_Valid(&TypeTokenIter)) {
		if (!Meta_Replace_Next_For_Token(Parser, &TypeTokenIter, NewTokens, Stack)) return NULL;
		Meta_Token_Iter_Move_Next(&TypeTokenIter);
	}

	if (!NewTokens->Count) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Invalid META_FOR parameters");
		return NULL;
	}

	return TokenIter.Token;
}

function meta_token* Meta_Parse_And_Expand_Macro(meta_parser* Parser, meta_macro* Macro, meta_token* MacroToken, meta_token_list* NewTokens, meta_for_loop_context_stack* ForLoopStack) {
	Assert(MacroToken->Type == META_TOKEN_TYPE_IDENTIFIER && String_Equals(Macro->Name, MacroToken->Identifier));

	meta_tokenizer* Tokenizer = Parser->Tokenizer;

	meta_token* Token = NULL;
	if (ForLoopStack) {
		meta_token_list NewMacroTokens = { 0 };
		Token = Meta_Parse_For_Params(Parser, MacroToken->Next, ForLoopStack, &NewMacroTokens);
		if (!Token) return NULL;
		meta_token* EndToken = Meta_Get_Macro_Parameter_Values(Parser, Macro, NewMacroTokens.First);
		if (!EndToken) return NULL;
		Token = Token->Next;
	} else {
		meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(MacroToken->Next);
		Token = Meta_Get_Macro_Parameter_Values(Parser, Macro, TokenIter.Token);
		if (!Token) { return NULL; }
		Token = Token->Next;
	}

	if (!Meta_Check_Token(Token, ';')) {
		Report_Error(Parser->Tokenizer->FilePath, Token->LineNumber, "Unexpected end of macro. Expected ';' after ')'");
		return NULL;
	}

	meta_token* EndMacroToken = Token;

	meta_token_list MacroTokenList = Macro->CodeTokens;

	//Iterative algorithm for replacing replacement tokens. Replacement token values can have replacement tokens within
	//themselves, so we need to iterate until we find no replacement tokens. Final replacement tokens are stored in
	//the NewToken list
	b32 FirstLoop = true;
	b32 FoundReplacements = false;
	do {

		FoundReplacements = false;

		meta_token_iter MacroTokenIter = Meta_Begin_Token_Iter(&MacroTokenList);
		while(Meta_Token_Iter_Is_Valid(&MacroTokenIter)) {

			meta_token_list ReplacementTokens = { 0 };
			b32 ReplaceToken = ((MacroTokenIter.Token->Type == META_TOKEN_TYPE_REPLACEMENT) &&
				Hashmap_Find(&Parser->MacroParamMap, &MacroTokenIter.Token->Identifier, &ReplacementTokens));

			if (ReplaceToken) {
				FoundReplacements = true;

				string FirstReplacementIdentifier = Meta_Get_Replacement_Identifier(Parser->Arena, MacroTokenIter.Token->Flags, ReplacementTokens.First->Identifier);
				if (MacroTokenIter.Token->Flags & META_TOKEN_FLAG_HAS_PREV_IDENTIFIER) {
					meta_token* Last = NewTokens->Last;
					Assert(Last->Type == META_TOKEN_TYPE_IDENTIFIER);
					Last->Identifier = String_Concat((allocator*)Parser->Arena, Last->Identifier, FirstReplacementIdentifier);
				} else {
					meta_token* FirstNewToken = Meta_Allocate_Token(Tokenizer, ReplacementTokens.First->Type);
					FirstNewToken->LineNumber = ReplacementTokens.First->LineNumber;
					FirstNewToken->Identifier = FirstReplacementIdentifier;
					FirstNewToken->Flags = ReplacementTokens.First->Flags;

					DLL_Push_Back(NewTokens->First, NewTokens->Last, FirstNewToken);
					NewTokens->Count++;
				}

				meta_token* SrcToken = ReplacementTokens.First;
				for (size_t j = 1; j < ReplacementTokens.Count; j++) {
					SrcToken = SrcToken->Next;

					meta_token* DstToken = Meta_Allocate_Token(Tokenizer, SrcToken->Type);
					DstToken->LineNumber = SrcToken->LineNumber;
					DstToken->Identifier = Meta_Get_Replacement_Identifier(Parser->Arena, MacroTokenIter.Token->Flags, SrcToken->Identifier);
					DstToken->Flags = SrcToken->Flags;
					DLL_Push_Back(NewTokens->First, NewTokens->Last, DstToken);
					NewTokens->Count++;
				}
				Assert(SrcToken == ReplacementTokens.Last);

				if (MacroTokenIter.Token->Flags & META_TOKEN_FLAG_HAS_NEXT_IDENTIFIER) {
					meta_token* LastToken = NewTokens->Last;

					//Could be another replacement token that will be combined
					if (LastToken->Type != META_TOKEN_TYPE_REPLACEMENT) {
						Assert(LastToken->Type == META_TOKEN_TYPE_IDENTIFIER);

						Meta_Token_Iter_Move_Next(&MacroTokenIter);
						Assert(MacroTokenIter.Token);
					
						meta_token* NextToken = MacroTokenIter.Token;
						Assert(NextToken->Type == META_TOKEN_TYPE_IDENTIFIER);
						LastToken->Identifier = String_Concat((allocator*)Tokenizer->Arena, LastToken->Identifier, NextToken->Identifier);
					}
				}

			} else {
				meta_token* NewToken = Meta_Allocate_Token(Tokenizer, MacroTokenIter.Token->Type);
				NewToken->LineNumber = MacroTokenIter.Token->LineNumber;
				NewToken->Identifier = MacroTokenIter.Token->Identifier;
				NewToken->Flags = MacroTokenIter.Token->Flags;
				NewToken->Parameters = MacroTokenIter.Token->Parameters;
				NewToken->StackUpwardOffset = MacroTokenIter.Token->StackUpwardOffset;

				DLL_Push_Back(NewTokens->First, NewTokens->Last, NewToken);
				NewTokens->Count++;
			}

			Meta_Token_Iter_Move_Next(&MacroTokenIter);
		}

		if (FoundReplacements) {
			//Only free the tokens if the macro token list is not the original macro
			//code tokens. Those tokens can be reused if the code ever uses the 
			//macro again. This only happens on the first loop
			if (!FirstLoop) {
				meta_token* OldToken = MacroTokenList.First;
				for (size_t i = 0; i < MacroTokenList.Count; i++) {
					meta_token* TokenToDelete = OldToken;
					OldToken = OldToken->Next;
					SLL_Push_Front(Tokenizer->FreeTokens, TokenToDelete);
				}
			}

			MacroTokenList = *NewTokens;
			Memory_Clear(NewTokens, sizeof(meta_token_list));
		}

		FirstLoop = false;
	} while (FoundReplacements);

	return EndMacroToken;
}

function meta_token* Meta_Parse_And_Replace_Macro(meta_parser* Parser, meta_token* Token, meta_macro* Macro) {
	meta_token_list ReplaceTokens = { Token };
	meta_token_list NewTokens = { 0 };

	ReplaceTokens.Last = Meta_Parse_And_Expand_Macro(Parser, Macro, Token, &NewTokens, NULL);
	if (!ReplaceTokens.Last) return NULL;

	meta_token* NextToken = NULL;
	if (NewTokens.Count) {
		Meta_Tokens_Replace(&ReplaceTokens, &NewTokens);
		Meta_Tokens_Free(Parser->Tokenizer, &ReplaceTokens);

		//New iterator starts on the token right before the macro expansion since
		//we will be iterating at the end of the loop to retry parsing with the newly
		//expanded tokens
		NextToken = NewTokens.First->Prev;
	} else {
		NextToken = ReplaceTokens.Last;
	}
	return NextToken;
}

function meta_token* Meta_Parse_Tags(meta_parser* Parser, meta_token* Token, meta_tag_list* Tags, arena* Scratch) {
	if (Meta_Check_Token(Token, META_TOKEN_TYPE_IDENTIFIER) &&
		String_Equals(Token->Identifier, String_Lit("Tags"))) {
		
		meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token);

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (!Meta_Check_Token(TokenIter.Token, '(')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '(' after 'Tags'");
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		while (TokenIter.Token && TokenIter.Token->Type != ')') {
			if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected tag identifer after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
				return NULL;
			}

			meta_tag* Tag = Arena_Push_Struct(Parser->Arena, meta_tag);
			Tag->Name = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);
			Tag->Value = Tag->Name;
			SLL_Push_Back(Tags->First, Tags->Last, Tag);
			Tags->Count++;

			Meta_Token_Iter_Move_Next(&TokenIter);

			//Token has a value
			if (Meta_Check_Token(TokenIter.Token, ':')) {
				sstream_writer ValueWriter = Begin_Stream_Writer((allocator*)Scratch);

				size_t ParamStackIndex = 0;
				Meta_Token_Iter_Move_Next(&TokenIter);

				if (!TokenIter.Token || TokenIter.Token->Type != META_TOKEN_TYPE_IDENTIFIER) {
					Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of tag value");
					return NULL;
				}

				while (TokenIter.Token) {
					if (TokenIter.Token->Type == '(') {
						ParamStackIndex++;
					}

					if (TokenIter.Token->Type == ')') {
						if (ParamStackIndex == 0) {
							break;
						}
						ParamStackIndex--;
					}

					if (TokenIter.Token->Type == ',') {
						if (ParamStackIndex == 0) {
							break;
						}
						ParamStackIndex--;
					}

					SStream_Writer_Add(&ValueWriter, TokenIter.Token->Identifier);
					Meta_Token_Iter_Move_Next(&TokenIter);
				}

				Tag->Value = SStream_Writer_Join(&ValueWriter, (allocator*)Parser->Arena, String_Lit(" "));
			}

			if (Meta_Check_Token(TokenIter.Token, ')')) {
				continue;
			}

			if (!Meta_Check_Token(TokenIter.Token, ',')) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
				return NULL;
			}

			Meta_Token_Iter_Move_Next(&TokenIter);
		}

		if (!Meta_Check_Token(TokenIter.Token, ')')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		return TokenIter.Token;
	}

	return Token;
}

function meta_token* Meta_Parse_Enum_Entry(meta_parser* Parser, meta_enum_type* Enum, meta_token* Token, arena* Scratch) {
	Assert(Token->Type == META_TOKEN_TYPE_ENUM_ENTRY);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after enum entry declaration");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing enum entry name '('");
		return NULL;
	}

	meta_token* NameToken = TokenIter.Token;

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!TokenIter.Token) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of enum declaration");
		return NULL;
	}

	string OptionalValue = String_Empty();
	meta_tag_list Tags = { 0 };

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);

		sstream_writer ValueWriter = Begin_Stream_Writer((allocator*)Scratch);
			
		size_t ParenStackIndex = 0;
		while (TokenIter.Token && (TokenIter.Token->Type != ',' && (TokenIter.Token->Type != ')' || ParenStackIndex))) {
			SStream_Writer_Add(&ValueWriter, TokenIter.Token->Identifier);
				
			if (TokenIter.Token->Type == '(') {
				ParenStackIndex++;
			}

			if (TokenIter.Token->Type == ')') {
				if (ParenStackIndex == 0) continue;
				ParenStackIndex--;
			}

			Meta_Token_Iter_Move_Next(&TokenIter);
		}

		OptionalValue = SStream_Writer_Join(&ValueWriter, (allocator*)Scratch, String_Empty());
	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, ';')) {
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected token after variable entry. Expected 'Tags()' or ';'");
			return NULL;
		}

		if (!String_Equals(TokenIter.Token->Identifier, String_Lit("Tags"))) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected identifier '%.*s' after variable entry. Expected 'Tags()' or ';'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
			return NULL;
		}

		arena* Scratch2 = Scratch_Get();
		meta_token* NextToken = Meta_Parse_Tags(Parser, TokenIter.Token, &Tags, Scratch2);
		Scratch_Release();

		if (!NextToken) {
			return NULL;
		}

		TokenIter = Meta_Begin_Simple_Token_Iter(NextToken);
	}

	if (!Meta_Check_Token(TokenIter.Token, ';')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of enum entry declaration. Expected ';' after ')'");
		return NULL;
	}

	meta_enum_entry* Entry = Arena_Push_Struct(Parser->Arena, meta_enum_entry);
	Entry->Name = String_Copy((allocator*)Parser->Arena, NameToken->Identifier);
	Entry->OptionalValue = String_Copy((allocator*)Parser->Arena, OptionalValue);
	Entry->Tags = Tags;

	SLL_Push_Back(Enum->FirstEntry, Enum->LastEntry, Entry);
	Enum->EntryCount++;

	return TokenIter.Token;
}

function meta_token* Meta_Parse_Variable_Entry(meta_parser* Parser, meta_struct_type* Struct, meta_token* Token) {
	Assert(Token->Type == META_TOKEN_TYPE_VARIABLE_ENTRY);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after variable declaration");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	meta_token* TypeToken = TokenIter.Token;
	if (!Meta_Check_Token(TypeToken, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing variable type identifier after '('");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, ',')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TypeToken->Identifier.Size, TypeToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	meta_token* NameToken = TokenIter.Token;
	if (!Meta_Check_Token(NameToken, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing variable name identifier after ','");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);

	meta_tag_list Tags = { 0 };

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, ';')) {
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected token after variable entry. Expected 'Tags()' or ';'");
			return NULL;
		}

		if (!String_Equals(TokenIter.Token->Identifier, String_Lit("Tags"))) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected identifier '%.*s' after variable entry. Expected 'Tags()' or ';'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
			return NULL;
		}

		arena* Scratch2 = Scratch_Get();
		meta_token* NextToken = Meta_Parse_Tags(Parser, TokenIter.Token, &Tags, Scratch2);
		Scratch_Release();

		if (!NextToken) {
			return NULL;
		}

		TokenIter = Meta_Begin_Simple_Token_Iter(NextToken);
	}

	if (!Meta_Check_Token(TokenIter.Token, ';')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of variable declaration. Expected ';' after ')'");
		return NULL;
	}

	meta_variable_entry* Entry = Arena_Push_Struct(Parser->Arena, meta_variable_entry);
	Entry->Type = String_Copy((allocator*)Parser->Arena, TypeToken->Identifier);
	Entry->Name = String_Copy((allocator*)Parser->Arena, NameToken->Identifier);
	Entry->Tags = Tags;

	SLL_Push_Back(Struct->FirstEntry, Struct->LastEntry, Entry);
	Struct->EntryCount++;

	return TokenIter.Token;
}

function meta_token* Meta_Parse_Enum(meta_parser* Parser, meta_token* Token) {
	Assert(Token->Type == META_TOKEN_TYPE_ENUM);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after enum declaration");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Enum must start with a valid name");
		return NULL;
	}

	meta_token* NameToken = TokenIter.Token;

	string EnumName = TokenIter.Token->Identifier;
	string EnumType = String_Lit("u64");

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing ',' after enum name '%.*s' for optional parameters", NameToken->Identifier.Size, NameToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected token. Expected an identifier for enum type");
			return NULL;
		}

		EnumType = TokenIter.Token->Identifier;
		
		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", Token->Identifier.Size, Token->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '{' after ')'");
		return NULL;
	}

	hashmap* EnumMap = &Parser->EnumMap;
	meta_enum_type* Enum = NULL;
	if (Hashmap_Find(EnumMap, &EnumName, &Enum)) {
		Report_Error(Parser->Tokenizer->FilePath, NameToken->LineNumber, "Duplicate enum names '%.*s'", EnumName.Size, EnumName.Ptr);
		return NULL;
	}

	Assert(!Enum);

	Enum = Arena_Push_Struct(Parser->Arena, meta_enum_type);
	Enum->Name = String_Copy((allocator*)Parser->Arena, EnumName);
	Enum->Type = String_Copy((allocator*)Parser->Arena, EnumType);

	Meta_Token_Iter_Move_Next(&TokenIter);
	
	size_t BracketStackIndex = 0;
	while (TokenIter.Token && (TokenIter.Token->Type != '}' || BracketStackIndex)) {
		switch (TokenIter.Token->Type) {
			case '{': { BracketStackIndex++; } break;
			case '}': { BracketStackIndex--; } break;

			case META_TOKEN_TYPE_IDENTIFIER: {
				//If we see an identifier in a struct, the only valid
				//code it can be is if its a macro expansion call. This
				//must be in the format of MACRO_NAME -> ( -> parameters -> )
				//with the amount of parameters matching whats in the macro
				//definition

				meta_macro* Macro;
				if (Hashmap_Find(&Parser->MacroMap, &TokenIter.Token->Identifier, &Macro)) {
					Token = Meta_Parse_And_Replace_Macro(Parser, TokenIter.Token, Macro);
					if (!Token) return NULL;
					TokenIter = Meta_Begin_Simple_Token_Iter(Token);
				} else {
					Report_Error(Parser->Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected identifier '%.*s'", Token->Identifier.Size, Token->Identifier.Ptr);
					return NULL;
				}
			} break;

			case META_TOKEN_TYPE_ENUM_ENTRY: {
				arena* Scratch = Scratch_Get();
				meta_token* EnumToken = Meta_Parse_Enum_Entry(Parser, Enum, TokenIter.Token, Scratch);
				TokenIter = Meta_Begin_Simple_Token_Iter(EnumToken);
				Scratch_Release();
			} break;

			default: {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected token '%.*s'", Token->Identifier.Size, Token->Identifier.Ptr);
				return NULL;
			} break;
		}

		if (!TokenIter.Token) {
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, '}') || BracketStackIndex) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of enum");
		return NULL;
	}

	Hashmap_Add(EnumMap, &Enum->Name, &Enum);
	return TokenIter.Token;
}

function meta_token* Meta_Parse_For_Loop_Flags(meta_parser* Parser, meta_for_loop_context* Context, meta_token* Token, meta_for_loop_flags* OutFlags) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token);
	while (!Meta_Check_Token(TokenIter.Token, ')')) {
		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing identifier after ','");
			return NULL;
		}

		if (String_Equals(TokenIter.Token->Identifier, String_Lit("flags"))) {

			Meta_Token_Iter_Move_Next(&TokenIter);
			if (!Meta_Check_Token(TokenIter.Token, ':')) {
				Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ':' after 'flags'");
				return NULL;
			}

			Meta_Token_Iter_Move_Next(&TokenIter);
			while (!Meta_Check_Token(TokenIter.Token, ')')) {
				if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
					return NULL;
				}

				string FlagStr = TokenIter.Token->Identifier;

				meta_for_loop_flags_metadata* FlagMeta;
				if (!Hashmap_Find(&G_MetaForLoopFlagsMetadataMap, &FlagStr, &FlagMeta)) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Invalid for loop flag '%.*s'", FlagStr.Size, FlagStr.Ptr);
					return NULL;
				}

				if (!(FlagMeta->SupportFlag & Context->SupportFlag)) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Invalid %.*s flag '%.*s'", Context->TypeStr.Size, Context->TypeStr.Ptr, FlagStr.Size, FlagStr.Ptr);
					return NULL;
				}

				*OutFlags |= FlagMeta->Flag;

				Meta_Token_Iter_Move_Next(&TokenIter);
				if (Meta_Check_Token(TokenIter.Token, ')')) {
					continue;
				}

				if (!Meta_Check_Token(TokenIter.Token, ',')) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
					return NULL;
				}

				Meta_Token_Iter_Move_Next(&TokenIter);
			}

		}
	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	return TokenIter.Token;
}

function meta_for_loop_entries Meta_Get_Struct_Entries(arena* Arena, meta_struct_type* Struct) {
	meta_for_loop_entries Result = { 0 };
	for(meta_variable_entry* StructEntry = Struct->FirstEntry; StructEntry; StructEntry = StructEntry->Next) {
		meta_for_loop_entry* Entry = Arena_Push_Struct(Arena, meta_for_loop_entry);
		Entry->Name = StructEntry->Name;
		Entry->Type = StructEntry->Type;
		Entry->Tags = StructEntry->Tags;
		Entry->IsArray = StructEntry->IsArray;

		SLL_Push_Back(Result.First, Result.Last, Entry);
	}
	return Result;
}

function meta_for_loop_entries Meta_Get_Enum_Entries(arena* Arena, meta_enum_type* Enum) {
	meta_for_loop_entries Result = { 0 };
	for(meta_enum_entry* EnumEntry = Enum->FirstEntry; EnumEntry; EnumEntry = EnumEntry->Next) {
		meta_for_loop_entry* Entry = Arena_Push_Struct(Arena, meta_for_loop_entry);
		Entry->Name = EnumEntry->Name;
		Entry->ShortName = Meta_Get_Enum_Value_Short_Name(Arena, Enum, EnumEntry);
		Entry->Tags = EnumEntry->Tags;
		SLL_Push_Back(Result.First, Result.Last, Entry);
	}
	return Result;
}

function meta_for_loop_entries Meta_Get_Functions_With_Tag_Routine(arena* Arena, meta_function_type** Functions, size_t FunctionCount, string Tag) {
	meta_for_loop_entries Result = { 0 };
	for (size_t i = 0; i < FunctionCount; i++) {
		if (Meta_Contains_Tag(&Functions[i]->Tags, Tag)) {
			meta_for_loop_entry* Entry = Arena_Push_Struct(Arena, meta_for_loop_entry);
			Entry->Name = Functions[i]->Name;
			Entry->Tags = Functions[i]->Tags;
			SLL_Push_Back(Result.First, Result.Last, Entry);
		}
	}
	return Result;
}

function meta_token* Meta_Parse_Parameters(meta_parser* Parser, meta_token* Token, string_list* Parameters, arena* Arena) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	Memory_Clear(Parameters, sizeof(string_list));

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token);

	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '(' after identifier '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	while (!Meta_Check_Token(TokenIter.Token, ')')) {
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		string_entry* StrEntry = Arena_Push_Struct(Arena, string_entry);
		StrEntry->String = String_Copy((allocator*)Arena, TokenIter.Token->Identifier);
		SLL_Push_Back(Parameters->First, Parameters->Last, StrEntry);
		Parameters->Count++;

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (Meta_Check_Token(TokenIter.Token, ')')) {
			continue;
		}

		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after identifier '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after routine name");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);

	return TokenIter.Token;
}

function meta_token* Meta_Evaluate_Routine(meta_parser* Parser, meta_for_loop_context* Context, meta_routine Routine, meta_token* Token, arena* Scratch) {
	meta_tokenizer* Tokenizer = Parser->Tokenizer;

	string_list Parameters;
	Token = Meta_Parse_Parameters(Parser, Token->Next, &Parameters, Scratch);
	if (!Token) return NULL;

	switch (Routine) {
		case META_GET_FUNCTIONS_WITH_TAG: {
			if (Parameters.Count != 1) {
				Report_Error(Tokenizer->FilePath, Token->LineNumber, "Invalid amount of parameters for META_GET_FUNCTIONS_WITH_TAG. Expected '1', got '%d'", Parameters.Count);
				return NULL;
			}

			Context->SupportFlag = META_FOR_LOOP_SUPPORT_FUNCTION;
			Context->TypeStr = String_Lit("function");
			Context->Entries = Meta_Get_Functions_With_Tag_Routine(Parser->Arena, (meta_function_type* *)Parser->FunctionMap.Values, Parser->FunctionMap.ItemCount, Parameters.First->String);
		} break;

		Invalid_Default_Case;
	}

	return Token;
}

function meta_token* Meta_Parse_If(meta_parser* Parser, meta_token* Token, meta_for_loop_context_stack* ForLoopStack, meta_for_loop_entry* Entry, meta_if_state* IfState, arena* Scratch) {
	Assert(Token->Type == META_TOKEN_TYPE_IF);
	meta_tokenizer* Tokenizer = Parser->Tokenizer;

	meta_token_list NewTokens = { 0 };
	meta_token* EndParamsToken = Meta_Parse_For_Params(Parser, Token->Next, ForLoopStack, &NewTokens);
	if (!EndParamsToken) return NULL;
	
	//Parse the parameters with the new tokens 
	meta_token_iter TokenIter = Meta_Begin_Token_Iter(&NewTokens);
	Meta_Token_Iter_Move_Next(&TokenIter);

	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after '('");
		return NULL;
	}

	string PredicateName = TokenIter.Token->Identifier;

	string_list Parameters;
	Token = Meta_Parse_Parameters(Parser, TokenIter.Token->Next, &Parameters, Scratch);
	if (!Token) return NULL;

	if (!Meta_Check_Token(Token, ')')) {
		Report_Error(Tokenizer->FilePath, Token->LineNumber, "Expected ')' after predicate call");
		return NULL;
	}

	//Now start parsing the if statement. We will continue with the original token list
	meta_token_iter IfTokenIter = Meta_Begin_Simple_Token_Iter(EndParamsToken->Next);
	if (!Meta_Check_Token(IfTokenIter.Token, '{')) {
		Report_Error(Tokenizer->FilePath, IfTokenIter.PrevToken->LineNumber, "Expected '{' after after ')'");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&IfTokenIter);

	size_t BracketStackIndex = 0;
	while (IfTokenIter.Token && (IfTokenIter.Token->Type != '}' || BracketStackIndex)) {
		if (IfTokenIter.Token->Type == '{') {
			BracketStackIndex++;
		}

		if (IfTokenIter.Token->Type == '}') {
			BracketStackIndex--;
		}

		Meta_Token_Iter_Move_Next(&IfTokenIter);
	}

	if (!Meta_Check_Token(IfTokenIter.Token, '}') || BracketStackIndex) {
		Report_Error(Tokenizer->FilePath, IfTokenIter.PrevToken->LineNumber, "Unexpected end of if statement");
		return NULL;
	}

	IfState->ElseToken = IfTokenIter.Token;

	Meta_Token_Iter_Move_Next(&IfTokenIter);

	//Check for an optional else statement
	if (Meta_Check_Token(IfTokenIter.Token, META_TOKEN_TYPE_ELSE)) {
		Meta_Token_Iter_Move_Next(&IfTokenIter);

		if (!Meta_Check_Token(IfTokenIter.Token, '{')) {
			Report_Error(Tokenizer->FilePath, IfTokenIter.PrevToken->LineNumber, "Expected '{' after after 'META_ELSE'");
			return NULL;
		}

		BracketStackIndex = 0;
		meta_token_iter ElseTokenIter = Meta_Begin_Simple_Token_Iter(IfTokenIter.Token->Next);
		while (ElseTokenIter.Token && (ElseTokenIter.Token->Type != '}' || BracketStackIndex)) {
			if (ElseTokenIter.Token->Type == '{') {
				BracketStackIndex++;
			}

			if (ElseTokenIter.Token->Type == '}') {
				BracketStackIndex--;
			}

			Meta_Token_Iter_Move_Next(&ElseTokenIter);
		}

		if (!Meta_Check_Token(ElseTokenIter.Token, '}') || BracketStackIndex) {
			Report_Error(Tokenizer->FilePath, ElseTokenIter.PrevToken->LineNumber, "Unexpected end of if statement");
			return NULL;
		}

		IfState->EndToken = ElseTokenIter.Token;
	} else {
		IfState->EndToken = IfState->ElseToken;
	}

	meta_parser_predicate_func* PredicateFunc = Meta_Parser_Get_Predicate(PredicateName);
	if (!PredicateFunc) {
		Report_Error(Tokenizer->FilePath, TokenIter.Token->LineNumber, "Invalid META_IF predicate '%.*s'", PredicateName.Size, PredicateName.Ptr);
		return NULL;
	}
	IfState->PassedPredicate = PredicateFunc(Parser, Entry, Parameters);
	return EndParamsToken->Next;
}

function void Meta_If_State_Stack_Push(meta_if_state_stack* Stack) {
	Stack->Index++;
	Assert(Stack->Index < Array_Count(Stack->States));
	Stack->CurrentState = Stack->States + Stack->Index;
}

function void Meta_If_State_Stack_Pop(meta_if_state_stack* Stack) {
	Assert(Stack->Index != (size_t)-1);
	Stack->States[Stack->Index].EndToken = NULL;
	Stack->Index--;
	Stack->CurrentState = (Stack->Index == (size_t)-1) ? NULL : Stack->States + Stack->Index;
}

function void Meta_For_Loop_Stack_Push(meta_for_loop_context_stack* Stack) {
	Assert(Stack->Count < Array_Count(Stack->Stack));
	size_t ContextIndex = Stack->Count++;
	meta_for_loop_context* Context = &Stack->Stack[ContextIndex];
	Memory_Clear(Context, sizeof(meta_for_loop_context));
	Stack->CurrentContext = Context;
}

function void Meta_For_Loop_Stack_Pop(meta_for_loop_context_stack* Stack) {
	Assert(Stack->Count > 0);
	Stack->Count--;
	Stack->CurrentContext = Stack->Count ? Stack->Stack + (Stack->Count-1) : NULL;
}

function meta_token* Meta_Parse_And_Expand_For(meta_parser* Parser, meta_token* Token, meta_for_loop_context_stack* ForLoopStack, meta_token_list* NewTokens);
function b32 Meta_Replace_For_Tokens(meta_parser* Parser, meta_token_iter* TokenIter, meta_for_loop_context_stack* ForLoopStack, meta_if_state_stack* IfStateStack, meta_token_list* NewTokens) {
	while (Meta_Token_Iter_Is_Valid(TokenIter)) {
		switch (TokenIter->Token->Type) {
			case META_TOKEN_TYPE_IF: {
				Meta_If_State_Stack_Push(IfStateStack);

				arena* Scratch = Scratch_Get();
				meta_token* Token = Meta_Parse_If(Parser, TokenIter->Token, ForLoopStack, ForLoopStack->CurrentContext->CurrentEntry, IfStateStack->CurrentState, Scratch);
				Scratch_Release();

				if (!Token) return false;

				Meta_Token_Iter_Skip_To(TokenIter, Token);

				//Skip passed the if statement tokens if the routine failed
				//and begin the else statement (if it has one)
				if (!IfStateStack->CurrentState->PassedPredicate) {
					Meta_Token_Iter_Skip_To(TokenIter, IfStateStack->CurrentState->ElseToken);
						
					//If there is no else statement pop the stack
					if (IfStateStack->CurrentState->ElseToken == IfStateStack->CurrentState->EndToken) {
						Meta_If_State_Stack_Pop(IfStateStack);
					}
				}
			} break;

			case META_TOKEN_TYPE_ELSE: {
				//Skip passed the else statement tokens if the routine passed
				//and begin the else and pop the stack
				if (IfStateStack->CurrentState->PassedPredicate) {
					Meta_Token_Iter_Skip_To(TokenIter, IfStateStack->CurrentState->EndToken);
					Meta_If_State_Stack_Pop(IfStateStack);
				} else {
					Meta_Token_Iter_Move_Next(TokenIter);
				}
			} break;

			case META_TOKEN_TYPE_FOR: {
				//For loops can have internal for loops. Recursive call to
				//build more NewTokens
				meta_token* Token = Meta_Parse_And_Expand_For(Parser, TokenIter->Token, ForLoopStack, NewTokens);
				if (!Token) return false;
				Meta_Token_Iter_Skip_To(TokenIter, Token);
			} break;

			case META_TOKEN_TYPE_REPLACEMENT: {
				if (!Meta_Replace_Next_For_Token(Parser, TokenIter, NewTokens, ForLoopStack)) {
					return false;
				}
			} break;

			default: {
				//Replace the token if we know we this isn't an if statement
				//terminating character
				b32 IsEndOfBranch = IfStateStack->CurrentState && ((TokenIter->Token == IfStateStack->CurrentState->ElseToken) || (TokenIter->Token == IfStateStack->CurrentState->EndToken));
				if (IsEndOfBranch) {
					if (TokenIter->Token == IfStateStack->CurrentState->EndToken) {
						Meta_If_State_Stack_Pop(IfStateStack);
					}
				} else {
					meta_macro* Macro = NULL;
					if (TokenIter->Token->Type == META_TOKEN_TYPE_IDENTIFIER) {
						if (!Hashmap_Find(&Parser->MacroMap, &TokenIter->Token->Identifier, &Macro)) {
							Macro = NULL;
						}
					}
					
					if (!Macro) {
						Meta_Replace_Next_For_Token(Parser, TokenIter, NewTokens, ForLoopStack);
					} else {
						meta_token_list MacroTokens = { 0 };

						meta_token* EndMacroToken = Meta_Parse_And_Expand_Macro(Parser, Macro, TokenIter->Token, &MacroTokens, ForLoopStack);

						if (!EndMacroToken) return false;

						meta_token_iter MacroTokenIter = Meta_Begin_Token_Iter(&MacroTokens);
						if (!Meta_Replace_For_Tokens(Parser, &MacroTokenIter, ForLoopStack, IfStateStack, NewTokens)) {
							return false;
						}

						Meta_Token_Iter_Skip_To(TokenIter, EndMacroToken);
					}
				}
			} break;
		}

		Meta_Token_Iter_Move_Next(TokenIter);
	}
	return true;
}

function meta_token* Meta_Parse_And_Expand_For(meta_parser* Parser, meta_token* Token, meta_for_loop_context_stack* ForLoopStack, meta_token_list* NewTokens) {
	Assert(Token->Type == META_TOKEN_TYPE_FOR);

	meta_tokenizer* Tokenizer = Parser->Tokenizer;
	
	meta_token_list ParamsToken = { 0 };
	meta_token* EndParamsToken = Meta_Parse_For_Params(Parser, Token->Next, ForLoopStack, &ParamsToken);
	if (!EndParamsToken) return NULL;

	meta_token_iter TokenIter = Meta_Begin_Token_Iter(&ParamsToken);
	Meta_Token_Iter_Move_Next(&TokenIter);

	//Get the type we are looping on (struct, global, enum, functions, etc..)
	string TypeIdentifier = TokenIter.Token->Identifier;

	Meta_For_Loop_Stack_Push(ForLoopStack);

	meta_routine Routine;
	meta_struct_type* Struct;
	meta_enum_type* Enum;

	if (Meta_Parser_Is_Routine(TypeIdentifier, &Routine)) {
		arena* Scratch = Scratch_Get();
		Token = Meta_Evaluate_Routine(Parser, ForLoopStack->CurrentContext, Routine, TokenIter.Token, Scratch);
		Scratch_Release();
		if (!Token) return NULL;
		TokenIter = Meta_Begin_Simple_Token_Iter(Token);
	} else if (Meta_Parser_Is_Struct(Parser, TypeIdentifier, &Struct) || Meta_Parser_Is_Global(Parser, TypeIdentifier, &Struct) || Meta_Parser_Is_Union(Parser, TypeIdentifier, &Struct)) {
		ForLoopStack->CurrentContext->Entries = Meta_Get_Struct_Entries(Parser->Arena, Struct);
		ForLoopStack->CurrentContext->SupportFlag |= META_FOR_LOOP_SUPPORT_STRUCT;
		ForLoopStack->CurrentContext->TypeStr = String_Lit("struct");
		Meta_Token_Iter_Move_Next(&TokenIter);
	} else if (Meta_Parser_Is_Enum(Parser, TypeIdentifier, &Enum)) {
		ForLoopStack->CurrentContext->Entries = Meta_Get_Enum_Entries(Parser->Arena, Enum);
		ForLoopStack->CurrentContext->SupportFlag |= META_FOR_LOOP_SUPPORT_ENUM;
		ForLoopStack->CurrentContext->TypeStr = String_Lit("enum");
		Meta_Token_Iter_Move_Next(&TokenIter);
	} else {
		Report_Error(Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected identifier '%.*s'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
		return NULL;
	}

	//Additional generation flags
	meta_for_loop_flags ForLoopFlags = META_FOR_LOOP_FLAG_NONE;
	Token = Meta_Parse_For_Loop_Flags(Parser, ForLoopStack->CurrentContext, TokenIter.Token, &ForLoopFlags);
	if (!Token) return NULL;

	//CodeTokens contains the body of the for loop to generate on every loop
	meta_token_list CodeTokens = { 0 };
	Token = Meta_Parse_Code_Tokens(Parser, EndParamsToken->Next, &CodeTokens);
	if (!Token) return NULL;

	meta_if_state_stack IfStateStack = { .Index = (size_t)-1 };
	for (meta_for_loop_entry* Entry = ForLoopStack->CurrentContext->Entries.First; Entry; Entry = Entry->Next) {
		ForLoopStack->CurrentContext->CurrentEntry = Entry;
		TokenIter = Meta_Begin_Token_Iter(&CodeTokens);

		if (!(ForLoopFlags & META_FOR_LOOP_NO_BRACE_FLAG)) {
			meta_token* NewToken = Meta_Allocate_Token(Tokenizer, '{');
			DLL_Push_Back(NewTokens->First, NewTokens->Last, NewToken);
		}

		if (!Meta_Replace_For_Tokens(Parser, &TokenIter, ForLoopStack, &IfStateStack, NewTokens)) {
			return NULL;
		}
		
		if (!(ForLoopFlags & META_FOR_LOOP_NO_BRACE_FLAG)) {
			meta_token* NewToken = Meta_Allocate_Token(Tokenizer, '}');
			DLL_Push_Back(NewTokens->First, NewTokens->Last, NewToken);
		}
	}

	Assert(IfStateStack.Index == (size_t)-1);

	Meta_For_Loop_Stack_Pop(ForLoopStack);
	return CodeTokens.Last->Next;
}

function meta_token* Meta_Parse_Function(meta_parser* Parser, meta_token* Token) {
	Assert(Token->Type == META_TOKEN_TYPE_FUNCTION);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after function declaration");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Function must start with a valid name");
		return NULL;
	}

	meta_token* FunctionNameToken = TokenIter.Token;
	string FunctionName = FunctionNameToken->Identifier;

	meta_function_type* Function;
	if (Hashmap_Find(&Parser->FunctionMap, &FunctionName, &Function)) {
		Report_Error(Parser->Tokenizer->FilePath, FunctionNameToken->LineNumber, "Duplicate function names '%.*s'", FunctionName.Size, FunctionName.Ptr);
		return NULL;
	}

	Function = Arena_Push_Struct(Parser->Arena, meta_function_type);
	Function->Name = String_Copy((allocator*)Parser->Arena, FunctionName);
	
	Meta_Token_Iter_Move_Next(&TokenIter);
	while (TokenIter.Token && TokenIter.Token->Type != ')') {
		if (!Meta_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after ','");
			return NULL;
		}

		meta_parameter* Parameter = Arena_Push_Struct(Parser->Arena, meta_parameter);
		Parameter->Type = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);

		Meta_Token_Iter_Move_Next(&TokenIter);
		if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}

		Parameter->Name = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);

		SLL_Push_Back(Function->FirstParameter, Function->LastParameter, Parameter);
		Function->ParameterCount++;

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	
	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_RETURN_ARROW)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '->' after ')'");
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected identifier after '->'");
		return NULL;
	}

	Function->ReturnType = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '{' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}

	meta_token_list CodeTokenList = { 0 };

	Meta_Token_Iter_Move_Next(&TokenIter);

	size_t BracketStackIndex = 0;
	while (TokenIter.Token && (TokenIter.Token->Type != '}' || BracketStackIndex)) {
		switch (TokenIter.Token->Type) {
			case '{': { BracketStackIndex++; } break;
			case '}': { BracketStackIndex--; } break;

			case META_TOKEN_TYPE_IDENTIFIER: {
				meta_macro* Macro;
				if (Hashmap_Find(&Parser->MacroMap, &TokenIter.Token->Identifier, &Macro)) {
					Token = Meta_Parse_And_Replace_Macro(Parser, TokenIter.Token, Macro);
					if (!Token) return NULL;
					TokenIter = Meta_Begin_Simple_Token_Iter(Token);
					Meta_Token_Iter_Move_Next(&TokenIter);
					continue;
				}
			} break;

			case META_TOKEN_TYPE_FOR: {
				meta_for_loop_context_stack ForLoopStack = { 0 };
				meta_token_list NewTokens = { 0 };
				meta_token_list ForEachTokens = { .First = TokenIter.Token };

				ForEachTokens.Last = Meta_Parse_And_Expand_For(Parser, TokenIter.Token, &ForLoopStack, &NewTokens);
				
				if (!ForEachTokens.Last) return NULL;

				if (NewTokens.Count) {
					Meta_Tokens_Replace(&ForEachTokens, &NewTokens);
					Meta_Tokens_Free(Parser->Tokenizer, &ForEachTokens);
					Token = NewTokens.First->Prev;
				} Invalid_Else;

				TokenIter = Meta_Begin_Simple_Token_Iter(Token);
				Meta_Token_Iter_Move_Next(&TokenIter);
				continue;
			} break;
		}

		if (!TokenIter.Token) {
			return NULL;
		}

		if (!CodeTokenList.Count) {
			CodeTokenList.First = TokenIter.Token;
		}
		CodeTokenList.Count++;

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!TokenIter.Token || TokenIter.Token->Type != '}' || BracketStackIndex) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexepected end of function");
		return NULL;
	}

	CodeTokenList.Last = TokenIter.PrevToken;
	Function->CodeTokens = CodeTokenList;

	meta_token* Result = TokenIter.Token;
	if (TokenIter.Token->Next &&
		TokenIter.Token->Next->Type == META_TOKEN_TYPE_IDENTIFIER &&
		String_Equals(TokenIter.Token->Next->Identifier, String_Lit("Tags"))) {
		arena* Scratch2 = Scratch_Get();
		Result = Meta_Parse_Tags(Parser, TokenIter.Token->Next, &Function->Tags, Scratch2);
		Scratch_Release();

		if (!Result) return NULL;
		Result = Result->Prev;
	}

	Hashmap_Add(&Parser->FunctionMap, &FunctionName, &Function);
	return Result;
}

function meta_token* Meta_Parse_Struct_Or_Global(meta_parser* Parser, meta_token* Token, arena* Scratch) {
	Assert(Token->Type == META_TOKEN_TYPE_STRUCT || Token->Type == META_TOKEN_TYPE_GLOBAL);

	meta_token_type StructType = Token->Type;
	string StructOrGlobal = StructType == META_TOKEN_TYPE_STRUCT ? String_Lit("struct") : String_Lit("global");
	string StructOrGlobalPascal = String_Pascal_Case((allocator*)Scratch, StructOrGlobal);

	meta_token_iter TokenIter = Meta_Begin_Simple_Token_Iter(Token->Next);
	if (!Meta_Check_Token(TokenIter.Token, '(')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing '(' after %.*s declaration", StructOrGlobal.Size, StructOrGlobal.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, META_TOKEN_TYPE_IDENTIFIER)) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "%.*s must start with a valid struct name", StructOrGlobalPascal.Size, StructOrGlobalPascal.Ptr);
		return NULL;
	}

	meta_token* StructNameToken = TokenIter.Token;
	string StructName = TokenIter.Token->Identifier;

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, ')')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after the %.*s name", StructOrGlobal.Size, StructOrGlobal.Ptr);
		return NULL;
	}

	Meta_Token_Iter_Move_Next(&TokenIter);
	if (!Meta_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '{' after ')'");
		return NULL;
	}

	hashmap* StructMap = StructType == META_TOKEN_TYPE_STRUCT ? &Parser->StructMap : &Parser->GlobalMap;

	meta_struct_type* Struct = NULL;
	if (Hashmap_Find(StructMap, &StructName, &Struct)) {
		Report_Error(Parser->Tokenizer->FilePath, StructNameToken->LineNumber, "Duplicate %.*s names '%.*s'", StructOrGlobal.Size, StructOrGlobal.Ptr, StructName.Size, StructName.Ptr);
		return NULL;
	}

	Assert(!Struct);

	Struct = Arena_Push_Struct(Parser->Arena, meta_struct_type);
	Struct->Name = String_Copy((allocator*)Parser->Arena, StructName);

	Meta_Token_Iter_Move_Next(&TokenIter);

	size_t BracketStackIndex = 0;
	while (TokenIter.Token && (TokenIter.Token->Type != '}' || BracketStackIndex)) {
		switch (TokenIter.Token->Type) {
			case '{': { BracketStackIndex++; } break;
			case '}': { BracketStackIndex--; } break;

			case META_TOKEN_TYPE_IDENTIFIER: {
				//If we see an identifier in a struct, the only valid
				//code it can be is if its a macro expansion call. This
				//must be in the format of MACRO_NAME -> ( -> parameters -> )
				//with the amount of parameters matching whats in the macro
				//definition

				meta_macro* Macro;
				if (Hashmap_Find(&Parser->MacroMap, &TokenIter.Token->Identifier, &Macro)) {
					Token = Meta_Parse_And_Replace_Macro(Parser, TokenIter.Token, Macro);
					if (!Token) return NULL;
					TokenIter = Meta_Begin_Simple_Token_Iter(Token);
				} else {
					Report_Error(Parser->Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected identifier '%.*s'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
					return NULL;
				}
			} break;

			case META_TOKEN_TYPE_VARIABLE_ENTRY: {
				meta_token* StructToken = Meta_Parse_Variable_Entry(Parser, Struct, TokenIter.Token);
				TokenIter = Meta_Begin_Simple_Token_Iter(StructToken);
			} break;

			case META_TOKEN_TYPE_STRUCT: 
			case META_TOKEN_TYPE_GLOBAL: {
				arena* Scratch2 = Scratch_Get();
				meta_token* StructToken = Meta_Parse_Struct_Or_Global(Parser, TokenIter.Token, Scratch2);
				Scratch_Release();
				TokenIter = Meta_Begin_Simple_Token_Iter(StructToken);
			} break;

			case META_TOKEN_TYPE_ENUM: {
				meta_token* StructToken = Meta_Parse_Enum(Parser, TokenIter.Token);
				TokenIter = Meta_Begin_Simple_Token_Iter(StructToken);
			} break;

			case META_TOKEN_TYPE_FUNCTION: {
				meta_token* StructToken = Meta_Parse_Function(Parser, TokenIter.Token);
				TokenIter = Meta_Begin_Simple_Token_Iter(StructToken);
			} break;

			case META_TOKEN_TYPE_MACRO: {
				meta_token* StructToken = Meta_Parse_Macro(Parser, TokenIter.Token);
				TokenIter = Meta_Begin_Simple_Token_Iter(StructToken);
			} break;

			default: {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected token '%.*s'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
				return NULL;
			} break;
		}

		if (!TokenIter.Token) {
			return NULL;
		}

		Meta_Token_Iter_Move_Next(&TokenIter);
	}

	if (!Meta_Check_Token(TokenIter.Token, '}') || BracketStackIndex) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of struct");
		return NULL;
	}

	Hashmap_Add(StructMap, &Struct->Name, &Struct);
	return TokenIter.Token;
}

function b32 Meta_Parser_Evaluate(meta_parser* Parser) {
	meta_token_list* Tokens = &Parser->Tokenizer->Tokens;

	meta_token* Token = Tokens->First;
	while (Token) {

		switch (Token->Type) {
			case META_TOKEN_TYPE_MACRO: {
				Token = Meta_Parse_Macro(Parser, Token);
			} break;

			case META_TOKEN_TYPE_STRUCT: 
			case META_TOKEN_TYPE_GLOBAL: {
				arena* Scratch = Scratch_Get();
				Token = Meta_Parse_Struct_Or_Global(Parser, Token, Scratch);
				Scratch_Release();
			} break;

			case META_TOKEN_TYPE_ENUM: {
				Token = Meta_Parse_Enum(Parser, Token);
			} break;

			case META_TOKEN_TYPE_FUNCTION: {
				Token = Meta_Parse_Function(Parser, Token);
			} break;

			case META_TOKEN_TYPE_IDENTIFIER: {
				meta_macro* Macro;
				if (Hashmap_Find(&Parser->MacroMap, &Token->Identifier, &Macro)) {
					Token = Meta_Parse_And_Replace_Macro(Parser, Token, Macro);
				} else {
					Report_Error(Parser->Tokenizer->FilePath, Token->LineNumber, "Unexpected identifier '%.*s'", Token->Identifier.Size, Token->Identifier.Ptr);
					return false;
				}
			} break;

			default: {
				Report_Error(Parser->Tokenizer->FilePath, Token->LineNumber, "Unexpected token '%.*s'", Token->Identifier.Size, Token->Identifier.Ptr);
				return false;
			} break;
		}

		if (!Token) {
			return false;
		}

		Token = Token->Next;
	}

	return true;
}

function b32 Meta_Parser_Parse(meta_parser* Parser, string FilePath, string Content, arena* ParserArena, meta_parser_process_info* ProcessInfo) {
	b32 Result = false;
	
	meta_tokenizer Tokenizer = { 0 };
	Tokenizer.FilePath = FilePath;
	
	if (Meta_Parser_Generate_Tokens(&Tokenizer, ParserArena, Content)) {
		meta_token_list* Tokens = &Tokenizer.Tokens;
		if (Tokens->First) {
			Parser->Arena = ParserArena;
			Parser->Tokenizer = &Tokenizer;
			Parser->MacroMap      = Hashmap_Init((allocator*)ParserArena, sizeof(meta_macro*), sizeof(string), Hash_String, Compare_String);
			Parser->StructMap     = Hashmap_Init((allocator*)ParserArena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->GlobalMap     = Hashmap_Init((allocator*)ParserArena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->StructInfoMap = Hashmap_Init((allocator*)ParserArena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->UnionInfoMap  = Hashmap_Init((allocator*)ParserArena, sizeof(meta_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->EnumMap 	  = Hashmap_Init((allocator*)ParserArena, sizeof(meta_enum_type*), sizeof(string), Hash_String, Compare_String);
			Parser->EnumInfoMap   = Hashmap_Init((allocator*)ParserArena, sizeof(meta_enum_type*), sizeof(string), Hash_String, Compare_String);
			Parser->FunctionMap   = Hashmap_Init((allocator*)ParserArena, sizeof(meta_function_type*), sizeof(string), Hash_String, Compare_String);
			Parser->MacroParamMap = Hashmap_Init((allocator*)ParserArena, sizeof(meta_token_list), sizeof(string), Hash_String, Compare_String);
			
			//Add the global structs

			if (ProcessInfo) {
				for (size_t i = 0; i < ProcessInfo->StructCount; i++) {
					Hashmap_Add(&Parser->StructInfoMap, &ProcessInfo->Structs[i]->Name, &ProcessInfo->Structs[i]);
				}

				for (size_t i = 0; i < ProcessInfo->UnionCount; i++) {
					Hashmap_Add(&Parser->UnionInfoMap, &ProcessInfo->Unions[i]->Name, &ProcessInfo->Unions[i]);
				}

				for (size_t i = 0; i < ProcessInfo->EnumCount; i++) {
					Hashmap_Add(&Parser->EnumInfoMap, &ProcessInfo->Enums[i]->Name, &ProcessInfo->Enums[i]);
				}
			}
			Result = Meta_Parser_Evaluate(Parser);
		}

	}

	return Result;
}