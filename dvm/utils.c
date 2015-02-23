/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_dvm.h"

static int verbose_flag = 0;

int is_verbose()
{
    return verbose_flag;
}

int enable_verbose()
{
    verbose_flag = 1;
    return 0;
}

int disable_verbose()
{
    verbose_flag = 0;
    return 0;
}

int set_verbose(int l)
{
    verbose_flag = l;
    return 0;
}

void load_reg_to(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    ptr[0] = r->data[0];
    ptr[1] = r->data[1];
    ptr[2] = r->data[2];
    ptr[3] = r->data[3];
}

void load_reg_to_double(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    ptr[0] = r->data[2];
    ptr[1] = r->data[3];
    ptr[2] = r->data[0];
    ptr[3] = r->data[1];
}
void load_result_to_double(simple_dalvik_vm *vm, unsigned char *ptr)
{
    ptr[0] = vm->result[2];
    ptr[1] = vm->result[3];
    ptr[2] = vm->result[0];
    ptr[3] = vm->result[1];
    ptr[4] = vm->result[6];
    ptr[5] = vm->result[7];
    ptr[6] = vm->result[4];
    ptr[7] = vm->result[5];
}

void store_double_to_result(simple_dalvik_vm *vm, unsigned char *ptr)
{
    vm->result[0] = ptr[2];
    vm->result[1] = ptr[3];
    vm->result[2] = ptr[0];
    vm->result[3] = ptr[1];
    vm->result[4] = ptr[6];
    vm->result[5] = ptr[7];
    vm->result[6] = ptr[4];
    vm->result[7] = ptr[5];
}

void store_double_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    r->data[0] = ptr[2];
    r->data[1] = ptr[3];
    r->data[2] = ptr[0];
    r->data[3] = ptr[1];
}

void store_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    r->data[0] = ptr[0];
    r->data[1] = ptr[1];
    r->data[2] = ptr[2];
    r->data[3] = ptr[3];
}

void move_top_half_result_to_reg(simple_dalvik_vm *vm, int id)
{
    simple_dvm_register *r = &vm->regs[id];
    r->data[0] = vm->result[0];
    r->data[1] = vm->result[1];
    r->data[2] = vm->result[2];
    r->data[3] = vm->result[3];
}

void move_bottom_half_result_to_reg(simple_dalvik_vm *vm, int id)
{
    simple_dvm_register *r = &vm->regs[id];
    r->data[0] = vm->result[4];
    r->data[1] = vm->result[5];
    r->data[2] = vm->result[6];
    r->data[3] = vm->result[7];
}

void store_to_bottom_half_result(simple_dalvik_vm *vm, unsigned char *ptr) {
    vm->result[4] = ptr[0];
    vm->result[5] = ptr[1];
    vm->result[6] = ptr[2];
    vm->result[7] = ptr[3];
}

void load_reg_to_long(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    ptr[0] = r->data[2];
    ptr[1] = r->data[3];
    ptr[2] = r->data[0];
    ptr[3] = r->data[1];
}

void store_long_to_result(simple_dalvik_vm *vm, unsigned char *ptr)
{
    vm->result[0] = ptr[2];
    vm->result[1] = ptr[3];
    vm->result[2] = ptr[0];
    vm->result[3] = ptr[1];
    vm->result[4] = ptr[6];
    vm->result[5] = ptr[7];
    vm->result[6] = ptr[4];
    vm->result[7] = ptr[5];
}

void store_long_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr)
{
    simple_dvm_register *r = &vm->regs[id];
    r->data[0] = ptr[2];
    r->data[1] = ptr[3];
    r->data[2] = ptr[0];
    r->data[3] = ptr[1];
}

#if 0
void push(simple_dalvik_vm *vm, const u4 data) {
    if (vm->stack_ptr >= 8192) {
        printf("* Error! Stack Full !\n");
        return;
    }
    if (is_verbose() > 4) {
        printf("    (push 0x%x, stack_ptr = %d)\n", data, vm->stack_ptr);
    }
    vm->stack[vm->stack_ptr] = data;
    vm->stack_ptr ++;
}

uint pop(simple_dalvik_vm *vm) {
    if (vm->stack_ptr == 0) {
        printf("* Error! Stack Empty !\n");
        return 0xffffffff;
    }
    vm->stack_ptr --;
    if (is_verbose() > 4) {
        printf("    (pop 0x%x, stack_ptr = %d)\n", vm->stack[vm->stack_ptr], vm->stack_ptr);
    }
    return vm->stack[vm->stack_ptr];
}
#endif

