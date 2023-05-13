#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "include.h"

// 读入语法状态的表达式，并转换成数据结构
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
        // 有意义的下标区间[1, terminal_nums]，并且最后一个是$，占用[terminal_nums]位置
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
void get_lex(const int id, char temp[MAX_TOKEN_LEN])
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

void processGrammar(FILE *fp, pGrammar grammar)
{
    Production start_production = {.id = 0, .production = 0, .gen_nums = 1, .generative = {1}};
    Production production_buffer[1024];
    production_buffer[0] = start_production;
    int now_production_num = 1;

    char token[100];
    int state = 0; //表示识别状态，0未识别符号; >0正在识别符号
    int prod = 1; //正在识别的是左侧还是右侧
    int read_production = 0; //当前是否读入了产生式
    while (1)
    {
        char ch = (char) fgetc(fp);
        if (feof(fp))
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
                    read_production = 1;
                }
                else
                {
                    production_buffer[now_production_num].generative[production_buffer[now_production_num].gen_nums++] = get_id(token);
                }
                state = 0;
            }
            if (ch == '\n')
            {
                if (prod == 1 && read_production == 1)
                {
                    print_error("Not a Correct Grammar, please recheck your grammar file, exit");
                    exit(-1);
                }
                else if (prod == 0)
                {
                    production_buffer[now_production_num].id = now_production_num;
                    ++now_production_num;
                    if (now_production_num >= 1024)
                    {
                        print_error("Too many productions, exit.");
                        exit(-1);
                    }
                    production_buffer[now_production_num].gen_nums = 0;
                    prod = 1;
                    read_production = 0;
                }
            }
        }
        else
            token[state++] = ch;
    }
    fclose(fp);
    get_id("$");    // 将$加入终级符表，$位于最后一个终极符
    grammar->production_nums = now_production_num;
    grammar->productions = (pProduction) malloc(sizeof(Production) * now_production_num);
    grammar->item_nums = terminal_nums + non_terminal_nums;
    grammar->follow_nums = non_terminal_nums;
    memcpy(grammar->productions, production_buffer, sizeof(Production) * now_production_num);
}

// 用于合并两个集合，结果放入a中。b中的epsilon不会加入a中。返回值是新增的元素个数
// epsilon_pos用于记录b中epsilon的位置。传入NULL表示不记录
int mergeSet(int *a, int* a_size, const int *b, int b_size, int* epsilon_pos)
{
    int pos = *a_size;
    int epsilon_pos_ = -1;
    for (int i = 0; i < b_size; ++i)
    {
        if (b[i] == 0)  // b[i]是epsilon，不能加入follow集合。返回值为epsilon的位置
        { epsilon_pos_ = i; continue; }

        int j;
        for  (j = 0; j < *a_size; ++j)
            if (a[j] == b[i])
                break;
        if (j == *a_size)
            a[pos++] = b[i];
    }
    int ret = pos - *a_size;
    *a_size = pos;
    if (epsilon_pos != NULL)
        *epsilon_pos = epsilon_pos_;
    return ret;
}

