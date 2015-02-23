/*
 * Simple Java Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_jvm.h"

extern SimpleConstantPool simpleConstantPool;
static const int tag_additional_byte_size[13] = {
    0, 2, 0, 4, 4,
    8, 8, 2, 2, 4,
    4, 4, 4
};

/* parse UTF-8 String */
static int parseCPString(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantUTF8 *ptr = &simpleConstantPool.utf8CP[simpleConstantPool.utf8_used];

    ptr->tag = CONSTANT_UTF8;
    ptr->index = index;

    fread(short_tmp, 2, 1, fp);
    ptr->string_size = short_tmp[0] << 8 | short_tmp[1];
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_UTF8] + ptr->string_size;

    ptr->ptr = (unsigned char *) malloc(sizeof(unsigned char) * ptr->string_size + 1);
    memset(ptr->ptr, 0, ptr->string_size);
    fread(ptr->ptr , ptr->string_size , 1, fp);
    ptr->ptr[ptr->string_size] = '\0';
    simpleConstantPool.utf8_used++;
    return 0;
}

/* parse Integer */
static int parseCPInteger(FILE *fp, int index)
{
    unsigned char tmp[4];
    ConstantInteger *ptr = &simpleConstantPool.integerCP[ simpleConstantPool.integer_used];

    ptr->tag = CONSTANT_INTEGER;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_INTEGER];

    fread(tmp, 4, 1, fp);
    ptr->value = (tmp[0] << 24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3]);

    simpleConstantPool.integer_used++;
    return 0;
}

/* parse Float */
static int parseCPFloat(FILE *fp, int index)
{
    unsigned char tmp[4];
    ConstantFloat *ptr = &simpleConstantPool.floatCP[ simpleConstantPool.float_used];

    ptr->tag = CONSTANT_FLOAT;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_FLOAT];

    fread(tmp, 4, 1, fp);
    ptr->value = (tmp[0] << 24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3]);

    simpleConstantPool.float_used++;
    return 0;
}

/* parse LONG */
static int parseCPLong(FILE *fp, int index)
{
    unsigned char tmp[8];
    ConstantLong *ptr = &simpleConstantPool.longCP[ simpleConstantPool.long_used];

    ptr->tag = CONSTANT_LONG;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_LONG];

    fread(tmp, 8, 1, fp);
    memcpy(&ptr->value, tmp, 8);

    simpleConstantPool.long_used++;
    return 0;
}

/* parse Double */
static int parseCPDouble(FILE *fp, int index)
{
    unsigned char tmp[8];
    unsigned char tmp2[8];
    int i, j;
    ConstantDouble *ptr = &simpleConstantPool.doubleCP[ simpleConstantPool.double_used];

    ptr->tag = CONSTANT_DOUBLE;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_DOUBLE];

    fread(tmp, 8, 1, fp);
    for (i = 0, j = 7 ; i < 8 ; i++, j--)
        tmp2[j] = tmp[i];
    memcpy(&ptr->value, tmp2, 8);

    simpleConstantPool.double_used++;
    return 0;
}

/* parse Constant Pool Class */
static int parseCPClass(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantClassRef *ptr = &simpleConstantPool.clasz[simpleConstantPool.clasz_used];

    ptr->tag = CONSTANT_CLASS;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size [CONSTANT_CLASS];

    fread(short_tmp, 2, 1, fp);
    ptr->stringIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.clasz_used++;
    return 0;
}

/* parse Constant Pool String Ref */
static int parseCPStringRef(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantStringRef *ptr = &simpleConstantPool.stringRef[simpleConstantPool.stringRef_used];

    ptr->tag = CONSTANT_STRING_REF;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_STRING_REF];

    fread(short_tmp, 2, 1, fp);
    ptr->stringIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.stringRef_used++;
    return 0;
}

