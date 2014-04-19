/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2014 Jim Huang <jserv.tw@gmail.com>
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"
#include "java_lib.h"

static int find_const_string(DexFileFormat *dex, char *entry)
{
    int i = 0;
    for (i = 0; i < dex->header.stringIdsSize; i++) {
        if (memcmp(dex->string_data_item[i].data, entry, strlen(entry)) == 0) {
            if (is_verbose())
                printf("find %s in dex->string_data_item[%d]\n", entry, i);
            return i;
        }
    }
    return -1;
}

static void printRegs(simple_dalvik_vm *vm)
{
    int i = 0;
    if (is_verbose()) {
        printf("pc = %08x\n", vm->pc);
        for (i = 0; i < 16 ; i++) {
            printf("Reg[%2d] = %4d (%04x) ",
                   i, *((int *)&vm->regs[i]), *((unsigned int *)&vm->regs[i]));
            if ((i + 1) % 4 == 0) printf("\n");
        }
    }
}

/*
    { "move"              , 0x01, 2,  op_move },
    { "goto"              , 0x28, 2,  op_goto },
    { "if-ge"             , 0x35, 4,  op_if_ge },
    { "long-to-int"       , 0x84, 2,  op_long_to_int},
    { "sub-long/2addr"    , 0xbc, 2,  op_sub_long_2addr },
    { "add-int/lit8"      , 0xd8, 4,  op_add_int_lit8 },
 */

/* move vx,vy
 * Moves the content of vy into vx. Both registers must be in the first 256
 * register range.
 *
 * 0110 - move v0, v1   Moves v1 into v0.
 */
static int op_move(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = ptr[*pc + 1] & 0x0F;
    int reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    if (is_verbose())
        printf("move v%d,v%d\n", reg_idx_vx, reg_idx_vy);
    simple_dvm_register *rx = &vm->regs[reg_idx_vx];
    simple_dvm_register *ry = &vm->regs[reg_idx_vy];
    rx->data[0] = ry->data[0];
    rx->data[1] = ry->data[1];
    rx->data[2] = ry->data[2];
    rx->data[3] = ry->data[3];
    *pc = *pc + 2;
    return 0;
}

/* goto target
 * Unconditional jump by short offset
 *
 * 28F0 - goto 0005 // -0010
 * Jumps to current position-16 words (hex 10). 0005 is the label of the target
 * instruction.
 */
static int op_goto(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    s1 offset = (s1) ptr[*pc + 1];
    if (is_verbose()) {
        printf("goto %d\n", (int) offset);
    }
    *pc = *pc + offset * 2;
    return 0;
}

/* if-ge vx, vy,target
 * Jumps to target if vx>=vy. vx and vy are integer values.
 *
 * 3510 1B00 - if-ge v0, v1, 002b // +001b
 * Jumps to the current position+1BH words if v0>=v1. 002b is the label of the
 * target instruction.
 */
static int op_if_ge(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = ptr[*pc + 1] & 0x0F;
    int reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    int offset = (int) ptr[*pc + 2] + (int) ptr[*pc + 3] * (int) 256;
    int x, y;
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    load_reg_to(vm, reg_idx_vx, (unsigned char *) &x);

    if (is_verbose()) {
        printf("if-ge v%d (%d), v%d (%d), %d\n", reg_idx_vx, x, reg_idx_vy, y, offset);
    }
    if(x >= y)
        *pc = *pc + offset * 2;
    else
        *pc = *pc + 4;
    return 0;
}

/* long-to-int vx,vy
 * Converts the long value in vy,vy+1 into an integer in vx.
 *
 * 8424 - long-to-int v4, v2
 * Converts the long value in v2,v3 into an integer value in v4.
 */
static int op_long_to_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    long l = 0;
    unsigned char *ptr_l = (unsigned char *) &l;
    int i = 0;
    int i2 = 0 ;
    reg_idx_vx = ptr[*pc + 1] & 0x0F;
    reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    reg_idx_vz = reg_idx_vy + 1;
    load_reg_to_long(vm, reg_idx_vy , ptr_l + 4);
    load_reg_to_long(vm, reg_idx_vz , ptr_l);
    i = (int)l;
    if (is_verbose()) {
        printf("long-to-int v%d, v%d\n", reg_idx_vx, reg_idx_vy);
        printf("(%ld) to (%d) \n", l , i);
    }
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &i);
    *pc = *pc + 2;
    return 0;
}

/* sub-long/2addr vx,vy
 * Calculates vx-vy and puts the result into vx
 *
 * BC70 - sub-long/2addr v0, v7
 * Subtracts the long value in v7,v8 from the long value in v0,v1 and puts the
 * result into v0,v1.
 */
