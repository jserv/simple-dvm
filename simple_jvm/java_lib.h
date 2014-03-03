/*
 * Simple Java Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#ifndef SIMPLE_JVM_JAVA_LIBRARY_H
#define SIMPLE_JVM_JAVA_LIBRARY_H
#include "simple_jvm.h"
typedef int (*java_lang_lib) ( StackFrame *stack, SimpleConstantPool *p, char *type);

typedef struct _java_lang_method {
    char *clzname;
    char *methodname;
    java_lang_lib method_runtime;
} java_lang_method; 

int invoke_java_lang_library(StackFrame *stack, SimpleConstantPool *p,
                             char *cls_name, char *method_name, char *type);
#endif
