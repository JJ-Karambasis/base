global hashmap G_SourceKeywordMap;
global u32 G_GeneratedIndex;

function void Source_Parser_Init_Globals(arena* Arena) {
	string Keywords[] = {
		String_Lit("struct"),
		String_Lit("typedef"),
		String_Lit("Meta"),
		String_Lit("enum"),
		String_Lit("union"),
		String_Lit("//"),
		String_Lit("/*"),
		String_Lit("*/"),
	};
    
	source_token_type KeywordsType[] = {
		SOURCE_TOKEN_TYPE_STRUCT,
		SOURCE_TOKEN_TYPE_TYPEDEF,
		SOURCE_TOKEN_TYPE_META,
		SOURCE_TOKEN_TYPE_ENUM,
		SOURCE_TOKEN_TYPE_UNION,
		SOURCE_TOKEN_TYPE_SINGLE_LINE_COMMENT,
		SOURCE_TOKEN_TYPE_MULTI_LINE_COMMENT_START,
		SOURCE_TOKEN_TYPE_MULTI_LINE_COMMENT_END,
	};
	Static_Assert(Array_Count(Keywords) == Array_Count(KeywordsType));
    
	G_SourceKeywordMap = Hashmap_Init((allocator*)Arena, sizeof(source_token_type), sizeof(string), Hash_String, Compare_String);
	for (size_t i = 0; i < Array_Count(Keywords); i++) {
		Hashmap_Add(&G_SourceKeywordMap, &Keywords[i], &KeywordsType[i]);
	}
}

function source_token* Source_Allocate_Token(source_tokenizer* Tokenizer, source_token_type Type) {
	source_token* Token = Tokenizer->FreeTokens;
	if (Token) SLL_Pop_Front(Tokenizer->FreeTokens);
	else Token = Arena_Push_Struct_No_Clear(Tokenizer->Arena, source_token);
	Memory_Clear(Token, sizeof(source_token));
	
	Token->Type = Type;
	if (Type < 256) {
		Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, Make_String((const char*)&Type, 1));
	}
	return Token;
}

function void Source_Add_Token(source_tokenizer* Tokenizer, source_token_type Type, size_t c0, size_t c1, size_t LineNumber) {
	source_token* Token = Source_Allocate_Token(Tokenizer, Type);
	Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, String_Substr(Tokenizer->Content, c0, c1));
	Token->c0 = c0;
	Token->LineNumber = LineNumber;
	DLL_Push_Back(Tokenizer->Tokens.First, Tokenizer->Tokens.Last, Token);
	Tokenizer->Tokens.Count++;
}

function inline void Source_Add_Token_Char(source_tokenizer* Tokenizer, sstream_char Char) {
	Source_Add_Token(Tokenizer, (source_token_type)Char.Char, Char.Index, Char.Index + 1, Char.LineIndex);
}

function void Source_Tokenizer_Begin_Identifier(source_tokenizer* Tokenizer) {
	sstream_char Char = SStream_Reader_Consume_Char(&Tokenizer->Stream);
	if (!Tokenizer->BeginIdentifier) {
		Tokenizer->StartIdentifierChar = Char;
		Tokenizer->BeginIdentifier = true;
	}
}

function void Source_Tokenizer_End_Identifier(source_tokenizer* Tokenizer) {
	if (Tokenizer->BeginIdentifier) {
		size_t Index = SStream_Reader_Get_Index(&Tokenizer->Stream);
		Source_Add_Token(Tokenizer, SOURCE_TOKEN_TYPE_IDENTIFIER, Tokenizer->StartIdentifierChar.Index, Index, Tokenizer->StartIdentifierChar.LineIndex);
		Tokenizer->BeginIdentifier = false;
	}
}