void calcFIRSTSet(pGrammar grammar)
{
    int total_num = terminal_nums + non_terminal_nums;
    grammar->firsts = (pFIRST) malloc(sizeof(FIRST) * total_num);
    grammar->item_nums = total_num;
    for (int i=-terminal_nums; i<non_terminal_nums;i++)
    {
        grammar->firsts[i + terminal_nums].item_nums = 0;
    }
    for (int i = -terminal_nums; i < 0; ++i) {
        grammar->firsts[i + terminal_nums].items = (int*) malloc(sizeof(int) * 1);
        grammar->firsts[i + terminal_nums].item_nums = 1;
        grammar->firsts[i + terminal_nums].items[0] = i;
    }

    while(1){
        int is_stable = 1;
        for (int i = 0; i < grammar->production_nums; i++) {
            int item[100], val_pos = 0;
            memset(item, 0, sizeof(int) * 100);

            // 空串处理
            if(grammar->productions[i].gen_nums == 0)
            {
                grammar->productions[i].gen_nums += 1;
                grammar->productions[i].generative[0] = 10;
            }

            int k = 0;
            for (; k < grammar->productions[i].gen_nums; k++) {
                val_pos=0;
                int xk = grammar->productions[i].generative[k];
                int alpha = grammar->productions[i].production;
                int is_null = 0;
                if (xk == 10)
                {
                    item[0] = 10;
                    val_pos = 1;
                    int pos = grammar->firsts[alpha + terminal_nums].item_nums;
                    if (pos != 0) {
                        memcpy(item + val_pos, grammar->firsts[alpha + terminal_nums].items, pos * sizeof(int));
                        free(grammar->firsts[alpha + terminal_nums].items);
                    }
                    grammar->firsts[alpha + terminal_nums].items = (int *) malloc(sizeof(int) * (val_pos + pos));
                    memcpy(grammar->firsts[alpha + terminal_nums].items, item, (val_pos + pos) * sizeof(int));
                    grammar->firsts[alpha + terminal_nums].item_nums += val_pos;
                    break;
                }
                for (int index_xk = 0; index_xk < grammar->firsts[xk + terminal_nums].item_nums; index_xk++) {
                    int repeat = 0;
                    if (grammar->firsts[xk + terminal_nums].items[index_xk] == 10) {
                        is_null = 1;
                        continue;
                    }
                    for (int index_alpha = 0;
                         index_alpha < grammar->firsts[alpha + terminal_nums].item_nums; index_alpha++) {
                        if (grammar->firsts[alpha + terminal_nums].items[index_alpha] ==
                            grammar->firsts[xk + terminal_nums].items[index_xk]) {
                            repeat = 1;
                            break;
                        }
                    }
                    if (repeat == 0) {
                        item[val_pos] = grammar->firsts[xk + terminal_nums].items[index_xk];
                        val_pos += 1;
                        is_stable = 0;
                    }
                }

                // 处理xk的first集中有空串的情况
                if (is_null==1 && k == grammar->productions[i].gen_nums - 1)
                {
                    item[val_pos] = 10;
                    val_pos += 1;
                    is_stable = 0;
                }
                // 加入first集新元素

                int pos = grammar->firsts[alpha + terminal_nums].item_nums;
                if (pos != 0) {
                    memcpy(item + val_pos, grammar->firsts[alpha + terminal_nums].items, pos * sizeof(int));
                    free(grammar->firsts[alpha + terminal_nums].items);
                }
                grammar->firsts[alpha + terminal_nums].items = (int *) malloc(sizeof(int) * (val_pos + pos));
                memcpy(grammar->firsts[alpha + terminal_nums].items, item, (val_pos + pos) * sizeof(int));
                grammar->firsts[alpha + terminal_nums].item_nums += val_pos;
                char temp[100];
                get_lex(alpha, temp);
                printf("FIRST ITEM [%s]: ", temp);
                for (int j = 0; j < val_pos + pos; ++j) {
                    get_lex(item[j], temp);
                    printf("%s ", temp);
                }
                printf("\n");
                if (is_null==0){break;}
            }
        }
        if (is_stable) {break;}
    }
}

void calcFOLLOWSet(pGrammar grammar)
{
    grammar->follows = (pFIRST) malloc(sizeof(FIRST) * grammar->follow_nums);
    for (int i = 0; i < grammar->follow_nums; ++i)
    {
        grammar->follows[i].items = (int*) malloc(sizeof(int) * grammar->item_nums);
        grammar->follows[i].item_nums = 0;
    }
    grammar->follows[0].items[0] = -terminal_nums; // 起始符号的follow集合中加入$
    grammar->follows[0].item_nums = 1;
    int changed = 1;    // 记录每次循环follow集合中新增元素的个数
    while (changed != 0) // 当follow集合不再变化时退出
    {
        changed = 0;
        // 对于每一条产生式遍历
        for (int i = 0; i < grammar->production_nums; ++i)
        {
            // 对于每一个产生式右侧的符号遍历
            for (int j = 0; j < grammar->productions[i].gen_nums; ++j)
            {
                int ch = grammar->productions[i].generative[j];
                if (ch <= 0) // 终极符号没有follow集合，开始符号follow集合固定为$
                    continue;

                int k; // 对于每一个产生式右侧的符号的所有后继符号遍历
                for (k = j + 1; k < grammar->productions[i].gen_nums; ++k)
                {
                    int next = grammar->productions[i].generative[k];
                    if (next < 0)  // 下一个符号是终极符号，停止遍历并将其加入到follow集合中
                    {
                        changed += mergeSet(grammar->follows[ch].items,
                                            &grammar->follows[ch].item_nums,
                                            &next, 1, NULL);
                        break;
                    }
                    else
                    {
                        // 下一个符号是非终极符号，将其first集合加入到follow集合中
                        int epsilon_pos;
                        changed += mergeSet(grammar->follows[ch].items,
                                             &grammar->follows[ch].item_nums,
                                             grammar->firsts[next + terminal_nums].items,
                                             grammar->firsts[next + terminal_nums].item_nums,
                                             &epsilon_pos);
                        // 后继符号的first集合中没有epsilon，停止遍历
                        if (-1 == epsilon_pos)
                            break;
                    }
                }
                // j已经是最后一个符号，或者j后面的符号都是非终极符号并且FIRST集合均包含epsilon
                if (j + 1 == grammar->productions[i].gen_nums || k == grammar->productions[i].gen_nums)
                {
                    // 把产生式的左侧符号的follow集合加入到j的follow集合中
                    // 如果产生式的左侧符号和右侧符号相同，不需要加入
                    if (grammar->productions[i].production != ch)
                        changed += mergeSet(grammar->follows[ch].items, &grammar->follows[ch].item_nums,
                                             grammar->follows[grammar->productions[i].production].items,
                                             grammar->follows[grammar->productions[i].production].item_nums,
                                             NULL);
                }
            }
        }
    }
    for (int i = 0; i < grammar->follow_nums; ++i)
        grammar->follows[i].items = (int*) realloc(grammar->follows[i].items,
                                                   sizeof(int) * grammar->follows[i].item_nums);
}

