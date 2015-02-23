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

/* Get Major Version String */
static char *getMajorVersionString(u2 major_number)
{
    if (major_number == 0x33)
        return "J2SE 7";
    if (major_number == 0x32)
        return "J2SE 6.0";
    return "NONE";
}

/* Print Class File Format */
void printClassFileFormat(ClassFileFormat *cff)
{
    int i;
    printf("Magic Number = ");
    for (i = 0; i < 4 ; i++)
        printf("0x%02X ", cff->magic_number[i]);
    printf("\n");
    printf("Minor Number = 0x(%02X) %d \n",
           cff->minor_version, cff->minor_version);
    printf("Major Number(%s) = 0x(%02X) %d \n",
           getMajorVersionString(cff->major_version),
           cff->major_version, cff->major_version);
    printf("Constant Pool Count = 0x(%02X) %d \n",
           cff->constant_pool_count, cff->constant_pool_count);
    printf("access flag = 0x(%02X) %d \n", cff->access_flags, cff->access_flags);
    printf("this class = 0x(%02X) %d \n",
           cff->this_class, cff->this_class);
    printf("super class = 0x(%02X) %d \n",
           cff->super_class, cff->super_class);
    printf("interface count = 0x(%02X) %d \n",
           cff->interface_count, cff->interface_count);
    printf("field count = 0x(%02X) %d \n",
           cff->fields_count, cff->fields_count);
    printf("method count = 0x(%02X) %d \n",
           cff->methods_count, cff->methods_count);
}

/* Parse Class File */
int parseJavaClassFile(char *file, ClassFileFormat *cff)
{
    FILE *fp = 0;
    unsigned char short_tmp[2];
    fp = fopen(file, "rb");
    if (fp == 0) {
        printf("Open file %s failed\n", file);
        return -1;
    }
    /* magic number */
    fread(cff->magic_number, 4, 1, fp);

    /* minor_version */
    fread(short_tmp, 2, 1, fp);
    cff->minor_version = short_tmp[0] << 8 | short_tmp[1];

    /* major_version */
    fread(short_tmp, 2, 1, fp);
    cff->major_version = short_tmp[0] << 8 | short_tmp[1];

    /* constant pool */
    fread(short_tmp, 2, 1, fp);
    cff->constant_pool_count = short_tmp[0] << 8 | short_tmp[1];

    /* constant pool table */
    parseConstantPool(fp, cff->constant_pool_count);

    /* access flag */
    fread(short_tmp, 2, 1, fp);
    cff->access_flags = short_tmp[0] << 8 | short_tmp[1];

    /* this class */
    fread(short_tmp, 2, 1, fp);
    cff->this_class = short_tmp[0] << 8 | short_tmp[1];

    /* super class */
    fread(short_tmp, 2, 1, fp);
    cff->super_class = short_tmp[0] << 8 | short_tmp[1];

    /* interface count */
    fread(short_tmp, 2, 1, fp);
    cff->interface_count = short_tmp[0] << 8 | short_tmp[1];

    /* interface pool table */
    parseInterfacePool(fp, cff->interface_count);

    /* field count */
    fread(short_tmp, 2, 1, fp);
    cff->fields_count = short_tmp[0] << 8 | short_tmp[1];

    /* field pool table */
    parseFieldPool(fp , cff->fields_count);

    /* method count */
    fread(short_tmp, 2, 1, fp);
    cff->methods_count = short_tmp[0] << 8 | short_tmp[1];

    /* method pool table */
    parseMethodPool(fp, cff->methods_count);

    fclose(fp);
    return 0;
}
