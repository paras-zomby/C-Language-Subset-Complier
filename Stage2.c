#include <stdio.h>
#include <stdlib.h>
#include "include.h"


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
    AutomatonStates automaton_states;
    ActionTable action_table;
    getActionTable(grammar, &automaton_states, &action_table);
    printStates(automaton_states);
    printActionTable(action_table);

    FILE *fp1 = fopen(DEFAULT_INPUT_FILE, "r");
    if (fp == NULL)
    {
        print_error("Input File can't be opened, exit........");
        exit(-2);
    }
    int ret = ParseAndGenerate(fp1, action_table, grammar, NULL);
    if (ret)
        printf("Parse success!\n");
    else
        printf("Parse failed!\n");
    return 0;
}
