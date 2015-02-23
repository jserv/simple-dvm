/*
 * Simple Java Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_jvm.h"

extern SimpleInterfacePool simpleInterfacePool;
extern SimpleFieldPool simpleFieldPool;
extern SimpleConstantPool simpleConstantPool;
extern SimpleMethodPool simpleMethodPool;

/* Stack Initialization */
void stackInit(StackFrame *stack, int entry_size)
{
    int i;
    memset(stack, 0, sizeof(StackFrame));
    stack->store = (StackEntry *) malloc(sizeof(StackEntry) * entry_size);
    for (i = 0; i < entry_size ; i++)
        memset(&stack->store[i], 0, sizeof(StackEntry));
    stack->size = 0;
}
/* push Integer */
void pushInt(StackFrame *stack, int value)
{
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(tmp, &value, sizeof(int));
    stack->store[stack->size].type = STACK_ENTRY_INT;
    stack->size++;
}

/* push Long */
void pushLong(StackFrame *stack, long long value)
{
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(tmp, &value, sizeof(long long));
    stack->store[stack->size].type = STACK_ENTRY_LONG;
    stack->size++;
}

/* push Double */
void pushDouble(StackFrame *stack, double value)
{
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(tmp, &value, sizeof(double));
    stack->store[stack->size].type = STACK_ENTRY_DOUBLE;
    stack->size++;
}

/* push Float */
void pushFloat(StackFrame *stack, float value)
{
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(tmp, &value, sizeof(float));
    stack->store[stack->size].type = STACK_ENTRY_FLOAT;
    stack->size++;
}

/* push Ref */
void pushRef(StackFrame *stack, int value)
{
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(tmp, &value, sizeof(int));
    stack->store[stack->size].type = STACK_ENTRY_REF;
    stack->size++;
}

/* pop Integer */
int popInt(StackFrame *stack)
{
    stack->size--;
    unsigned char *tmp = stack->store[stack->size].entry;
    int value = 0;
    memcpy(&value, tmp, sizeof(int));
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    return value;
}

/* pop Long */
long long popLong(StackFrame *stack)
{
    stack->size--;
    long long value = 0;
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(&value, tmp, sizeof(long long));
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    return value;
}

/* pop Double */
double popDouble(StackFrame *stack)
{
    stack->size--;
    double value = 0;
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(&value, tmp, sizeof(double));
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    return value;
}

/* pop Float */
float popFloat(StackFrame *stack)
{
    stack->size--;
    float value = 0;
    unsigned char *tmp = stack->store[stack->size].entry;
    memcpy(&value, tmp, sizeof(float));
    memset(&stack->store[stack->size], 0, sizeof(StackEntry));
    return value;
}

/* Pop Stack Entry */
StackEntry *popEntry(StackFrame *stack)
{
    stack->size--;
    return &stack->store[stack->size];
}

/* Entry to Int */
int EntryToInt(StackEntry *entry)
{
    int value = 0;
    memcpy(&value, entry->entry, sizeof(int));
    return value;
}

double EntryToDouble(StackEntry *entry)
{
    double value = 0;
    memcpy(&value, entry->entry, sizeof(double));
    return value;
}

int is_ref_entry(StackFrame *stack)
{
    if (stack->store[stack->size - 1].type == STACK_ENTRY_REF)
        return 1;
    return 0;
}
