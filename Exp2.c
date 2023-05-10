#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "include.h"

// 读入语法状态的表达式，并转换成数据结构
typedef struct {
    int production;
    int generative[10];
    int gen_nums;
} Production, *pProduction;

typedef struct {
    pProduction productions;
    int production_nums;
} Grammar, *pGrammar;

Token terminal_table[MAX_TERMINAL_NUM + 1] = {0};
char non_terminal_table[MAX_NON_TERMINAL_NUM + 1] = {0};
int terminal_nums = 0;
int non_terminal_nums = 1;      // 非终结符的第一个[0]是保留的起始符号位

// lex -> id
int get_id(const char *lex)
{
    // 是终结符的情况
    if (is_operator(lex) || is_keyword(lex) ||
        strcmp(lex, "id") == 0 || strcmp(lex, "digits") == 0 || strcmp(lex, "$") == 0)
    {
        // 终结符占用<0的部分，没有0，所以从1开始
        // 有意义的下标区间[1, terminal_nums]
        for (int i = 1; i <= terminal_nums; ++i)
        {
            if (strcmp(terminal_table[i].str, lex) == 0)
                return -i;
        }
        strcpy(terminal_table[++terminal_nums].str, lex);
        return -terminal_nums;
    }
    else // 是非终结符的情况
    {   // 非终结符的第一个[0]是保留的起始符号位
        for (int i = 1; i < non_terminal_nums; ++i)
        {
            if (non_terminal_table[i] == lex[0])
                return i;
        }
        non_terminal_table[non_terminal_nums] = lex[0];
        return non_terminal_nums++;
    }

}
// id -> lex
void get_lex(int id, char temp[MAX_TOKEN_LEN])
{
    if (id > 0)
    {
        temp[0] = non_terminal_table[id];
        temp[1] = '\0';
    }
    else if (id < 0)
        strcpy(temp, terminal_table[-id].str);
    else
        strcpy(temp, "START");
}

Grammar processGrammar(FILE *fp)
{
    Production start_production = {.production = 0, .gen_nums = 1, .generative = {1}};
    Production production_buffer[1024];
    production_buffer[0] = start_production;
    int now_production_num = 1;

    char token[100];
    int state = 0; //表示识别状态，0未识别符号; >0正在识别符号
    int prod = 1; //正在识别的是左侧还是右侧
    while (1)
    {
        char ch = (char) fgetc(fp);
        if (feof(fp))  // TODO: 错误处理
            break;
        if (ch == '#')
        {
            while (ch != '\n' && !feof(fp))
                ch = (char) fgetc(fp);
            if (feof(fp))
                break;
        }
        else if (ch == '@')
        {
            prod = 0;
        }
        else if (ch == '\n' || ch == ' ' || ch == '\r' || ch == '\t')
        {
            if (state != 0)
            {
                token[state] = '\0';
                if (prod)
                {
                    production_buffer[now_production_num].production = get_id(token);
                }
                else
                {
                    production_buffer[now_production_num].generative[production_buffer[now_production_num].gen_nums++] = get_id(token);
                }
                state = 0;
            }
            if (ch == '\n')
            {
                if (prod == 0 && production_buffer[now_production_num].gen_nums > 0)
                {
                    ++now_production_num;
                    if (now_production_num >= 1024)
                    {
                        print_error("Too many productions, exit.");
                        exit(-1);
                    }
                    production_buffer[now_production_num].gen_nums = 0;
                    prod = 1;
                }
                else if (prod == 0 && production_buffer[now_production_num].gen_nums == 0)
                {
                    print_error("Not a Correct Grammar, please recheck your grammar file, exit");
                    exit(-1);
                }
            }
        }
        else
            token[state++] = ch;
    }
    fclose(fp);
    Grammar grammar;
    grammar.production_nums = now_production_num;
    grammar.productions = (pProduction) malloc(sizeof(Production) * now_production_num);
    memcpy(grammar.productions, production_buffer, sizeof(Production) * now_production_num);
    return grammar;
}

