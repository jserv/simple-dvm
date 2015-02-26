/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2014 cycheng <createinfinite@yahoo.com.tw>
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */
#include <stdio.h>
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

int java_io_buffered_reader_init(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    if (is_verbose())
        printf("    call java_io_buffered_reader_init.<init> (%s)\n", type);
    return 0;
}

int java_io_input_stream_reader_init(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    if (is_verbose())
        printf("    call java_io_input_stream_reader_init.<init> (%s)\n", type);
    return 0;
}

int java_io_buffered_reader(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    char *str = NULL;
    /* %m : http://blog.markloiseau.com/2012/02/two-safer-alternatives-to-scanf/
     *  => str = malloc(..), so we have to free 'str' by our self
     */
    int status = scanf("%ms", &str);

    if (status != 1) {
        printf("Warning! scanf encounter error\n");
    }

    sdvm_obj *obj = create_sdvm_obj();
    obj->ref_count = 1;
    //load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&obj);
    //*((long *)&obj->other_data) = val;
    obj->other_data = str;

    assert(sizeof(long) == 8);
    assert(sizeof(obj->other_data) == 8);
    assert(((u8)obj >> 32) == 0);

    /* save the object reference to result */
    store_to_bottom_half_result(vm, (u1 *)&obj);

    if (is_verbose())
        printf("    call java_io_buffered_reader (%s), read string = %s\n"
               "    store obj (%p) to result, other_data = %p\n",
               type, str, obj, obj->other_data);

    return 0;
}

int java_lang_exception_print_stack_trace(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    if (is_verbose())
        printf("    call java_lang_exception_print_stack_trace (%s)\n", type);
    return 0;
}

int java_lang_long_valueof(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    sdvm_obj *obj = NULL;
    sdvm_obj *newobj = create_sdvm_obj();

    load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&obj);

    char *str = (char *)obj->other_data;
    long val = strtol(str, NULL, 10);
    newobj->ref_count = 1;
    *(long *)&newobj->other_data = val;

    //store_long_to_result(vm, (u1 *)&val);
    /* save the object reference to result */
    store_to_bottom_half_result(vm, (u1 *)&newobj);

    if (is_verbose())
        printf("    call java_lang_long_valueof (%s), val = %p, store obj (%p) to result\n",
               type, newobj->other_data, newobj);
    return 0;
}

int java_lang_long_long_value(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    sdvm_obj *obj = NULL;

    load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&obj);

    u1 *val = (u1 *)&obj->other_data;
    store_long_to_result(vm, val);

    if (is_verbose())
        printf("    call java_lang_long_long_value (%s), store val (%p) to result\n",
               type, obj->other_data);
    return 0;
}

/*  e.g.
 *  invoke-static {v1, v0} method_id 0x002e Ljava/lang/reflect/Array;,newInstance,(Ljava/lang/Class;)Ljava/lang/Object;
 *  move-result-object v0
 *
 *  Notes! If we use 2-dim array, e.g. int [128][128], then its array layout is
 *  looks like this :
 *      int[0] -> new_array_object -> 1-d int 128
 *      int[1] -> new_array_object -> 1-d int 128
 *      ..
 *
 *  If we want to get its element, e.g. int [8][7], then we will :
 *      int *ptr = &int[8][0]
 *      ptr[7]
 *
 *      => (v6 int [128][128] object, v1 = 8)
 *      aget-object v0, v6, v1
 *      add-int/lit8 v2, v1, #int -1 // #ff
 *      aget v3, v0, v2
 *
 */