/* parse Constant Pool Field */
static int parseCPField(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantFieldRef *ptr = &simpleConstantPool.field[simpleConstantPool.field_used];

    ptr->tag = CONSTANT_FIELD_REF;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size [CONSTANT_FIELD_REF];

    fread(short_tmp, 2, 1, fp);
    ptr->classIndex = short_tmp[0] << 8 | short_tmp[1];

    fread(short_tmp, 2, 1, fp);
    ptr->nameAndTypeIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.field_used++;
    return 0;
}

/* parse Constant Pool Method */
static int parseCPMethod(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantMethodRef *ptr = &simpleConstantPool.method[simpleConstantPool.method_used];

    ptr->tag = CONSTANT_METHOD_REF;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size [CONSTANT_METHOD_REF];

    fread(short_tmp, 2, 1, fp);
    ptr->classIndex = short_tmp[0] << 8 | short_tmp[1];

    fread(short_tmp, 2, 1, fp);
    ptr->nameAndTypeIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.method_used++;
    return 0;
}

/* parse Constant Pool Interface */
static int parseCPInterface(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantInterfaceRef *ptr = &simpleConstantPool.interface[simpleConstantPool.interface_used];

    ptr->tag = CONSTANT_INTERFACE_REF;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_INTERFACE_REF];

    fread(short_tmp, 2, 1, fp);
    ptr->classIndex = short_tmp[0] << 8 | short_tmp[1];

    fread(short_tmp, 2, 1, fp);
    ptr->nameAndTypeIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.interface_used++;
    return 0;
}

/* parse Constant Pool Interface */
static int parseCPNameAndType(FILE *fp, int index)
{
    unsigned char short_tmp[2];
    ConstantNameAndType *ptr = &simpleConstantPool.name_and_type[simpleConstantPool.name_and_type_used];

    ptr->tag = CONSTANT_NAME_AND_TYPE;
    ptr->index = index;
    ptr->additional_byte_size = tag_additional_byte_size[CONSTANT_NAME_AND_TYPE];

    fread(short_tmp, 2, 1, fp);
    ptr->nameIndex = short_tmp[0] << 8 | short_tmp[1];

    fread(short_tmp, 2, 1, fp);
    ptr->typeIndex = short_tmp[0] << 8 | short_tmp[1];

    simpleConstantPool.name_and_type_used++;
    return 0;
}

int parseConstantPool(FILE *fp, int count)
{
    unsigned char tag = 0;
    int i = 0 ;
    for (i = 1 ; i < count ; i++) {
        fread(&tag, 1, 1, fp);
        switch (tag) {
        case CONSTANT_UTF8:
            parseCPString(fp, i);
            break;
        case CONSTANT_INTEGER:
            parseCPInteger(fp, i);
            break;
        case CONSTANT_FLOAT:
            parseCPFloat(fp, i);
            break;
        case CONSTANT_LONG:
            parseCPLong(fp, i);
            i++;
            break;
        case CONSTANT_DOUBLE:
            parseCPDouble(fp, i);
            i++;
            break;
        case CONSTANT_STRING_REF:
            parseCPStringRef(fp, i);
            break;
        case CONSTANT_CLASS:
            parseCPClass(fp, i);
            break;
        case CONSTANT_FIELD_REF:
            parseCPField(fp, i);
            break;
        case CONSTANT_METHOD_REF:
            parseCPMethod(fp, i);
            break;
        case CONSTANT_INTERFACE_REF:
            parseCPInterface(fp, i);
            break;
        case CONSTANT_NAME_AND_TYPE:
            parseCPNameAndType(fp, i);
            break;
        default:
            printf("\n!!!unknow tag = %02x!!!\n\n", tag);
            fseek(fp, -1, SEEK_CUR);
            break;
        };
    }
    return 0;
}

/* find UTF8 */
ConstantUTF8 *findUTF8(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->utf8_used; i++)
        if (p->utf8CP[i].index == (index))
            return &p->utf8CP[i];
    return 0;
}

/* Find Class Reference */
ConstantStringRef *findStringRef(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->stringRef_used; i++)
        if (p->stringRef[i].index == (index))
            return &p->stringRef[i];
    return 0;
}

