cmake_minimum_required(VERSION 3.25)
project(CompilersPrinciplesExp C)

set(CMAKE_C_STANDARD 11)

add_executable(CompilersPrinciplesStage1 Stage1.c LexicalAnalysis.c)
add_executable(CompilersPrinciplesStage2
        Stage2.c
        include.h
        LexicalAnalysis.c
        GrammarProcess.c
        ActionTableGeneration.c
        SyntaxAnalysis.c
        CodeGeneration.c
        )
add_executable(CompilersPrinciplesStage3
        Stage3.c
        include.h
        LexicalAnalysis.c
        GrammarProcess.c
        ActionTableGeneration.c
        SyntaxAnalysis.c
        CodeGeneration.c
        )
