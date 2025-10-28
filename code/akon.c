function void AKON_Report_Error(akon_node_tree* NodeTree, akon_token* Token, const char* Format, ...) {
	arena* Scratch = Scratch_Get();

	if (!NodeTree->ErrorStream.Allocator) {
		NodeTree->ErrorStream = SStream_Writer_Begin((allocator*)NodeTree->Arena);
	}

	va_list List;
	va_start(List, Format);
	string String = String_FormatV((allocator*)Scratch, Format, List);
	va_end(List);

	SStream_Writer_Add_Format(&NodeTree->ErrorStream, "%u: error: %.*s", Token->Line, String.Size, String.Ptr);

	Scratch_Release();
}

export_function akon_node* AKON_Create_Node(akon_node_tree* NodeTree, string Identifier, akon_node* ParentNode, akon_node_type NodeType) {
	size_t AllocationSize = ((Identifier.Size+1) * sizeof(char)) + sizeof(akon_node);
	akon_node* Node = (akon_node*)Arena_Push(NodeTree->Arena, AllocationSize);
	
	char* NamePtr = (char*)(Node + 1);
	NamePtr[Identifier.Size] = 0;
	Memory_Copy(NamePtr, Identifier.Ptr, Identifier.Size * sizeof(char));
	Node->Name = Make_String(NamePtr, Identifier.Size);

	Node->NodeType = NodeType;

	Node->Parent = ParentNode;
	u64 Seed = 0;

	if (ParentNode) {
		Seed = ParentNode->Hash;
		DLL_Push_Back_NP(ParentNode->FirstChild, ParentNode->LastChild, Node, NextSibling, PrevSibling);
		ParentNode->NumChildren++;
	}

	if (!String_Is_Empty(Node->Name)) {
		Node->Hash = U64_Hash_String_With_Seed(Node->Name, Seed);
	} else {
		u64 Random;
		OS_Get_Entropy(&Random, sizeof(u64));
		Node->Hash = U64_Hash_Bytes_With_Seed(&Random, sizeof(u64), Seed);
	}

	u64 SlotMask = AKON_NODE_MAX_SLOT_COUNT - 1;
	u64 SlotIndex = Node->Hash & SlotMask;

	akon_node_slot* Slot = NodeTree->Slots + SlotIndex;
	DLL_Push_Back_NP(Slot->First, Slot->Last, Node, NextInHash, PrevInHash);

	return Node;
}

export_function akon_node* AKON_Create_Object(akon_node_tree* NodeTree, string Identifier, akon_node* ParentNode) {
	akon_node* VariableNode = AKON_Create_Node(NodeTree, Identifier, ParentNode, AKON_NODE_TYPE_VARIABLE);
	akon_node* Result = AKON_Create_Node(NodeTree, String_Empty(), VariableNode, AKON_NODE_TYPE_OBJECT);
	return Result;
}

export_function akon_node* AKON_Get_Node(akon_node_tree* NodeTree, akon_node* ParentNode, string Name) {	
	u64 Hash = 0;
	if (ParentNode) Hash = ParentNode->Hash;
	Hash = U64_Hash_String_With_Seed(Name, Hash);
	u64 SlotMask = AKON_NODE_MAX_SLOT_COUNT - 1;
	u64 SlotIndex = Hash & SlotMask;
	
	akon_node_slot* Slot = NodeTree->Slots + SlotIndex;
	for (akon_node* Node = Slot->First; Node; Node = Node->NextInHash) {
		if (Node->Hash == Hash) {
			return Node;
		}
	}

	return NULL;
}

export_function akon_node* AKON_Get_Object(akon_node_tree* NodeTree, akon_node* ParentNode, string Name) {
	akon_node* Node = AKON_Get_Node(NodeTree, ParentNode, Name);
	if (!Node) return NULL;
	if (Node->NumChildren != 1 || Node->FirstChild->NodeType != AKON_NODE_TYPE_OBJECT) return NULL;
	return Node->FirstChild;
}

function akon_token_iter AKON_Begin_Token_Iter(akon_token_list* List) {
	akon_token_iter Result = { 0 };
	Result.Token = List->First;
	Result.PrevToken = (List->First && List->First->Prev) ? List->First->Prev : NULL;
	Result.Count = List->Count;
	return Result;
}

