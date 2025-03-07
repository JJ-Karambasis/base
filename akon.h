#ifndef AKON_H
#define AKON_H

typedef struct akon_node akon_node;

typedef enum {
	AKON_TOKEN_TYPE_IDENTIFIER=256,
	AKON_TOKEN_TYPE_COUNT=257
} akon_token_type;

typedef struct akon_token akon_token;
struct akon_token {
	akon_token_type Type;
	string 			Str;
	akon_token* 	Next;
	akon_token*     Prev;
};

typedef enum {
	AKON_NODE_TYPE_NONE,
	AKON_NODE_TYPE_OBJECT,
	AKON_NODE_TYPE_ARRAY,
	AKON_NODE_TYPE_VALUE
} akon_node_type;

typedef enum {
	AKON_VALUE_TYPE_NONE,
	AKON_VALUE_TYPE_STRING,
	AKON_VALUE_TYPE_BOOL,
	AKON_VALUE_TYPE_NUMBER
} akon_value_type;

struct akon_node {
	akon_node* Parent;
	akon_node* FirstChild;
	akon_node* LastChild;
	akon_node* NextSibling;
	akon_node* PrevSibling;
	size_t 	   ChildCount;

	string 		   Name;
	akon_node_type Type;

	akon_value_type ValueType;
	string Str;
	union {
		b32 Bool;
		f64 Number;
	};
};

typedef struct {
	arena* 	   Arena;
	akon_node* FirstFreeNode;
} akon_context;

typedef struct {
	akon_token* First;
	akon_token* Last;
} akon_token_list;

typedef struct {
	arena* Arena;
	string Content;
	sstream_reader Stream;
	akon_token_list Tokens;
	akon_token* FreeTokens;
	b32 BeginIdentifier;
	sstream_char StartIdentifierChar;
} akon_tokenizer;

typedef struct {
	akon_context* Context;
	akon_tokenizer* Tokenizer;
	akon_node* RootNode;
} akon_parser;

#endif