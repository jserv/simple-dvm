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
extern SimpleConstantPool simpleConstantPool;

/* parse Interface Pool Class */
static int parseIPClass(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantClassRef *ptr = &simpleInterfacePool.clasz[simpleInterfacePool.clasz_used];

    ptr->tag = CONSTANT_CLASS;
    ptr->index = index;
    ptr->additional_byte_size = 2;

    fread(short_tmp, 2, 1, fp);
    ptr->stringIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleInterfacePool.clasz_used++;
}

void printInterfacePool(SimpleConstantPool *p, SimpleInterfacePool *ip)
{
    int i, j;

    if (ip->clasz_used > 0) {
        printf("Interface Class Pool= \n");
        for (i = 0; i < ip->clasz_used; i++) {
            ConstantUTF8 *ptr = findUTF8(p, ip->clasz[i].stringIndex);
            printf(" ip_index[%d], class[%d], tag = %d, size = %d, %d",
                   ip->clasz[i].index, i, ip->clasz[i].tag,
                   ip->clasz[i].additional_byte_size,
                   ip->clasz[i].stringIndex);
            if (ptr != 0) {
                printf(" ");
                for (j = 0 ; j < ptr->string_size ; j++)
                    printf("%c", ptr->ptr[j]);
                printf(" \n");
            } else {
                printf("\n");
            }
        }
    }
}

int parseInterfacePool(FILE *fp, int count)
{
    int i;
    for (i = 1; i < count; i++)
        parseIPClass(fp, 1);
    return 0;
}