static int op_sub_long_2addr(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = ptr[*pc + 1] & 0x0F;
    int reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    long x = 0L, y = 0L;
    unsigned char *ptr_x = (unsigned char *) &x;
    unsigned char *ptr_y = (unsigned char *) &y;
    load_reg_to_long(vm, reg_idx_vx, ptr_x + 4);
    load_reg_to_long(vm, reg_idx_vx + 1, ptr_x);

    load_reg_to_long(vm, reg_idx_vy, ptr_y + 4);
    load_reg_to_long(vm, reg_idx_vy + 1, ptr_y);
    if (is_verbose()) {
        printf("sub-long/2addr v%d, v%d\n", reg_idx_vx, reg_idx_vy);
        printf("%ld - %ld = %ld\n", x, y, x - y);
    }
    x = x - y;
    store_long_to_reg(vm, reg_idx_vx, ptr_x + 4);
    store_long_to_reg(vm, reg_idx_vx + 1, ptr_x);
    *pc = *pc + 2;
    return 0;
}

/* add-int/lit8 vx,vy,lit8
 * Adds vy to lit8 and stores the result into vx.
 *
 * D800 0201 - add-int/lit8 v0,v2, #int1
 * Adds literal 1 to v2 and stores the result into v0.
 */
static int op_add_int_lit8(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int x = 0, y = 0 ;
    int z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    z = ptr[*pc + 3];
    if (is_verbose())
        printf("add-int v%d, v%d, #int%d\n", reg_idx_vx, reg_idx_vy, z);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    x = y + z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);
    *pc = *pc + 4;
    return 0;
}

/* 0x0b, move-result-wide
 *
 * Move the long/double result value of the previous method invocation
 * into vx, vx+1.
 *
 * 0B02 - move-result-wide v2
 * Move the long/double result value of the previous method
 * invocation into v2,v3.
 */
static int op_move_result_wide(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = reg_idx_vx + 1;
    if (is_verbose())
        printf("move-result-wide v%d,v%d\n", reg_idx_vx, reg_idx_vy);
    move_bottom_half_result_to_reg(vm, reg_idx_vx);
    move_top_half_result_to_reg(vm, reg_idx_vy);
    *pc = *pc + 2;
    return 0;
}

/* 0x0c, move-result-object vx
 *
 * Move the result object reference of
 * the previous method invocation into vx.
 *
 * 0C00 - move-result-object v0
 */
static int op_move_result_object(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    reg_idx_vx = ptr[*pc + 1];
    if (is_verbose())
        printf("move-result-object v%d\n", reg_idx_vx);
    move_bottom_half_result_to_reg(vm, reg_idx_vx);
    *pc = *pc + 2;
    return 0;
}

/* 0x0e , return-void
 * Return without a return value
 * 0E00 - return-void
 */
static int op_return_void(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    if (is_verbose())
        printf("return-void\n");
    *pc = *pc + 2;
    return 0;
}

/* 0x12, const/4 vx,lit4
 * Puts the 4 bit constant into vx
 * 1221 - const/4 v1, #int2
 * Moves literal 2 into v1.
 * The destination register is in the lower 4 bit
 * in the second byte, the literal 2 is in the higher 4 bit.
 */
static int op_const_4(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int value = 0;
    int reg_idx_vx = 0;
    value = ptr[*pc + 1] >> 4;
    reg_idx_vx = ptr[*pc + 1] & 0x0F;
    if (value & 0x08) {
        value = 0x0F - value + 1;
        value = -value;
    }
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &value);
    if (is_verbose())
        printf("const/4 v%d, #int%d\n", reg_idx_vx , value);
    *pc = *pc + 2;
    return 0;
}

/* 0x13, const/16 vx,lit16
 * Puts the 16 bit constant into vx
 * 1300 0A00 - const/16 v0, #int 10
 * Puts the literal constant of 10 into v0.
 */
static int op_const_16(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int value = 0;
    reg_idx_vx = ptr[*pc + 1];
    value = (ptr[*pc + 3] << 8 | ptr[*pc + 2]);

    store_to_reg(vm, reg_idx_vx, (unsigned char *) &value);
    if (is_verbose())
        printf("const/16 v%d, #int%d\n", reg_idx_vx, value);
    *pc = *pc + 4;
    return 0;
}

/* 0x19, const-wide/high16 vx,lit16
 * Puts the 16 bit constant into the highest 16 bit of vx
 * and vx+1 registers.
 * Used to initialize double values.
 * 1900 2440 - const-wide/high16 v0, #double 10.0 // #402400000
 * Puts the double constant of 10.0 into v0 register.
 */
