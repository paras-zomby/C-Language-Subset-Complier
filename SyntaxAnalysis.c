#include "include.h"
#include <stdio.h>

void freeStack(pStack stack)
{
    free(stack->pool);
}

void initialStack(pStack stack, int size)
{
    stack->pool = (int *) malloc(sizeof(int) * size);
    stack->top = -1;
}

void pushStack(pStack stack, int item)
{
    stack->pool[++stack->top] = item;
}

int popStack(pStack stack)
{
    return stack->pool[stack->top--];
}

int topStack(pStack stack)
{
    return stack->pool[stack->top];
}

void printStack(const Stack stack)
{
    printf("stack: ");
    for (int i = 0; i <= stack.top; ++i)
    {
        printf("%3d ", stack.pool[i]);
    }
    printf("\n");
}

// 根据action表和goto表，分析输入串。
int Parsing(FILE *fp, ActionTable action_table, Grammar grammar)
{
    // 初始化状态栈
    Stack state_stack;
    initialStack(&state_stack, MAX_STATE_STACK_NUM);
    // 逐个字符分析源程序
    Token current_token; //词法分析结果
    int token_id; // 对应的标号
    int flag_s = 1;
    int result;
    printf("======[Syntax Analysis]======\n");
    while (1)
    {
        // 输入指针变化，读入新的输入
        if (flag_s)
        {
            current_token = get_token(fp);
            if (current_token.type == END_OF_FILE_TOKEN)
            {
                token_id = get_id("$");
            }
            else if (current_token.type == ERROR_TOKEN)
            {
                continue;
            }
            else if (current_token.type == IDENTIFIER_TOKEN)
            {
                token_id = get_id("id");
            }
            else if (current_token.type == NUMBER_TOKEN)
            {
                token_id = get_id("digits");
            }
            else
            {
                token_id = get_id(current_token.str);
            }
            token_id += terminal_nums;
            flag_s = 0;
        }
        int elem_nums = action_table.action_nums / action_table.state_nums;
        int current_state = topStack(&state_stack);
        if (action_table.actions[current_state * elem_nums + token_id].action_type == ERROR_STATE)
        {
            char temp[MAX_TOKEN_LEN];
            get_lex(token_id - terminal_nums, temp);
            printStack(state_stack);
            printf("input: %6s  ERROR\n", temp);
            freeStack(&state_stack);
            result = 0;
            break;
        }
        else if (action_table.actions[current_state * elem_nums + token_id].action_type == ACCEPT_STATE)
        {
            char temp[MAX_TOKEN_LEN];
            get_lex(token_id - terminal_nums, temp);
            printStack(state_stack);
            printf("input: %6s  ACCEPT\n", temp);
            freeStack(&state_stack);
            result = 1;
            break;
        }
        else if (action_table.actions[current_state * elem_nums + token_id].action_type == REDUCE_STATE)
        {
            char temp[MAX_TOKEN_LEN];
            get_lex(token_id - terminal_nums, temp);
            printStack(state_stack);
            printf("INPUT: %6s  ACTION: R%3d\n", temp, action_table.actions[current_state * elem_nums + token_id].value);
            int num = action_table.actions[current_state * elem_nums + token_id].value;
            Production prod = grammar.productions[num];
            int new_symbol = prod.production + terminal_nums;
            int reduce_num = prod.gen_nums;
            while(reduce_num--)
            {
                popStack(&state_stack);
            }
            current_state = topStack(&state_stack);
            int new_state = action_table.actions[current_state * elem_nums + new_symbol].value;
            pushStack(&state_stack, new_state);
        }
        else
        {
            char temp[MAX_TOKEN_LEN];
            get_lex(token_id - terminal_nums, temp);
            printStack(state_stack);
            printf("INPUT: %6s  ACTION: T%3d\n", temp, action_table.actions[current_state * elem_nums + token_id].value);
            pushStack(&state_stack, action_table.actions[current_state * elem_nums + token_id].value);
            flag_s = 1; //可以接受新的输入
        }
    }
    return result;
}
