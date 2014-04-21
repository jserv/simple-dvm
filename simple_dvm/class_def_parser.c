/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"
#include <assert.h>

static sdvm_obj DEFAULT_SDVM_OBJ = {1, NULL};

static void parse_encoded_method(unsigned char *buf, encoded_method *method)
{
    int i = 0;
    int offset = 0;

    if (is_verbose() > 3)
        printf("      (parsing encoded method)\n");
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

const uint OBJ_BASE_SIZE = sizeof(sdvm_obj);

static encoded_field * load_encoded_field(DexFileFormat *dex, int is_static,
                                          unsigned char *buf, int offset,
                                          int count, int *size)
{
    int j, i = offset;
    int field_id = 0;
    int num_byte = 0;
    uint offset_from_this = OBJ_BASE_SIZE;
    encoded_field * sfields = (encoded_field *)
        malloc(sizeof(encoded_field) * count);

    for (j = 0; j < count; j++) {
        int field_idx_diff = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;

        field_id += field_idx_diff;
        sfields[j].field_id = field_id;

        sfields[j].access_flags = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;

        if (is_static) {
            sfields[j].field_size = 0;
            sfields[j].offset = 0;
        } else {
            /* how many bytes I am ? */
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
                field_size = sizeof(sdvm_obj);
            }
            sfields[j].field_size = field_size;
            sfields[j].offset = offset_from_this;
            offset_from_this += field_size;
        }

        if (is_verbose() > 3)
            printf("    . field_id = %d (field_idx_diff = %d), access_flags = %04x, size = %d, offset = %d\n",
                   sfields[j].field_id, field_idx_diff, sfields[j].access_flags,
                   sfields[j].field_size, sfields[j].offset);
    }

    *size = i - offset;
    return sfields;
}

static encoded_method * load_encoded_method(unsigned char *buf, int offset,
                                            int count, int *size)
{
    int j, i = offset;
    int method_id = 0;
    int num_byte = 0;
    encoded_method * method = (encoded_method *)
        malloc(sizeof(encoded_method) * count);

    for (j = 0; j < count; j++) {
        int method_idx_diff = get_uleb128_len(buf, i, &num_byte);
        method_id += method_idx_diff;
        method[j].method_id = method_id;
        i += num_byte;
        method[j].access_flags = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;
        method[j].code_off = get_uleb128_len(buf, i, &num_byte);
        i += num_byte;

        if (is_verbose() > 3)
            printf("    . encoded_method, method_id = %d (method_idx_diff = %d), access_flag = %04x, code_off = %04x\n",
                   method[j].method_id,
                   method_idx_diff,
                   method[j].access_flags,
                   method[j].code_off);

        // virtual method can be pure virtual (interface) => no code
        if (method[j].code_off)
            parse_encoded_method(buf, &method[j]);
    }
    *size = i - offset;
    return method;
}

static long read_encoded_value (unsigned char *buf, int *offset, int byteLength)
{
    long value = 0;
    int i = 0;
    int shift = 8 * byteLength;
    for (i = 0; i < byteLength; i++) {
        value |= (long)buf[*offset] << (8 * i);
        *offset = *offset + 1;
    }
    return (value << shift) >> shift;
}

void parse_static_data_item(unsigned char *buf, int static_data_offset,
                            static_field_data *sdata)
{
    assert(static_data_offset > 0);

    int size = 0, j;
    int offset = static_data_offset - sizeof(DexHeader);

    /*  Note! len may <= num_elements_of(sdata)
     *  in dex-format (Android website) :
     *  static_values_off uint
     *  - offset from the start of the file to the list of initial values for
     *    static fields, or 0 if there are none
     *  - the elements correspond to the static fields in the same order as
     *    declared in the corresponding field_list
     *  - the type of each array element must match the declared type of its
     *    corresponding field
     *  - If there are fewer elements in the array than there are static fields,
     *    then the leftover fields are initialized with a type-appropriate 0 or
     *    null
     */

    /*  TODO :
     *  - Necessary type check
     */
    int len = get_uleb128_len(buf, offset, &size);
    offset += size;

    printf("    offset = 0x%x, len = %d\n", static_data_offset, len);
    for (j = 0; j < len; ++j) {
        int data = 0;
        /* read 1 byte */
        memcpy(&data, buf + offset, 1);
        ++offset;
        int valueType = data & 0x1F;
        int valueArgument = data >> 5;
        switch(valueType){
            case VALUE_BYTE : {
                sdata[j].int_value = (byte)read_encoded_value(buf, &offset, 1);
                sdata[j].type = VALUE_BYTE;
                break;
            }
            case VALUE_CHAR : {
                sdata[j].int_value = (char)read_encoded_value(buf, &offset, valueArgument + 1);
                sdata[j].type = VALUE_CHAR;
                break;
            }
            case VALUE_SHORT : {
                sdata[j].int_value = (short)read_encoded_value(buf, &offset, valueArgument + 1);
                sdata[j].type = VALUE_SHORT;
                break;
            }
            case VALUE_INT : {
                sdata[j].int_value = (int)read_encoded_value(buf, &offset, valueArgument + 1);
                sdata[j].type = VALUE_INT;
                break;
            }
            case VALUE_LONG : {
                sdata[j].long_value = (long)read_encoded_value(buf, &offset, valueArgument + 1);
                sdata[j].type = VALUE_LONG;
                break;
            }
            /*
            case VALUE_STRING : {
                //sdata[j].objectValue = strings[(int)readValueByTypeArgument(valueArgument)];
                break;
            }
            case VALUE_NULL : {
                //sdata[j].objectValue = NULL;
                break;
            }*/
            case VALUE_BOOLEAN : {
                sdata[j].int_value = valueArgument;
                sdata[j].type = VALUE_BOOLEAN;
                break;
            }
            default : {
                printf("Error ! Unknow type id (0x%x)\n", valueType);
            }
        }
        if (is_verbose() > 3) {
            printf("    . type 0x%x, arg %d, value = (int, long) = 0x%x, 0x%lx\n",
                   valueType, valueArgument, sdata[j].int_value, sdata[j].long_value);
        }
    }
}

