/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

void parse_type_ids(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i = 0;
    if (is_verbose() > 3)
        printf("parse type ids offset = %04x\n", (uint)(offset + sizeof(DexHeader)));
    dex->type_id_item = malloc(
                            sizeof(type_id_item) * dex->header.typeIdsSize);

    for (i = 0; i < dex->header.typeIdsSize; i++) {
        memcpy(&dex->type_id_item[i],
               buf + i * sizeof(type_id_item) + offset,
               sizeof(type_id_item));

        if (is_verbose() > 3)
            printf(" type_ids [%d], = %s\n", i,
                   dex->string_data_item[
                       dex->type_id_item[i].descriptor_idx].data);
    }
}

type_id_item *get_type_item(DexFileFormat *dex, int type_id)
{
    if (type_id >= 0 && type_id < dex->header.typeIdsSize)
        return &dex->type_id_item[type_id];
    return 0;
}

char *get_type_item_name(DexFileFormat *dex, int type_id)
{
    type_id_item *type_item = get_type_item(dex, type_id);
    if (type_item != 0)
        return get_string_data(dex, type_item->descriptor_idx);
    return 0;
}

void parse_proto_ids(DexFileFormat *dex, unsigned char *buf, int offset)
{
    volatile int i = 0, j = 0;
    int idx = 0;
    if (is_verbose() > 3)
        printf("parse proto ids offset = %04x\n", (uint)(offset + sizeof(DexHeader)));
    dex->proto_id_item = malloc(
                             sizeof(proto_id_item) * dex->header.protoIdsSize);

    dex->proto_type_list = malloc(
                               sizeof(type_list) * dex->header.protoIdsSize);
    for (i = 0 ; i < dex->header.protoIdsSize; i++) {
        memcpy(&dex->proto_id_item[i],
               buf + i * sizeof(proto_id_item) + offset,
               sizeof(proto_id_item));
        memset(&dex->proto_type_list[i], 0, sizeof(type_list));
        idx = dex->proto_id_item[i].return_type_idx;
        if (is_verbose() > 3)
            printf(" proto_id_item [%d], %s, type_id = %d %s, parameters_off = %08x\n", i,
                   dex->string_data_item[dex->proto_id_item[i].shorty_idx].data,
                   idx, get_type_item_name(dex, idx),
                   dex->proto_id_item[i].parameters_off);
        if (dex->proto_id_item[i].parameters_off == 0)
            continue;
        if (is_verbose() > 3)
            printf(" proto_typ_list[%d] offset %p ", i,
                   buf + dex->proto_id_item[i].parameters_off - sizeof(DexHeader));
        memcpy(&dex->proto_type_list[i].size,
               buf + dex->proto_id_item[i].parameters_off - sizeof(DexHeader),
               sizeof(int));

        if (is_verbose() > 3)
            printf("proto_type_list[%d].size = %d\n", i,
                   dex->proto_type_list[i].size);
        if (dex->proto_type_list[i].size > 0) {
            dex->proto_type_list[i].type_item = (type_item *)
                                                malloc(sizeof(type_item) * dex->proto_type_list[i].size);

            for (j = 0 ; j < dex->proto_type_list[i].size ; j++) {
                memset(&dex->proto_type_list[i].type_item[j], 0, sizeof(type_item));
                type_item *item = &dex->proto_type_list[i].type_item[j];
                memcpy(item,
                       buf
                       + dex->proto_id_item[i].parameters_off
                       - sizeof(DexHeader)
                       + 4
                       + (sizeof(type_item) * j),
                       sizeof(type_item));

                if (is_verbose() > 3)
                    printf("item[%d], type_idx = %d, type = %s\n",
                           j, item->type_idx,
                           get_type_item_name(dex, item->type_idx));
            }
        }
    }
}

proto_id_item *get_proto_item(DexFileFormat *dex, int proto_id)
{
    if (proto_id >= 0 && proto_id < dex->header.protoIdsSize)
        return &dex->proto_id_item[proto_id];
    return 0;
}

type_list *get_proto_type_list(DexFileFormat *dex, int proto_id)
{
    if (proto_id >= 0 && proto_id < dex->header.protoIdsSize)
        return &dex->proto_type_list[proto_id];
    return 0;
}

void parse_field_ids(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i;
    if (is_verbose() > 3)
        printf("parse feild ids offset = %04x\n", (uint)(offset + sizeof(DexHeader)));
    dex->field_id_item = malloc(sizeof(field_id_item) * dex->header.fieldIdsSize);

    if (is_verbose() > 3)
        printf("dex->header.fieldIdsSize = %d\n", dex->header.fieldIdsSize);
    for (i = 0; i < dex->header.fieldIdsSize; i++) {
        memcpy(&dex->field_id_item[i],
               buf + i * sizeof(field_id_item) + offset,
               sizeof(field_id_item));

        if (is_verbose() > 3) {
            printf(" field_id_item [%d], class_id = %d %s, type_id = %d %s, name_idx=%d %s\n",
                   i, dex->field_id_item[i].class_idx,
                   dex->string_data_item[
                       dex->type_id_item[
                           dex->field_id_item[i].class_idx].descriptor_idx].data,

                   dex->field_id_item[i].type_idx,
                   dex->string_data_item[
                       dex->type_id_item[
                           dex->field_id_item[i].type_idx].descriptor_idx].data,
                   dex->field_id_item[i].name_idx,
                   dex->string_data_item[dex->field_id_item[i].name_idx].data);
        }
    }
}

field_id_item *get_field_item(DexFileFormat *dex, int field_id)
{
    if (field_id >= 0 && field_id < dex->header.fieldIdsSize)
        return &dex->field_id_item[field_id];
    return 0;
}