function b32 AKON_Token_Iter_Move_Next(akon_token_iter* Iter) {
	if (!Iter->Token) return false;

	Iter->PrevToken = Iter->Token;
	Iter->Token = Iter->Token->Next;
	b32 Result = Iter->Index < Iter->Count;
	if (Result) {
		Iter->Index++;
	}
	return Result;
}

function inline b32 AKON_Token_Iter_Is_Valid(akon_token_iter* Iter) {
	return Iter->Token != NULL;
}

function b32 AKON_Token_Iter_Skip_To(akon_token_iter* Iter, akon_token* TargetToken) {
	while (AKON_Token_Iter_Is_Valid(Iter) && Iter->Token != TargetToken) {
		AKON_Token_Iter_Move_Next(Iter);
	}
	return AKON_Token_Iter_Is_Valid(Iter);
}

function b32 AKON_Parse_Value(akon_token_iter* TokenIter, akon_node_tree* NodeTree, akon_node* ParentNode) {
	akon_node* Node = AKON_Create_Node(NodeTree, String_Empty(), ParentNode, AKON_NODE_TYPE_VALUE);

	double NumberValue;
	b32 BoolValue;
	if (Try_Parse_Bool(TokenIter->Token->Identifier, &BoolValue)) {
		Node->ValueType = AKON_VALUE_TYPE_BOOL;
		Node->Bool = BoolValue;
	} else if (Try_Parse_F64(TokenIter->Token->Identifier, &NumberValue)) {
		Node->ValueType = AKON_VALUE_TYPE_NUMBER;
		Node->Number = NumberValue;
	} else {
		Node->ValueType = AKON_VALUE_TYPE_STRING;
	}

	Node->Str = String_Copy((allocator*)NodeTree->Arena, TokenIter->Token->Identifier);
	AKON_Token_Iter_Move_Next(TokenIter);

	return true;
}

function b32 AKON_Parse_Variable(akon_token_iter* TokenIter, akon_node_tree* NodeTree, akon_node* ParentNode);
function b32 AKON_Parse_Object(akon_token_iter* TokenIter, akon_node_tree* NodeTree, akon_node* ParentNode) {

	akon_node* Node = AKON_Create_Node(NodeTree, String_Empty(), ParentNode, AKON_NODE_TYPE_OBJECT);

	if (TokenIter->Token->Type != '{') {
		AKON_Report_Error(NodeTree, TokenIter->Token, "Unexpected token '%.*s'. Expected '{'", TokenIter->Token->Identifier.Size, TokenIter->Token->Identifier.Ptr);
		return false;
	}

	AKON_Token_Iter_Move_Next(TokenIter);
	if (!AKON_Token_Iter_Is_Valid(TokenIter)) {
		AKON_Report_Error(NodeTree, TokenIter->Token, "Unexpected end of object '%.*s'", ParentNode->Name.Size, ParentNode->Name.Ptr);
		return false;
	}

	b32 Quit = false;
	while (AKON_Token_Iter_Is_Valid(TokenIter) && !Quit) {
		switch (TokenIter->Token->Type) {
			case AKON_TOKEN_TYPE_VARIABLE: {
				if (!AKON_Parse_Variable(TokenIter, NodeTree, Node)) {
					return false;
				}
			} break;

			case '}': {
				Quit = true;
			} break;
		}
	}

	AKON_Token_Iter_Move_Next(TokenIter);

	return true;
}