function b32 Source_Parser_Generate_Tokens(source_tokenizer* Tokenizer, arena* Arena, string FilePath, string Content) {
	Tokenizer->Arena = Arena;
	Tokenizer->FilePath = FilePath;
	Tokenizer->Content = Content;
    
	Tokenizer->Stream = SStream_Reader_Begin(Tokenizer->Content);
    
	sstream_reader* Stream = &Tokenizer->Stream;
	while (SStream_Reader_Is_Valid(Stream)) {
		sstream_char Char = SStream_Reader_Peek_Char(Stream);
		switch (Char.Char) {
			case '(':
			case ')':
			case '{':
			case '}':
			case ',':
			case ';':
			case '*':
			case '=':
			case ':':
			case '[':
			case ']':
			{
				Source_Tokenizer_End_Identifier(Tokenizer);
				Source_Add_Token_Char(Tokenizer, SStream_Reader_Consume_Char(Stream));
			} break;
            
			default: {
				if (Is_Whitespace(Char.Char)) {
					Source_Tokenizer_End_Identifier(Tokenizer);
					SStream_Reader_Eat_Whitespace(Stream);
				} else {
					Source_Tokenizer_Begin_Identifier(Tokenizer);
				}
			} break;
		}
	}
	Source_Tokenizer_End_Identifier(Tokenizer);
    
	for (source_token* Token = Tokenizer->Tokens.First; Token; Token = Token->Next) {
		if (Token->Type == SOURCE_TOKEN_TYPE_IDENTIFIER) {
			source_token_type Type;
			if (Hashmap_Find(&G_SourceKeywordMap, &Token->Identifier, &Type)) {
				Token->Type = Type;
			}
		}
	}
    
	return true;
}

function inline b32 Source_Check_Token(source_token* Token, u32 Type) {
	return Token && Token->Type == (source_token_type)Type;
}

function source_token_iter Source_Begin_Token_Iter(source_token* Token) {
	source_token_iter Result = { 0 };
	Result.Token = Token;
	Result.PrevToken = (Token != NULL) ? Token->Prev : NULL;
	Result.NextToken = (Token != NULL) ? Token->Next : NULL;
	return Result;
}

function void Source_Token_Iter_Move_Next(source_token_iter* Iter) {
	if (!Iter->Token) return;
	Iter->PrevToken = Iter->Token;
	Iter->Token = Iter->Token->Next;
	Iter->NextToken = Iter->Token ? Iter->Token->Next : NULL;
}

function void Source_Token_Iter_Move_Prev(source_token_iter* Iter) {
	if (!Iter->Token) return;
	Iter->NextToken = Iter->Token;
	Iter->Token = Iter->Token->Prev;
	Iter->PrevToken = Iter->Token ? Iter->Token->Prev : NULL;
} 