void invoke_clazz_method(DexFileFormat *dex, simple_dalvik_vm *vm,
                         class_data_item *clazz, invoke_parameters *p)
{
    int iter;
    encoded_method *method = NULL;
    /* TODO : create a quick mapping of (method_id -> encoded_method) */
    for (iter = 0; iter < clazz->direct_methods_size; iter++) {
        if (p->method_id == clazz->direct_methods[iter].method_id)
            method = &clazz->direct_methods[iter];
    }
    if (! method) {
        for (iter = 0; iter < clazz->virtual_methods_size; iter++) {
            if (p->method_id == clazz->virtual_methods[iter].method_id)
                method = &clazz->virtual_methods[iter];
        }
    }
    assert(method);
    /*  e.g. Func_1(int x) use 3 registers totally :
     *      i.e. v0, v1, v2 in order (registers_size = 3)
     *      parameter = v1, v2 (p->reg_count)
     *      v1 = this pointer
     *      v2 = x
     *
     *      If I call Func_1
     *      v9 = this
     *      v5 = int
     *      invoke-direct {v9, v5} Func_1
     *
     *      So I have to :
     *      push v1, v2 to stack
     *      v1 = v9
     *      v2 = v5
     *      call Func_1
     *      pop v2, v1 back to v5, v9
     */
    assert(p->reg_count);   /* at least we have this pointer */

    /* push those be used registers to stack */
    const uint total_reg_size = method->code_item.registers_size;
    uint stack[total_reg_size];
    uint param_value[p->reg_count];

    for (iter = 0; iter < total_reg_size; iter++) {
        u4 *data = (u4 *)&vm->regs[iter].data;
        //push(vm, *data);
        stack[iter] = *data;
        if (is_verbose())
            printf("    push v%d (0x%x) to stack\n", iter, *data);
    }

    /* copy parameters to the method defined parameter registers
     *  NOTE ! copy to temporary space first, then mv to register
     *  Consider the case :
     *      invoke-virtual {v12, v2, v9, v3} method_id 0x0011 Ldhry;,Proc_7,(I)V
     *      =>
     *          mv v12 to v2
     *          mv v2  to v3  <= that's why we need temporary space
     *          mv v9  to v4
     *          mv v3  to v5
     */
    for (iter = 0; iter < p->reg_count; iter++) {
        param_value[iter] = *(u4 *)&vm->regs[p->reg_idx[iter]].data;
    }

    const u1 reg_start_id = total_reg_size - p->reg_count;
    for (iter = 0; iter < p->reg_count; iter++) {
        u4 *dst = (u4 *)&vm->regs[reg_start_id + iter].data;
        if (is_verbose()) {
            printf("    mv v%d (0x%x) to v%d\n", p->reg_idx[iter], param_value[iter], reg_start_id + iter);
        }
        *dst = param_value[iter];
    }

    /*for (iter = 0; iter < p->reg_count; iter++) {
        u4 *data_1 = (u4 *)&vm->regs[reg_start_id + iter].data;
        u4 *data_2 = (u4 *)&vm->regs[p->reg_idx[iter]].data;

        if (is_verbose()) {
            printf("    mv v%d (0x%x) to v%d\n", p->reg_idx[iter], *data_2, reg_start_id + iter);
        }
        *data_1 = *data_2;
    }*/

    if (is_verbose()) { printf("    save invoke info & pc (0x%x)\n", vm->pc); }
    //push(vm, vm->pc);
    uint pc = vm->pc;
    invoke_parameters param = *p;
    //vm->object_ref = ;

    runMethod(dex, vm, method);
    /* restore pc, invoke parameters */
    vm->pc = pc;
    vm->p = param;
    if (is_verbose()) { printf("    restore invoke info & pc (0x%x)\n", vm->pc); }

    /* restore registers */
    for (iter = total_reg_size - 1; iter >= 0; iter--) {
        //uint orig_data = pop(vm);
        uint orig_data = stack[iter];
        u4 *data = (u4 *)&vm->regs[iter].data;
        *data = orig_data;

        if (is_verbose()) {
            printf("    pop (0x%x) back to v%d\n", *data, iter);
        }
    }
}

uint get_field_size(DexFileFormat *dex, const uint field_id) {
    uint field_size = 0;
    field_id_item *field = get_field_item(dex, field_id);
    char *type_name = dex->string_data_item[
        dex->type_id_item[field->type_idx].descriptor_idx].data;
    assert(field);

    if (strlen(type_name) == 1) {
        /* ref : android dex-format, "TypeDescriptor Semantics" */
        switch (type_name[0]) {
            case 'B' :
            case 'C' :
            case 'Z' :
                field_size = 1;
                break;
            case 'S' :
                field_size = 2;
                break;
            case 'I' :
            case 'F' :
                field_size = 4;
                break;
            case 'J' :
            case 'D' :
                field_size = 8;
                break;

            default :
                printf("Error! Un-support field type (%c)\n", type_name[0]);
                assert(0);
        }

    } else {
        /* treat all other case are objects */
        field_size = sizeof(sdvm_obj*);
    }

    return field_size;
}

int get_field_type(DexFileFormat *dex, const uint field_id) {
    uint field_size = 0;
    field_id_item *field = get_field_item(dex, field_id);
    char *type_name = dex->string_data_item[
        dex->type_id_item[field->type_idx].descriptor_idx].data;
    assert(field);

    if (strlen(type_name) == 1) {
        /* ref : android dex-format, "TypeDescriptor Semantics" */
        switch (type_name[0]) {
            case 'B' : return VALUE_BOOLEAN;
            case 'C' : return VALUE_CHAR;
            case 'Z' : return VALUE_BYTE;
            case 'S' : return VALUE_SHORT;
            case 'I' : return VALUE_INT;
            case 'F' : return VALUE_FLOAT;
            case 'J' : return VALUE_LONG;
            case 'D' : return VALUE_DOUBLE;

            default :
                printf("Error! Un-support field type (%c)\n", type_name[0]);
                assert(0);
        }

    } else {
        /* treat all other case are objects */
        return VALUE_SDVM_OBJ;
    }

    return -1;
}

sdvm_obj *create_sdvm_obj() {
    sdvm_obj *obj = malloc(sizeof(sdvm_obj));
    obj->clazz = NULL;
    obj->other_data = NULL;
    obj->ref_count = 0;
    if (is_verbose()) {
        printf("    Create new obj %p\n", obj);
    }
    return obj;
}