/* get UTF8String */
int getUTF8String(SimpleConstantPool *p, int index, int size, char *out)
{
    int copy_size = 0;
    ConstantUTF8 *utf8 = findUTF8(p, index);
    memset(out, 0, size);
    if (utf8 != 0) {
        copy_size = utf8->string_size < size ? utf8->string_size : size;
        memcpy(out, utf8->ptr, copy_size);
        return copy_size;
    }
    return 0;
}

/* Find Class Reference */
ConstantClassRef *findClassRef(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->clasz_used; i++)
        if (p->clasz[i].index == (index))
            return &p->clasz[i];
    return 0;
}

/* Find Method Reference */
ConstantMethodRef *findMethodRef(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->method_used; i++)
        if (p->method[i].index == (index))
            return &p->method[i];
    return 0;
}

/* Find Name and Type Reference */
ConstantNameAndType *findNameAndType(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->name_and_type_used; i++)
        if (p->name_and_type[i].index == (index))
            return &p->name_and_type[i];
    return 0;
}

/* get integer from constant pool */
int get_integer_from_constant_pool(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->integer_used; i++)
        if (p->integerCP[i].index == (index))
            return p->integerCP[i].value;
    return 0;
}

/* get long from constant pool */
long long get_long_from_constant_pool(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->long_used; i++)
        if (p->longCP[i].index == (index))
            return p->longCP[i].value;
    return 0;
}

/* get float from constant pool */
float get_float_from_constant_pool(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->float_used; i++)
        if (p->floatCP[i].index == (index))
            return p->floatCP[i].value;
    return 0.0f;
}

/* get double from constant pool */
double get_double_from_constant_pool(SimpleConstantPool *p, int index)
{
    int i;
    for (i = 0; i < p->double_used; i++)
        if (p->doubleCP[i].index == (index))
            return p->doubleCP[i].value;
    return 0.0f;
}