static int op_const_wide_high16(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    long long value = 0L;
    unsigned char *ptr2 = (unsigned char *) &value;
    int reg_idx_vx = 0;
    reg_idx_vx = ptr[*pc + 1];
    ptr2[1] = ptr[*pc + 3];
    ptr2[0] = ptr[*pc + 2];
    if (is_verbose())
        printf("const-wide/hight16 v%d, #long %lld\n", reg_idx_vx, value);
    store_to_reg(vm, reg_idx_vx, ptr2);
    value = 1L;
    *pc = *pc + 4;
    return 0;
}

/* 0x1a, const-string vx,string_id
 * Puts reference to a string constant identified by string_id into vx.
 * 1A08 0000 - const-string v8, "" // string@0000
 * Puts reference to string@0000 (entry #0 in the string table) into v8.
 */
static int op_const_string(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int string_id = 0;
    reg_idx_vx = ptr[*pc + 1];
    string_id = ((ptr[*pc + 3] << 8) | ptr[*pc + 2]);

    if (is_verbose())
        printf("const-string v%d, string_id 0x%04x\n",
               reg_idx_vx , string_id);
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &string_id);
    *pc = *pc + 4;
    return 0;
}

/* 0x22 new-instance vx,type
 * Instantiates an object type and puts
 * the reference of the newly created instance into vx
 * 2200 1500 - new-instance v0, java.io.FileInputStream // type@0015
 * Instantiates type@0015 (entry #15H in the type table)
 * and puts its reference into v0.
 */
static int op_new_instance(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int type_id = 0;
    type_id_item *type_item = 0;
    class_data_item *clazz = NULL;
    uint class_inst_size;

    reg_idx_vx = ptr[*pc + 1];
    type_id = ((ptr[*pc + 3] << 8) | ptr[*pc + 2]);

    type_item = get_type_item(dex, type_id);
    clazz = get_class_data_by_typeid(dex, type_id);
    //assert(clazz);
    sdvm_obj *obj;
    if (clazz) {
        class_inst_size = clazz->class_inst_size;
    } else {
        /* currently, we don't support java framework class, just allocate an
           sdvm_obj */
        class_inst_size = sizeof(sdvm_obj);
    }
    obj = (sdvm_obj *)malloc(class_inst_size);

    obj->ref_count = 1;
    obj->this_ptr = obj;  // todo : do we need this_ptr ?

    if (is_verbose()) {
        printf("new-instance v%d, type_id 0x%04x, size %d, ptr %p",
               reg_idx_vx, type_id, class_inst_size, obj);
        if (type_item != 0) {
            printf(", %s", get_string_data(dex, type_item->descriptor_idx));
        }
        printf("\n");
    }
    //store_to_reg(vm, reg_idx_vx, (unsigned char*)&type_id);
    store_to_reg(vm, reg_idx_vx, (unsigned char*)&obj);
    /* TODO */
    *pc = *pc + 4;
    return 0;
}

/* 35c format
 * A|G|op BBBB F|E|D|C
 * [A=5] op {vC, vD, vE, vF, vG}, meth@BBBB
 * [A=5] op {vC, vD, vE, vF, vG}, type@BBBB
 * [A=4] op {vC, vD, vE, vF}, kind@BBBB
 * [A=3] op {vC, vD, vE}, kind@BBBB
 * [A=2] op {vC, vD}, kind@BBBB
 * [A=1] op {vC}, kind@BBBB
 * [A=0] op {}, kind@BBBB
 * The unusual choice in lettering here reflects a desire to
 * make the count and the reference index have the same label as in format 3rc.
 */
static int op_utils_invoke_35c_parse(DexFileFormat *dex, u1 *ptr, int *pc,
                                     invoke_parameters *p)
{
    unsigned char tmp = 0;
    int i = 0;
    if (dex != 0 && ptr != 0 && p != 0) {
        memset(p, 0, sizeof(invoke_parameters));

        tmp = ptr[*pc + 1];
        p->reg_count = tmp >> 4;
        p->reg_idx[4] = tmp & 0x0F;

        p->method_id = ptr[*pc + 2];
        p->method_id |= (ptr[*pc + 3] << 4);

        tmp = ptr[*pc + 4];
        p->reg_idx[1] = tmp >> 4;
        p->reg_idx[0] = tmp & 0x0F;

        tmp = ptr[*pc + 5];
        p->reg_idx[3] = tmp >> 4;
        p->reg_idx[2] = tmp & 0x0F;
    }
    return 0;
}

