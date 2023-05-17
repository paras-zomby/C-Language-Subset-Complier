#include "include.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void freeStack(pStack stack)
{
    free(stack->pool);
}

void initialStack(pStack stack, int size)
{
    stack->pool = (pStackElem) malloc(sizeof(StackElem) * size);
    stack->top = -1;
}

void pushStack(pStack stack, StackElem item)
{
    stack->pool[++stack->top] = item;
}

StackElem popStack(pStack stack)
{
    return stack->pool[stack->top--];
}

StackElem topStack(pStack stack)
{
    return stack->pool[stack->top];
}

void printStack(const Stack stack)
{
    printf("stack: ");
    for (int i = 0; i <= stack.top; ++i)
    {
        printf("%3d ", stack.pool[i].state_id);
    }
    printf("\n");
}

// 根据action表和goto表，分析输入串。
int ParseAndGenerate(FILE *fp, ActionTable action_table, Grammar grammar, pGenerateCodes generate_codes)
{
    // 初始化状态栈
    Stack stack;
    initialStack(&stack, MAX_STATE_STACK_NUM);
    pushStack(&stack, (StackElem){.state_id = 0});
    // 初始化生成部分
    if (generate_codes != NULL)
    {
        generate_codes->codes = (pGenerateCode) malloc(sizeof(GenerateCode) * MAX_GEN_CODE_NUM);
        generate_codes->code_nums = 0;
    }
    // 逐个字符分析源程序
    Token current_token; //词法分析结果
    int input_token_id; // 输入符号的id
    int flag_s = 1;     // 标记是否需要读入新的输入
    int result = 0;     // 标记是否分析成功
    int elem_nums = action_table.action_nums / action_table.state_nums;
    printf("======[Syntax Analysis]======\n");
    while (1)
    {
        // 输入指针变化，读入新的输入
        if (flag_s)
        {
            current_token = get_token(fp);
            if (current_token.type == IDENTIFIER_TOKEN)
                input_token_id = get_id("id");
            else if (current_token.type == NUMBER_TOKEN)
                input_token_id = get_id("digits");
            else if (current_token.type == ERROR_TOKEN)
                continue;
            else
                input_token_id = get_id(current_token.str);

            input_token_id += terminal_nums;
            flag_s = 0;
        }
        StackElem current = topStack(&stack);
        if (action_table.actions[current.state_id * elem_nums + input_token_id].action_type == ERROR_STATE)
        {
            printStack(stack);
            printf("input: %6s  ERROR\n", current_token.str);
            freeStack(&stack);
            if (generate_codes != NULL)
                free(generate_codes->codes);
            result = 0;
            break;
        }
        else if (action_table.actions[current.state_id * elem_nums + input_token_id].action_type == ACCEPT_STATE)
        {
            printStack(stack);
            printf("input: %6s  ACCEPT\n", current_token.str);
            freeStack(&stack);
            if (generate_codes != NULL)
                generate_codes->codes = (pGenerateCode) realloc(generate_codes->codes, sizeof(GenerateCode) * generate_codes->code_nums);
            result = 1;
            break;
        }
        else if (action_table.actions[current.state_id * elem_nums + input_token_id].action_type == REDUCE_STATE)
        {
            printStack(stack);
            int num = action_table.actions[current.state_id * elem_nums + input_token_id].value;
            printf("INPUT: %6s  ACTION: R%3d\n", current_token.str, num);
            Production prod = grammar.productions[num];
            int new_symbol = prod.production + terminal_nums;
            pStackElem pop_elems = (pStackElem) malloc(sizeof(StackElem) * prod.gen_nums);
            for (int i = 0; i < prod.gen_nums; ++i)
                pop_elems[i] = popStack(&stack);
            if (generate_codes != NULL)
                generateCode(generate_codes, prod, pop_elems);
            current = topStack(&stack);
            int new_state = action_table.actions[current.state_id * elem_nums + new_symbol].value;
            StackElem next = {new_state, new_symbol};
            pushStack(&stack, next);
        }
        else
        {
            printStack(stack);
            StackElem next = {action_table.actions[current.state_id * elem_nums + input_token_id].value, input_token_id};
            printf("INPUT: %6s  ACTION: T%3d\n", current_token.str, next.state_id);
            pushStack(&stack, next);
            flag_s = 1; //可以接受新的输入
        }
    }
    return result;
}