function source_token* Source_Parse_Tags(source_parser* Parser, source_token* Token, meta_tag_list* Tags, arena* Scratch) {
	if (Source_Check_Token(Token, SOURCE_TOKEN_TYPE_IDENTIFIER) &&
		String_Equals(Token->Identifier, String_Lit("Tags"))) {
		
		source_token_iter TokenIter = Source_Begin_Token_Iter(Token);
        
		Source_Token_Iter_Move_Next(&TokenIter);
		if (!Source_Check_Token(TokenIter.Token, '(')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '(' after 'Tags'");
			return NULL;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
		while (TokenIter.Token && TokenIter.Token->Type != ')') {
			if (!Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected tag identifer after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
				return NULL;
			}
            
			meta_tag* Tag = Arena_Push_Struct(Parser->Arena, meta_tag);
			Tag->Name = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);
			Tag->Value = Tag->Name;
			SLL_Push_Back(Tags->First, Tags->Last, Tag);
			Tags->Count++;
            
			Source_Token_Iter_Move_Next(&TokenIter);
            
			//Token has a value
			if (Source_Check_Token(TokenIter.Token, ':')) {
				sstream_writer ValueWriter = SStream_Writer_Begin((allocator*)Scratch);
                
				size_t ParamStackIndex = 0;
                size_t BracketStackIndex = 0;
				Source_Token_Iter_Move_Next(&TokenIter);
                
                b32 IsValidToken = (TokenIter.Token && 
                                    (TokenIter.Token->Type == SOURCE_TOKEN_TYPE_IDENTIFIER ||
                                     TokenIter.Token->Type == '(' ||
                                     TokenIter.Token->Type == '{'));
                
				if (!IsValidToken) {
					Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of tag value");
					return NULL;
				}
                
				while (TokenIter.Token) {
					if (TokenIter.Token->Type == '(') {
						ParamStackIndex++;
					}
                    
					if (TokenIter.Token->Type == ')') {
						if (!BracketStackIndex && !ParamStackIndex) {
							break;
						} else if(!ParamStackIndex) {
                            Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of tag value");
                            return NULL;
                        }
                        
						ParamStackIndex--;
					}
                    
                    if(TokenIter.Token->Type == '{') {
                        BracketStackIndex++;
                    }
                    
                    if(TokenIter.Token->Type == '}') {
                        if (!BracketStackIndex && !ParamStackIndex) {
                            break;
                        } else if(!BracketStackIndex) {
                            Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Unexpected end of tag value");
                            return NULL;
                        }
                        
                        BracketStackIndex--;
                    }
                    
					if (TokenIter.Token->Type == ',') {
						if (!ParamStackIndex && !BracketStackIndex) {
							break;
						} 
                        
						ParamStackIndex--;
					}
                    
					SStream_Writer_Add(&ValueWriter, TokenIter.Token->Identifier);
					Source_Token_Iter_Move_Next(&TokenIter);
				}
                
				Tag->Value = SStream_Writer_Join(&ValueWriter, (allocator*)Parser->Arena, String_Empty());
			}
            
			if (Source_Check_Token(TokenIter.Token, ')')) {
				continue;
			}
            
			if (!Source_Check_Token(TokenIter.Token, ',')) {
				Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
				return NULL;
			}
            
			Source_Token_Iter_Move_Next(&TokenIter);
		}
        
		if (!Source_Check_Token(TokenIter.Token, ')')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
		return TokenIter.Token;
	}
    
	return Token;
}

function source_token* Source_Parse_Enum(source_parser* Parser, source_token* Token) {
	Assert(Token && Token->Type == SOURCE_TOKEN_TYPE_ENUM);
    
	source_token* EnumToken = Token;
	source_token* PrevToken = Token->Prev;
	source_token_iter TokenIter = Source_Begin_Token_Iter(EnumToken->Next);
    
	source_enum_type* Enum = Arena_Push_Struct(Parser->Arena, source_enum_type);
    
	string EnumName = String_Empty();
	if (Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
		if (!Source_Check_Token(PrevToken, SOURCE_TOKEN_TYPE_TYPEDEF)) {
			EnumName = TokenIter.Token->Identifier;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (!Source_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '{' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	Enum->Enum = Arena_Push_Struct(Parser->Arena, meta_enum_type);
	Enum->Enum->IsTrueName = true;
	Source_Token_Iter_Move_Next(&TokenIter);
	while (TokenIter.Token && (TokenIter.Token->Type != '}')) {
		if (TokenIter.Token->Type != SOURCE_TOKEN_TYPE_IDENTIFIER) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.Token->LineNumber, "Expected identifier after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}
        
		meta_enum_entry* Entry = Arena_Push_Struct(Parser->Arena, meta_enum_entry);
		Entry->Name = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);
        
		Source_Token_Iter_Move_Next(&TokenIter);
		if (Source_Check_Token(TokenIter.Token, '=')) {
			Source_Token_Iter_Move_Next(&TokenIter);
			b32 IsDone = false;
			while (TokenIter.Token && !IsDone) {
				switch (TokenIter.Token->Type) {
					case ',':
					case '}': {
						IsDone = true;
					} break;
                    
					case SOURCE_TOKEN_TYPE_IDENTIFIER: {
						if (String_Equals(TokenIter.Token->Identifier, String_Lit("Tags"))) {
							IsDone = true;
						}
					} break;
				}
                
				if (!IsDone) {
					Source_Token_Iter_Move_Next(&TokenIter);
				}
			}
		}
        
		arena* Scratch = Scratch_Get();
		Token = Source_Parse_Tags(Parser, TokenIter.Token, &Entry->Tags, Scratch);
		Scratch_Release();
        
		if (!Token) { return NULL; }
        
		TokenIter = Source_Begin_Token_Iter(Token);
        
		SLL_Push_Back(Enum->Enum->FirstEntry, Enum->Enum->LastEntry, Entry);
		Enum->Enum->EntryCount++;
        
		if (Source_Check_Token(TokenIter.Token, '}')) {
			continue;
		}
        
		if (!Source_Check_Token(TokenIter.Token, ',')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ',' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
			return NULL;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (!Source_Check_Token(TokenIter.Token, '}')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '}' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	Source_Token_Iter_Move_Next(&TokenIter);
	if (Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
		EnumName = TokenIter.Token->Identifier;
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (!Source_Check_Token(TokenIter.Token, ';')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ';' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	if (String_Is_Empty(EnumName)) {
		Source_Token_Iter_Move_Next(&TokenIter);
		if (!Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_TYPEDEF)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected 'typedef' after ';' if we want to introspect an enum");
			return NULL;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
		if (!Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected an identifier type after typedef");
			return NULL;
		}
        
		Enum->Enum->Type = String_Copy((allocator*)Parser->Arena, TokenIter.Token->Identifier);
        
		Source_Token_Iter_Move_Next(&TokenIter);
		if (!Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected an enum name identifer after the type");
			return NULL;
		}
        
		EnumName = TokenIter.Token->Identifier;
        
		Source_Token_Iter_Move_Next(&TokenIter);
		if (!Source_Check_Token(TokenIter.Token, ';')) {
			Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ';' after enum name");
			return NULL;
		}
	}
    
	if (String_Is_Empty(EnumName)) {
		Report_Error(Parser->Tokenizer->FilePath, EnumToken->LineNumber, "Cannot introspect anonymous enum");
		return NULL;
	}
    
	Enum->Token = EnumToken;
	Enum->Enum->Name = String_Copy((allocator*)Parser->Arena, EnumName);
    
	Hashmap_Add(&Parser->EnumMap, &Enum->Enum->Name, &Enum);
	
	return TokenIter.Token;
}

function inline string Source_Generate_Struct_Name(source_parser* Parser, arena* Arena) {
    s32 Index = G_GeneratedIndex++;
	string Result = String_Format((allocator*)Arena, "meta_generated_struct_%d", Index);
	return Result;
}

function source_token* Source_Parse_Struct(source_parser* Parser, source_token* Token, arena* Scratch, source_struct_type** OutStructType) {
	Assert(Token && (Token->Type == SOURCE_TOKEN_TYPE_STRUCT || Token->Type == SOURCE_TOKEN_TYPE_UNION));
	
	source_token_type StructOrUnion = Token->Type;
    
	source_token* StructToken = Token;
	source_token* PrevToken = Token->Prev;
	source_token_iter TokenIter = Source_Begin_Token_Iter(StructToken->Next);
    
	source_struct_type* Struct = Arena_Push_Struct(Parser->Arena, source_struct_type);
    
	string StructName = String_Empty();
	if (Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
		if (!Source_Check_Token(PrevToken, SOURCE_TOKEN_TYPE_TYPEDEF)) {
			StructName = TokenIter.Token->Identifier;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (String_Is_Empty(StructName)) {
		StructName = Source_Generate_Struct_Name(Parser, Scratch);
	}
    
	if (!Source_Check_Token(TokenIter.Token, '{')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '{' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	Struct->Struct = Arena_Push_Struct(Parser->Arena, meta_struct_type);
    
	Source_Token_Iter_Move_Next(&TokenIter);
	while (TokenIter.Token && (TokenIter.Token->Type != '}')) {
		switch (TokenIter.Token->Type) {
			
			//Cannot introspect nested unions or structs. So skip the tokens
			case SOURCE_TOKEN_TYPE_STRUCT:
			case SOURCE_TOKEN_TYPE_UNION: {
                
				source_struct_type* NestedStruct;
				Token = Source_Parse_Struct(Parser, TokenIter.Token, Scratch, &NestedStruct);
				if (!Token) return NULL;
                
				TokenIter = Source_Begin_Token_Iter(Token);
                
				//Tags and names are not supported. Only anonymous unions and structs right now. No tags
				meta_variable_entry* VariableEntry = Arena_Push_Struct(Parser->Arena, meta_variable_entry);
				VariableEntry->Type = String_Copy((allocator*)Parser->Arena, NestedStruct->Struct->Name);
				SLL_Push_Back(Struct->Struct->FirstEntry, Struct->Struct->LastEntry, VariableEntry);
				Struct->Struct->EntryCount++;
			} break;
            
			case SOURCE_TOKEN_TYPE_IDENTIFIER: {
				//Easiest way to parse types is to find the semi-colon in the structure
				//entry, and then reverse the order. It'll go 
				// ; -> Optional Tags -> Value -> Type List
				source_token* StartEntryToken = TokenIter.Token;
                
				source_token_iter TagIter = { 0 };
				while (TokenIter.Token && TokenIter.Token->Type != ';') {
					if (Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER) &&
						String_Equals(TokenIter.Token->Identifier, String_Lit("Tags"))) {
						TagIter = Source_Begin_Token_Iter(TokenIter.Token);
					}
                    
					Source_Token_Iter_Move_Next(&TokenIter);
				}
                
				if (!Source_Check_Token(TokenIter.Token, ';')) {
					Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Missing ';' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
					return NULL;
				}
				source_token* EndEntryToken = TokenIter.Token;
                
				meta_tag_list Tags = { 0 };
				if (TagIter.Token) {
					Assert(TagIter.Token->Type == SOURCE_TOKEN_TYPE_IDENTIFIER && String_Equals(TagIter.Token->Identifier, String_Lit("Tags")));
                    
					arena* Scratch2 = Scratch_Get();
					source_token* TagToken = Source_Parse_Tags(Parser, TagIter.Token, &Tags, Scratch2);
					Scratch_Release();
                    
					if (!TagToken) { return NULL; }
				}
                
				source_token_iter StructEntryTokenIter = Source_Begin_Token_Iter(EndEntryToken);
				Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
                
				//Skip over tags
				while(StructEntryTokenIter.Token->Type == ')') {
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
                    
					size_t ParamStackIndex = 0;
					while (StructEntryTokenIter.Token && (StructEntryTokenIter.Token->Type != '(' || ParamStackIndex)) {
						if (StructEntryTokenIter.Token->Type == '(') {
							if (ParamStackIndex == 0) continue;
							ParamStackIndex--;
						}
                        
						if (StructEntryTokenIter.Token->Type == ')') {
							ParamStackIndex++;
						}
                        
						Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					}
                    
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					if (!Source_Check_Token(StructEntryTokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
						Report_Error(Parser->Tokenizer->FilePath, StructEntryTokenIter.NextToken->LineNumber, "Expected 'Tags' before '%.*s'", StructEntryTokenIter.NextToken->Identifier.Size, StructEntryTokenIter.NextToken->Identifier.Ptr);
						return NULL;
					}
					
					if (!String_Equals(StructEntryTokenIter.Token->Identifier, String_Lit("Tags"))) {
						Report_Error(Parser->Tokenizer->FilePath, StructEntryTokenIter.Token->LineNumber, "Unexpected identifier '%.*s'", StructEntryTokenIter.Token->Identifier.Size, StructEntryTokenIter.Token->Identifier.Ptr);
						return NULL;
					}
                    
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
				}
                
				//Check if its an array first
                
				b32 IsArray = false;
				if (Source_Check_Token(StructEntryTokenIter.Token, ']')) {
					while (StructEntryTokenIter.Token->Type != '[') {
						Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					}
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					IsArray = true;
				}
                
				if (!Source_Check_Token(StructEntryTokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
					Report_Error(Parser->Tokenizer->FilePath, StructEntryTokenIter.NextToken->LineNumber, "Expected identifer before '%.*s'", StructEntryTokenIter.NextToken->Identifier.Size, StructEntryTokenIter.NextToken->Identifier.Ptr);
					return NULL;
				}
                
				string Name = StructEntryTokenIter.Token->Identifier;
                
				b32 IsPointer = false;
				if (Source_Check_Token(StructEntryTokenIter.PrevToken, '*')) {
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					IsPointer = true;
				}
                
				sstream_writer TypeWriter = SStream_Writer_Begin((allocator*)Scratch);
				while (StructEntryTokenIter.Token != StartEntryToken) {
					Source_Token_Iter_Move_Prev(&StructEntryTokenIter);
					SStream_Writer_Add_Front(&TypeWriter, StructEntryTokenIter.Token->Identifier);
				}
				string Type = SStream_Writer_Join(&TypeWriter, (allocator*)Scratch, String_Lit(" "));
				Assert(StructEntryTokenIter.Token == StartEntryToken);
                
				meta_variable_entry* VariableEntry = Arena_Push_Struct(Parser->Arena, meta_variable_entry);
				VariableEntry->Type = String_Copy((allocator*)Parser->Arena, Type);
				VariableEntry->Name = String_Copy((allocator*)Parser->Arena, Name);
				VariableEntry->IsPointer = IsPointer;
				VariableEntry->IsArray = IsArray;
				VariableEntry->Tags = Tags;
                
				SLL_Push_Back(Struct->Struct->FirstEntry, Struct->Struct->LastEntry, VariableEntry);
				Struct->Struct->EntryCount++;
			} break;
		}
        
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (!Source_Check_Token(TokenIter.Token, '}')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '}' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	Source_Token_Iter_Move_Next(&TokenIter);
	if (Source_Check_Token(TokenIter.Token, SOURCE_TOKEN_TYPE_IDENTIFIER)) {
		StructName = TokenIter.Token->Identifier;
		Source_Token_Iter_Move_Next(&TokenIter);
	}
    
	if (!Source_Check_Token(TokenIter.Token, ';')) {
		Report_Error(Parser->Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ';' after '%.*s'", TokenIter.PrevToken->Identifier.Size, TokenIter.PrevToken->Identifier.Ptr);
		return NULL;
	}
    
	if (String_Is_Empty(StructName)) {
		Report_Error(Parser->Tokenizer->FilePath, StructToken->LineNumber, "Cannot introspect anonymous structure");
		return NULL;
	}
    
	Struct->Token = StructToken;
	Struct->Struct->Name = String_Copy((allocator*)Parser->Arena, StructName);
    
	hashmap* Hashmap = StructOrUnion == SOURCE_TOKEN_TYPE_STRUCT ? &Parser->StructMap : &Parser->UnionMap;
	Hashmap_Add(Hashmap, &Struct->Struct->Name, &Struct);
    
	if(OutStructType) *OutStructType = Struct;
    
	return TokenIter.Token;
}

function b32 Source_Parser_Evaluate(source_parser* Parser) {
	source_tokenizer* Tokenizer = Parser->Tokenizer;
	source_token_list* Tokens = &Parser->Tokenizer->Tokens;
	source_token_iter TokenIter = Source_Begin_Token_Iter(Tokens->First);
    
	while (TokenIter.Token) {
		switch (TokenIter.Token->Type) {
			case SOURCE_TOKEN_TYPE_META: {
				Source_Token_Iter_Move_Next(&TokenIter);
				if (!Source_Check_Token(TokenIter.Token, '(')) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected '(' after 'Meta'");
					return false;
				}
                
				Source_Token_Iter_Move_Next(&TokenIter);
				if (!Source_Check_Token(TokenIter.Token, ')')) {
					Report_Error(Tokenizer->FilePath, TokenIter.PrevToken->LineNumber, "Expected ')' after '('");
					return false;
				}
				
				Source_Token_Iter_Move_Next(&TokenIter);
				
				b32 Done = false;
				while (TokenIter.Token && !Done) {
					switch (TokenIter.Token->Type) {
						case SOURCE_TOKEN_TYPE_STRUCT: {
							arena* Scratch = Scratch_Get();
							source_token* Token = Source_Parse_Struct(Parser, TokenIter.Token, Scratch, NULL);
							Scratch_Release();
                            
							if (!Token) return false;
							TokenIter = Source_Begin_Token_Iter(Token);
							Done = true;
						} break;
                        
						case SOURCE_TOKEN_TYPE_ENUM: {
							source_token* Token = Source_Parse_Enum(Parser, TokenIter.Token);
							if (!Token) return false;
							TokenIter = Source_Begin_Token_Iter(Token);
							Done = true;
						} break;
                        
						case SOURCE_TOKEN_TYPE_TYPEDEF: {
							//Skip with no validation. Can belong on either enums or structs
						} break;
                        
						default: {
							Report_Error(Tokenizer->FilePath, TokenIter.Token->LineNumber, "Unexpected token '%.*s'", TokenIter.Token->Identifier.Size, TokenIter.Token->Identifier.Ptr);
							return false;
						} break;
					}
					
					if (!Done) {
						Source_Token_Iter_Move_Next(&TokenIter);
					}
				}
			} break;
		}
		Source_Token_Iter_Move_Next(&TokenIter);
	}
	
	return true;
}

function b32 Source_Parser_Parse(source_parser* Parser, string FilePath, string Content, arena* ParserArena) {
	b32 Result = false;
    
	source_tokenizer Tokenizer = { 0 };
	Tokenizer.FilePath = FilePath;
    
	arena* Scratch = Scratch_Get();
	if (Source_Parser_Generate_Tokens(&Tokenizer, Scratch, FilePath, Content)) {
		source_token_list* Tokens = &Tokenizer.Tokens;
		if (Tokens->First) {
			Parser->Arena = ParserArena;
			Parser->Tokenizer = &Tokenizer;
			Parser->StructMap = Hashmap_Init((allocator*)ParserArena, sizeof(source_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->UnionMap = Hashmap_Init((allocator*)ParserArena, sizeof(source_struct_type*), sizeof(string), Hash_String, Compare_String);
			Parser->EnumMap = Hashmap_Init((allocator*)ParserArena, sizeof(source_enum_type*), sizeof(string), Hash_String, Compare_String);
			
			Result = Source_Parser_Evaluate(Parser);
		}
	}
	Scratch_Release();
    
	return Result;
}