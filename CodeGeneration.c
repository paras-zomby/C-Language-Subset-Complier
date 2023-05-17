#include "include.h"
#include <stdlib.h>
#include <string.h>

int MergeList(int *dest, const int *src1, int size1, const int *src2, int size2)
{
    dest = (int *) malloc(sizeof(int) * (size1 + size2));
    int pos = 0;
    for (int i=0; i < size1;i++)
    {
        dest[pos++] = src1[i];
    }
    for (int i=0; i < size2;i++)
    {
        dest[pos++] = src2[i];
    }
    return pos;
}

int* MakeList(int i)
{
    int *a = (int*) malloc(sizeof(int) * 1);
    a[0] = i;
    return a;
}

void BackPatch(pGenerateCodes generatedCodes ,const int * list, int list_size, int target)
{
    for (int i=0;i<list_size;i++)
    {
        generatedCodes->codes[list[i]].code.jump_target = target;
    }
}


void generateCode(pGenerateCodes generate_codes, Production production, pAttribute attributes, pStackElem elems, Stack stack)
{
    int *empty;
    Attribute temp;
    switch (production.id)
    {
        case 6: // S -> id = E
            generate_codes->codes[generate_codes->code_nums].code_type = ASSIGNMENT;
            generate_codes->codes[generate_codes->code_nums].code.op_id = get_id("=");
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0],elems[2].token_str);
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1],"t");
            generate_codes->code_nums += 1;
            break;

        case 7: // if (B) M S1
            // 回填
            BackPatch(generate_codes,
                      attributes[stack.top - 3].condition_elem.true_list,
                      attributes[stack.top - 3].condition_elem.true_nums,
                      attributes[stack.top - 1].auxiliary_elem.ins_pos);

            attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_nums = MergeList(
                    attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_list,
                    attributes[stack.top - 3].condition_elem.true_list,
                    attributes[stack.top - 3].condition_elem.true_nums,
                    attributes[stack.top].statement_elem.next_list,
                    attributes[stack.top].statement_elem.next_nums
                    );
            break;

        case 8: // if (B) M1 S1 N else M2 S2
            // 回填1
            BackPatch(generate_codes,
                      attributes[stack.top - 7].condition_elem.true_list,
                      attributes[stack.top - 7].condition_elem.true_nums,
                      attributes[stack.top - 5].auxiliary_elem.ins_pos);
            // 回填2
            BackPatch(generate_codes,
                      attributes[stack.top - 7].condition_elem.false_list,
                      attributes[stack.top - 7].condition_elem.false_nums,
                      attributes[stack.top - 1].auxiliary_elem.ins_pos);

            temp.statement_elem.next_nums= MergeList(
                    temp.statement_elem.next_list,
                    attributes[stack.top - 4].condition_elem.true_list,
                    attributes[stack.top - 4].condition_elem.true_nums,
                    attributes[stack.top - 5].statement_elem.next_list,
                    attributes[stack.top - 5].statement_elem.next_nums
            );

            attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_nums = MergeList(
                    attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_list,
                    temp.statement_elem.next_list,
                    temp.statement_elem.next_nums,
                    attributes[stack.top].statement_elem.next_list,
                    attributes[stack.top].statement_elem.next_nums
            );
            break;

        case 9: // while M1 (B) M2 S1
            // 回填1
            BackPatch(generate_codes,
                      attributes[stack.top].statement_elem.next_list,
                      attributes[stack.top].statement_elem.next_nums,
                      attributes[stack.top - 5].auxiliary_elem.ins_pos);

            // 回填2
            BackPatch(generate_codes,
                      attributes[stack.top - 3].condition_elem.true_list,
                      attributes[stack.top - 3].condition_elem.true_nums,
                      attributes[stack.top - 1].auxiliary_elem.ins_pos);


            attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_nums = MergeList(
                    attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_list,
                    attributes[stack.top - 3].condition_elem.false_list,
                    attributes[stack.top - 3].condition_elem.false_nums,
                    empty, 0
            );

            // 生成无条件跳转
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            generate_codes->codes[generate_codes->code_nums].code.jump_target = attributes[production.generative[1]].auxiliary_elem.ins_pos;
            generate_codes->code_nums += 1;
            break;

        case 10: // M -> 空
            attributes[MAX_STATE_STACK_NUM - 1].auxiliary_elem.ins_pos = generate_codes->code_nums;
            break;

        case 11: // N -> 空
            attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_list = MakeList(generate_codes->code_nums);
            attributes[MAX_STATE_STACK_NUM - 1].condition_elem.true_nums = 1;

            // 生成无条件跳转
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            generate_codes->code_nums += 1;
            break;

        case 12:
            BackPatch(generate_codes, attributes[stack.top - 3].statement_elem.next_list,
                      attributes[stack.top - 3].statement_elem.next_nums,
                      attributes[stack.top - 1].auxiliary_elem.ins_pos);


            attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_nums = MergeList(
                    attributes[MAX_STATE_STACK_NUM - 1].statement_elem.next_list,
                    attributes[stack.top].condition_elem.false_list,
                    attributes[stack.top].condition_elem.false_nums,
                    empty, 0
            );


        case 16:
        case 15:
        case 14:// 关系运算符
            attributes[MAX_STATE_STACK_NUM - 1].condition_elem.true_list = MakeList(generate_codes->code_nums);
            attributes[MAX_STATE_STACK_NUM - 1].condition_elem.true_nums = 1;
            attributes[MAX_STATE_STACK_NUM - 1].condition_elem.false_list = MakeList(generate_codes->code_nums);
            attributes[MAX_STATE_STACK_NUM - 1].condition_elem.false_nums = 1;

            // 生成中间代码 ‘if‘ E1 rel E2 ’goto_‘
            generate_codes->codes[generate_codes->code_nums].code_type = CONDITIONAL_JUMP;
            generate_codes->codes[generate_codes->code_nums].code.op_id = elems[1].token_id; // 关系运算符对应id
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0],elems[2].token_str); // 第一个对象
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1], elems[0].token_str); // 第二个对象
            generate_codes->code_nums += 1;

            // 生成无符号跳转
            generate_codes->codes[generate_codes->code_nums].code_type = JUMP;
            generate_codes->code_nums += 1;

            break;

        case 22:
        case 21:
        case 17:
        case 18: // E -> E + T
            generate_codes->codes[generate_codes->code_nums].code_type = ASSIGNMENT;
            generate_codes->codes[generate_codes->code_nums].code.op_id = elems[1].token_id;
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[0],"t");
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[1],elems[2].token_str);
            strcpy(generate_codes->codes[generate_codes->code_nums].code.params[2],elems[0].token_str);
            generate_codes->code_nums += 1;
            break;

        case 19:
        case 20:

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