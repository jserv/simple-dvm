/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#ifndef SIMPLE_DVM_JAVA_LIBRARY_H
#define SIMPLE_DVM_JAVA_LIBRARY_H

#include "simple_dvm.h"

typedef int (*java_lang_lib)(DexFileFormat *dex, simple_dalvik_vm *vm, char*type);

typedef struct _java_lang_method {
    char *clzname;
    char *methodname;
    java_lang_lib method_runtime;
} java_lang_method;

int invoke_java_lang_library(DexFileFormat *dex, simple_dalvik_vm *vm,
                             char *cls_name, char *method_name, char *type);

#endif