static int op_utils_invoke(char *name, DexFileFormat *dex, simple_dalvik_vm *vm,
                           invoke_parameters *p)
{
    method_id_item *m = 0;
    type_id_item *type_class = 0;
    proto_id_item *proto_item = 0;
    type_list *proto_type_list = 0;

    if (p != 0) {
        m = get_method_item(dex, p->method_id);
        if (m != 0) {
            type_class = get_type_item(dex, m->class_idx);
            proto_item = get_proto_item(dex, m->proto_idx);
        }
        switch (p->reg_count) {
        case 0:
            if (is_verbose())
                printf("%s {} method_id 0x%04x", name, p->method_id);
            break;
        case 1:
            if (is_verbose())
                printf("%s, {v%d} method_id 0x%04x",
                       name, p->reg_idx[0], p->method_id);
            break;
        case 2:
            if (is_verbose())
                printf("%s {v%d, v%d} method_id 0x%04x",
                       name,
                       p->reg_idx[0], p->reg_idx[1],
                       p->method_id);
            break;
        case 3:
            if (is_verbose())
                printf("%s {v%d, v%d, v%d} method_id 0x%04x",
                       name,
                       p->reg_idx[0], p->reg_idx[1], p->reg_idx[2],
                       p->method_id);
            break;
        case 4:
            if (is_verbose())
                printf("%s {v%d, v%d, v%d, v%d} method_id 0x%04x",
                       name,
                       p->reg_idx[0], p->reg_idx[1],
                       p->reg_idx[2], p->reg_idx[3],
                       p->method_id);
            break;
        case 5:
            if (is_verbose())
                printf("%s {v%d, v%d, v%d, v%d, v%d} method_id 0x%04x",
                       name,
                       p->reg_idx[0], p->reg_idx[1], p->reg_idx[2],
                       p->reg_idx[3], p->reg_idx[4],
                       p->method_id);
            break;
        default:
            break;
        }

        if (m != 0 && type_class != 0 && p->reg_count <= 5) {
            class_data_item *clazz = get_class_data_by_typeid(dex, m->class_idx);
            if (proto_item != 0)
                proto_type_list = get_proto_type_list(dex, m->proto_idx);
            if (proto_type_list != 0 && proto_type_list->size > 0) {
                if (is_verbose())
                    printf(" %s,%s,(%s)%s \n",
                           get_string_data(dex, type_class->descriptor_idx),
                           get_string_data(dex, m->name_idx),
                           get_type_item_name(dex,
                                              proto_type_list->type_item[0].type_idx),
                           get_type_item_name(dex,
                                              proto_item->return_type_idx));

                if (clazz) {
                    int iter;
                    for (iter = 0; iter < clazz->direct_methods_size; iter++) {
                        if (p->method_id == clazz->direct_methods[iter].method_id)
                            break;
                    }
                    assert(iter < clazz->direct_methods_size);
                    runMethod(dex, vm, &clazz->direct_methods[iter]);
                } else {
                    invoke_java_lang_library(dex, vm,
                                             get_string_data(dex, type_class->descriptor_idx),
                                             get_string_data(dex, m->name_idx),
                                             get_type_item_name(dex, proto_type_list->type_item[0].type_idx));
                }
            } else {
                if (is_verbose())
                    printf(" %s,%s,()%s \n",
                           get_string_data(dex, type_class->descriptor_idx),
                           get_string_data(dex, m->name_idx),
                           get_type_item_name(dex,
                                              proto_item->return_type_idx));
                if (clazz) {
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
                } else {
                    invoke_java_lang_library(dex, vm,
                                             get_string_data(dex, type_class->descriptor_idx),
                                             get_string_data(dex, m->name_idx), 0);
                }
            }

        } else {
            if (is_verbose())
                printf("\n");
        }
    }
    return 0;
}

/* iput-wide vx,vy, field_id
 * Puts the wide value located in vx and vx+1 registers into an instance field.
 * The instance is referenced by vy.
 *
 * 5A20 0000 - iput-wide v0,v2, Test2.d0:D // field@0000
 * Stores the wide value in v0, v1 registers into field@0000 (entry #0 in the
 * field id table). The instance is referenced by v2.
 *
 * (dhry) dh.Number_Of_Runs = Long.valueOf(rdr.readLine()).longValue();
 */
