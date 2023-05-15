#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#define MAX_TOKEN_LEN 100           // 最大单词长度
#define MAX_IDENT_LEN 32            // 最大关键词长度
#define MAX_NUM_LEN 1024            // 最多读入单词数目
#define MAX_TERMINAL_NUM 64         // 语法定义时直接定义的终结符数目最大值
#define MAX_NON_TERMINAL_NUM 1024   // 最多非终结符数目
#define MAX_STATE_STACK_NUM 256     // 状态栈最大大小


#define DEFAULT_INPUT_FILE "../input.txt"
#define DEFAULT_GRAMMAR_FILE "../Language.grammar"

// 定义字符表（数字和字母的字符表没用）
extern const char number_table[];
extern const char alpha_table[];
extern const char keyword_table[][MAX_IDENT_LEN];
extern const char operator_elem_table[];
extern const char operator_table[][MAX_IDENT_LEN];

typedef enum {
    END_OF_FILE_TOKEN,
    KEYWORD_TOKEN,
    IDENTIFIER_TOKEN,
    NUMBER_TOKEN,
    OPERATOR_TOKEN,
    ERROR_TOKEN
} TokenType;

typedef struct {
    TokenType type;
    char str[MAX_TOKEN_LEN];
} Token;

// 共有函数定义
// LexicalAnalysis
Token get_token(FILE *fp);
void print_error(const char *msg);
void print_token(Token t);
int is_keyword(const char *str);
int is_operator_elem(char ch);
int is_operator(const char *str);
int is_blank(char ch);

#endif //_INCLUDE_H_