function b32 AKON_Parse_Variable(akon_token_iter* TokenIter, akon_node_tree* NodeTree, akon_node* ParentNode) {
	if (TokenIter->Token->Type != AKON_TOKEN_TYPE_VARIABLE) {
		AKON_Report_Error(NodeTree, TokenIter->Token, "Unexpected token '%.*s'. Expected a variable", TokenIter->Token->Identifier.Size, TokenIter->Token->Identifier.Ptr);
		return false;
	}

	akon_node* VariableNode = AKON_Create_Node(NodeTree, TokenIter->Token->Identifier, ParentNode, AKON_NODE_TYPE_VARIABLE);
	AKON_Token_Iter_Move_Next(TokenIter);

	if (!AKON_Token_Iter_Is_Valid(TokenIter)) {
		AKON_Report_Error(NodeTree, TokenIter->Token, "Unexpected end of variable '%.*s'", VariableNode->Name.Size, VariableNode->Name.Ptr);
		return false;
	}

	b32 Quit = false;
	while (AKON_Token_Iter_Is_Valid(TokenIter) && !Quit) {
		switch (TokenIter->Token->Type) {
			case '{': {
				if (!AKON_Parse_Object(TokenIter, NodeTree, VariableNode)) {
					return false;
				}
			} break;

			case AKON_TOKEN_TYPE_IDENTIFIER: {
				if (!AKON_Parse_Value(TokenIter, NodeTree, VariableNode)) {
					return false;
				}
			} break;

			case AKON_TOKEN_TYPE_VARIABLE:
			case '}': {
				Quit = true;
			} break;
		}
	}

	return true;
}

function akon_token* AKON_Allocate_Token(akon_tokenizer* Tokenizer, akon_token_type Type) {
	akon_token* Token = Tokenizer->FreeTokens;
	if (Token) SLL_Pop_Front(Tokenizer->FreeTokens);
	else Token = Arena_Push_Struct_No_Clear(Tokenizer->Arena, akon_token);
	Memory_Clear(Token, sizeof(akon_token));
	
	Token->Type = Type;
	if (Type < 256) {
		Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, Make_String((const char*)&Type, 1));
	}
	return Token;
}

function void AKON_Add_Token(akon_tokenizer* Tokenizer, akon_token_type Type, size_t c0, size_t c1, size_t Line) {
	akon_token* Token = AKON_Allocate_Token(Tokenizer, Type);
	Token->Identifier = String_Copy((allocator*)Tokenizer->Arena, String_Substr(Tokenizer->Content, c0, c1));
	Token->Line = Line;
	DLL_Push_Back(Tokenizer->Tokens.First, Tokenizer->Tokens.Last, Token);
}

function inline void AKON_Add_Token_Char(akon_tokenizer* Tokenizer, sstream_char Char) {
	AKON_Add_Token(Tokenizer, (akon_token_type)Char.Char, Char.Index, Char.Index + 1, Char.LineIndex);
}