static int op_iput_wide(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc) {
    int reg_idx_vx = ptr[*pc + 1] & 0x0F;
    int reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    u4 field_id = ((ptr[*pc + 3] << 8) | ptr[*pc + 2]);

    u8 val = 0;
    u4 *_pval = (u4 *)&val;
    u4 instance = 0;

    load_reg_to_long(vm, reg_idx_vx, (u1 *)&_pval[0]);
    load_reg_to_long(vm, reg_idx_vx+1, (u1 *)&_pval[1]);
    load_reg_to(vm, reg_idx_vy, (u1 *)&instance);

    //load_reg_to(vm, reg_idx_vx, (unsigned char *) &);
    *pc = *pc + 4;

    if (is_verbose()) {
        //printf("iput-wide v%d, v%d, field %x (val=lld%, instance=%x)\n", reg_idx_vx, reg_idx_vy, field_id, val, instance);
        printf("iput-wide v%d, v%d, field 0x%04x (val=%lld L, instance=%x)\n",
               reg_idx_vx, reg_idx_vy, field_id, val, instance);
    }

    return 0;
}

/* invoke-virtual { parameters }, methodtocall */
/*
 * 6E53 0600 0421 - invoke-virtual { v4, v0, v1, v2, v3}, Test2.method5:(IIII)V // method@0006
 * 6e20 0200 3200   invoke-virtual {v2, v3}, Ljava/io/PrintStream;.println:(Ljava/lang/String;)V // method@0002
 */
static int op_invoke_virtual(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int string_id = 0;

    op_utils_invoke_35c_parse(dex, ptr, pc, &vm->p);
    op_utils_invoke("invoke-virtual", dex, vm, &vm->p);
    /* TODO */
    *pc = *pc + 6;
    return 0;
}

/*  0x70 invoke-direct { parameters }, methodtocall
 *  . Invokes a method with parameters without the virtual method resolution
 *  . 7010 0800 0100 - invoke-direct {v1}, java.lang.Object.<init>:()V // method@0008
 *    Invokes the 8th method in the method table with just one parameter,
 *    v1 is the "this" instance
 */
static int op_invoke_direct(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    invoke_parameters p;
    int string_id = 0;

    op_utils_invoke_35c_parse(dex, ptr, pc, &vm->p);
    op_utils_invoke("invoke-direct", dex, vm, &vm->p);
    /* TODO */
    *pc = *pc + 6;
    return 0;
}

/* 0x71 invoke-direct
 * 7100 0300 0000  invoke-static {}, Ljava/lang/Math;.random:()D // method@0003
 */
static int op_invoke_static(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    invoke_parameters p;
    int string_id = 0;

    op_utils_invoke_35c_parse(dex, ptr, pc, &vm->p);
    op_utils_invoke("invoke-static", dex, vm, &vm->p);
    /* TODO */
    *pc = *pc + 6;
    return 0;
}

/* 0x62 sget-object vx,field_id
 * Reads the object reference field identified by the field_id into vx.
 * 6201 0C00 - sget-object v1, Test3.os1:Ljava/lang/Object; // field@000c
 * Reads field@000c (entry #CH in the field id table) into v1.
 */
static int op_sget_object(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int field_id = 0;
    int reg_idx_vx = 0;
    reg_idx_vx = ptr[*pc + 1];
    field_id = ((ptr[*pc + 3] << 8) | ptr[*pc + 2]);

    sdvm_obj *obj = get_static_obj_by_fieldid(dex, field_id);

    assert(((u8)obj >> 32) == 0);
    /*  we store object ptr to dalvik 32-bit register
     *  Note! In 64-bit linux, this ptr is 64-bit, but if the ptr comes from
     *  malloc, its high 32 bit = 0x0
     */
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &obj);
    if (is_verbose()) {
        printf("sget-object v%d, field 0x%04x, obj_ptr %p \n", reg_idx_vx, field_id, obj);
    }
    /* TODO */
    *pc = *pc + 4;
    return 0;
}

/*  0x69 sput-object vx, dst_field_id
 *  . Puts object reference in vx (reg_idx_vx) into a static field (dst_field_id).
 *  . 6900 0c00 - sput-object v0, Test3.os1:Ljava/lang/Object; // field@000c
 *    Puts the object reference value in v0 into the field@000c static field
 *    (entry #CH in the field id table).
 */
static int op_sput_object(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = ptr[*pc + 1];
    int dst_field_id = ((ptr[*pc + 3] << 8) | ptr[*pc + 2]);
    sdvm_obj *src_obj = NULL;
    load_reg_to(vm, reg_idx_vx, (unsigned char *) &src_obj);

    sdvm_obj *dst_obj = get_static_obj_by_fieldid(dex, dst_field_id);
    assert(((u8)dst_obj >> 32) == 0);
    assert(dst_obj->ref_count > 0);
    dst_obj->ref_count --;

    *dst_obj = *src_obj;
    dst_obj->ref_count++;

    if (is_verbose()) {
        printf("sput-object v%d, dst field 0x%04x, src_ptr %p, dst_ptr %p\n",
               reg_idx_vx, dst_field_id, src_obj, dst_obj);
    }

    /* TODO */
    *pc = *pc + 4;
    return 0;
}

