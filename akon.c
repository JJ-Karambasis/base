function akon_context* AKON_Context_Create() {
	arena* Arena = Arena_Create();
	akon_context* Context = Arena_Push_Struct(Arena, akon_context);
	Context->Arena = Arena;
	return Context;
}

function void AKON_Context_Delete(akon_context* Context) {
	if (Context && Context->Arena) {
		Arena_Delete(Context->Arena);
	}
}

function akon_token* AKON_Allocate_Token(akon_tokenizer* Tokenizer, akon_token_type Type) {
	akon_token* Token = Tokenizer->FreeTokens;
	if (Token) SLL_Pop_Front(Tokenizer->FreeTokens);
	else Token = Arena_Push_Struct_No_Clear(Tokenizer->Arena, akon_token);
	Memory_Clear(Token, sizeof(akon_token));
	
	Token->Type = Type;
	if (Type < 256) {
		Token->Str = String_Copy((allocator*)Tokenizer->Arena, Make_String((const char*)&Type, 1));
	}
	return Token;
}

function void AKON_Add_Token(akon_tokenizer* Tokenizer, akon_token_type Type, size_t c0, size_t c1) {
	akon_token* Token = AKON_Allocate_Token(Tokenizer, Type);
	Token->Str = String_Copy((allocator*)Tokenizer->Arena, String_Substr(Tokenizer->Content, c0, c1));
	DLL_Push_Back(Tokenizer->Tokens.First, Tokenizer->Tokens.Last, Token);
}

function inline void AKON_Add_Token_Char(akon_tokenizer* Tokenizer, sstream_char Char) {
	AKON_Add_Token(Tokenizer, Char.Char, Char.Index, Char.Index + 1);
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
		AKON_Add_Token(Tokenizer, AKON_TOKEN_TYPE_IDENTIFIER, Tokenizer->StartIdentifierChar.Index, Index);
		Tokenizer->BeginIdentifier = false;
	}
}