static void parse_class_data_item(DexFileFormat *dex,
                                  unsigned char *buf, int offset, int index)
{
    int i = 0;
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
            load_encoded_field(dex, TRUE, buf, i, static_fields_size, &size);
        i += size;
    }
    if (instance_fields_size > 0) {
        int j;
        if (is_verbose() > 3) {
            printf("  - instance_fields =\n");
        }
        dex->class_data_item[index].instance_fields_size = instance_fields_size;
        dex->class_data_item[index].instance_fields =
            load_encoded_field(dex, FALSE, buf, i, instance_fields_size, &size);
        i += size;

        for (j = 0; j < instance_fields_size; j++) {
            dex->class_data_item[index].class_inst_size +=
                dex->class_data_item[index].instance_fields[j].field_size;
        }
        dex->class_data_item[index].class_inst_size += OBJ_BASE_SIZE;

        printf("  - class instance size = %d\n",
               dex->class_data_item[index].class_inst_size);
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

    // load static data :
    if (static_fields_size > 0) {
        int j;
        const uint static_values_off = dex->class_def_item[index].static_values_off;
        if (is_verbose() > 3) {
            printf("  - load static data value\n");
        }

        static_field_data *sdata = (static_field_data *)malloc(sizeof(static_field_data) * static_fields_size);
        memset(sdata, 0, sizeof(static_field_data) * static_fields_size);

        for (j = 0; j < static_fields_size; j++) {
            //sdata[j].obj.ref_count = 1;
            //sdata[j].obj.clazz = &dex->class_def_item[index];
            sdata[j].obj = &DEFAULT_SDVM_OBJ;
            sdata[j].type = VALUE_SDVM_OBJ;
        }

        if (static_values_off > 0) {
            parse_static_data_item(buf, static_values_off, sdata);
        } else {
            printf("    (None Value)\n");
        }
        dex->class_data_item[index].sdata = sdata;
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

class_data_item *get_class_data_by_fieldid(DexFileFormat *dex, const int fieldid) {
    int iter;
    field_id_item *field = get_field_item(dex, fieldid);

    for (iter = 0; iter < dex->header.classDefsSize; iter++)
        if (field->class_idx == dex->class_def_item[iter].class_idx)
            return &dex->class_data_item[iter];
    return NULL;
}

class_data_item *get_class_data_by_typeid(DexFileFormat *dex, const int type_id) {
    int iter;

    for (iter = 0; iter < dex->header.classDefsSize; iter++)
        if (type_id == dex->class_def_item[iter].class_idx)
            return &dex->class_data_item[iter];
    return NULL;
}

sdvm_obj * get_static_obj_by_fieldid(DexFileFormat *dex, const int fieldid) {
    int iter;
    class_data_item *clazz = get_class_data_by_fieldid(dex, fieldid);
    field_id_item *field = get_field_item(dex, fieldid);

    assert(field);

    if (clazz) {
        for (iter = 0; iter < clazz->static_fields_size; iter++)
            if (clazz->static_fields[iter].field_id == fieldid) {
                assert(clazz->sdata[iter].type == VALUE_SDVM_OBJ);
                if (is_verbose() > 3) {
                    printf("    field_id = %d --> static_id = %d\n", fieldid, iter);
                }
                return clazz->sdata[iter].obj;
            }
        assert(FALSE);
    } else {
        /*  the clazz is defined in another dex, e.g. Ljava/lang/System, we
         *  don't care here, just return a default one */
        return &DEFAULT_SDVM_OBJ;
    }
}

static_field_data * get_static_field_data_by_fieldid(DexFileFormat *dex, const int fieldid) {
    int iter;
    class_data_item *clazz = get_class_data_by_fieldid(dex, fieldid);

    if (clazz) {
        for (iter = 0; iter < clazz->static_fields_size; iter++)
            if (clazz->static_fields[iter].field_id == fieldid) {
                if (is_verbose() > 3) {
                    printf("    field_id = %d --> static_id = %d\n", fieldid, iter);
                }
                return &clazz->sdata[iter];
            }
    }
    return NULL;
}

void put_static_obj_by_fieldid(DexFileFormat *dex, const int fieldid) {
}