/* print constant pool table */
void printConstantPool(SimpleConstantPool *p)
{
    int i, j;
    printf("ConstantUTF8 = \n");
    for (i = 0; i < p->utf8_used ; i++) {
        printf("cp_index[%d], utf8[%d], tag = %d, size = %d, ",
               p->utf8CP[i].index, i, p->utf8CP[i].tag,
               p->utf8CP[i].string_size);
        for (j = 0 ; j < p->utf8CP[i].string_size ; j++)
            printf("%c", p->utf8CP[i].ptr[j]);
        printf("\n");
    }
    printf("p->integer_used = %d\n", p->integer_used);
    if (p->integer_used > 0) {
        printf("Constant Integer= \n");
        for (i = 0; i < p->integer_used ; i++)
            printf(" cp_index[%d], integer[%d], tag = %d, size = %d, %d\n",
                   p->integerCP[i].index, i, p->integerCP[i].tag,
                   p->integerCP[i].additional_byte_size,
                   p->integerCP[i].value);
    }
    printf("p->float_used = %d\n", p->float_used);
    if (p->float_used > 0) {
        printf("Constant Float= \n");
        for (i = 0; i < p->float_used ; i++)
            printf(" cp_index[%d], float[%d], tag = %d, size = %d, %.4f\n",
                   p->floatCP[i].index, i, p->floatCP[i].tag,
                   p->floatCP[i].additional_byte_size,
                   p->floatCP[i].value);
    }
    printf("p->double_used = %d\n", p->double_used);
    if (p->double_used > 0) {
        printf("Constant Double= \n");
        for (i = 0; i < p->double_used ; i++)
            printf(" cp_index[%d], double[%d], tag = %d, size = %d, %.5f\n",
                   p->doubleCP[i].index, i, p->doubleCP[i].tag,
                   p->doubleCP[i].additional_byte_size,
                   p->doubleCP[i].value);
    }

    printf("p->clasz_used = %d\n", p->clasz_used);
    if (p->clasz_used > 0) {
        printf("Constant Class Pool= \n");
        for (i = 0; i < p->clasz_used ; i++) {
            ConstantUTF8 *ptr = findUTF8(p, p->clasz[i].stringIndex);
            printf(" cp_index[%d], class[%d], tag = %d, size = %d, %d",
                   p->clasz[i].index, i, p->clasz[i].tag,
                   p->clasz[i].additional_byte_size,
                   p->clasz[i].stringIndex
                  );
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

    printf("p->stringRef_used = %d\n", p->stringRef_used);
    if (p->stringRef_used > 0) {
        printf("Constant String Reference= \n");
        for (i = 0; i < p->stringRef_used; i++) {
            ConstantUTF8 *ptr = findUTF8(p, p->stringRef[i].stringIndex);
            printf(" cp_index[%d], strRef[%d], tag = %d, size = %d, %d",
                   p->stringRef[i].index, i, p->stringRef[i].tag,
                   p->stringRef[i].additional_byte_size,
                   p->stringRef[i].stringIndex);
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
    printf("p->field_used = %d\n", p->field_used);
    if (p->field_used > 0) {
        printf("Constant Field = \n");
        for (i = 0; i < p->field_used; i++) {
            ConstantClassRef *ptr = findClassRef(p, p->field[i].classIndex);
            ConstantNameAndType *ptr2 = findNameAndType(p, p->field[i].nameAndTypeIndex);

            printf(" cp_index[%d], field[%d], tag = %d, size = %d, cls %d, nat= %d",
                   p->field[i].index, i, p->field[i].tag,
                   p->field[i].additional_byte_size,
                   p->field[i].classIndex,
                   p->field[i].nameAndTypeIndex);
            if (ptr != 0) {
                ConstantUTF8 *name = findUTF8(p, ptr->stringIndex);
                if (name != 0) {
                    printf(" ");
                    for (j = 0 ; j < name->string_size ; j++)
                        printf("%c", name->ptr[j]);
                    printf(" \n");
                }
            } else {
                printf("\n");
            }
            if (ptr2 != 0) {
                ConstantUTF8 *name = findUTF8(p, ptr2->nameIndex);
                ConstantUTF8 *type = findUTF8(p, ptr2->typeIndex);
                if (name != 0) {
                    printf(" ");
                    for (j = 0 ; j < name->string_size ; j++)
                        printf("%c", name->ptr[j]);
                    printf(" \n");
                }
                if (type != 0) {
                    printf(" ");
                    for (j = 0 ; j < type->string_size ; j++)
                        printf("%c", type->ptr[j]);
                    printf(" \n");
                }
            }
        }
    }
    printf("p->method_used= %d\n", p->method_used);
    if (p->method_used > 0) {
        printf("Constant Method= \n");
        for (i = 0; i < p->method_used; i++) {
            ConstantClassRef *ptr = findClassRef(p, p->method[i].classIndex);
            ConstantNameAndType *ptr2 = findNameAndType(p, p->method[i].nameAndTypeIndex);

            printf(" cp_index[%d], method[%d], tag = %d, size = %d, cls %d, nat= %d",
                   p->method[i].index, i, p->method[i].tag,
                   p->method[i].additional_byte_size,
                   p->method[i].classIndex,
                   p->method[i].nameAndTypeIndex);
            if (ptr != 0) {
                ConstantUTF8 *name = findUTF8(p, ptr->stringIndex);
                if (name != 0) {
                    printf(" ");
                    for (j = 0 ; j < name->string_size ; j++)
                        printf("%c", name->ptr[j]);
                    printf(" \n");
                }
            } else {
                printf("\n");
            }
            if (ptr2 != 0) {
                ConstantUTF8 *name = findUTF8(p, ptr2->nameIndex);
                ConstantUTF8 *type = findUTF8(p, ptr2->typeIndex);
                if (name != 0) {
                    printf(" ");
                    for (j = 0 ; j < name->string_size ; j++)
                        printf("%c", name->ptr[j]);
                    printf(" \n");
                }
                if (type != 0) {
                    printf(" ");
                    for (j = 0 ; j < type->string_size ; j++)
                        printf("%c", type->ptr[j]);
                    printf(" \n");
                }
            }
        }
    }
}