Grammar generateGrammar(FILE* fp)
{
    Grammar grammar;
    processGrammar(fp, &grammar);
    calcFIRSTSet(&grammar);
    calcFOLLOWSet(&grammar);
    return grammar;
}

void printGrammar(const Grammar grammar)
{
    char temp[MAX_TOKEN_LEN];
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
    printf("FIRST Sets: \n");
    for (int i = 0; i < grammar.item_nums; ++i)
    {
        get_lex(i - terminal_nums, temp);
        printf("%s: ", temp);
        for (int j = 0; j < grammar.firsts[i].item_nums; ++j)
        {
            get_lex(grammar.firsts[i].items[j], temp);
            printf("%s ", temp);
        }
        printf("\n");
    }
    printf("FOLLOW Sets: \n");
    for (int i = 0; i < grammar.item_nums; ++i)
    {
        get_lex(i - terminal_nums, temp);
        printf("%s: ", temp);
        for (int j = 0; j < grammar.follows[i].item_nums; ++j)
        {
            get_lex(grammar.follows[i].items[j], temp);
            printf("%s ", temp);
        }
        printf("\n");
    }
}

// 根据读入的语法状态数目，生成自动机状态们（即规范项集族，I1 I2 I3 I4...）
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
    ret.non_core_production_nums = 0;
    int * tmp = (int *)malloc(sizeof(int) * (a.non_core_production_nums + b.non_core_production_nums));
    for (int i = 0; i < a.non_core_production_nums; ++i)
    {
        int j;
        for (j = 0; j < b.non_core_production_nums; ++j)
            if (a.non_core_productions[i] == b.non_core_productions[j])
                break;
        if (j == b.non_core_production_nums)
            tmp[ret.non_core_production_nums++] = a.non_core_productions[i];
    }
    ret.non_core_productions = (int *)malloc(sizeof(int) * (ret.non_core_production_nums + b.non_core_production_nums));
    memcpy(ret.non_core_productions, tmp, sizeof(int) * ret.non_core_production_nums);
    memcpy(ret.non_core_productions + ret.non_core_production_nums, b.non_core_productions, sizeof(int) * b.non_core_production_nums);
    ret.non_core_production_nums += b.non_core_production_nums;
    free(tmp);
    free(a.non_core_productions);
    free(b.non_core_productions);
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
            if (a.pos_productions[i].production.id == b.pos_productions[j].production.id
            && a.pos_productions[i].dot_pos == b.pos_productions[j].dot_pos
           )
            {
                break;
            }
        }
        // 说明a中有一条规则b中没有相同的
        if (j == b.pos_production_nums)
            return 0;
    }
    for (int i = 0; i < a.non_core_production_nums; ++i)
    {
        int j;
        for (j = 0; j < b.non_core_production_nums; ++j)
            // 如果a中的某条非核心规则与b一致
            if (a.non_core_productions[i] == b.non_core_productions[j])
                break;
        if (j == b.non_core_production_nums)
            return 0;
    }
    return 1;
}

