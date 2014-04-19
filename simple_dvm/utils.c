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

void push(simple_dalvik_vm *vm, const u4 data) {
    if (vm->stack_ptr >= 8192) {
        printf("* Error! Stack Full !\n");
        return;
    }
    if (is_verbose()) {
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
    if (is_verbose()) {
        printf("    (pop 0x%x, stack_ptr = %d)\n", vm->stack[vm->stack_ptr], vm->stack_ptr);
    }
    return vm->stack[vm->stack_ptr];
}

void invoke_clazz_method(DexFileFormat *dex, simple_dalvik_vm *vm,
                         class_data_item *clazz, invoke_parameters *p)
{
    int iter;
    encoded_method *method = NULL;
    for (iter = 0; iter < clazz->direct_methods_size; iter++) {
        if (p->method_id == clazz->direct_methods[iter].method_id)
            method = &clazz->direct_methods[iter];
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

    const u1 reg_start_id = method->code_item.registers_size - p->reg_count;
    for (iter = 0; iter < p->reg_count; iter++) {
        //u4 *data = (u4 *)&vm->regs[p->reg_idx[iter]].data;
        u4 *data_1 = (u4 *)&vm->regs[reg_start_id + iter].data;
        u4 *data_2 = (u4 *)&vm->regs[p->reg_idx[iter]].data;

        if (is_verbose()) {
            printf("    push v%d (0x%x) to stack\n", reg_start_id + iter, *data_1);
            printf("    mv v%d (0x%x) to v%d\n", p->reg_idx[iter], *data_2, reg_start_id + iter);
        }
        push(vm, *data_1);
        *data_1 = *data_2;
    }

    if (is_verbose()) { printf("    save invoke info & pc (0x%x)\n", vm->pc); }
    //push(vm, vm->pc);
    uint pc = vm->pc;
    invoke_parameters param = *p;

    runMethod(dex, vm, method);
    /* restore pc, invoke parameters */
    vm->pc = pc;
    vm->p = param;
    if (is_verbose()) { printf("    restore invoke info & pc (0x%x)\n", vm->pc); }

    /* restore registers */
    for (iter = p->reg_count - 1; iter >= 0; iter--) {
        uint data = pop(vm);
        u4 *data_1 = (u4 *)&vm->regs[reg_start_id + iter].data;
        *data_1 = data;
        if (is_verbose()) {
            printf("    pop (0x%x) back to v%d\n", data, reg_start_id + iter);
        }
    }
}