function void AKON_Tokens_Replace(akon_token_list* ReplaceList, akon_token_list* InsertLink) {
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

function void AKON_Tokens_Free(akon_tokenizer* Tokenizer, akon_token_list* FreeList) {
	akon_token* TokenToFree = FreeList->First;
	
	while (TokenToFree) {
		akon_token* TokenToDelete = TokenToFree;
		TokenToFree = TokenToFree->Next;

		TokenToDelete->Next = NULL;
		TokenToDelete->Prev = NULL;
		SLL_Push_Front(Tokenizer->FreeTokens, TokenToDelete);
		if (FreeList->Last->Next == TokenToFree) {
			break;
		}
	}
}

function void AKON_Tokenizer_Begin_Identifier(akon_tokenizer* Tokenizer) {
	sstream_char Char = SStream_Reader_Consume_Char(&Tokenizer->Stream);
	if (!Tokenizer->BeginIdentifier) {
		Tokenizer->StartIdentifierChar = Char;
		Tokenizer->BeginIdentifier = true;
	}
}

function void AKON_Tokenizer_End_Identifier(akon_tokenizer* Tokenizer) {
	if (Tokenizer->BeginIdentifier) {
		size_t Index = SStream_Reader_Get_Index(&Tokenizer->Stream);
		AKON_Add_Token(Tokenizer, AKON_TOKEN_TYPE_IDENTIFIER, Tokenizer->StartIdentifierChar.Index, Index, Tokenizer->StartIdentifierChar.LineIndex);
		Tokenizer->BeginIdentifier = false;
	}
}

function b32 AKON_Generate_Tokens(akon_tokenizer* Tokenizer) {
	sstream_reader* Stream = &Tokenizer->Stream;
	while (SStream_Reader_Is_Valid(Stream)) {
		sstream_char Char = SStream_Reader_Peek_Char(Stream);
		switch (Char.Char) {
			case '{':
			case '}':
			case ',':
			case ':':
			{
				AKON_Tokenizer_End_Identifier(Tokenizer);
				AKON_Add_Token_Char(Tokenizer, SStream_Reader_Consume_Char(Stream));
			} break;

			default: {
				if (Is_Whitespace(Char.Char)) {
					AKON_Tokenizer_End_Identifier(Tokenizer);
					SStream_Reader_Eat_Whitespace(Stream);
				} else {
					AKON_Tokenizer_Begin_Identifier(Tokenizer);
				}
			} break;
		}
	}

	AKON_Tokenizer_End_Identifier(Tokenizer);

	for (akon_token* Token = Tokenizer->Tokens.First; Token; Token = Token->Next) {
		if (Token->Type == AKON_TOKEN_TYPE_IDENTIFIER) {
			if (Token->Next && Token->Next->Type == ':') {
				akon_token* ReplacementToken = AKON_Allocate_Token(Tokenizer, AKON_TOKEN_TYPE_VARIABLE);
				ReplacementToken->Identifier = Token->Identifier;
				ReplacementToken->Line = Token->Line;


				akon_token_list InsertList = { ReplacementToken, ReplacementToken };
				akon_token_list ReplaceList = { Token, Token->Next };
				AKON_Tokens_Replace(&ReplaceList, &InsertList);
				AKON_Tokens_Free(Tokenizer, &ReplaceList);
				
				if (Token == Tokenizer->Tokens.First) {
					Tokenizer->Tokens.First = ReplacementToken;
				}
				
				Token = ReplacementToken;
			}
		}
	}

	return true;
}

function akon_node* AKON_Parse_Internal(akon_node_tree* NodeTree, arena* Scratch, string Content) {
	akon_node* RootNode = NULL;

	akon_tokenizer Tokenizer = { 0 };
	Tokenizer.Content = Content;
	Tokenizer.Arena = Scratch;
	Tokenizer.Stream = SStream_Reader_Begin(Content);
	if (AKON_Generate_Tokens(&Tokenizer)) {
		akon_token_iter TokenIter = AKON_Begin_Token_Iter(&Tokenizer.Tokens);
		
		RootNode = AKON_Create_Node(NodeTree, String_Lit("Root"), NULL, AKON_NODE_TYPE_OBJECT);
		while (AKON_Token_Iter_Is_Valid(&TokenIter)) {
			if (!AKON_Parse_Variable(&TokenIter, NodeTree, RootNode)) {
				return NULL;
			}
		}
	}
	return RootNode;
}

function void AKON_Free_Node_Tree(akon_node_tree* NodeTree) {
	if (NodeTree && NodeTree->Arena) {
		Arena_Delete(NodeTree->Arena);
	}
}

export_function akon_node_tree* AKON_Allocate_Node_Tree(allocator* Allocator) {
	arena* Arena = Arena_Create_With_Allocator(Allocator);
	akon_node_tree* NodeTree = Arena_Push_Struct(Arena, akon_node_tree);
	NodeTree->Arena = Arena;
	NodeTree->RootNode = AKON_Create_Node(NodeTree, String_Lit("Root"), NULL, AKON_NODE_TYPE_OBJECT);
	return NodeTree;
}

export_function akon_node_tree* AKON_Parse(allocator* Allocator, string Content) {
	arena* Arena = Arena_Create_With_Allocator(Allocator);
	akon_node_tree* NodeTree = Arena_Push_Struct(Arena, akon_node_tree);
	NodeTree->Arena = Arena;

	arena* Scratch = Scratch_Get();
	NodeTree->RootNode = AKON_Parse_Internal(NodeTree, Scratch, Content);
	Scratch_Release();

	NodeTree->HasErrors = NodeTree->RootNode == NULL;
	return NodeTree;
}

function void AKON_Generate_String_For_Node(akon_node_tree* NodeTree, akon_node* Node, sstream_writer* Stream, int Level, arena* Scratch) {
	Assert(Node->NodeType == AKON_NODE_TYPE_VARIABLE);
	size_t TabCount = Level;

	for (size_t i = 0; i < TabCount; i++) SStream_Writer_Add_Tab(Stream);
	SStream_Writer_Add_Format(Stream, "%.*s: ", Node->Name.Size, Node->Name.Ptr);

	akon_node* Child = Node->FirstChild;
	for (size_t i = 0; i < Node->NumChildren; i++) {
		switch (Child->NodeType) {
			case AKON_NODE_TYPE_VALUE: {
				switch (Child->ValueType) {
					case AKON_VALUE_TYPE_BOOL: {
						SStream_Writer_Add(Stream, String_From_Bool(Child->Bool));
					} break;

					case AKON_VALUE_TYPE_NUMBER: {
						SStream_Writer_Add(Stream, String_From_F64((allocator*)Scratch, Child->Number));
					} break;

					case AKON_VALUE_TYPE_STRING: {
						SStream_Writer_Add(Stream, Child->Str);
					} break;

					Invalid_Default_Case;
				}

				SStream_Writer_Add_Space(Stream);
			} break;

			case AKON_NODE_TYPE_OBJECT: {
				akon_node* Object = Child;
				SStream_Writer_Add_Char(Stream, '{');

				if (Object->NumChildren) {
					SStream_Writer_Add_Newline(Stream);

					akon_node* ObjectEntryNode = Object->FirstChild;
					for (size_t j = 0; j < Object->NumChildren; j++) {
						AKON_Generate_String_For_Node(NodeTree, ObjectEntryNode, Stream, Level+1, Scratch);
						if(j != (Object->NumChildren-1)) SStream_Writer_Add_Newline(Stream);
						ObjectEntryNode = ObjectEntryNode->NextSibling;
					}

					SStream_Writer_Add_Newline(Stream);
					for (size_t i = 0; i < TabCount; i++) SStream_Writer_Add_Tab(Stream);
				} else {
				}
				SStream_Writer_Add_Char(Stream, '}');
				SStream_Writer_Add_Space(Stream);
			} break;

			Invalid_Default_Case;
		}

		Child = Child->NextSibling;
	}
}

export_function string AKON_Generate_String(akon_node_tree* NodeTree, allocator* Allocator) {
	arena* Scratch = Scratch_Get();
	sstream_writer Stream = SStream_Writer_Begin((allocator*)Scratch);

	akon_node* Node = NodeTree->RootNode->FirstChild;
	for (size_t i = 0; i < NodeTree->RootNode->NumChildren; i++) {
		AKON_Generate_String_For_Node(NodeTree, Node, &Stream, 0, Scratch);
		if(i != (NodeTree->RootNode->NumChildren-1)) SStream_Writer_Add_Newline(&Stream);
		Node = Node->NextSibling;
	}

	string Result = SStream_Writer_Join(&Stream, Allocator, String_Empty());
	Scratch_Release();
	return Result;
}

export_function b32 AKON_Node_Is_String_Var(akon_node* Node) {
	b32 Result = ((Node->NodeType == AKON_NODE_TYPE_VARIABLE) && 
				  (Node->NumChildren == 1) && 
				  (Node->FirstChild->NodeType == AKON_NODE_TYPE_VALUE) && 
				  (Node->FirstChild->ValueType == AKON_VALUE_TYPE_STRING));
	return Result;
}

export_function b32 AKON_Node_Is_Number_Var(akon_node* Node) {
	b32 Result = ((Node->NodeType == AKON_NODE_TYPE_VARIABLE) && 
				  (Node->NumChildren == 1) && 
				  (Node->FirstChild->NodeType == AKON_NODE_TYPE_VALUE) && 
				  (Node->FirstChild->ValueType == AKON_VALUE_TYPE_NUMBER));
	return Result;
}

export_function b32 AKON_Node_Is_Bool_Var(akon_node* Node) {
	b32 Result = ((Node->NodeType == AKON_NODE_TYPE_VARIABLE) && 
				  (Node->NumChildren == 1) && 
				  (Node->FirstChild->NodeType == AKON_NODE_TYPE_VALUE) && 
				  (Node->FirstChild->ValueType == AKON_VALUE_TYPE_BOOL));
	return Result;
}

export_function b32 AKON_Node_Read_String(akon_node* Node, string* String) {
	if (Node->NodeType != AKON_NODE_TYPE_VALUE || Node->ValueType != AKON_VALUE_TYPE_STRING) return false;
	*String = Node->Str;
	return true;
}

export_function b32 AKON_Node_Read_F32(akon_node* Node, f32* Value) {
	if (Node->NodeType != AKON_NODE_TYPE_VALUE || Node->ValueType != AKON_VALUE_TYPE_NUMBER) return false;
	*Value = (f32)Node->Number;
	return true;
}

export_function b32 AKON_Node_Read_Bool(akon_node* Node, b32* Value) {
	if (Node->NodeType != AKON_NODE_TYPE_VALUE || Node->ValueType != AKON_VALUE_TYPE_BOOL) return false;
	*Value = (b32)Node->Bool;
	return true;
}

export_function b32 AKON_Node_Read_String_Var(akon_node* Node, string* String) {
	if (Node->NodeType != AKON_NODE_TYPE_VARIABLE || Node->NumChildren != 1) return false;
	return AKON_Node_Read_String(Node->FirstChild, String);
}

export_function b32 AKON_Node_Read_F32_Var(akon_node* Node, f32* Value) {
	if (Node->NodeType != AKON_NODE_TYPE_VARIABLE || Node->NumChildren != 1) return false;
	return AKON_Node_Read_F32(Node->FirstChild, Value);
}

export_function b32 AKON_Node_Read_Bool_Var(akon_node* Node, b32* Value) {
	if (Node->NodeType != AKON_NODE_TYPE_VARIABLE || Node->NumChildren != 1) return false;
	return AKON_Node_Read_Bool(Node->FirstChild, Value);
}

export_function b32 AKON_Node_Read_F32_Array(akon_node* Node, f32* Data, size_t Count) {
	if (Node->NumChildren != Count) return false;

	akon_node* ValueNode = Node->FirstChild;
	for (size_t i = 0; i < Count; i++) {
		if (!AKON_Node_Read_F32(ValueNode, &Data[i])) return false;
		ValueNode = ValueNode->NextSibling;
	}

	return true;
}

export_function akon_node* AKON_Node_Write_String(akon_node_tree* NodeTree, akon_node* ParentNode, string Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, String_Empty(), ParentNode, AKON_NODE_TYPE_VALUE);
	Node->ValueType = AKON_VALUE_TYPE_STRING;
	Node->Str = String_Copy((allocator*)NodeTree->Arena, Value);
	return Node;
}