State closure(PosProduction pos_production, Grammar grammar)
{
    PosProduction prods[100];
    prods[0] = pos_production;
    int prod_nums = 1;
    int non_core_productions[100];
    int non_core_prod_nums = 0;
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
                            if (j == prods[k].production.id)
                            {
                                break;
                            }
                        }
                        if (k == prod_nums)
                        {
                            PosProduction temp = {.production = grammar.productions[j], .dot_pos = 0};
                            prods[prod_nums++] = temp;

                            // 判断当前规则对应的产生符号是否已经加入了非核心产生式
                            int l;
                            for (l = 0; l < non_core_prod_nums; ++l)
                            {
                                if (non_core_productions[l] == grammar.productions[j].production)
                                {
                                    break;
                                }
                            }
                            if (l == non_core_prod_nums)
                                non_core_productions[non_core_prod_nums++] = grammar.productions[j].production;
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
    // 只有第一条产生式是核心产生式
    ret.pos_productions = (pPosProduction)malloc(sizeof(PosProduction));
    *ret.pos_productions = pos_production;
    ret.pos_production_nums = 1;
    if (non_core_prod_nums > 0)
    {
        ret.non_core_productions = (int *) malloc(sizeof(int) * non_core_prod_nums);
        memcpy(ret.non_core_productions, non_core_productions, sizeof(int) * non_core_prod_nums);
    }
    else
        ret.non_core_productions = NULL;
    ret.non_core_production_nums = non_core_prod_nums;
    return ret;
}

State gotoState(State state, int ch, Grammar grammar)
{
    State ret = {NULL, 0, NULL, 0};
    for (int i = 0; i < state.pos_production_nums; ++i)
    {
        if (state.pos_productions[i].dot_pos < state.pos_productions[i].production.gen_nums
            && state.pos_productions[i].production.generative[state.pos_productions[i].dot_pos] == ch)
        {
            PosProduction temp = state.pos_productions[i];
            ++(temp.dot_pos);
            ret = mergeState(ret, closure(temp, grammar));
        }
    }
    for (int i = 0; i < state.non_core_production_nums; ++i)
    {
        for (int j = 0; j < grammar.production_nums; ++j)
        {
            if (grammar.productions[j].production == state.non_core_productions[i]
                && grammar.productions[j].generative[0] == ch)
            {
                PosProduction temp = {.production = grammar.productions[j], .dot_pos = 1};
                ret = mergeState(ret, closure(temp, grammar));
            }
        }
    }
    return ret;
}

void printState(State state)
{
    char temp[MAX_TOKEN_LEN];
    for (int j = 0; j < state.pos_production_nums; ++j)
    {
        get_lex(state.pos_productions[j].production.production, temp);
        printf("%s -> ", temp);
        for (int k = 0; k < state.pos_productions[j].production.gen_nums; ++k)
        {
            if (k == state.pos_productions[j].dot_pos)
                printf(". ");
            get_lex(state.pos_productions[j].production.generative[k], temp);
            printf("%s ", temp);
        }
        if (state.pos_productions[j].production.gen_nums == state.pos_productions[j].dot_pos)
                printf(".");
        printf("\n");
    }
    printf("Non-core productions: ");
    for (int i = 0; i < state.non_core_production_nums; ++i)
    {
        get_lex(state.non_core_productions[i], temp);
        printf("%s ", temp);
    }
    printf("\n");
}

void printStates(AutomatonStates automaton_states)
{
    char temp[MAX_TOKEN_LEN];
    printf("=====Automaton States=====\n");
    printf("Automaton state nums: %d\n", automaton_states.state_nums);
    for (int i = 0; i < automaton_states.state_nums; ++i)
    {
        printf("State %d:\n", i);
        printState(automaton_states.states[i]);
    }
}

// 根据自动机状态们生成action表和goto表
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

