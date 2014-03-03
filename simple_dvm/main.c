/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_dvm.h"

int main(int argc, char *argv[])
{
    DexFileFormat dex;
    simple_dalvik_vm vm;
    int x = 0;

    memset(&dex, 0, sizeof(DexFileFormat));
    if (argc < 2) {
        printf("%s [dex_file] \n", argv[0]);
        return 0;
    }
    if (argc >= 3)
        set_verbose(atoi(argv[2]));
    parseDexFile(argv[1], &dex);
    if (is_verbose() > 3) printDexFile(&dex);
    simple_dvm_startup(&dex, &vm, "main");

    return 0;
}
