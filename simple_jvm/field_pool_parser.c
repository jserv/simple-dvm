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

static int parseAttr(FieldInfo *ptr, FILE *fp)
{
    int i;
    AttributeInfo *tmp = 0;
    unsigned char short_tmp[2];
    unsigned char integer_tmp[4];

    printf("field attributes_count = %d\n", ptr->attributes_count);
    for (i = 0; i < ptr->attributes_count ; i ++) {
        tmp = &ptr->attributes[i];
        fread(short_tmp, 2, 1, fp);
        tmp->attribute_name_index = short_tmp[0] << 8 | short_tmp[1];

        fread(integer_tmp, 4, 1, fp);
        tmp->attribute_length = integer_tmp[0] << 24 | integer_tmp[1] << 16 |
                                integer_tmp[2] <<  8 | integer_tmp[0] ;

        printf("field tmp->attribute_length = %d\n", tmp->attribute_length);

        tmp->info = (unsigned char *) malloc(sizeof(unsigned char) * tmp->attribute_length);
        fread(tmp->info, tmp->attribute_length, 1, fp);
    }
}

/* parse Field Pool */
static int parseFP(FILE *fp)
{
    int i = 0;
    unsigned char short_tmp[2];
    FieldInfo *ptr = &simpleFieldPool.field[simpleFieldPool.field_used];

    /* access flag */
    fread(short_tmp, 2, 1, fp);
    ptr->access_flags = short_tmp[0] << 8 | short_tmp[1];

    /* name index */
    fread(short_tmp, 2, 1, fp);
    ptr->name_index = short_tmp[0] << 8 | short_tmp[1];

    /* descriptor index */
    fread(short_tmp, 2, 1, fp);
    ptr->descriptor_index = short_tmp[0] << 8 | short_tmp[1];

    /* attributes count */
    fread(short_tmp, 2, 1, fp);
    ptr->attributes_count = short_tmp[0] << 8 | short_tmp[1];

    ptr->attributes = (AttributeInfo *) malloc(sizeof(AttributeInfo) * ptr->attributes_count);
    memset(ptr->attributes, 0, sizeof(AttributeInfo) * ptr->attributes_count);

    /* parse attributes */
    parseAttr(ptr, fp);
    simpleFieldPool.field_used++;
    return 0;
}

void printFieldPool(SimpleConstantPool *p, SimpleFieldPool *fp)
{
    int i, j;

    if (fp->field_used > 0) {
        printf("Field Pool= \n");
        for (i = 0; i < fp->field_used ; i++) {
            ConstantUTF8 *ptr = findUTF8(p, fp->field[i].name_index);
            printf("field[%d], attr_count = %d, %d",
                   i, fp->field[i].attributes_count,
                   fp->field[i].name_index);
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

int parseFieldPool(FILE *fp, int count)
{
    int i;
    for (i = 0; i < count ; i ++)
        parseFP(fp);
    return 0;
}