/* 0x90 add-int vx,vy vz
 * Calculates vy+vz and puts the result into vx.
 * 9000 0203 - add-int v0, v2, v3
 * Adds v3 to v2 and puts the result into v0.
 */
static int op_add_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    int x = 0, y = 0 , z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    reg_idx_vz = ptr[*pc + 3];

    if (is_verbose())
        printf("add-int v%d, v%d, v%d\n", reg_idx_vx, reg_idx_vy,
               reg_idx_vz);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    load_reg_to(vm, reg_idx_vz, (unsigned char *) &z);
    x = y + z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);
    *pc = *pc + 4;
    return 0;

}

/* 0x91 sub-int vx,vy,vz
 * Calculates vy-vz and puts the result into vx.
 * 9100 0203 - sub-int v0, v2, v3
 * Subtracts v3 from v2 and puts the result into v0.
 */
static int op_sub_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    int x = 0, y = 0 , z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    reg_idx_vz = ptr[*pc + 3];

    if (is_verbose())
        printf("sub-int v%d, v%d, v%d\n", reg_idx_vx, reg_idx_vz,
               reg_idx_vy);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    load_reg_to(vm, reg_idx_vz, (unsigned char *) &z);
    x = y - z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);
    *pc = *pc + 4;
    return 0;
}

/* 0x92 mul-int vx, vy, vz
 * Multiplies vz with wy and puts the result int vx.
 * 9200 0203 - mul-int v0,v2,v3
 * Multiplies v2 with w3 and puts the result into v0
 */
static int op_mul_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    int x = 0, y = 0 , z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    reg_idx_vz = ptr[*pc + 3];

    if (is_verbose())
        printf("add-int v%d, v%d, v%d\n", reg_idx_vx, reg_idx_vy, reg_idx_vz);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    load_reg_to(vm, reg_idx_vz, (unsigned char *) &z);
    x = y * z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);
    *pc = *pc + 4;
    return 0;

}

/* 0x93 div-int vx,vy,vz
 * Divides vy with vz and puts the result into vx.
 * 9303 0001 - div-int v3, v0, v1
 * Divides v0 with v1 and puts the result into v3.
 */
static int op_div_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    int x = 0, y = 0 , z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    reg_idx_vz = ptr[*pc + 3];

    if (is_verbose())
        printf("add-int v%d, v%d, v%d\n", reg_idx_vx, reg_idx_vy, reg_idx_vz);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    load_reg_to(vm, reg_idx_vz, (unsigned char *) &z);
    x = y % z;
    x = (y - x) / z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);
    *pc = *pc + 4;
    return 0;

}

/* 0x8A double-to-int vx, vy
 * Converts the double value in vy,vy+1 into an integer value in vx.
 * 8A40  - double-to-int v0, v4
 * Converts the double value in v4,v5 into an integer value in v0.
 */
static int op_double_to_int(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    double d = 0;
    unsigned char *ptr_d = (unsigned char *) &d;
    int i = 0;
    int i2 = 0 ;
    reg_idx_vx = ptr[*pc + 1] & 0x0F;
    reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;
    reg_idx_vz = reg_idx_vy + 1;

    load_reg_to_double(vm, reg_idx_vy , ptr_d + 4);
    load_reg_to_double(vm, reg_idx_vz , ptr_d);

    i = (int)d;
    if (is_verbose()) {
        printf("double-to-int v%d, v%d\n", reg_idx_vx, reg_idx_vy);
        printf("(%f) to (%d) \n", d , i);
    }

    store_to_reg(vm, reg_idx_vx, (unsigned char *) &i);
    *pc = *pc + 2;
    return 0;
}

/* 0xb0 add-int/2addr vx,vy
 * Adds vy to vx.
 * B010 - add-int/2addr v0,v1 Adds v1 to v0.
 */
static int op_add_int_2addr(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int x = 0, y = 0;
    reg_idx_vx = ptr[*pc + 1] & 0x0F ;
    reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F ;
    if (is_verbose())
        printf("add-int/2addr v%d, v%d\n", reg_idx_vx, reg_idx_vy);
    load_reg_to(vm, reg_idx_vx, (unsigned char *) &x);
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    x = x + y;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);

    *pc = *pc + 2;
    return 0;
}

/* 0xcb , add-double/2addr
 * Adds vy to vx.
 * CB70 - add-double/2addr v0, v7
 * Adds v7 to v0.
 */
