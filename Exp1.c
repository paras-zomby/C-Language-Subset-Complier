#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "include.h"

int main(int argc, char *argv[])
{
    Token tokens[MAX_NUM_LEN] = {};
    int num_tokens = 0;
    FILE *fp;
    printf("Enter a C program:\n");
    if (argc == 2)
    {
        fp = fopen(argv[1], "r");
    }
    else
    {
        fp = fopen(DEFAULT_INPUT_FILE, "r");
    }
    if (fp == NULL)
    {
        print_error("File is not opened, exit.");
        exit(-1);
    }
    Token current_token;

    while (1)
    {
        current_token = get_token(fp);
        if (current_token.type == END_OF_FILE_TOKEN)
        {
            break;
        }
        else if (current_token.type == ERROR_TOKEN)
        {
            continue;
        }
        tokens[num_tokens++] = current_token;
    }
    printf("End of file\n");
    fclose(fp);

    for (int i = 0; i < num_tokens; ++i)
    {
        print_token(tokens[i]);
    }
}