// 生成first集合和follow集合(?)

// 根据读入的语法状态数目，生成自动机状态们（即规范项集族，I1 I2 I3 I4...）

typedef struct {
    Production production;
    int dot_pos;
} PosProduction, *pPosProduction;

typedef struct {
    pPosProduction pos_productions;
    int pos_production_nums;
} State, *pState;

typedef struct {
    pState states;
    int state_nums;
} AutomatonStates;

int sameProduction(const Production a, const Production b)
{
    return a.production == b.production && a.gen_nums == b.gen_nums
    && memcmp(a.generative, b.generative, sizeof(int) * a.gen_nums) == 0;
}

State mergeState(State a, State b)
{
    if (a.pos_production_nums == 0)
        return b;
    if (b.pos_production_nums == 0)
        return a;
    State ret;
    ret.pos_production_nums = a.pos_production_nums + b.pos_production_nums;
    ret.pos_productions = (pPosProduction)malloc(sizeof(PosProduction) * ret.pos_production_nums);
    memcpy(ret.pos_productions, a.pos_productions, sizeof(PosProduction) * a.pos_production_nums);
    memcpy(ret.pos_productions + a.pos_production_nums, b.pos_productions, sizeof(PosProduction) * b.pos_production_nums);
    free(a.pos_productions);
    free(b.pos_productions);
    return ret;
}

int sameState(const State a, const State b)
{
    if (a.pos_production_nums != b.pos_production_nums)
        return 0;
    for (int i = 0; i < a.pos_production_nums; ++i)
    {
        int j;
        for (j = 0; j < b.pos_production_nums; ++j)
        {
            // 如果a中的某条点规则与b完全一致
            if (a.pos_productions[i].production.production == b.pos_productions[j].production.production
            && a.pos_productions[i].production.gen_nums == b.pos_productions[j].production.gen_nums
            && 0 == memcmp(a.pos_productions[i].production.generative, b.pos_productions[j].production.generative,
                           sizeof(int) * a.pos_productions[i].production.gen_nums))
            {
                break;
            }
        }
        // 说明a中有一条规则b中没有相同的
        if (j == b.pos_production_nums)
            return 0;
    }
    return 1;
}

State closure(PosProduction pos_production, Grammar grammar)
{
    PosProduction prods[100];
    prods[0] = pos_production;
    int prod_nums = 1;

    while (1)
    {
        int last_prod_nums = prod_nums;
        for (int i = 0; i < prod_nums; ++i)
        {
            int ch;
            if (prods[i].dot_pos >= prods[i].production.gen_nums)
                ch = -1;    // 当前规则已经到达末尾，与ch是终结符效果一样，都不会匹配下一个字符，因此直接置为-1
            else
                ch = prods[i].production.generative[prods[i].dot_pos];
            if (ch > 0)
            {
                for (int j = 0; j < grammar.production_nums; ++j)
                {
                    if (grammar.productions[j].production == ch)
                    {
                        int k;
                        for (k = 0; k < prod_nums; ++k)
                        {
                            // 因为新加入的点规则的点一定在生成式的最前面，因此没必要比较点的位置
                            if (sameProduction(grammar.productions[j], prods[k].production))
                            {
                                break;
                            }
                        }
                        if (k == prod_nums)
                        {
                            PosProduction temp = {.production = grammar.productions[j], .dot_pos = 0};
                            prods[prod_nums++] = temp;
                            if (prod_nums >= 100)
                            {
                                print_error("Closure buffer overflow, exit.");
                                exit(-1);
                            }
                        }
                    }
                }
            }
        }
        if (last_prod_nums == prod_nums)
            break;
    }
    State ret;
    ret.pos_productions = (pPosProduction)malloc(sizeof(PosProduction) * prod_nums);
    memcpy(ret.pos_productions, prods, sizeof(PosProduction) * prod_nums);
    ret.pos_production_nums = prod_nums;
    return ret;
}

