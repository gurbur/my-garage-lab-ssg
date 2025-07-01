#include "list_head.h"

typedef enum {
	TOKEN_HASH,        // #
	TOKEN_ASTERISK,    // *
	TOKEN_TEXT,        // normal text
	TOKEN_NUMBER,      // number (used for number list)
	TOKEN_DOT,         // . (used for number list)
	TOKEN_DASH,        // - (used for unnumbered list)
	TOKEN_NEWLINE,     // \n
	TOKEN_TAB,         // \t
	TOKEN_LBRACKET,    // [
	TOKEN_RBRACKET,    // ]
	TOKEN_LPAREN,      // (
	TOKEN_RPAREN,      // )
	TOKEN_BACKTICK,    // `
	TOKEN_EXCLAMATION, // !
	TOKEN_GREATER_THAN,// >
	TOKEN_BACKSLASH,   // \.
	TOKEN_EOF,         // End of File
} TokenType;

typedef struct {
	TokenType type;
	char* value;
	struct list_head list;
} Token;

void tokenize_file(FILE* file, struct list_head* output);

