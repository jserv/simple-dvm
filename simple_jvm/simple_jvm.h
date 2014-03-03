/*
 * Simple Java Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#ifndef SIMPLE_JVM_H
#define SIMPLE_JVM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u2;
typedef unsigned char byte;

/* Java Class File */
typedef struct _ClassFileFormat { 
    byte magic_number[4];
    u2   minor_version;
    u2   major_version;
    u2   constant_pool_count;
    /* constant pool */
    u2   access_flags;
    u2   this_class;
    u2   super_class;

    u2   interface_count;
    /* interface pool */
    u2   fields_count;
    /* fields pool */
    u2   methods_count;
    /* method pool */
    u2   attributes_count;
    /* attributes pool */
} ClassFileFormat;

/*
 *  TAG 
 *  1 UTF-8 String
 *  3 Integer
 *  4 Float
 *  5 Long
 *  6 Double
 *  7 Class reference
 *  8 String reference
 *  9 Field reference
 *  10 Method reference
 *  11 Interface method reference
 *  12 Name and type descriptor
 * */
#define CONSTANT_UTF8           1
#define CONSTANT_INTEGER        3
#define CONSTANT_FLOAT          4
#define CONSTANT_LONG           5
#define CONSTANT_DOUBLE         6

#define CONSTANT_CLASS          7
#define CONSTANT_STRING_REF     8
#define CONSTANT_FIELD_REF      9
#define CONSTANT_METHOD_REF     10
#define CONSTANT_INTERFACE_REF  11
#define CONSTANT_NAME_AND_TYPE  12

typedef struct _ConstantUTF8 {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 string_size;
    unsigned char *ptr;
} ConstantUTF8;

typedef struct _ConstantInteger {
    int index;
    unsigned char tag;
    int additional_byte_size;
    int value;
} ConstantInteger;

typedef struct _ConstantFloat {
    int index;
    unsigned char tag;
    int additional_byte_size;
    float value;
} ConstantFloat;

typedef struct _ConstantLong {
    int index;
    unsigned char tag;
    int additional_byte_size;
    long long value;
} ConstantLong;

typedef struct _ConstantDouble {
    int index;
    unsigned char tag;
    int additional_byte_size;
    double value;
} ConstantDouble;

typedef struct _ConstantClassRef {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 stringIndex;
} ConstantClassRef;

typedef struct _ConstantStringRef {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 stringIndex;
} ConstantStringRef;

typedef struct _ConstantFieldRef {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 classIndex;
    u2 nameAndTypeIndex;
} ConstantFieldRef;

typedef struct _ConstantMethodRef {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 classIndex;
    u2 nameAndTypeIndex;
} ConstantMethodRef;

typedef struct _ConstantInterfaceRef {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 classIndex;
    u2 nameAndTypeIndex;
} ConstantInterfaceRef;

typedef struct _ConstantNameAndType {
    int index;
    unsigned char tag;
    int additional_byte_size;
    u2 nameIndex;
    u2 typeIndex;
} ConstantNameAndType;

typedef struct _SimpleConstantPool {
    /* UTF-8 String */
    ConstantUTF8 utf8CP[200];
    int utf8_used; 

    /* Integer */
    ConstantInteger integerCP[100];
    int integer_used;

    /* Float */
    ConstantFloat floatCP[100];
    int float_used; 

    /* Long */
    ConstantLong longCP[100];
    int long_used;
    
    /* Double */
    ConstantDouble doubleCP[100];
    int double_used;

    /* Class Reference */
    ConstantClassRef clasz[100];
    int clasz_used;

    /* String Reference */
    ConstantStringRef stringRef[100];
    int stringRef_used;  

    /* Field Reference */
    ConstantFieldRef field[100];
    int field_used;

    /* Method Reference */
    ConstantMethodRef method[100];
    int method_used;

    /* Interface Reference */
    ConstantInterfaceRef interface[100];
    int interface_used;

    /* Name And Type Reference */
    ConstantNameAndType  name_and_type[100];
    int name_and_type_used;
} SimpleConstantPool;

typedef struct _SimpleInterfacePool {
    /* Class Reference */
    ConstantClassRef clasz[100];
    int clasz_used;
} SimpleInterfacePool;

/* Attribute Info */
typedef struct _AttributeInfo {
    u2 attribute_name_index;
    int attribute_length;
    unsigned char *info;
} AttributeInfo;

