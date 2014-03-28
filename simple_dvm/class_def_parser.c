/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

static void parse_encoded_method(unsigned char *buf, encoded_method *method)
{
    int i = 0;
    int offset = 0;

    if (is_verbose() > 3)
        printf("    . parse encoded method\n");
    offset = method->code_off - sizeof(DexHeader);

    memcpy(&method->code_item.registers_size, buf + offset, sizeof(ushort));
    offset += sizeof(ushort);
    memcpy(&method->code_item.ins_size, buf + offset, sizeof(ushort));
    offset += sizeof(ushort);
    memcpy(&method->code_item.outs_size, buf + offset, sizeof(ushort));
    offset += sizeof(ushort);
    memcpy(&method->code_item.tries_size, buf + offset, sizeof(ushort));
    offset += sizeof(ushort);
    memcpy(&method->code_item.debug_info_off, buf + offset, sizeof(uint));
    offset += sizeof(uint);
    memcpy(&method->code_item.insns_size, buf + offset, sizeof(uint));
    offset += sizeof(uint);

    if (is_verbose() > 3) {
        printf("      - ins_size = %d\n", method->code_item.ins_size);
        printf("      - outs_size = %d\n", method->code_item.outs_size);
        printf("      - registers_size = %d\n", method->code_item.registers_size);
        printf("      - insns_size = %d\n", method->code_item.insns_size);
    }
    method->code_item.insns = malloc(sizeof(ushort) * method->code_item.insns_size);
    memcpy(method->code_item.insns, buf + offset,
           sizeof(ushort) * method->code_item.insns_size);
    offset += sizeof(ushort) * method->code_item.insns_size;
}

static encoded_field * load_encoded_field(unsigned char *buf, int offset,
                                          int count, int *size)
{
    int j, i = offset;
    int num_byte = 0;
    encoded_field * sfields = (encoded_field *)
        malloc(sizeof(encoded_field) * count);

    for (j = 0; j < count; j++) {
        sfields[j].field_idx_diff = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;
        sfields[j].access_flags = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;

        if (is_verbose() > 3)
            printf("    . field_idx_diff = %d, access_flags = %04x\n",
                   sfields[j].field_idx_diff, sfields[j].access_flags);
    }

    *size = i - offset;
    return sfields;
}

static encoded_method * load_encoded_method(unsigned char *buf, int offset,
                                            int count, int *size)
{
    int j, i = offset;
    int num_byte = 0;
    encoded_method * method = (encoded_method *)
        malloc(sizeof(encoded_method) * count);

    for (j = 0; j < count; j++) {
        method[j].method_idx_diff = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;
        method[j].access_flags = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;
        method[j].code_off = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;

        if (is_verbose() > 3)
            printf("    . encoded_method, method_idx_diff = %d, access_flag = %04x, code_off = %04x\n",
                   method[j].method_idx_diff,
                   method[j].access_flags,
                   method[j].code_off);

        // virtual method can be pure virtual (interface) => no code
        if (method[j].code_off)
            parse_encoded_method(buf, &method[j]);
    }
    *size = i - offset;
    return method;
}

static void parse_class_data_item(DexFileFormat *dex,
                                  unsigned char *buf, int offset, int index)
{
    int i = 0;
    int j = 0;
    int size = 0;
    int len = 0;
    int static_fields_size = 0;
    int instance_fields_size = 0;
    int direct_methods_size = 0;
    int virtual_methods_size = 0;
    i = offset;

    static_fields_size = get_uleb128_len(buf, i, &size);
    i += size;

    instance_fields_size = get_uleb128_len(buf, i, &size);
    i += size;

    direct_methods_size = get_uleb128_len(buf, i, &size);
    i += size;

    virtual_methods_size = get_uleb128_len(buf, i, &size);
    i += size;

    if (is_verbose() > 3) {
        printf("  - class_data_item[%d] (size means count)\n", index);
        printf("    . static_fields_size = %d\n", static_fields_size);
        printf("    . instance_fields_size = %d\n", instance_fields_size);
        printf("    . direct_method_size = %d\n", direct_methods_size);
        printf("    . virtual_method_size = %d\n", virtual_methods_size);
        printf("i = %d\n", i);
    }

    /** ref : http://source.android.com/devices/tech/dalvik/dex-format.html
        class_data_item
            static_fields_size      uleb128
            instance_fields_size    uleb128
            direct_methods_size     uleb128
            virtual_methods_size    uleb128
            static_fields           encoded_field[static_fields_size]
            instance_fields         encoded_field[instance_fields_size]
            direct_methods          encoded_method[direct_methods_size]
            virtual_methods         encoded_method[virtual_methods_size]
     */
    if (static_fields_size > 0) {
        if (is_verbose() > 3) {
            printf("  - static_fields =\n");
        }
        dex->class_data_item[index].static_fields_size = static_fields_size;
        dex->class_data_item[index].static_fields =
            load_encoded_field(buf, i, static_fields_size, &size);
        i += size;
    }
    if (instance_fields_size > 0) {
        if (is_verbose() > 3) {
            printf("  - instance_fields =\n");
        }
        dex->class_data_item[index].instance_fields_size = instance_fields_size;
        dex->class_data_item[index].instance_fields =
            load_encoded_field(buf, i, instance_fields_size, &size);
        i += size;
    }
    if (direct_methods_size > 0) {
        if (is_verbose() > 3) {
            printf("  - direct_methods =\n");
        }
        dex->class_data_item[index].direct_methods_size = direct_methods_size;
        dex->class_data_item[index].direct_methods =
            load_encoded_method(buf, i, direct_methods_size, &size);
        i += size;
    }
    if (virtual_methods_size > 0) {
        if (is_verbose() > 3) {
            printf("  - virtual_methods =\n");
        }
        dex->class_data_item[index].virtual_methods_size = virtual_methods_size;
        dex->class_data_item[index].virtual_methods =
            load_encoded_method(buf, i, virtual_methods_size, &size);
        i += size;
    }
}

void parse_class_defs(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i = 0;
    if (is_verbose() > 3)
        printf("parse class defs offset = %04x\n", (unsigned int)(offset + sizeof(DexHeader)));
    if (dex->header.classDefsSize <= 0)
        return;
    dex->class_def_item = malloc(
                              sizeof(class_def_item) * dex->header.classDefsSize);
    dex->class_data_item = malloc(
                               sizeof(class_data_item) * dex->header.classDefsSize);

    for (i = 0 ; i < dex->header.classDefsSize; i++) {
        memcpy(&dex->class_def_item[i],
               buf + i * sizeof(class_def_item) + offset,
               sizeof(class_def_item));
        if (is_verbose() > 3) {
            printf(" class_defs[%d], cls_id = %d, data_off = 0x%04x, source_file_idx = %d\n",
                   i,
                   dex->class_def_item[i].class_idx,
                   dex->class_def_item[i].class_data_off,
                   dex->class_def_item[i].source_file_idx);
        }
        parse_class_data_item(dex, buf,
                              dex->class_def_item[i].class_data_off - sizeof(DexHeader), i);
    }
}
