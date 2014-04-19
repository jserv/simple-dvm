/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */
#include <time.h>
#include <sys/time.h>
#include "java_lib.h"

int java_lang_math_random(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    double r = 0.0f;
    double test = 0.0f;
    int i = 0;
    int times = 0;
    srand(time(0));
    times = rand() % 100;
    for (i = 0; i < times; i++)
        r = ((double) rand() / (double) RAND_MAX);

    if (is_verbose() > 3) printf("get random number = %f \n", r);
    store_double_to_result(vm, (unsigned char *) &r);
    load_result_to_double(vm, (unsigned char *) &test);

    return 0;
}

int java_lang_object_init(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    if (is_verbose())
        printf("    call java.lang.object.<init> (%s)\n", type);
    return 0;
}

/* java.io.PrintStream.println */
static char buf[1024];
static int buf_ptr = 0;
static int use_buf = 0;
int java_io_print_stream_println(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    invoke_parameters *p = &vm->p;
    int i = 0;
    int string_id = 0;
    if (is_verbose())
        printf("    call java.io.PrintStream.println\n");

    load_reg_to(vm, p->reg_idx[1], (unsigned char *) &string_id);
    if (use_buf == 1) {
        printf("    %s\n", buf);
        use_buf = 0;
        memset(buf, 0, 1024);
        buf_ptr = 0;
    } else {
        printf("    %s\n", get_string_data(dex, string_id));
    }
    return 0;
}

int java_io_print_stream_flush(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    if (is_verbose())
        printf("    call java.io.PrintStream.flush\n");

    fflush(stdout);
    return 0;
}

/* java.lang.StringBuilder.<init> */
int java_lang_string_builder_init(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    invoke_parameters *p = &vm->p;
    if (is_verbose())
        printf("    call java.lang.StringBuilder.<init>\n");
    memset(buf, 0, 1024);
    buf_ptr = 0;
    return 0;
}

int java_lang_string_builder_append(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    invoke_parameters *p = &vm->p;
    int string_id = 0;
    if (is_verbose())
        printf("    call java.lang.StringBuilder.append\n");
    load_reg_to(vm, p->reg_idx[1], (unsigned char *) &string_id);
    if (type != 0) {
        if (strcmp(type, "Ljava/lang/String;") == 0) {
            buf_ptr += snprintf(buf + buf_ptr, 1024, "%s", get_string_data(dex, string_id));
        } else if (strcmp(type, "I") == 0) {
            buf_ptr += snprintf(buf + buf_ptr, 1024, "%d", string_id);
        }
    }
    return 0;
}

int java_lang_string_builder_to_string(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    invoke_parameters *p = &vm->p;
    if (is_verbose())
        printf("    call java.lang.StringBuilder.toString\n");
    use_buf = 1;
    return 0;
}

// https://ntu-android-2014.hackpad.com/Notes-and-QAs-for-Homework-2-lkOqx4tgdBe
//  Note: The return value of currentTimeMillis() should be stored into result
//        by store_long_to_result instead of returning to the virtual machine
int java_lang_system_currenttimemillis(DexFileFormat *dex, simple_dalvik_vm *vm, char *type)
{
    invoke_parameters *p = &vm->p;
    if (is_verbose())
        printf("    call java.lang.System.currentTimeMillis\n");

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    long secs = current_time.tv_sec;
    long usecs = current_time.tv_usec;
    long mtime = secs * 1000 + usecs / 1000;
    store_long_to_result(vm, (unsigned char *) &mtime);
    return 0;
}

static java_lang_method method_table[] = {
    {"Ljava/lang/Math;",          "random",   java_lang_math_random},
    {"Ljava/lang/Object;",        "<init>",   java_lang_object_init},
    {"Ljava/io/PrintStream;",     "println",  java_io_print_stream_println},
    {"Ljava/io/PrintStream;",     "print",    java_io_print_stream_println},
    {"Ljava/io/PrintStream;",     "flush",    java_io_print_stream_flush},
    {"Ljava/lang/StringBuilder;", "<init>",   java_lang_string_builder_init},
    {"Ljava/lang/StringBuilder;", "append",   java_lang_string_builder_append},
    {"Ljava/lang/StringBuilder;", "toString", java_lang_string_builder_to_string},
    {"Ljava/lang/System;",        "currentTimeMillis",      java_lang_system_currenttimemillis}
};

static int java_lang_method_size = sizeof(method_table) / sizeof(java_lang_method);

java_lang_method *find_java_lang_method(char *cls_name, char *method_name)
{
    int i = 0;
    for (i = 0; i < java_lang_method_size; i++)
        if (strcmp(cls_name, method_table[i].clzname) == 0 &&
            strcmp(method_name, method_table[i].methodname) == 0)
            return &method_table[i];
    return 0;
}

int invoke_java_lang_library(DexFileFormat *dex, simple_dalvik_vm *vm,
                             char *cls_name, char *method_name, char *type)
{
    java_lang_method *method = find_java_lang_method(cls_name, method_name);
    if (method != 0) {
        if (is_verbose())
            printf("    invoke %s/%s %s\n", method->clzname, method->methodname, type);
        method->method_runtime(dex, vm, type);
        return 1;
    }
    return 0;
}
