/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

static char *map_item_type_name_00[7] = {
    "TYPE_HEADER_ITEM",
    "TYPE_STRING_ID_ITEM",
    "TYPE_TYPE_ID_ITEM",
    "TYPE_PROTO_ID_ITEM",
    "TYPE_FIELD_ID_ITEM",
    "TYPE_METHOD_ID_ITEM",
    "TYPE_CLASS_DEF_ITEM"
};

static char *map_item_type_name_10[4] = {
    "TYPE_MAP_LIST",
    "TYPE_TYPE_LIST",
    "TYPE_ANNOTATION_SET_REF_LIST",
    "TYPE_ANNOTATION_SET_ITEM"
};

static char *map_item_type_name_20[7] = {
    "TYPE_CLASS_DATA_ITEM",
    "TYPE_CODE_ITEM",
    "TYPE_STRING_DATA_ITEM",
    "TYPE_DEBUG_INFO_ITEM",
    "TYPE_ANNOTATION_ITEM",
    "TYPE_ENCODED_ARRAY_ITEM",
    "TYPE_ANNOTATIONS_DIRECTORY_ITEM"
};

static char *get_map_item_type_name(int type_id)
{
    int table = 0;
    int value = 0;

    table = (type_id >> 8) & 0xFF;
    value = type_id & 0xFF;
    if (table == 0x00 && value >= 0 && value <= 6) {
        return map_item_type_name_00[value];
    } else if (table == 0x10 && value >= 0 && value <= 3) {
        return map_item_type_name_10[value];
    } else if (table == 0x20 && value >= 0 && value <= 6) {
        return map_item_type_name_20[value];
    }
    return 0;
}

/* Parse type list */
static void parse_type_list(DexFileFormat *dex,
                            unsigned char *buf, int offset)
{
    int i = 0;
    if (is_verbose() > 3)
        printf("parse type_list offset = %04x\n", (uint)(offset + sizeof(DexHeader)));
    memcpy(&dex->type_list.size , buf + offset, 4);
    if (is_verbose() > 3)
        printf("type_list size = %d\n", dex->type_list.size);
    if (dex->type_list.size > 0) {
        dex->type_list.type_item = malloc(sizeof(type_item) * dex->type_list.size);
        for (i = 0 ; i < dex->type_list.size; i++) {
            memcpy(&dex->type_list.type_item[i],
                   buf + offset + 4 + (sizeof(type_item) * i),
                   sizeof(type_item));
            if (is_verbose() > 3)
                printf("type_list[%d], type_idx = %d\n", i,
                       dex->type_list.type_item[i].type_idx);
        }
    }
}

/* Parse Map Item */
static void parse_map_item(DexFileFormat *dex,
                           unsigned char *buf, int offset, int index)
{
    int size_in_bytes = 0;
    memcpy(&dex->map_list.map_item[index], buf + offset, sizeof(map_item));
    size_in_bytes = 4 + (dex->map_list.map_item[index].size * 2);
    if (is_verbose() > 3) {
        printf("offset = %04x ", (uint)(offset + sizeof(DexHeader)));
        printf("map_item[%d] : type = %04x(%s), size = %04x, "
               "offset = 0x%04x, item_size_in_byte = %d\n",
               index,
               dex->map_list.map_item[index].type,
               get_map_item_type_name(dex->map_list.map_item[index].type),
               dex->map_list.map_item[index].size,
               dex->map_list.map_item[index].offset,
               size_in_bytes);
    }
    if (dex->map_list.map_item[index].type == 0x1001)  /* TYPE_TYPE_LIST */
        parse_type_list(dex, buf,
                        dex->map_list.map_item[index].offset - sizeof(DexHeader));
}

/* Parse Map list */
void parse_map_list(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i = 0;
    if (is_verbose() > 3)
        printf("parse map_list offset = %04x\n", (uint)(offset + sizeof(DexHeader)));
    memcpy(&dex->map_list.size , buf + offset, 4);
    if (is_verbose() > 3)
        printf("map_list size = %d\n", dex->map_list.size);
    if (dex->map_list.size > 0) {
        dex->map_list.map_item = malloc(sizeof(map_item) * dex->map_list.size);
        for (i = 0 ; i < dex->map_list.size; i++)
            parse_map_item(dex, buf, offset + 4 + (sizeof(map_item) * i), i);
    }
}
