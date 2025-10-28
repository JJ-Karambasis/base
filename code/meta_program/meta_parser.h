#ifndef META_PARSER_H
#define META_PARSER_H

typedef enum {
	META_TOKEN_TYPE_IDENTIFIER=256,
	META_TOKEN_TYPE_STRUCT=257,
	META_TOKEN_TYPE_VARIABLE_ENTRY=258,
	META_TOKEN_TYPE_MACRO=259,
	META_TOKEN_TYPE_FUNCTION=260,
	META_TOKEN_TYPE_RETURN_ARROW=261,
	META_TOKEN_TYPE_FOR=262,
	META_TOKEN_TYPE_VARIABLE_ENTRY_NAME=263,
	META_TOKEN_TYPE_VARIABLE_ENTRY_TYPE=264,
	META_TOKEN_TYPE_REPLACEMENT=265,
	META_TOKEN_TYPE_GLOBAL=266,
	META_TOKEN_TYPE_IF=267,
	META_TOKEN_TYPE_ENUM=268,
	META_TOKEN_TYPE_ENUM_ENTRY=269,
	META_TOKEN_TYPE_PREPROCESSOR=270,
	META_TOKEN_TYPE_ELSE=271,
	META_TOKEN_TYPE_COUNT=272
} meta_token_type;
typedef struct meta_token meta_token;

enum { 
	META_TOKEN_FLAG_NONE,
	META_TOKEN_FLAG_PASCAL = (1 << 0),
	META_TOKEN_FLAG_HAS_PREV_IDENTIFIER = (1 << 1),
	META_TOKEN_FLAG_HAS_NEXT_IDENTIFIER = (1 << 2),
	META_TOKEN_FLAG_UPPER = (1 << 3),
	META_TOKEN_FLAG_LOWER = (1 << 4)
};
typedef u32 meta_token_flag;

enum {
	META_CONTAINS_TAG_PREDICATE,
	META_NOT_CONTAINS_TAG_PREDICATE,
	META_IS_STRUCT_PREDICATE,
	META_IS_UNION_PREDICATE,
	META_IS_ENUM_PREDICATE,
	META_IS_ARRAY_PREDICATE,
	META_IS_TYPE_PREDICATE,
	META_IS_NOT_TYPE_PREDICATE,
	META_IS_NAME_PREDICATE,
	META_IS_NOT_NAME_PREDICATE,
	META_PREDICATE_COUNT
};
typedef u32 meta_predicate;

enum {
	META_GET_FUNCTIONS_WITH_TAG,
	META_ROUTINE_COUNT
};
typedef u32 meta_routine;

struct meta_token {
	meta_token_type Type;
	string 	   		Identifier;
	meta_token_flag Flags;
	size_t 	   		c0;
	size_t 			LineNumber;
	size_t 			StackUpwardOffset;
	string_list 	Parameters;
	meta_token*     Next;
	meta_token*     Prev;
};

typedef struct {
	meta_token* First;
	meta_token* Last;
	size_t Count;
} meta_token_list;

typedef struct {
	arena* 		    Arena;
	string 			FilePath;
	string 		    Content;
	sstream_reader  Stream;
	meta_token_list Tokens;
	meta_token* 	FreeTokens;
	b32 		    BeginIdentifier;
	sstream_char    StartIdentifierChar;
} meta_tokenizer;

typedef struct meta_parameter meta_parameter;
struct meta_parameter {
	string 	   		Type;
	string 	   		Name;
	meta_parameter* Next;
};

typedef struct {
	string 	   		Name;
	meta_parameter* FirstParameter;
	meta_parameter* LastParameter;
	size_t 	   		ParameterCount;
	meta_token_list CodeTokens;
} meta_macro;

typedef struct meta_tag meta_tag;
struct meta_tag {
	string Name;
	string Value;
	meta_tag* Next;
};

typedef struct {
	meta_tag* First;
	meta_tag* Last;
	size_t Count;
} meta_tag_list;

typedef struct meta_variable_entry meta_variable_entry;
struct meta_variable_entry {
	string 				 Type;
	string 				 Name;
	b32 				 IsPointer;
	b32 				 IsArray;
	meta_tag_list 		 Tags;
	meta_variable_entry* Next;
};

struct meta_struct_type {
	string 		    	 Name;
	meta_variable_entry* FirstEntry;
	meta_variable_entry* LastEntry;
	size_t 		    	 EntryCount;
};

typedef struct meta_enum_entry meta_enum_entry;
struct meta_enum_entry {
	string 			 Name;
	string 			 OptionalValue;
	meta_tag_list    Tags;
	meta_enum_entry* Next;
};