static int op_add_double_2addr(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    double x = 0.0, y = 0.0;
    unsigned char *ptr_x = (unsigned char *) &x;
    unsigned char *ptr_y = (unsigned char *) &y;
    reg_idx_vx = ptr[*pc + 1] & 0x0F;
    reg_idx_vy = (ptr[*pc + 1] >> 4) & 0x0F;

    load_reg_to_double(vm, reg_idx_vx, ptr_x + 4);
    load_reg_to_double(vm, reg_idx_vx + 1, ptr_x);
    load_reg_to_double(vm, reg_idx_vy, ptr_y + 4);
    load_reg_to_double(vm, reg_idx_vy + 1, ptr_y);


    if (is_verbose()) {
        printf("add-double/2addr v%d, v%d\n", reg_idx_vx, reg_idx_vy);
        printf("%f(%llx) + %f(%llx) = %f\n",
               x, *((long long unsigned int *)&x), y, *((long long unsigned int *)&y) , y + x);
    }
    x = x + y;
    store_double_to_reg(vm, reg_idx_vx, ptr_x + 4);
    store_double_to_reg(vm, reg_idx_vx + 1, ptr_x);
    *pc = *pc + 2;
    return 0;
}

/* 0xcd , mul-double/2addr
 * Multiplies vx with vy
 * CD20 - mul-double/2addr v0, v2
 * Multiplies the double value in v0,v1 with the
 * double value in v2,v3 and puts the result into v0,v1.
 */
static int op_mul_double_2addr(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int reg_idx_vz = 0;
    int reg_idx_vw = 0;
    double x = 0.0, y = 0.0;
    unsigned char *ptr_x = (unsigned char *) &x;
    unsigned char *ptr_y = (unsigned char *) &y;

    reg_idx_vx = ptr[*pc + 1] & 0x0F;
    reg_idx_vy = reg_idx_vx + 1;
    reg_idx_vz = (ptr[*pc + 1] >> 4) & 0x0F;
    reg_idx_vw = reg_idx_vz + 1 ;

    load_reg_to_double(vm, reg_idx_vx, ptr_x + 4);
    load_reg_to_double(vm, reg_idx_vy, ptr_x);

    load_reg_to_double(vm, reg_idx_vz, ptr_y + 4);
    load_reg_to_double(vm, reg_idx_vw, ptr_y);

    if (is_verbose()) {
        printf("mul-double/2addr v%d, v%d\n", reg_idx_vx, reg_idx_vz);
        printf(" %f * %f = %f\n", x, y, x * y);
    }

    x = x * y;

    store_double_to_reg(vm, reg_idx_vx, ptr_x + 4);
    store_double_to_reg(vm, reg_idx_vy, ptr_x);

    load_reg_to_double(vm, reg_idx_vx, ptr_y + 4);
    load_reg_to_double(vm, reg_idx_vy, ptr_y);

    *pc = *pc + 2;
    return 0;
}

/* 0xdb div-int/lit8 vx,vy,lit8
 * Calculates vy/lit8 and stores the result into vx.
 * DB00 0203 - div-int/lit8 v0,v2, #int3
 * Calculates v2/3 and stores the result into v0.
 */
static int op_div_int_lit8(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc)
{
    int reg_idx_vx = 0;
    int reg_idx_vy = 0;
    int x = 0, y = 0 ;
    int z = 0;
    reg_idx_vx = ptr[*pc + 1];
    reg_idx_vy = ptr[*pc + 2];
    z = ptr[*pc + 3];

    if (is_verbose())
        printf("add-int v%d, v%d, #int%d\n", reg_idx_vx, reg_idx_vy, z);
    /* x = y + z */
    load_reg_to(vm, reg_idx_vy, (unsigned char *) &y);
    x = y % z;
    x = (y - x) / z;
    store_to_reg(vm, reg_idx_vx, (unsigned char *) &x);

    *pc = *pc + 4;
    return 0;
}

