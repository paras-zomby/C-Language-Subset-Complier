#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "include.h"

// 定义字符表（数字和字母的字符表没用）
const char number_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
const char alpha_table[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                            'i', 'j', 'k', 'l', 'm', 'n', 'o',
                            'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
                            'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E',
                            'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                            'V', 'W', 'X', 'Y', 'Z', '_'};
const char keyword_table[][MAX_IDENT_LEN] = {"if", "else", "while", "int", "float"};
const char operator_elem_table[] = {'+', '-', '*', '/', '>', '<', '=', '(',
                                              ')', ';', '\'', '!'};
const char operator_table[][MAX_IDENT_LEN] = {"+", "-", "*", "/", ">", "<", "=", "(",
                                              ")", ";", "'", "==", ">=", "<=", "!="};


// 从输入流中获取一个词法单元
Token get_token(FILE *fp)
{
    static char last_char = 0;
    Token current_token;
    // 如果已经到达文件结尾，则直接返回
    if (feof(fp))
    {
        current_token.type = END_OF_FILE_TOKEN;
        strcpy(current_token.str, "");
        return current_token;
    }
    char current_char;
    if (last_char != 0)
        current_char = last_char;
    else
        current_char = fgetc(fp);
    char lexeme_buffer[MAX_TOKEN_LEN] = {0};
    // 如果遇到空白字符，则跳过继续读取下一个词法单元
    if (is_blank(current_char))
    {
        while (is_blank(current_char) && !feof(fp))
        {
            current_char = fgetc(fp);
        }
    }
    // 如果遇到字母，则识别标识符或关键字
    if (isalpha(current_char) || current_char == '_')
    {
        int i = 0;
        while (isalpha(current_char) || isdigit(current_char) || current_char == '_')
        {
            lexeme_buffer[i++] = current_char;
            current_char = fgetc(fp);
        }
        last_char = current_char;
        lexeme_buffer[i] = '\0';
        // 如果识别出来的是关键字，则将类型设为关键字
        if (is_keyword(lexeme_buffer))
        {
            current_token.type = KEYWORD_TOKEN;
        }
        // 否则将类型设为标识符
        else
        {
            current_token.type = IDENTIFIER_TOKEN;
        }
        strcpy(current_token.str, lexeme_buffer);
    }
    // 如果遇到数字，则识别常数
    else if (isdigit(current_char))
    {
        int i = 0;
        while (isdigit(current_char))
        {
            lexeme_buffer[i++] = current_char;
            current_char = fgetc(fp);
        }
        last_char = current_char;
        lexeme_buffer[i] = '\0';
        current_token.type = NUMBER_TOKEN;
        strcpy(current_token.str, lexeme_buffer);
    }
    // 如果遇到运算符，则识别运算符
    else if (is_operator_elem(current_char))
    {
        int i = 0;
        while (is_operator_elem(current_char))
        {
            lexeme_buffer[i++] = current_char;
            current_char = fgetc(fp);
        }
        last_char = current_char;
        lexeme_buffer[i] = '\0';
        if (is_operator(lexeme_buffer))
        {
            current_token.type = OPERATOR_TOKEN;
            strcpy(current_token.str, lexeme_buffer);
        }
        else
        {
            print_error("Invalid operator");
            strcpy(current_token.str, "");
            current_token.type = ERROR_TOKEN;
        }
    }
    // 如果这次尝试读取发现已经到达文件结尾
    else if (feof(fp))
    {
        current_token.type = END_OF_FILE_TOKEN;
        strcpy(current_token.str, "");
    }
    // 如果遇到无法识别的字符，则忽略掉并打印错误信息
    else
    {
        print_error("Invalid character");
        strcpy(current_token.str, "");
        current_token.type = ERROR_TOKEN;
    }
    return current_token;
}

void print_token(Token t)
{
    switch (t.type)
    {
        case IDENTIFIER_TOKEN:
            printf("<IDENTIFIER, %s>\n", t.str);
            break;
        case KEYWORD_TOKEN:
            printf("<KEYWORD, %s>\n", t.str);
            break;
        case NUMBER_TOKEN:
            printf("<CONSTANT, %s>\n", t.str);
            break;
        case OPERATOR_TOKEN:
            printf("<OPERATOR, %s>\n", t.str);
            break;
        default:
            printf("<INVALID, %s>\n", t.str);
            break;
    }
}

void print_error(const char *msg)
{
    printf("Error: %s\n", msg);
}

int is_keyword(const char *str)
{
    int num_keywords = sizeof(keyword_table) / sizeof(keyword_table[0]);
    for (int i = 0; i < num_keywords; i++)
    {
        if (strcmp(str, keyword_table[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_operator(const char *str)
{
    int num_operators = sizeof(operator_table) / sizeof(operator_table[0]);
    for (int i = 0; i < num_operators; i++)
    {
        if (strcmp(str, operator_table[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_blank(char ch)
{
    return isblank(ch) || ch == '\r' || ch == '\n';
}

int is_operator_elem(char ch)
{
    int num_operator_elems = sizeof(operator_elem_table);
    for (int i = 0; i < num_operator_elems; i++)
    {
        if (ch == operator_elem_table[i])
        {
            return 1;
        }
    }
    return 0;
}