function b32 AKON_Tokenize(akon_tokenizer* Tokenizer) {
	sstream_reader* Stream = &Tokenizer->Stream;
	while (SStream_Reader_Is_Valid(Stream)) {
		sstream_char Char = SStream_Reader_Peek_Char(Stream);
		switch (Char.Char) {
			case '{':
			case '}':
			case ',':
			case ':':
			case '[':
			case ']':
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

	return true;
}

function akon_node* AKON_Create_Node(akon_context* Context, string NodeName, akon_node* Parent, akon_node_type Type) {
	akon_node* Node = Context->FirstFreeNode;
	if (Node) SLL_Pop_Front_N(Context->FirstFreeNode, NextSibling);
	else Node = Arena_Push_Struct_No_Clear(Context->Arena, akon_node);
	Memory_Clear(Node, sizeof(akon_node));

	Node->Name = String_Copy((allocator*)Context->Arena, NodeName);
	Node->Type = Type;

	Node->Parent = Parent;
	u64 Seed = 0;
	if (Node->Parent) {
		Seed = Node->Parent->Hash;
		DLL_Push_Back_NP(Parent->FirstChild, Parent->LastChild, Node, NextSibling, PrevSibling);
		Parent->ChildCount++;
	}	

	Node->Hash = U64_Hash_String_With_Seed(Node->Name, Seed);
	u64 SlotMask = AKON_NODE_MAX_SLOT_COUNT - 1;
	u64 SlotIndex = Node->Hash & SlotMask;

	akon_node_slot* Slot = Context->Slots + SlotIndex;
	DLL_Push_Back_NP(Slot->First, Slot->Last, Node, NextInHash, PrevInHash);

	return Node;
}

function akon_node* AKON_Get_Node(akon_context* Context, akon_node* ParentNode, string Name) {
	u64 Hash = 0;
	if (ParentNode) Hash = ParentNode->Hash;
	Hash = U64_Hash_String_With_Seed(Name, Hash);
	u64 SlotMask = AKON_NODE_MAX_SLOT_COUNT - 1;
	u64 SlotIndex = Hash & SlotMask;
	
	akon_node_slot* Slot = Context->Slots + SlotIndex;
	for (akon_node* Node = Slot->First; Node; Node = Node->NextInHash) {
		if (Node->Hash == Hash) {
			return Node;
		}
	}

	return NULL;
}

function akon_token* AKON_Parse_Value(akon_parser* Parser, akon_token* Token, akon_node* ValueNode) {
	akon_context* Context = Parser->Context;

	double NumberValue;
	b32 BoolValue;
	if (Try_Parse_Bool(Token->Str, &BoolValue)) {
		ValueNode->ValueType = AKON_VALUE_TYPE_BOOL;
		ValueNode->Bool = BoolValue;
	} else if (Try_Parse_Number(Token->Str, &NumberValue)) {
		ValueNode->ValueType = AKON_VALUE_TYPE_NUMBER;
		ValueNode->Number = NumberValue;
	} else {
		ValueNode->ValueType = AKON_VALUE_TYPE_STRING;
	}

	ValueNode->Str = String_Copy((allocator*)Context->Arena, Token->Str);

	return Token->Next;
}

function akon_token* AKON_Parse_Object(akon_parser* Parser, akon_token* Token, akon_node* ObjectNode);
function akon_token* AKON_Parse_Array(akon_parser* Parser, akon_token* Token, akon_node* ArrayNode) {
	Assert(Token->Type == '[' && ArrayNode->Type == AKON_NODE_TYPE_ARRAY);
	Token = Token->Next;

	akon_context* Context = Parser->Context;

	u32 Index = 0;
	while (Token) {
		arena* Scratch = Scratch_Get();
		string ChildName = String_Format((allocator*)Scratch, "%.*s_array_%u", ArrayNode->Name.Size, ArrayNode->Name.Ptr, Index);
		switch (Token->Type) {
			case '{': {
				akon_node* ChildNode = AKON_Create_Node(Context, ChildName, ArrayNode, AKON_NODE_TYPE_OBJECT);
				Scratch_Release();
				Token = AKON_Parse_Object(Parser, Token, ChildNode);
			} break;

			case AKON_TOKEN_TYPE_IDENTIFIER: {
				akon_node* ChildNode = AKON_Create_Node(Context, ChildName, ArrayNode, AKON_NODE_TYPE_VALUE);
				Scratch_Release();
				Token = AKON_Parse_Value(Parser, Token, ChildNode);
			} break;

			default: {
				Scratch_Release();
				//todo: Diagnostic and error logging
				return NULL;
			} break;
		}

		if (!Token) {
			//todo: Diagnostic and error logging
			return NULL;
		}

		if (Token->Type == ',') {
			//Continues processing the array
			Token = Token->Next;
			Index++;
		} else if (Token->Type == ']') {
			//Finishes processing the array
			break;
		} else {
			//todo: Diagnostic and error logging
			return NULL;	
		}
	}

	if (!Token || Token->Type != ']') {
		//todo: Diagnostic and error logging
		return NULL;
	}

	return Token->Next;
}

function akon_token* AKON_Parse_Object_Internal(akon_parser* Parser, akon_token* Token, akon_node* ObjectNode) {
	Assert(ObjectNode->Type == AKON_NODE_TYPE_OBJECT);
	akon_context* Context = Parser->Context;
	while(Token) {
		if (Token->Type != AKON_TOKEN_TYPE_IDENTIFIER) {
			//todo: Diagnostic and error logging
			return NULL;
		}

		if (!Token->Next || Token->Next->Type != ':') {
			//todo: Diagnostic and error logging
			return NULL;
		}

		string ChildName = Token->Str;

		Token = Token->Next->Next;
		if (!Token) {
			//todo: Diagnostic and error logging
			return NULL;
		}

		switch (Token->Type) {
			case '{': {
				akon_node* ChildNode = AKON_Create_Node(Context, ChildName, ObjectNode, AKON_NODE_TYPE_OBJECT);
				Token = AKON_Parse_Object(Parser, Token, ChildNode);
			} break;

			case '[': {
				akon_node* ChildNode = AKON_Create_Node(Context, ChildName, ObjectNode, AKON_NODE_TYPE_ARRAY);
				Token = AKON_Parse_Array(Parser, Token, ChildNode);
			} break;

			case AKON_TOKEN_TYPE_IDENTIFIER: {
				akon_node* ChildNode = AKON_Create_Node(Context, ChildName, ObjectNode, AKON_NODE_TYPE_VALUE);
				Token = AKON_Parse_Value(Parser, Token, ChildNode);
			} break;
		}

		if (!Token) {
			//This can only be valid if the node we are processing is the root node
			if (ObjectNode != Parser->RootNode) {
				//todo: Diagnostic and error logging
				return NULL;
			}

			//Return a valid pointer to prevent an error from being processed
			return Parser->Tokenizer->Tokens.First;
		}

		if (Token->Type == ',') {
			//Continues processing the object
			Token = Token->Next;
		} else if (Token->Type == '}') {
			//Finish processing the object
			break;
		} else {
			//todo: Diagnostic and error logging
		}
	}

	return Token;
}

function akon_token* AKON_Parse_Object(akon_parser* Parser, akon_token* Token, akon_node* ObjectNode) {
	Assert(Token->Type == '{' && ObjectNode->Type == AKON_NODE_TYPE_OBJECT);
	Token = AKON_Parse_Object_Internal(Parser, Token->Next, ObjectNode);
	if (!Token) return NULL;
	Assert(Token->Type == '}');
	return Token->Next;
}

function akon_node* AKON_Parse_Internal(akon_context* Context, string Contents, arena* Scratch) {
	akon_tokenizer Tokenizer = {
		.Arena = Scratch,
		.Content = Contents,
		.Stream = SStream_Reader_Begin(Contents)
	};

	if (!AKON_Tokenize(&Tokenizer)) {
		return NULL;
	}

	akon_node* RootNode = AKON_Create_Node(Context, String_Lit("Root"), NULL, AKON_NODE_TYPE_OBJECT);
	akon_parser Parser = {
		.Context = Context,
		.Tokenizer = &Tokenizer,
		.RootNode = RootNode
	};

	if (!AKON_Parse_Object_Internal(&Parser, Tokenizer.Tokens.First, RootNode)) {
		return NULL;
	}

	return RootNode;
}

function akon_node* AKON_Parse(akon_context* Context, string Contents) {
	arena* Scratch = Scratch_Get();
	akon_node* Node = AKON_Parse_Internal(Context, Contents, Scratch);
	Scratch_Release();
	return Node;
}

function const char G_TabString[16] = {
	'\t', '\t', '\t', '\t',
	'\t', '\t', '\t', '\t',
	'\t', '\t', '\t', '\t',
	'\t', '\t', '\t', '\t'
};

function void AKON_Serialize_Value(sstream_writer* Stream, akon_node* Node, size_t Level, b32 IsNewline, b32 WriteName, b32 WriteTab, b32 IsSpace) {
	if (WriteTab) {
		Assert(Level < Array_Count(G_TabString) && Node->Type == AKON_NODE_TYPE_VALUE);
		if (Level > 0) {
			SStream_Writer_Add(Stream, Make_String(G_TabString, Level));
		}
	}

	if (WriteName) {
		SStream_Writer_Add_Format(Stream, "%.*s: ", Node->Name.Size, Node->Name.Ptr);
	}

	switch (Node->ValueType) {
		case AKON_VALUE_TYPE_BOOL: {
			SStream_Writer_Add(Stream, Node->Bool ? String_Lit("true") : String_Lit("false"));
		} break;

		case AKON_VALUE_TYPE_NUMBER: {
			SStream_Writer_Add_Format(Stream, "%f", Node->Number);
		} break;

		case AKON_VALUE_TYPE_STRING: {
			SStream_Writer_Add(Stream, Node->Str);
		} break;

		Invalid_Default_Case;
	}

	if (Node->Parent->LastChild != Node) {
		SStream_Writer_Add(Stream, String_Lit(","));
		if (IsSpace) {
			SStream_Writer_Add(Stream, String_Lit(" "));
		}
	}

	if (IsNewline) {
		SStream_Writer_Add(Stream, String_Lit("\n"));
	}
}

function void AKON_Serialize_Array(sstream_writer* Stream, akon_node* Node, size_t Level);
function void AKON_Serialize_Object(sstream_writer* Stream, akon_node* Node, size_t Level, b32 WriteName) {
	Assert(Level < Array_Count(G_TabString) && Node->Type == AKON_NODE_TYPE_OBJECT);
	if (Level > 0) {
		SStream_Writer_Add(Stream, Make_String(G_TabString, Level));
	}

	if (WriteName) {
		SStream_Writer_Add_Format(Stream, "%.*s: ", Node->Name.Size, Node->Name.Ptr);
	}

	SStream_Writer_Add(Stream, String_Lit("{\n"));
	for (akon_node* Child = Node->FirstChild; Child; Child = Child->NextSibling) {
		switch (Child->Type) {
			case AKON_NODE_TYPE_OBJECT: {
				AKON_Serialize_Object(Stream, Child, Level + 1, true);
			} break;

			case AKON_NODE_TYPE_ARRAY: {
				AKON_Serialize_Array(Stream, Child, Level + 1);
			} break;

			case AKON_NODE_TYPE_VALUE: {
				AKON_Serialize_Value(Stream, Child, Level + 1, true, true, true, false);
			} break;
		}
	}

	if (Level > 0) {
		SStream_Writer_Add(Stream, Make_String(G_TabString, Level));
	}

	SStream_Writer_Add(Stream, String_Lit("}"));
	if (Node->Parent->LastChild != Node) {
		SStream_Writer_Add(Stream, String_Lit(","));
	}
}

function void AKON_Serialize_Array(sstream_writer* Stream, akon_node* Node, size_t Level) {
	Assert(Level < Array_Count(G_TabString) && Node->Type == AKON_NODE_TYPE_ARRAY);
	if (Level > 0) {
		SStream_Writer_Add(Stream, Make_String(G_TabString, Level));
	}

	SStream_Writer_Add_Format(Stream, "%.*s: [", Node->Name.Size, Node->Name.Ptr);

	for (akon_node* Child = Node->FirstChild; Child; Child = Child->NextSibling) {
		Assert(Child->Type != AKON_NODE_TYPE_ARRAY);
		switch (Child->Type) {
			case AKON_NODE_TYPE_OBJECT: {
				SStream_Writer_Add(Stream, String_Lit("\n"));
				AKON_Serialize_Object(Stream, Child, Level + 1, false);
			} break;

			case AKON_NODE_TYPE_VALUE: {
				AKON_Serialize_Value(Stream, Child, Level, false, false, false, true);
			} break;
		}
	}

	if (Node->LastChild && Node->LastChild->Type == AKON_NODE_TYPE_OBJECT) {
		SStream_Writer_Add(Stream, String_Lit("\n"));

		if (Level > 0) {
			SStream_Writer_Add(Stream, Make_String(G_TabString, Level));
		}
	}

	SStream_Writer_Add(Stream, String_Lit("]"));
	if (Node->Parent->LastChild != Node) {
		SStream_Writer_Add(Stream, String_Lit(","));
	}
	SStream_Writer_Add(Stream, String_Lit("\n"));
}

function string AKON_Serialize(akon_node* Root, allocator* Allocator) {
	arena* Scratch = Scratch_Get();
	sstream_writer Stream = Begin_Stream_Writer((allocator*)Scratch);
	for (akon_node* Node = Root->FirstChild; Node; Node = Node->NextSibling) {
		switch (Node->Type) {
			case AKON_NODE_TYPE_OBJECT: {
				AKON_Serialize_Object(&Stream, Node, 0, true);
			} break;

			case AKON_NODE_TYPE_ARRAY: {
				AKON_Serialize_Array(&Stream, Node, 0);
			} break;

			case AKON_NODE_TYPE_VALUE: {
				AKON_Serialize_Value(&Stream, Node, 0, true, true, true, false);
			} break;

			Invalid_Default_Case;
		}
	}
	string Result = SStream_Writer_Join(&Stream, Allocator, String_Empty());
	Scratch_Release();
	return Result;
}