static byteCode byteCodes[] = {
    { "move"              , 0x01, 2,  op_move },
    { "goto"              , 0x28, 2,  op_goto },
    { "if-ge"             , 0x35, 4,  op_if_ge },
    { "long-to-int"       , 0x84, 2,  op_long_to_int},
    { "sub-long/2addr"    , 0xbc, 2,  op_sub_long_2addr },
    { "add-int/lit8"      , 0xd8, 4,  op_add_int_lit8 },

    { "move-result-wide"  , 0x0B, 2,  op_move_result_wide },
    { "move-result-object", 0x0C, 2,  op_move_result_object },
    { "return-void"       , 0x0e, 2,  op_return_void },
    { "const/4"           , 0x12, 2,  op_const_4 },
    { "const/16"          , 0x13, 4,  op_const_16 },
    { "const-wide/high16" , 0x19, 4,  op_const_wide_high16 },
    { "const-string"      , 0x1a, 4,  op_const_string },
    { "new-instance"      , 0x22, 4,  op_new_instance },

    { "iput-wide"         , 0x5a, 4,  op_iput_wide },

    { "sget-object"       , 0x62, 4,  op_sget_object },
    { "sput-object"       , 0x69, 4,  op_sput_object },

    { "invoke-virtual"    , 0x6e, 6,  op_invoke_virtual },
    { "invoke-direct"     , 0x70, 6,  op_invoke_direct },
    { "invoke-static"     , 0x71, 6,  op_invoke_static },
    { "double-to-int"     , 0x8a, 2,  op_double_to_int},
    { "add-int"           , 0x90, 4,  op_add_int },
    { "sub-int"           , 0x91, 4,  op_sub_int },
    { "mul-int"           , 0x92, 4,  op_mul_int },
    { "div-int"           , 0x93, 4,  op_div_int },
    { "add-int/2addr"     , 0xb0, 2,  op_add_int_2addr},
    { "add-double/2addr"  , 0xcb, 2,  op_add_double_2addr},
    { "mul-double/2addr"  , 0xcd, 2,  op_mul_double_2addr},
    { "div-int/lit8"      , 0xdb, 4,  op_div_int_lit8 }
};
static size_t byteCode_size = sizeof(byteCodes) / sizeof(byteCode);

/**
    0007f4: 2310 1e00                              |0002: new-array v0, v1, [I // type@001e
    0007f8: 6900 0500                              |0004: sput-object v0, LGlobalVariables;.Array_Glob_1:[I // field@0005
    0007fc: 2420 1e00 1100                         |0006: filled-new-array {v1, v1}, [I // type@001e
    000810: 1f00 2000                              |0010: check-cast v0, [[I // type@0020

 */

static opCodeFunc findOpCodeFunc(unsigned char op)
{
    int i = 0;
    for (i = 0; i < byteCode_size; i++)
        if (op == byteCodes[i].opCode)
            return byteCodes[i].func;
    return 0;
}

void runMethod(DexFileFormat *dex, simple_dalvik_vm *vm, encoded_method *m)
{
    u1 *ptr = (u1 *) m->code_item.insns;
    unsigned char opCode = 0;
    opCodeFunc func = 0;

    vm->pc = 0;
    while (1) {
        if (vm->pc >= m->code_item.insns_size * sizeof(ushort))
            break;
        opCode = ptr[vm->pc];
        func = findOpCodeFunc(opCode);
        if (func != 0) {
            func(dex, vm, ptr, &vm->pc);
        } else {
            printRegs(vm);
            printf("Unknow OpCode =%02x \n", opCode);
            break;
        }
    }
}

void simple_dvm_startup(DexFileFormat *dex, simple_dalvik_vm *vm, char *entry)
{
    int i = 0, j = 0;
    int method_name_idx = -1;
    int method_idx = -1;
    int class_idx = -1;
    int direct_method_index = -1;

    method_name_idx = find_const_string(dex, entry);

    if (method_name_idx < 0) {
        printf("no method %s in dex\n", entry);
        return;
    }

    for (i = 0 ; i < dex->header.methodIdsSize; i++)
        if (dex->method_id_item[i].name_idx == method_name_idx) {
            int cls_id = dex->method_id_item[i].class_idx;
            method_idx = i;

            for (j = 0; j < dex->header.classDefsSize; j++) {
                if (dex->class_def_item[j].class_idx == cls_id) {
                    class_idx = j;
                    break;
                }
            }

            if (is_verbose() > 2)
                printf("find %s in class_defs[%d], method_id = %d\n",
                       entry, class_idx, method_idx);
            break;
        }

    for (i = 0; i < dex->class_data_item[class_idx].direct_methods_size; ++i) {
      if (dex->class_data_item[class_idx].direct_methods[i].method_id == method_idx) {
        if (is_verbose() > 2) {
          printf("find method %d in class of class_id[%d]\n", i, class_idx);
        }
        direct_method_index = i;
        break;
      }
    }

    if (class_idx < 0 || method_idx < 0 || direct_method_index < 0) {
        printf("no method %s in dex\n", entry);
        return;
    }

    encoded_method *m =
        &dex->class_data_item[class_idx].direct_methods[direct_method_index];

    if (is_verbose() > 2)
        printf("encoded_method method_id = %d, insns_size = %d\n",
               m->method_id, m->code_item.insns_size);

    memset(vm , 0, sizeof(simple_dalvik_vm));
    runMethod(dex, vm, m);
}