int java_lang_reflect_array_new_instance(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    sdvm_obj *obj = NULL;
    new_filled_array *array_size_info = NULL;
    uint total_size = 0, elem_size = 0;

    load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&obj);
    load_reg_to(vm, vm->p.reg_idx[1], (u1 *)&array_size_info);

    //for (int j = 0; j < array_size_info->count; j++)
    //    num_elem *= array_size_info->array_elem[j];

    /* TODO array dim > 2 (< 2 ??) */
    assert(array_size_info->count == 2);

    if (obj->clazz) {
        /* TODO ? */
        assert(0);
    } else {
        undef_static_obj *undef_obj = (undef_static_obj *)obj;
        assert(obj->other_data == (void *)ICT_UNDEF_STATIC_OBJ);

        const char *name = get_class_name(dex, undef_obj->field_id);
        if (0 == strcmp(name, "Ljava/lang/Integer;")) {
            elem_size = sizeof(int);
        } else {
            /* TODO */
            assert(0);
        }
    }
    assert(elem_size);

    multi_dim_array_object *mul_dim_ary_obj = NULL;
    const uint num_elem = array_size_info->array_elem[0];
    {
        total_size = sizeof(multi_dim_array_object) + sizeof(void *) * num_elem;

        mul_dim_ary_obj = (multi_dim_array_object *)malloc(total_size);
        memset(mul_dim_ary_obj, 0, total_size);

        mul_dim_ary_obj->count = num_elem;
        mul_dim_ary_obj->elem_size = sizeof(void *);
        mul_dim_ary_obj->obj.ref_count = 1;
        mul_dim_ary_obj->obj.other_data = (void *)ICT_MULTI_DIM_ARRAY_OBJ;

        if (is_verbose()) {
            printf("    call java_lang_reflect_array_new_instance (%s)\n"
                   "    object %p, total size %d = obj_size %d + elem_size %d * dim = ",
                   type, obj, total_size, (int)sizeof(multi_dim_array_object), (int)sizeof(void *));
            //for (int i = 0; i < array_size_info->count; i++)
                printf("%d, ", array_size_info->array_elem[0]);
            printf("\n");
        }
        store_to_bottom_half_result(vm, (u1 *)&mul_dim_ary_obj);
    }

    {
        const int each_1d_num_elem = array_size_info->array_elem[1];
        const int each_1d_size = elem_size * each_1d_num_elem;
        const int each_1d_total_size = sizeof(new_array_object) + each_1d_size;

        for (int j = 0; j < num_elem; j++) {
            new_array_object *ary_obj = (new_array_object *)malloc(each_1d_total_size);
            memset(ary_obj, 0, each_1d_total_size);

            ary_obj->count = each_1d_num_elem;
            ary_obj->elem_size = elem_size;
            ary_obj->obj.ref_count = 1;
            ary_obj->obj.other_data = (void *)ICT_NEW_ARRAY_OBJ;
            mul_dim_ary_obj->array[j] = ary_obj;

            if (is_verbose()) {
                printf("    Create 1d array object %p, num_elem %d, elem_size %d\n",
                       ary_obj, each_1d_num_elem, elem_size);
            }
        }
    }

    return 0;
}

/* e.g. invoke-virtual {v8, v2}, Ljava/lang/String;.charAt:(I)C // method@0026
 *  v8 : string object reference, v2 : index to string
 *
 *  ref case from dhrystone :
 *      const-string v1, "DHRYSTONE PROGRAM, SOME STRING" // string@0010
 *      iput-object v1, v0, LRecord_Type;.String_Comp:Ljava/lang/String; // field@0018
 *
 *      invoke-virtual {v8, v2}, Ljava/lang/String;.charAt:(I)C // method@0026
 *      move-result v5
 */
int java_lang_string_char_at(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    int string_id;
    int index;

    load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&string_id);
    load_reg_to(vm, vm->p.reg_idx[1], (u1 *)&index);

    /* If we hit this assertion, means the string is not const-string, it is
     * actually a string object, we may need to define an internal string class
     * to handle it */
    assert(string_id < dex->header.stringIdsSize);

    const char *str = dex->string_data_item[string_id].data;
    int val = str[index];

    store_to_bottom_half_result(vm, (u1 *)&val);

    if (is_verbose()) {
        printf("    call java.lang.String.charAt\n"
               "    string_id %d, index %d = %c\n",
               string_id, index, (char)val);
    }
    return 0;
}