struct meta_enum_type {
	string 			 Name;
	string 			 Type;
	b32 			 IsTrueName;
	meta_enum_entry* FirstEntry;
	meta_enum_entry* LastEntry;
	size_t 			 EntryCount;
};

typedef struct {
	string 	   		Name;
	meta_parameter* FirstParameter;
	meta_parameter* LastParameter;
	size_t 	   		ParameterCount;
	string 	   		ReturnType;
	meta_token_list CodeTokens;
	meta_tag_list   Tags;
} meta_function_type;

typedef struct {
	arena*     		Arena;
	meta_tokenizer* Tokenizer;
	hashmap    		MacroMap;
	hashmap    		StructMap;
	hashmap    		GlobalMap;
	hashmap 		StructInfoMap;
	hashmap 		UnionInfoMap;
	hashmap 		EnumMap;
	hashmap 		EnumInfoMap;
	hashmap    		FunctionMap;
	hashmap    		MacroParamMap;
} meta_parser;

enum {
	META_FOR_LOOP_SUPPORT_NONE = 0,
	META_FOR_LOOP_SUPPORT_STRUCT = (1 << 0),
	META_FOR_LOOP_SUPPORT_ENUM = (1 << 1),
	META_FOR_LOOP_SUPPORT_FUNCTION = (1 << 2)
};
typedef u32 meta_for_loop_support_flags;

#define META_FOR_LOOP_SUPPORT_ALL (META_FOR_LOOP_SUPPORT_STRUCT|META_FOR_LOOP_SUPPORT_ENUM|META_FOR_LOOP_SUPPORT_FUNCTION)

enum {
	META_FOR_LOOP_FLAG_NONE,
	META_FOR_LOOP_NO_BRACE_FLAG = (1 << 0)
};
typedef u32 meta_for_loop_flags;

typedef struct {
	meta_for_loop_flags Flag;
	meta_for_loop_support_flags SupportFlag;
} meta_for_loop_flags_metadata;

typedef enum {
	META_FOR_LOOP_REPLACEMENT_ENTRY_NAME,
	META_FOR_LOOP_REPLACEMENT_ENTRY_TYPE,
	META_FOR_LOOP_REPLACEMENT_SHORT_ENTRY_NAME,
	META_FOR_LOOP_REPLACEMENT_SHORT_NAME_SUBSTR_CHAR,
	META_FOR_LOOP_REPLACEMENT_GET_TAG_VALUE,
	META_FOR_LOOP_REPLACEMENT_VARIABLE_TYPE
} meta_for_loop_replacement;

typedef struct {
	meta_for_loop_replacement Replacement;
	meta_for_loop_support_flags SupportFlag;
} meta_for_loop_replacement_metadata;

typedef struct meta_for_loop_entry meta_for_loop_entry;
struct meta_for_loop_entry {
	string 				 Name;
	string 				 Type;
	string 				 ShortName;
	meta_tag_list 		 Tags;
	b32 				 IsArray;
	meta_for_loop_entry* Next;
};

typedef struct {
	meta_for_loop_entry* First;
	meta_for_loop_entry* Last;
} meta_for_loop_entries;

typedef struct {
	string 				  		TypeStr;
	meta_for_loop_entries 		Entries;
	meta_for_loop_flags   		Flags;
	meta_for_loop_support_flags SupportFlag;
	meta_for_loop_entry*  		CurrentEntry;
} meta_for_loop_context;

typedef struct {
	meta_for_loop_context  Stack[256];
	size_t 				   Count;
	meta_for_loop_context* CurrentContext;
} meta_for_loop_context_stack;

typedef struct {
	b32 		PassedPredicate;
	meta_token* EndToken;
	meta_token* ElseToken;
} meta_if_state;

typedef struct {
	meta_if_state States[256];
	meta_if_state* CurrentState;
	size_t Index;
} meta_if_state_stack;

typedef struct {
	meta_struct_type** Structs;
	size_t 			   StructCount;

	meta_struct_type** Unions;
	size_t 			   UnionCount;

	meta_enum_type** Enums;
	size_t 			 EnumCount;
} meta_parser_process_info;

typedef struct {
	meta_token* Token;
	meta_token* PrevToken;
	size_t 		Index;
	size_t 		Count;
} meta_token_iter;

#define META_PARSER_PREDICATE_DEFINE(name) b32 name(meta_parser* Parser, meta_for_loop_entry* Entry, string_list Parameters)
typedef META_PARSER_PREDICATE_DEFINE(meta_parser_predicate_func);

#endif