#ifndef AKON_H
#define AKON_H

typedef struct akon_error akon_error;
struct akon_error {
	string Message;
	akon_error* Next;
};

typedef struct {
	akon_error* First;
	akon_error* Last;
	size_t 		Count;
} akon_errors;

typedef enum {
	AKON_NODE_TYPE_NONE,
	AKON_NODE_TYPE_OBJECT,
	AKON_NODE_TYPE_VALUE,
	AKON_NODE_TYPE_VARIABLE
} akon_node_type;

typedef enum {
	AKON_VALUE_TYPE_NONE,
	AKON_VALUE_TYPE_STRING,
	AKON_VALUE_TYPE_BOOL,
	AKON_VALUE_TYPE_NUMBER
} akon_value_type;

typedef struct akon_node akon_node;
struct akon_node {
	//Hierarchy
	akon_node* Parent;
	akon_node* NextSibling;
	akon_node* PrevSibling;
	akon_node* FirstChild;
	akon_node* LastChild;
	size_t 	   NumChildren;

	//Hash
	akon_node* NextInHash;
	akon_node* PrevInHash;
	u64 	   Hash;

	//Info
	string 			Name;
	akon_node_type  NodeType;
	akon_value_type ValueType;

	union {
		b32 Bool;
		f64 Number;
	};
	string Str;
};

typedef struct {
	akon_node* First;
	akon_node* Last;
} akon_node_slot;

#define AKON_NODE_MAX_SLOT_COUNT 4096
typedef struct {
	arena* 	   	   Arena;
	akon_node* 	   RootNode;
	akon_node_slot Slots[AKON_NODE_MAX_SLOT_COUNT];

	//Errors
	b32 HasErrors;
	sstream_writer ErrorStream;

} akon_node_tree;

typedef enum {
	AKON_TOKEN_TYPE_IDENTIFIER=256,
	AKON_TOKEN_TYPE_VARIABLE=257,
	AKON_TOKEN_COUNT=258
} akon_token_type;

typedef struct akon_token akon_token;
struct akon_token {
	akon_token_type Type;
	string Identifier;
	size_t Line;

	akon_token* Next;
	akon_token* Prev;
};

typedef struct {
	akon_token* First;
	akon_token* Last;
	size_t Count;
} akon_token_list;

typedef struct {
	akon_token* Token;
	akon_token* PrevToken;
	size_t Index;
	size_t Count;
} akon_token_iter;

typedef struct {
	arena* 		    Arena;
	string 		    Content;
	sstream_reader  Stream;
	akon_token_list Tokens;
	akon_token* 	FreeTokens;
	b32 			BeginIdentifier;
	sstream_char 	StartIdentifierChar;
} akon_tokenizer;

export_function akon_node* AKON_Create_Node(akon_node_tree* NodeTree, string Identifier, akon_node* ParentNode, akon_node_type NodeType);
export_function akon_node_tree* AKON_Parse(allocator* Allocator, string Content);
export_function akon_node* AKON_Get_Node(akon_node_tree* NodeTree, akon_node* ParentNode, string Name);
export_function b32 AKON_Node_Is_String_Var(akon_node* Node);
export_function b32 AKON_Node_Is_Number_Var(akon_node* Node);
export_function b32 AKON_Node_Is_Bool_Var(akon_node* Node);
export_function b32 AKON_Node_Read_String(akon_node* Node, string* String);
export_function b32 AKON_Node_Read_F32(akon_node* Node, f32* Value);
export_function b32 AKON_Node_Read_Bool(akon_node* Node, b32* Value);
export_function b32 AKON_Node_Read_String_Var(akon_node* Node, string* String);
export_function b32 AKON_Node_Read_F32_Var(akon_node* Node, f32* Value);
export_function b32 AKON_Node_Read_Bool_Var(akon_node* Node, b32* Value);
export_function b32 AKON_Node_Read_F32_Array(akon_node* Node, f32* Data, size_t Count);
export_function akon_node* AKON_Node_Write_String(akon_node_tree* NodeTree, akon_node* ParentNode, string Value);
export_function akon_node* AKON_Node_Write_F32(akon_node_tree* NodeTree, akon_node* ParentNode, f32 Value);
export_function akon_node* AKON_Node_Write_Bool(akon_node_tree* NodeTree, akon_node* ParentNode, b32 Value);
export_function akon_node* AKON_Node_Write_String_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, string Value);
export_function akon_node* AKON_Node_Write_F32_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, f32 Value);
export_function akon_node* AKON_Node_Write_Bool_Var(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, b32 Value);
export_function akon_node* AKON_Node_Write_F32_Array(akon_node_tree* NodeTree, akon_node* ParentNode, string Name, f32* Data, size_t Count);


#endif