void getActionTable(Grammar grammar, AutomatonStates* automaton_states, ActionTable *action_table)
{
    // 检查传入的参数是否为空，不为空要释放原地址空间
    if (automaton_states->states != NULL)
    {
        free(automaton_states->states);
        automaton_states->states = NULL;
        automaton_states->state_nums = 0;
    }
    if (action_table->actions != NULL)
    {
        free(action_table->actions);
        action_table->actions = NULL;
        action_table->action_nums = 0;
        action_table->state_nums = 0;
    }

    // 前terminal_nums个是action表，后non_terminal_nums个是goto表。
    pAction action_buffer = (pAction)malloc(sizeof(Action) * grammar.item_nums * 512);
    memset(action_buffer, 0, sizeof(Action) * grammar.item_nums * 512);
    if(grammar.production_nums == 0)
        return;
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
                            // state[i]经过j转移得到了temp
                            int index = i * grammar.item_nums + j + terminal_nums;
                            action_buffer[index].value = k;
                            if (j < 0)
                                action_buffer[index].action_type = SHIFT_STATE;
                            else
                                action_buffer[index].action_type = GOTO_STATE;
                            break;
                        }
                    }
                    if (k == state_nums)    // 产生了一个新状态, state[i]经过j转移得到temp
                    {
                        // 此时新增的state在最后一轮for循环中一定会被处理，因此此处不用填写action表
                        states[state_nums++] = temp;

                        if (state_nums >= 100)
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

    // 填写要规约的ACTION表内容以及接受状态，并将空处置为error。
    for (int i = 0; i < state_nums; ++i)
    {
        for (int j = 0; j < states[i].pos_production_nums; ++j)
        {
            // 如果点在最后，就要规约
            if (states[i].pos_productions[j].dot_pos == states[i].pos_productions[j].production.gen_nums)
            {
                // 如果是开始符号，就接受
                if (states[i].pos_productions[j].production.production == 0)
                {
                    action_buffer[i * grammar.item_nums].action_type = ACCEPT_STATE;
                }
                else
                {
                    // TODO: 加入FIRST集合和FOLLOW集合的判断，进化成SLR(0)文法
                    for (int k = 0; k < grammar.item_nums; ++k)
                    {
                        if(action_buffer[i * grammar.item_nums + k].action_type != EMPTY_STATE)
                            printf("Warning: conflict in state %d, symbol %d\n", i, k - terminal_nums);
                        action_buffer[i * grammar.item_nums + k].action_type = REDUCE_STATE;
                        action_buffer[i * grammar.item_nums + k].value = states[i].pos_productions[j].production.id;
//                        // 如果符号k在状态i的产生式j的左侧元素的FOLLOW集合中，就要规约
//                        const FOLLOW follow = grammar.follows[states[i].pos_productions[j].production.production];
//                        for (int l = 0; l < follow.item_nums; ++l)
//                        {
//                            // 元素k在FOLLOW集合中，注意k从0开始，要平移到负数表示实际符号id
//                            if (follow.items[l] == k - terminal_nums)
//                            {
//                                if(action_buffer[i * grammar.item_nums + k].action_type != EMPTY_STATE)
//                                    printf("Warning: conflict in state %d, symbol %d\n", i, k - terminal_nums);
//                                action_buffer[i * grammar.item_nums + k].action_type = REDUCE_STATE;
//                                action_buffer[i * grammar.item_nums + k].value = states[i].pos_productions[j].production.id;
//                            }
//                        }
                    }
                }
            }
        }
        // 将空处置为error
        for (int j = 0; j < grammar.item_nums; ++j)
        {
            if (action_buffer[i * grammar.item_nums + j].action_type == EMPTY_STATE)
                action_buffer[i * grammar.item_nums + j].action_type = ERROR_STATE;
        }
    }

    automaton_states->state_nums = state_nums;
    automaton_states->states = (pState)malloc(sizeof(State) * state_nums);
    memcpy(automaton_states->states, states, sizeof(State) * state_nums);
    action_table->state_nums = state_nums;
    action_table->action_nums = state_nums * grammar.item_nums;
    action_table->actions = (pAction)malloc(sizeof(Action) * action_table->action_nums);
    memcpy(action_table->actions, action_buffer, sizeof(Action) * action_table->action_nums);
    free(action_buffer);
}

void printActionTable(ActionTable action_table)
{
    char temp[MAX_TOKEN_LEN];
    int elem_nums = action_table.action_nums / action_table.state_nums;
    printf("=====Action Table=====\n");
    for (int i = 0; i <action_table.state_nums; ++i)
    {
        printf("State %3d: ", i);
        for (int j = 0; j < elem_nums; ++j)
        {
            if (action_table.actions[i*elem_nums + j].action_type == ERROR_STATE)
                printf("ERR  ");
            else if (action_table.actions[i*elem_nums + j].action_type == ACCEPT_STATE)
                printf("ACC  ");
            else if (action_table.actions[i*elem_nums + j].action_type == REDUCE_STATE)
                printf("R%3d ", action_table.actions[i*elem_nums + j].value);
            else
                printf("T%3d ", action_table.actions[i*elem_nums + j].value);
        }
        printf("\n");
    }

}

// 根据action表和goto表，分析输入串。


int main(int argc, char *argv[])
{
    FILE *fp = fopen(DEFAULT_GRAMMAR_FILE, "r");

    if (fp == NULL)
    {
        print_error("File is not opened, exit.");
        exit(-1);
    }
    Grammar grammar = generateGrammar(fp);
    printGrammar(grammar);
    exit(0);
    AutomatonStates automaton_states = {NULL, 0};
    ActionTable action_table = {NULL, 0, 0};
    getActionTable(grammar, &automaton_states, &action_table);
    printStates(automaton_states);
    printActionTable(action_table);
    return 0;
}