State gotoState(State state, int ch, Grammar grammar)
{
    State ret = {NULL, 0};
    for (int i = 0; i < state.pos_production_nums; ++i)
    {
        if (state.pos_productions[i].dot_pos < state.pos_productions[i].production.gen_nums
            && state.pos_productions[i].production.generative[state.pos_productions[i].dot_pos] == ch)
        {
            PosProduction temp = state.pos_productions[i];
            temp.dot_pos++;
            ret = mergeState(ret, closure(temp, grammar));
        }
    }
    return ret;
}

AutomatonStates items(Grammar grammar)
{
    AutomatonStates automaton_states = {NULL, 0};
    if (grammar.production_nums == 0)
    {
        return automaton_states;
    }
    State states[512];
    int state_nums = 0;
    PosProduction start_prod;
    start_prod.production = grammar.productions[0];
    start_prod.dot_pos = 0;
    states[state_nums++] = closure(start_prod, grammar);
    while(1)
    {
        int last_state_nums = state_nums;
        for (int i = 0; i < state_nums; ++i)
        {
            for (int j = -terminal_nums; j < non_terminal_nums; ++j)
            {
                State temp = gotoState(states[i], j, grammar);
                if (temp.pos_production_nums != 0)
                {
                    int k;
                    for (k = 0; k < state_nums; ++k)
                    {
                        if (sameState(temp, states[k]))
                        {
                            break;
                        }
                    }
                    if (k == state_nums)
                    {
                        states[state_nums++] = temp;
                        if (state_nums >= 512)
                        {
                            print_error("State buffer overflow, exit.");
                            exit(-1);
                        }
                    }
                }
            }
        }
        if (state_nums == last_state_nums)
            break;
    }
    automaton_states.state_nums = state_nums;
    automaton_states.states = (pState)malloc(sizeof(State) * state_nums);
    memcpy(automaton_states.states, states, sizeof(State) * state_nums);
    return automaton_states;
}

// 根据自动机状态们生成action表和goto表

// 根据action表和goto表，分析输入串。


int main(int argc, char *argv[])
{
    FILE *fp = fopen(DEFAULT_GRAMMAR_FILE, "r");

    if (fp == NULL)
    {
        print_error("File is not opened, exit.");
        exit(-1);
    }
    Grammar grammar = processGrammar(fp);
    char temp[100];
    printf("=====Grammar=====\n");
    for (int i = 0; i < grammar.production_nums; ++i)
    {
        get_lex(grammar.productions[i].production, temp);
        printf("%s -> ", temp);
        for (int j = 0; j < grammar.productions[i].gen_nums; ++j)
        {
            get_lex(grammar.productions[i].generative[j], temp);
            printf("%s ", temp);
        }
        printf("\n");
    }

    AutomatonStates automaton_states = items(grammar);
    fflush(stdout);
    printf("=====Automaton state=====\n");
    printf("Automaton state nums: %d\n", automaton_states.state_nums);
    for (int i = 0; i < automaton_states.state_nums; ++i)
    {
        printf("State %d:\n", i);
        for (int j = 0; j < automaton_states.states[i].pos_production_nums; ++j)
        {
            get_lex(automaton_states.states[i].pos_productions[j].production.production, temp);
            printf("%s -> ", temp);
            for (int k = 0; k < automaton_states.states[i].pos_productions[j].production.gen_nums; ++k)
            {
                if (k == automaton_states.states[i].pos_productions[j].dot_pos)
                    printf(". ");
                get_lex(automaton_states.states[i].pos_productions[j].production.generative[k], temp);
                printf("%s ", temp);
            }
            if (automaton_states.states[i].pos_productions[j].production.gen_nums == automaton_states.states[i].pos_productions[j].dot_pos)
                    printf(".");
            printf("\n");
            fflush(stdout);
        }
    }
}
