/*
 * Simple Java Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_jvm.h"

/*
 * Simple JVM stores integer-only intrinsically,
 * and parses one class file called Foo.class in the same directory.
 *
 * We limit its capability for quick implementation.
 */
SimpleConstantPool simpleConstantPool;
SimpleInterfacePool simpleInterfacePool;
SimpleFieldPool simpleFieldPool;
SimpleMethodPool simpleMethodPool;
StackFrame stackFrame;
LocalVariables localVariables;

int main(int argc, char *argv[])
{
    ClassFileFormat cff;

    /* Initialize All Pools */
    memset(&cff, 0, sizeof(ClassFileFormat));
    memset(&simpleConstantPool, 0, sizeof(SimpleConstantPool));
    memset(&simpleInterfacePool, 0, sizeof(SimpleInterfacePool));
    memset(&simpleFieldPool, 0, sizeof(SimpleFieldPool));
    memset(&simpleMethodPool, 0, sizeof(SimpleMethodPool));
    memset(&localVariables, 0, sizeof(LocalVariables));

    if (argc < 2) {
        printf("%s [class] \n", argv[0]);
        return 0;
    }
    printf("open file %s\n", argv[1]);
    parseJavaClassFile(argv[1], &cff);

#if SIMPLE_JVM_DEBUG
    printConstantPool(&simpleConstantPool);
    printMethodPool(&simpleConstantPool, &simpleMethodPool);
    printClassFileFormat(&cff);
#endif

    /* TODO: list method attributes */
    printf("-------------------------------------\n");
    printf("Execute Simple JVM\n");
    printf("-------------------------------------\n");
    MethodInfo *init = findMethodInPool(&simpleConstantPool,
                                        &simpleMethodPool,
                                        "<init>", 6);
    if (init != 0) {
        printf("-------------------------------------\n");
        printf("find and execute <init> method\n");
        printf("-------------------------------------\n");
#if SIMPLE_JVM_DEBUG
        printMethodAttributes(&simpleConstantPool, init);
#endif
        stackInit(&stackFrame, 500);
        executeMethod(init, &stackFrame, &simpleConstantPool);
    }
    printf("-------------------------------------\n");
    printf("Terminate Simple JVM\n");
    printf("-------------------------------------\n");
    free_pools();

    return 0;
}
