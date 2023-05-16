#include <string.h>
#include "include.h"

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
                    prod = 1;
                    read_production = 0;
                    production_buffer[now_production_num].gen_nums = 0;
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

// 用于判断一个串是否可以推出空
int productEpsilon(const Grammar grammar, const int* string, int str_nums)
{
    // 遍历串中的每一个元素
    for (int i = 0; i < str_nums; ++i)
    {
        int j; // 遍历元素的first集，看是否有空串
        for (j = 0; j < grammar.firsts[string[i] + terminal_nums].item_nums; ++j)
            if (grammar.firsts[string[i] + terminal_nums].items[j] == 0)
                break;
        // 如果这个元素的first集没有空串，那么整个串就不能推出空
        if (j == grammar.firsts[string[i] + terminal_nums].item_nums)
            return 0;
    }
    return 1;
}


// 用于合并两个集合，结果放入a中。b中的epsilon不会加入a中。返回值是新增的元素个数
// epsilon_pos用于记录b中epsilon的位置。传入NULL表示不记录
// epsilon_val用于表示是否在first集中加入空串，生成follow集时用0
int mergeSet(int *a, int* a_size, const int *b, int b_size, int* epsilon_pos, int epsilon_val)
{
    int pos = *a_size;
    int epsilon_pos_ = -1;
    for (int i = 0; i < b_size; ++i)
    {
        if (b[i] == 0)  // b[i]是epsilon，不能加入first，follow集合。返回值为epsilon的位置
        {
            epsilon_pos_ = i;
            if (epsilon_val==0) {continue;}
        }

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
    grammar->firsts = (pFIRST) malloc(sizeof(FIRST) * grammar->item_nums);
    // 初始化非终极符的first集
    for (int i = 0; i < non_terminal_nums; ++i)
    {
        grammar->firsts[i + terminal_nums].items = malloc(sizeof(int) * terminal_nums);
        grammar->firsts[i + terminal_nums].item_nums = 0;
    }
    // 初始化终极符的first集（其本身）
    for (int i = -terminal_nums; i < 0; ++i)
    {
        grammar->firsts[i + terminal_nums].items = (int*) malloc(sizeof(int) * 1);
        grammar->firsts[i + terminal_nums].item_nums = 1;
        grammar->firsts[i + terminal_nums].items[0] = i;
    }
    // 循环求first集直到不再变化
    while (1)
    {
        int changed = 0;
        for (int i = 0; i < grammar->production_nums; ++i)
        {
            // 产生式左部
            int alpha = grammar->productions[i].production;

            // 产生式为空处理
            if(grammar->productions[i].gen_nums == 0)
            {
                int epsilon_set = 0;
                changed += mergeSet(grammar->firsts[alpha + terminal_nums].items,
                                 &grammar->firsts[alpha + terminal_nums].item_nums,
                                    &epsilon_set, 1, NULL, 1);
                continue;
            }

            // 产生式右部不为空
            for (int k = 0; k < grammar->productions[i].gen_nums; ++k)
            {
                int xk = grammar->productions[i].generative[k];
                int epsilon_pos;
                changed += mergeSet(grammar->firsts[alpha + terminal_nums].items,
                                    &grammar->firsts[alpha + terminal_nums].item_nums,
                                    grammar->firsts[xk + terminal_nums].items,
                                    grammar->firsts[xk + terminal_nums].item_nums, &epsilon_pos, 0);

                // xk的first集含空串
                if (epsilon_pos != -1)
                {
                    // 判断是否是最后一个符号
                    if(k == grammar->productions[i].gen_nums - 1)
                    {
                        int epsilon_set = 0;
                        changed += mergeSet(grammar->firsts[alpha + terminal_nums].items,
                                            &grammar->firsts[alpha + terminal_nums].item_nums,
                                            &epsilon_set, 1, NULL, 1);
                    }
                }
                // xk的first集不含空串，则跳过该产生式
                else
                    break;
            }
        }
        if (changed == 0)
            break;
    }
    // 重新为first集分配合适的空间大小
    for (int i = 0; i < non_terminal_nums; ++i)
    {
        grammar->firsts[i + terminal_nums].items = (int*) realloc(grammar->firsts[i + terminal_nums].items,
                                                   sizeof(int) * grammar->firsts[i + terminal_nums].item_nums);
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
                                            &next, 1, NULL, 0);
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
                                             &epsilon_pos, 0);
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
                                             NULL, 0);
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
    printf("=====[Grammar]=====\n");
    printf("===Productions===\n");
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
    printf("\n===FIRST Sets===\n");
    for (int i = 0; i < grammar.item_nums; ++i)
    {
        get_lex(i - terminal_nums, temp);
        printf("%7s :", temp);
        for (int j = 0; j < grammar.firsts[i].item_nums; ++j)
        {
            get_lex(grammar.firsts[i].items[j], temp);
            printf(" %s", temp);
        }
        printf("\n");
    }
    printf("\n===FOLLOW Sets===\n");
    for (int i = 0; i < grammar.follow_nums; ++i)
    {
        get_lex(i, temp);
        printf("%7s :", temp);
        for (int j = 0; j < grammar.follows[i].item_nums; ++j)
        {
            get_lex(grammar.follows[i].items[j], temp);
            printf(" %s", temp);
        }
        printf("\n");
    }
}