/* Code Attributes */
typedef struct _CodeAttribute {
        u2 attribute_name_index;
        int attribute_length;
        u2 max_stack;
        u2 max_locals;
        int code_length;
        unsigned char *code; // [code_length];
#if 0
        u2 exception_table_length;
        Exception_table* exception_table; //[exception_table_length];
        u2 attributes_count;
        attribute_info* attributes; //[attributes_count];
#endif
} CodeAttribute;

/* Field Info */
typedef struct _FieldInfo {
    u2   access_flags;
    u2   name_index;
    u2   descriptor_index;
    u2   attributes_count;
    AttributeInfo *attributes;
} FieldInfo;

/* Simple Field Pool */
typedef struct _SimpleFieldPool {
    FieldInfo field[100];
    int field_used;
} SimpleFieldPool;

/* Method Info */
typedef struct _MethodInfo {
    u2 access_flags; 
    u2 name_index; 
    u2 descriptor_index; 
    u2 attributes_count; 
    AttributeInfo *attributes;
} MethodInfo;

/* Simple Method Pool */
typedef struct _SimpleMethodPool {
    MethodInfo method[100];
    int method_used;
} SimpleMethodPool;

/* constant pool parser */
int parseConstantPool(FILE *fp, int count);
void printConstantPool(SimpleConstantPool *p);
ConstantUTF8 *findUTF8(SimpleConstantPool *p, int index);
ConstantStringRef *findStringRef(SimpleConstantPool *p, int index);
int getUTF8String(SimpleConstantPool *p, int index, int size, char *out);
ConstantClassRef *findClassRef(SimpleConstantPool *p , int index);
ConstantMethodRef *findMethodRef(SimpleConstantPool *p , int index);
ConstantNameAndType *findNameAndType(SimpleConstantPool *p , int index);

int get_integer_from_constant_pool(SimpleConstantPool *p, int index);
long long get_long_from_constant_pool(SimpleConstantPool *p, int index);
float get_float_from_constant_pool(SimpleConstantPool *p, int index);
double get_double_from_constant_pool(SimpleConstantPool *p, int index);

/* Interface Pool Parser */
int parseInterfacePool(FILE *fp, int count);
void printInterfacePool( SimpleConstantPool *p, SimpleInterfacePool *ip);

/* Field Pool Parser */
int parseFieldPool(FILE *fp, int count);
void printFieldPool( SimpleConstantPool *p, SimpleFieldPool *fp);

/* Method Pool Parser */
int parseMethodPool(FILE *fp, int count);
void printMethodPool( SimpleConstantPool *p, SimpleMethodPool *fp);
MethodInfo *findMethodInPool(SimpleConstantPool *p,
                             SimpleMethodPool *mp,
                             char *method_name, int size);
void printMethodAttributes(SimpleConstantPool *p,
                           MethodInfo *method);

/* Java Class File Parser */
int parseJavaClassFile(char *file, ClassFileFormat *cff);
void printClassFileFormat(ClassFileFormat *cff);

/* Free Pools */
void free_pools();

/* Stack Frame */
#define STACK_ENTRY_NONE        0
#define STACK_ENTRY_INT         1
#define STACK_ENTRY_REF         2
#define STACK_ENTRY_LONG        3
#define STACK_ENTRY_DOUBLE      4
#define STACK_ENTRY_FLOAT       5

typedef struct _StackEntry {
    unsigned char entry[8];
    int type;
} StackEntry;

typedef struct _StackFrame {
    int max_size;
    int size;
    StackEntry *store;
} StackFrame;
void stackInit(StackFrame *stack, int entry_size);

void pushInt    (StackFrame *stack, int value);
void pushLong   (StackFrame *stack, long long value);
void pushDouble (StackFrame *stack, double value);
void pushFloat  (StackFrame *stack, float value);
void pushRef    (StackFrame *stack, int value);

int         popInt      (StackFrame *stack);
long long   popLong     (StackFrame *stack);
double      popDouble   (StackFrame *stack);
float       popFloat    (StackFrame *stack);
StackEntry *popEntry    (StackFrame *stack);

int     EntryToInt(StackEntry *entry );
double  EntryToDouble(StackEntry *entry);
int     is_ref_entry(StackFrame *stack);

/* local variable */
typedef struct _LocalVariables {
    int integer[10];
} LocalVariables;

/* byte Codes */
typedef int (*opCodeFunc)(unsigned char **opCode, StackFrame *stack, SimpleConstantPool *p);

typedef struct _byteCode {
    char *name;
    unsigned char opCode;
    int offset;
    opCodeFunc func; 
} byteCode;
int executeMethod(MethodInfo *startup, StackFrame *stack, SimpleConstantPool *p);

#endif
