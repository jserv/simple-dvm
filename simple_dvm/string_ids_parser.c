/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

static void parse_string_data_item(DexFileFormat *dex,
                                   unsigned char *buf, int offset, int index)
{
    int i = 0;
    int size = 0;
    int len = 0;
    if (is_verbose() > 3)
        printf("parse string data item offset = %04x ",
               offset + sizeof(DexHeader));
    memset(&dex->string_data_item[index], 0, sizeof(string_data_item));
    dex->string_data_item[index].index = index;
    dex->string_data_item[index].uleb128_len =
        get_uleb128_len(buf, offset , &size) ;

    strncpy(dex->string_data_item[index].data,
            buf + offset + size,
            dex->string_data_item[index].uleb128_len);
    if (is_verbose() > 3) {
        printf("str[%2d], len = %4d, ",
               dex->string_data_item[index].index,
               dex->string_data_item[index].uleb128_len
              );
        printf("data = ");
        for (i = 0; i < dex->string_data_item[index].uleb128_len; i++)
            printf("%c", dex->string_data_item[index].data[i]);
        printf("\n");
    }
}

void parse_string_ids(DexFileFormat *dex, unsigned char *buf, int offset)
{
    int i = 0;
    if (is_verbose() > 3)
        printf("parse string ids offset = %04x\n", offset + sizeof(DexHeader));
    dex->string_ids = malloc(
                          sizeof(string_ids) * dex->header.stringIdsSize);
    dex->string_data_item = malloc(
                                sizeof(string_data_item) * dex->header.stringIdsSize);
    for (i = 0 ; i < dex->header.stringIdsSize ; i++) {
        memcpy(&dex->string_ids[i].string_data_off,
               buf + i * 4 + offset, 4);
        parse_string_data_item(dex, buf,
                               dex->string_ids[i].string_data_off - sizeof(dex->header),
                               i);
    }
}

static string_data_item *get_string_data_item(DexFileFormat *dex, int string_id)
{
    if (string_id >= 0 && string_id < dex->header.stringIdsSize)
        return &dex->string_data_item[string_id];
    return 0;
}

char *get_string_data(DexFileFormat *dex, int string_id)
{
    string_data_item *s = get_string_data_item(dex, string_id);
    if (s != 0)
        return s->data;
    return 0;
}
