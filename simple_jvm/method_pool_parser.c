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
extern SimpleMethodPool simpleMethodPool;
extern SimpleConstantPool simpleConstantPool;

static int parseMethodAttr(MethodInfo *ptr, FILE *fp)
{
    int i;
    AttributeInfo *tmp = 0;
    unsigned char short_tmp[2];
    unsigned char integer_tmp[4];
    printf("method attributes_count = %d\n", ptr->attributes_count);
    for (i = 0; i < ptr->attributes_count; i++) {
        tmp = &ptr->attributes[i];
        fread(short_tmp, 2, 1, fp);
        tmp->attribute_name_index = short_tmp[0] << 8 | short_tmp[1];
        printf("method tmp->attribute_name_index = %d\n", tmp->attribute_name_index);
        fread(integer_tmp, 4, 1, fp);
        tmp->attribute_length = integer_tmp[0] << 24 | integer_tmp[1] << 16 |
                                integer_tmp[2] <<  8 | integer_tmp[3];

        printf("method tmp->attribute_length = %d\n", tmp->attribute_length);
        tmp->info = (unsigned char *) malloc(sizeof(unsigned char) * tmp->attribute_length);
        fread(tmp->info, tmp->attribute_length, 1, fp);
    }
}

/* parse Method Pool */
static int parseMP(FILE *fp)
{
    int i = 0;
    unsigned char short_tmp[2];
    MethodInfo *ptr = &simpleMethodPool.method[simpleMethodPool.method_used];

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
    /* parse method attributes */
    parseMethodAttr(ptr, fp);
    simpleMethodPool.method_used++;
    return 0;
}

/* Find Method from Pool */
MethodInfo *findMethodInPool(SimpleConstantPool *p,
                             SimpleMethodPool *mp,
                             char *method_name, int size)
{
    int i;
    int cmp_size = 0;
    if (mp->method_used > 0) {
        for (i = 0; i < mp->method_used; i++) {
            ConstantUTF8 *name = findUTF8(p, mp->method[i].name_index);
            if (size == name->string_size &&
                strncmp(method_name, name->ptr, size) == 0)
                    return &mp->method[i];
        }
    }
    return 0;
}

/* Print Method Attributes */
void printMethodAttributes(SimpleConstantPool *p,
                           MethodInfo *method)
{
    int i;
    char name[255];
    AttributeInfo *attr = 0;

    for (i = 0; i < method->attributes_count; i++) {
        attr = &method->attributes[i];
        getUTF8String(p, attr->attribute_name_index, 255, name);
        printf("attribute name = %s\n", name);
    }
}

/* Print Method Pool */
void printMethodPool(SimpleConstantPool *p, SimpleMethodPool *mp)
{
    int i, j;
    if (mp->method_used > 0) {
        printf("Method Pool= \n");
        for (i = 0; i < mp->method_used; i++) {
            ConstantUTF8 *ptr = findUTF8(p, mp->method[i].name_index);
            ConstantUTF8 *ptr2 = findUTF8(p, mp->method[i].descriptor_index);
            ConstantMethodRef *mRefPtr = findMethodRef(p, mp->method[i].name_index);
            printf("method[%d], attr_count = %d, %d",
                   i, mp->method[i].attributes_count,
                   mp->method[i].name_index);
            if (ptr != 0) {
                printf(" ");
                for (j = 0 ; j < ptr->string_size ; j++)
                    printf("%c", ptr->ptr[j]);
                printf(" \n");
            }
#if 0
            else if (mRefPtr != 0) {
                ConstantClassRef *clazz = findClassRef(p, mRefPtr->classIndex);
                if (clazz != 0) {
                    ConstantUTF8 *name = findUTF8(p, clazz->stringIndex);
                    if (name != 0) {
                        printf(" ");
                        for (j = 0 ; j < name->string_size ; j++)
                            printf("%c", name->ptr[j]);
                        printf(" ");

                    }
                }
                ConstantNameAndType *nameAndType = findNameAndType(p, mRefPtr->nameAndTypeIndex);
                if (nameAndType != 0) {
                    ConstantUTF8 *name = findUTF8(p, nameAndType->nameIndex);
                    ConstantUTF8 *type = findUTF8(p, nameAndType->typeIndex);

                    if (name != 0) {
                        printf(" ");
                        for (j = 0 ; j < name->string_size ; j++)
                            printf("%c", name->ptr[j]);
                        printf(" ");
                    }
                    if (type != 0) {
                        printf(" ");
                        for (j = 0 ; j < type->string_size ; j++)
                            printf("%c", type->ptr[j]);
                        printf(" ");
                    }
                }
                printf("\n");
            }
#endif
            else {
                printf("\n");
                continue;
            }
            if (ptr2 != 0) {
                printf("Descriptor ");
                for (j = 0 ; j < ptr2->string_size ; j++)
                    printf("%c", ptr2->ptr[j]);
                printf(" \n");
            }
            printMethodAttributes(p, &mp->method[i]);
        }
    }
}

int parseMethodPool(FILE *fp, int count)
{
    int i;
    for (i = 0; i < count; i ++)
        parseMP(fp);
    return 0;
}
