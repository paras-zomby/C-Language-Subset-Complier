cmake_minimum_required(VERSION 3.25)
project(CompilersPrinciplesExp C)

set(CMAKE_C_STANDARD 11)

add_executable(CompilersPrinciplesExp1 Exp1.c LexicalAnalysis.c)
add_executable(CompilersPrinciplesExp2
        Exp2.c
        include.h
        LexicalAnalysis.c
        GrammarProcess.c
        ActionTableGeneration.c
        SyntaxAnalysis.c
        )
