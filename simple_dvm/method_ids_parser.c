/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

void parse_method_ids(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i = 0;
    printf("parse method ids offset = %04x\n", offset + sizeof(DexHeader));
    dex->method_id_item = malloc(
                              sizeof(method_id_item) * dex->header.methodIdsSize);

    for (i = 0 ; i < dex->header.methodIdsSize ; i++) {
        memcpy(&dex->method_id_item[i],
               buf + i * sizeof(method_id_item) + offset,
               sizeof(method_id_item));

        if (is_verbose() > 3)
            printf(" method[%d], cls_id = %d, proto_id = %d, name_id = %d, %s\n",
                   i,
                   dex->method_id_item[i].class_idx,
                   dex->method_id_item[i].proto_idx,
                   dex->method_id_item[i].name_idx,
                   dex->string_data_item[dex->method_id_item[i].name_idx].data);
    }
}

method_id_item *get_method_item(DexFileFormat *dex, int method_id)
{
    if (method_id >= 0 && method_id < dex->header.methodIdsSize)
        return &dex->method_id_item[method_id];
    return 0;
}

int get_method_name(DexFileFormat *dex, int method_id, char *name)
{
    method_id_item *m = get_method_item(dex, method_id);
    type_id_item *type_class = 0;
    proto_id_item *proto_item = 0;
    char *method_name = 0;
    char *class_name = 0;
    if (m != 0) {
        method_name = get_string_data(dex, m->name_idx);
        type_class = get_type_item(dex, m->class_idx);
        if (type_class != 0)
            class_name = get_string_data(dex, type_class->descriptor_idx);
        proto_item = get_proto_item(dex, m->proto_idx);
    }
    return 0;
}
