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
    AutomatonStates automaton_states = {NULL, 0};
    ActionTable action_table = {NULL, 0, 0};
    getActionTable(grammar, &automaton_states, &action_table);
    printStates(automaton_states);
    printActionTable(action_table);

    FILE *fp1 = fopen(DEFAULT_INPUT_FILE, "r");
    if (fp == NULL)
    {
        print_error("Input File can't be opened, exit........");
        exit(-2);
    }
    Parsing(fp1, action_table, grammar);
    return 0;
}