export_function akon_node* AKON_Node_Write_F32(akon_node_tree* NodeTree, akon_node* ParentNode, f32 Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, String_Empty(), ParentNode, AKON_NODE_TYPE_VALUE);
	Node->ValueType = AKON_VALUE_TYPE_NUMBER;
	Node->Number = Value;
	Node->Str = String_Format((allocator*)NodeTree->Arena, "%f", Value);
	return Node;
}

export_function akon_node* AKON_Node_Write_Bool(akon_node_tree* NodeTree, akon_node* ParentNode, b32 Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, String_Empty(), ParentNode, AKON_NODE_TYPE_VALUE);
	Node->ValueType = AKON_VALUE_TYPE_BOOL;
	Node->Bool = Value;
	Node->Str = Value ? String_Lit("true") : String_Lit("false");
	return Node;
}

export_function akon_node* AKON_Node_Write_String_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, string Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, Name, ParentNode, AKON_NODE_TYPE_VARIABLE);
	if (!AKON_Node_Write_String(NodeTree, Node, Value)) return NULL;
	return Node;
}

export_function akon_node* AKON_Node_Write_F32_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, f32 Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, Name, ParentNode, AKON_NODE_TYPE_VARIABLE);
	if (!AKON_Node_Write_F32(NodeTree, Node, Value)) return NULL;
	return Node;
}

export_function akon_node* AKON_Node_Write_Bool_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, b32 Value) {
	akon_node* Node = AKON_Create_Node(NodeTree, Name, ParentNode, AKON_NODE_TYPE_VARIABLE);
	if (!AKON_Node_Write_Bool(NodeTree, Node, Value)) return NULL;
	return Node;
}

export_function akon_node* AKON_Node_Write_F32_Array(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, f32* Data, size_t Count) {
	akon_node* Node = AKON_Create_Node(NodeTree, Name, ParentNode, AKON_NODE_TYPE_VARIABLE);
	for (size_t i = 0; i < Count; i++) {
		if (!AKON_Node_Write_F32(NodeTree, Node, Data[i])) return NULL;
	}
	return Node;
}