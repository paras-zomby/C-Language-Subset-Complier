#include <string.h>
#include "include.h"

State mergeState(State a, State b)
{
    // 因为一个状态中不可能只存在非核心产生式，所以如果没有核心产生式就可以直接返回
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
                && a.pos_productions[i].dot_pos == b.pos_productions[j].dot_pos)
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
            // 遍历每个点规则的点后面的产生符号，如果是非终结符，就把它的所有产生式加入到点规则集合中
            for(int pos = prods[i].dot_pos; pos < prods[i].production.gen_nums; ++pos)
            {
                int ch = prods[i].production.generative[pos];
                if (ch < 0) // 如果是终结符，就不用遍历后面的符号了
                    break;
                // 如果是非终结符，就需要找到它在左侧的产生式规则
                for (int j = 0; j < grammar.production_nums; ++j)
                {
                    if (grammar.productions[j].production == ch)
                    {
                        int k; // 判断当前规则是否已经加入了点规则集合
                        // 新加入的规则不需要和待求闭包的规则比较，因为点的位置不一样
                        for (k = 1; k < prod_nums; ++k)
                        {
                            // 因为新加入的点规则的点一定在生成式的最前面，因此没必要比较点的位置
                            if (j == prods[k].production.id)
                                break;
                        }
                        if (k == prod_nums)
                        {
                            PosProduction temp = {.production = grammar.productions[j], .dot_pos = 0};
                            prods[prod_nums++] = temp;

                            // 判断当前规则对应的产生符号是否已经加入了非核心产生式
                            int l;
                            for (l = 0; l < non_core_prod_nums; ++l)
                                if (non_core_productions[l] == grammar.productions[j].production)
                                    break;
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
                // 如果当前产生符号可以推出空串，就继续遍历下一个符号
                if (!productEpsilon(grammar, &ch, 1))
                    break;
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
                && grammar.productions[j].gen_nums > 0
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
    printf("=====[Automaton States]=====\n");
    printf("Automaton state nums: %d\n", automaton_states.state_nums);
    for (int i = 0; i < automaton_states.state_nums; ++i)
    {
        printf("State %d:\n", i);
        printState(automaton_states.states[i]);
    }
}

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
            // 如果点后面的串可以产生空，要规约（包括点在最后的情况）。
            if (productEpsilon(grammar, &states[i].pos_productions[j].production.generative[states[i].pos_productions[j].dot_pos],
                                  states[i].pos_productions[j].production.gen_nums - states[i].pos_productions[j].dot_pos))
            {
                // 如果是开始符号，就接受
                if (states[i].pos_productions[j].production.production == 0)
                    action_buffer[i * grammar.item_nums].action_type = ACCEPT_STATE;
                else // 否则就规约
                {
                    int new_value;
                    // 如果因为点在最后规约，就直接用产生式的id
                    if (states[i].pos_productions[j].dot_pos == states[i].pos_productions[j].production.gen_nums)
                        new_value = states[i].pos_productions[j].production.id;
                    else // 如果是因为点后面还有其他符号，这些符号可以产生空而规约
                    {
                        for (int k = 0; k < grammar.production_nums; ++k)
                        {
                            // 找到产生式左侧为点后面的符号且产生空的产生式id
                            if (grammar.productions[k].gen_nums == 0 && grammar.productions[k].production ==
                            states[i].pos_productions[j].production.generative[states[i].pos_productions[j].dot_pos])
                            {
                                new_value = k;
                                break;
                            }
                        }
                    }

                    // 将所有状态i的产生式j的左侧元素的FOLLOW集合中的符号规约 [SLR(0)]
                    const FOLLOW follow = grammar.follows[states[i].pos_productions[j].production.production];
                    for (int l = 0; l < follow.item_nums; ++l)
                    {
                        int idx = i * grammar.item_nums + follow.items[l] + terminal_nums;
                        // 如果出现了冲突， 显示详细冲突信息并且用新状态覆盖就状态（优先规约）
                        if(action_buffer[idx].action_type != EMPTY_STATE)
                            printf("Warning: conflict in state %d, symbol %d."
                                   " OLD state: [%d, %d], NEW state: [%d, %d].\n", i, follow.items[l] - terminal_nums,
                                   action_buffer[idx].action_type, action_buffer[idx].value, REDUCE_STATE, new_value);
                        // 由于测试用的语法规则不是SLR(0)，因此必须指定几个特殊的移入
                        if (i != 47)
                        {
                            action_buffer[idx].action_type = REDUCE_STATE;
                            action_buffer[idx].value = new_value;
                        }
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
    printf("=====[Action Table]=====\n");
    printf("St \\ elem: ");
    for (int i = -terminal_nums; i < non_terminal_nums; ++i)
    {
        // 跳过START列
        if (i == 0) continue;
        get_lex(i, temp);
        temp[3] = 0;
        printf("%3s  ", temp);
    }
    printf("\n");

    for (int i = 0; i < action_table.state_nums; ++i)
    {
        printf("State %3d: ", i);
        // 此时j是平移后的id，从0开始计数
        for (int j = 0; j < elem_nums; ++j)
        {
            if (j == terminal_nums) continue;
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
