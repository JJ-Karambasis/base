#ifndef SOURCE_PARSER_H
#define SOURCE_PARSER_H

typedef enum {
	SOURCE_TOKEN_TYPE_IDENTIFIER=256,
	SOURCE_TOKEN_TYPE_STRUCT=257,
	SOURCE_TOKEN_TYPE_TYPEDEF=258,
	SOURCE_TOKEN_TYPE_META=259,
	SOURCE_TOKEN_TYPE_ENUM=260,
	SOURCE_TOKEN_TYPE_UNION=261,
	SOURCE_TOKEN_TYPE_SINGLE_LINE_COMMENT=262,
	SOURCE_TOKEN_TYPE_MULTI_LINE_COMMENT_START=263,
	SOURCE_TOKEN_TYPE_MULTI_LINE_COMMENT_END=264,
	SOURCE_TOKEN_TYPE_COUNT=265
} source_token_type;

typedef struct source_token source_token;
struct source_token {
	source_token_type Type;
	string 	   		  Identifier;
	size_t 	   		  c0;
	size_t 			  LineNumber;
	source_token*     Next;
	source_token*     Prev;
};

typedef struct {
	source_token* First;
	source_token* Last;
	size_t 		  Count;
} source_token_list;

typedef struct {
	arena* 		      Arena;
	string 			  FilePath;
	string 		      Content;
	sstream_reader    Stream;
	source_token_list Tokens;
	source_token* 	  FreeTokens;
	b32 		      BeginIdentifier;
	sstream_char      StartIdentifierChar;
} source_tokenizer;

typedef struct {
	arena*  		  Arena;
	source_tokenizer* Tokenizer;
	hashmap 		  StructMap;
	hashmap 		  UnionMap;
	hashmap 		  EnumMap;
	size_t 			  GeneratedIndex;
} source_parser;

typedef struct meta_struct_type meta_struct_type;
typedef struct meta_enum_type meta_enum_type;
typedef struct {
	source_token* Token;
	meta_struct_type* Struct;
} source_struct_type;

typedef struct {
	source_token* Token;
	meta_enum_type* Enum;
} source_enum_type;

typedef struct {
	source_token* Token;
	source_token* PrevToken;
	source_token* NextToken;
} source_token_iter;

#endif