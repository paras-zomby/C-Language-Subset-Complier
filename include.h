#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include <stdio.h>
#include <stdlib.h>

// 定义常量
#define MAX_TOKEN_LEN 100           // 最大单词长度
#define MAX_IDENT_LEN 32            // 最大关键词长度
#define MAX_NUM_LEN 1024            // 最多读入单词数目
#define MAX_TERMINAL_NUM 64         // 语法定义时直接定义的终结符数目最大值
#define MAX_NON_TERMINAL_NUM 1024   // 最多非终结符数目
#define MAX_STATE_STACK_NUM 256     // 状态栈最大大小
#define MAX_GEN_CODE_NUM 256        // 最大中间代码数目


// 定义文件路径
#define DEFAULT_INPUT_FILE "../input.txt"
#define DEFAULT_GRAMMAR_FILE "../Language.grammar"


// 定义状态类型
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

typedef struct {
    int id;             // 表示这条产生式是第几条
    int production;
    int generative[10];
    int gen_nums;
} Production, *pProduction;

typedef struct {
    int *items;
    int item_nums;
}FIRST, FOLLOW, *pFIRST, *pFOLLOW;

typedef struct {
    pProduction productions;
    int production_nums;
    int item_nums;      // first集合数目，也是语法符号表中符号的数目
    pFIRST firsts;      // first集合，注意因为first集合包含所有符号，所以索引向右平移了terminal_nums位
    pFOLLOW follows;    // 但是follow集合只包含非终结符，所以索引不需要平移
    int follow_nums;    // follow集合数目，也是非终极符号表的符号数目
} Grammar, *pGrammar;

typedef struct {
    Production production;
    int dot_pos;
} PosProduction, *pPosProduction;

typedef struct {
    pPosProduction pos_productions;
    int pos_production_nums;
    int* non_core_productions;
    int non_core_production_nums;
} State, *pState;

typedef struct {
    pState states;
    int state_nums;
} AutomatonStates;

typedef struct {
    enum{
        EMPTY_STATE = 0,    // 空状态
        GOTO_STATE,         // 用GOTO表转移
        SHIFT_STATE,        // 直接ACTION表转移
        REDUCE_STATE,       // 使用某条语法规则规约
        ACCEPT_STATE,       // 接受状态
        ERROR_STATE         // 错误状态
    } action_type;
    int value;
} Action, *pAction;

typedef struct {
    pAction actions;    // 前terminal_nums个是action表（第一维是终止符），后non_terminal_nums个是goto表
    int action_nums;    // 元素总数
    int state_nums;     // 状态数，也是表格的行数
} ActionTable, *pActionTable;

typedef union {         // 符号属性
    struct {
        int ins_pos;
    } auxiliary_elem;
    struct {
        int type_id;
    } terminal_elem;
    struct {
        int type_id;
        char str[MAX_TOKEN_LEN];
    } variable_elem;
    struct {
        int * true_list;
        int * false_list;
        int true_nums;
        int false_nums;
    } condition_elem;
    struct {
        int * next_list;
        int next_nums;
    } statement_elem;
} Attribute, *pAttribute;

// 栈
typedef struct {
    int state_id;           // 状态栈id
    int token_id;           // 符号栈id
    Attribute attribute;    // 符号属性
} StackElem, *pStackElem;

typedef struct {
    pStackElem pool;
    int top;
}Stack, *pStack;

typedef struct {
    enum {
        JUMP,
        CONDITIONAL_JUMP,
        ASSIGNMENT,
    } code_type;
    struct {
      int jump_target;
      int op_id; // 取值和语法符号表中的id一致
      char params[3][MAX_TOKEN_LEN];
    } code;
} GenerateCode, *pGenerateCode;

typedef struct {
    pGenerateCode codes;
    int code_nums;
} GenerateCodes, *pGenerateCodes;


// 定义全局变量
// 定义字符表（数字和字母的字符表没用）
extern const char number_table[];
extern const char alpha_table[];
extern const char keyword_table[][MAX_IDENT_LEN];
extern const char operator_elem_table[];
extern const char operator_table[][MAX_IDENT_LEN];
// 定义符号表
extern Token terminal_table[MAX_TERMINAL_NUM + 1];
extern char non_terminal_table[MAX_NON_TERMINAL_NUM + 1];
extern int terminal_nums;
extern int non_terminal_nums;


// 共有函数定义
// LexicalAnalysis
Token get_token(FILE *fp);
void print_error(const char *msg);
void print_token(Token t);
int is_keyword(const char *str);
int is_operator_elem(char ch);
int is_operator(const char *str);
int is_blank(char ch);

// GrammarAnalysis
int get_id(const char *lex);
void get_lex(int id, char temp[MAX_TOKEN_LEN]);
void processGrammar(FILE *fp, pGrammar grammar);
int productEpsilon(Grammar grammar, const int* string, int str_nums);
int mergeSet(int *a, int* a_size, const int *b, int b_size, int* epsilon_pos, int epsilon_val);
void calcFIRSTSet(pGrammar grammar);
void calcFOLLOWSet(pGrammar grammar);
Grammar generateGrammar(FILE* fp);
void printGrammar(Grammar grammar);

// ActionTableGeneration
State mergeState(State a, State b);
int sameState(State a, State b);
State closure(PosProduction pos_production, Grammar grammar);
State gotoState(State state, int ch, Grammar grammar);
void printState(State state);
void printStates(AutomatonStates automaton_states);
void getActionTable(Grammar grammar, AutomatonStates* automaton_states, ActionTable *action_table);
void printActionTable(ActionTable action_table);

// SyntaxAnalysis
void freeStack(pStack stack);
void initialStack(pStack stack, int size);
void pushStack(pStack stack, StackElem item);
StackElem popStack(pStack stack);
StackElem topStack(pStack stack);
void printStack(Stack stack);
int ParseAndGenerate(FILE *fp, ActionTable action_table, Grammar grammar, pGenerateCodes generate_codes);

// GenerateCode
Attribute generateCode(pGenerateCodes generate_codes, Production production, pStackElem elems);
void assignAttribute(pAttribute attributes, Stack stack);
#endif //_INCLUDE_H_
