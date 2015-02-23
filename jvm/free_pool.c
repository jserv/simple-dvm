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

void free_pools()
{
    int i, j;
    MethodInfo *method = 0;
    AttributeInfo *attr = 0;
    for (i = 0; i < simpleMethodPool.method_used; i++) {
        method = &simpleMethodPool.method[i];
        for (j = 0; j < method->attributes_count ; j++) {
            attr = &method->attributes[j];
            free(attr->info);
            memset(attr, 0, sizeof(AttributeInfo));
        }
        free(method->attributes);
        memset(method, 0, sizeof(MethodInfo));
    }
}
