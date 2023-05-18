#include <stdlib.h>
#include <string.h>
#include "include.h"

int mergeList(int **dst, int *src1, const int src1_size, int *src2, const int src2_size)
{
    *dst = (int *) malloc(sizeof(int) * (src1_size + src2_size));
    memcpy(*dst, src1, sizeof(int) * src1_size);
    int pos = src1_size;
    for (int i = 0; i < src2_size; ++i)
    {
        int j;
        for (j = 0; j < src1_size; ++j)
            if (src2[i] == src1[j])
                break;
        if (j == src1_size)
            (*dst)[pos++] = src2[i];
    }
    *dst = (int *) realloc(*dst, sizeof(int) * pos);
    free(src1);
    free(src2);
    return pos;
}

int *makeList(int i)
{
    int *a = (int *) malloc(sizeof(int) * 1);
    a[0] = i;
    return a;
}

void backPatch(pGenerateCodes generate_codes, int *list, int list_size, int target)
{
    for (int i = 0; i < list_size; ++i)
    {
        generate_codes->codes[list[i]].code.jump_target = target;
    }
    free(list);
}

Attribute generateCode(pGenerateCodes generate_codes, Production production, pStackElem elems)
{
    Attribute attribute, temp;
    int *empty;
    switch (production.id)
    {
        case 6: // S -> id = E
            generate_codes->codes[generate_codes->code_nums].code_type = ASSIGNMENT;
            generate_codes->codes[generate_codes->code_nums].code.op_id = elems[1].token_id;
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0], elems[2].attribute.variable_elem.str);
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1],elems[0].attribute.variable_elem.str);
            ++generate_codes->code_nums;

            // S.next = NULL
            attribute.statement_elem.next_list = NULL;
            attribute.statement_elem.next_nums = 0;
            break;

        case 7: // S -> if ( B ) M S1
            // 回填B.true
            backPatch(generate_codes,
                      elems[3].attribute.condition_elem.true_list,
                      elems[3].attribute.condition_elem.true_nums,
                      elems[1].attribute.auxiliary_elem.ins_pos);

            // S.next = merge(B.false, S1.next)
            attribute.statement_elem.next_nums = mergeList(
                                                &attribute.statement_elem.next_list,
                                                elems[3].attribute.condition_elem.true_list,
                                                elems[3].attribute.condition_elem.true_nums,
                                                elems[0].attribute.statement_elem.next_list,
                                                elems[0].attribute.statement_elem.next_nums);
            break;

        case 8: // S -> if ( B ) M1 S1 N else M2 S2
            // 回填B.true
            backPatch(generate_codes,
                      elems[7].attribute.condition_elem.true_list,
                      elems[7].attribute.condition_elem.true_nums,
                      elems[5].attribute.auxiliary_elem.ins_pos);
            // 回填B.false
            backPatch(generate_codes,
                      elems[7].attribute.condition_elem.false_list,
                      elems[7].attribute.condition_elem.false_nums,
                      elems[1].attribute.auxiliary_elem.ins_pos);

            // S.next = merge(S2.next, merge(N.next, S1.next))
            temp.statement_elem.next_nums= mergeList(
                                            &temp.statement_elem.next_list,
                                            elems[3].attribute.statement_elem.next_list,
                                            elems[3].attribute.statement_elem.next_nums,
                                            elems[4].attribute.statement_elem.next_list,
                                            elems[4].attribute.statement_elem.next_nums);
            attribute.statement_elem.next_nums = mergeList(
                                                &attribute.statement_elem.next_list,
                                                temp.statement_elem.next_list,
                                                temp.statement_elem.next_nums,
                                                elems[0].attribute.statement_elem.next_list,
                                                elems[0].attribute.statement_elem.next_nums);
            break;

        case 9: // while M1 ( B ) M2 S1
            // 回填S1.next
            backPatch(generate_codes,
                      elems[0].attribute.statement_elem.next_list,
                      elems[0].attribute.statement_elem.next_nums,
                      elems[5].attribute.auxiliary_elem.ins_pos);

            // 回填B.true
            backPatch(generate_codes,
                      elems[3].attribute.condition_elem.true_list,
                      elems[3].attribute.condition_elem.true_nums,
                      elems[1].attribute.auxiliary_elem.ins_pos);

            // S.next = merge(B.false, NULL)
            attribute.statement_elem.next_nums = mergeList(
                                                &attribute.statement_elem.next_list,
                                                elems[3].attribute.condition_elem.false_list,
                                                elems[3].attribute.condition_elem.false_nums,
                                                NULL, 0);

            // 生成无条件跳转
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            generate_codes->codes[generate_codes->code_nums].code.jump_target = elems[5].attribute.auxiliary_elem.ins_pos;
            ++generate_codes->code_nums;
            break;

        case 10: // M -> 空
            attribute.auxiliary_elem.ins_pos = generate_codes->code_nums;
            break;

        case 11: // N -> 空
            // N.next = makeList(1)
            attribute.statement_elem.next_list = makeList(generate_codes->code_nums);
            attribute.statement_elem.next_nums = 1;

            // 生成无条件跳转
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            ++generate_codes->code_nums;
            break;

        case 12: // S -> S1 ; M S2
            // 回填S1.next
            backPatch(generate_codes,
                      elems[3].attribute.statement_elem.next_list,
                      elems[3].attribute.statement_elem.next_nums,
                      elems[1].attribute.auxiliary_elem.ins_pos);

            // S.next = S2.next
            attribute.statement_elem.next_nums = mergeList(
                                                &attribute.statement_elem.next_list,
                                                elems[0].attribute.statement_elem.next_list,
                                                elems[0].attribute.statement_elem.next_nums,
                                                NULL, 0);
            break;

        case 16:
        case 15:
        case 14: // 关系运算符
            // rel.true = makeList(1)
            attribute.condition_elem.true_list = makeList(generate_codes->code_nums);
            attribute.condition_elem.true_nums = 1;
            // rel.false = makeList(1)
            attribute.condition_elem.false_list = makeList(generate_codes->code_nums + 1);
            attribute.condition_elem.false_nums = 1;

            // 生成中间代码 if E1 rel E2 goto -
            generate_codes->codes[generate_codes->code_nums].code_type = CONDITIONAL_JUMP;
            generate_codes->codes[generate_codes->code_nums].code.op_id = elems[1].token_id; // 关系运算符对应id
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0], // 第一个对象
                   elems[2].attribute.variable_elem.str);
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1], // 第二个对象
                   elems[2].attribute.variable_elem.str);
            ++generate_codes->code_nums;

            // 生成无条件跳转，对应false
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            ++generate_codes->code_nums;
            break;

        case 22:
        case 21:
        case 17:
        case 18: // E -> E + T
            // 生成中间代码 t = E1 + E2
            generate_codes->codes[generate_codes->code_nums].code_type = ASSIGNMENT;
            generate_codes->codes[generate_codes->code_nums].code.op_id = elems[1].token_id;
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0], "t");
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1],elems[2].attribute.variable_elem.str);
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[2],elems[0].attribute.variable_elem.str);
            ++generate_codes->code_nums;

            // 分配E的属性
            strcpy(attribute.variable_elem.str, "t");
            break;

        case 19:
        case 20:
            break;
    }
}

void assignAttribute(pAttribute attributes, Stack stack)
{
    attributes[stack.top] = attributes[MAX_STATE_STACK_NUM - 1];
}

void printGenerateCodes(const GenerateCodes codes)
{
    char temp[MAX_TOKEN_LEN];
    printf("=====[Generate Codes]=====\n");
    for (int i = 0; i < codes.code_nums; ++i)
    {
        switch (codes.codes[i].code_type)
        {
            case ASSIGNMENT:
                get_lex(codes.codes[i].code.op_id, temp);
                printf("%s %s %s %s;\n", codes.codes[i].code.params[0], temp, codes.codes[i].code.params[1], codes.codes[i].code.params[2]);
                break;
            case JUMP:
                printf("goto %d;\n", codes.codes[i].code.jump_target);
                break;
            case CONDITIONAL_JUMP:
                get_lex(codes.codes[i].code.op_id, temp);
                printf("if %s %s %s goto %d;\n", codes.codes[i].code.params[0], temp,  codes.codes[i].code.params[1], codes.codes[i].code.jump_target);
                break;
            default:
                break;
        }
    }
    printf("\n");
}