/* e.g. invoke-virtual {v8, v9}, Ljava/lang/String;.compareTo:(Ljava/lang/String;)I // method@0027
 */
int java_lang_string_compare_to(DexFileFormat *dex, simple_dalvik_vm *vm, char *type) {
    int string_id_1;
    int string_id_2;

    load_reg_to(vm, vm->p.reg_idx[0], (u1 *)&string_id_1);
    load_reg_to(vm, vm->p.reg_idx[1], (u1 *)&string_id_2);

    /* If we hit this assertion, means the string is not const-string, it is
     * actually a string object, we may need to define an internal string class
     * to handle it */
    assert(string_id_1 < dex->header.stringIdsSize);
    assert(string_id_2 < dex->header.stringIdsSize);

    /* http://www.tutorialspoint.com/java/java_string_compareto.htm */
    const char *str_1 = dex->string_data_item[string_id_1].data;
    const char *str_2 = dex->string_data_item[string_id_2].data;

    int val = strcmp(str_1, str_2);
    store_to_bottom_half_result(vm, (u1 *)&val);

    if (is_verbose()) {
        printf("    call java.lang.String.compareTo\n"
               "    str_id 1 = 0x%x, str_id 2 = 0x%x, result = %d\n",
               string_id_1, string_id_2, val);
    }

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
        printf("%s\n", buf);
        use_buf = 0;
        memset(buf, 0, 1024);
        buf_ptr = 0;
    } else {
        printf("%s\n", get_string_data(dex, string_id));
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
    if (is_verbose())
        printf("    call java.lang.StringBuilder.append\n");

    assert(p->reg_count == 2 || p->reg_count == 3);

    if (type != 0) {
        if (strcmp(type, "Ljava/lang/String;") == 0) {
            int string_id = 0;
            load_reg_to(vm, p->reg_idx[1], (unsigned char *) &string_id);
            buf_ptr += snprintf(buf + buf_ptr, 1024, "%s", get_string_data(dex, string_id));

        } else {
            int val[2] = {0, 0};

            if (p->reg_count == 2) {
                load_reg_to(vm, p->reg_idx[1], (u1 *) &val[0]);
                buf_ptr += snprintf(buf + buf_ptr, 1024, "%d", val[0]);

            } else {
                load_reg_to_long(vm, p->reg_idx[1], (u1 *) &val[1]);
                load_reg_to_long(vm, p->reg_idx[2], (u1 *) &val[0]);
                buf_ptr += snprintf(buf + buf_ptr, 1024, "%lld", *(u8 *)&val);
            }

            /*if (strcmp(type, "I") == 0) {
                buf_ptr += snprintf(buf + buf_ptr, 1024, "%d", string_id);
            }*/
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
    {"Ljava/io/BufferedReader;",  "<init>",   java_io_buffered_reader_init},
    {"Ljava/io/BufferedReader;",  "readLine", java_io_buffered_reader},
    {"Ljava/io/InputStreamReader;","<init>",  java_io_input_stream_reader_init},
    {"Ljava/io/PrintStream;",     "println",  java_io_print_stream_println},
    {"Ljava/io/PrintStream;",     "print",    java_io_print_stream_println},
    {"Ljava/io/PrintStream;",     "flush",    java_io_print_stream_flush},
    {"Ljava/lang/Exception;",     "printStackTrace", java_lang_exception_print_stack_trace },
    {"Ljava/lang/Long;",          "valueOf",  java_lang_long_valueof},
    {"Ljava/lang/Long;",          "longValue",java_lang_long_long_value},
    {"Ljava/lang/reflect/Array;", "newInstance", java_lang_reflect_array_new_instance },
    {"Ljava/lang/String;",        "charAt",   java_lang_string_char_at },
    {"Ljava/lang/String;",        "compareTo",java_lang_string_compare_to },
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
    } else {
        printf("    Warning ! The method %s/%s is not found!\n", method->clzname, method->methodname);
    }
    return 0;
}
