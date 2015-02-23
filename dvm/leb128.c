/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

/* get string ids
 * little endian
 * 0b0xxxxxxx
 * 0b1xxxxxxx
 */
int get_uleb128_len(unsigned char *buf, int offset, int *size)
{
    int len = 0;
    int i = offset;
    unsigned char value = 0;
    int j = 0;
    do {
        value = buf[i];
        i++;
        if ((value & 0x80) != 0) {
            len = (len | (value ^ 0x80) << (j * 7));
        } else {
            len = (len | value << j * 7);
            break;
        }
        j++;
    } while (1);
    *size = i - offset;
    return len;
}
