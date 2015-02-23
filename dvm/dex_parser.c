/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#include "simple_dvm.h"

/* Print Dex File Format */
static void printDexHeader(DexHeader *dex)
{
    int i = 0;
    printf("Magic Number = ");
    for (i = 0 ; i < 8 ; i++)
        printf("0x%02x ", dex->magic[i]);
    /*printf("( ");
    for (i = 0 ; i < 8 ; i++)
        if (dex->magic[i] != '\n') {
            printf("%c", dex->magic[i]);
        } else {
            printf("\\n");
        }
    printf(" )\n");*/

    printf("\n");
    printf("Checksum      = ");
    for (i = 3 ; i >= 0 ; i--)
        printf("%02x ", dex->checksum[i]);
    printf("\n");

    printf("FileSize      = %4d (0x%04x)\n", dex->fileSize, dex->fileSize);
    printf("headerSize    = %4d (0x%04x)\n", dex->headerSize, dex->headerSize);
    printf("endianTag     = %4d (0x%04x)\n", dex->endianTag, dex->endianTag);
    printf("linkSize      = %4d (0x%04x)\n", dex->linkSize, dex->linkSize);
    printf("mapOff        = %4d (0x%04x)\n", dex->mapOff, dex->mapOff);
    printf("stringIdsSize = %4d (0x%04x)\n", dex->stringIdsSize, dex->stringIdsSize);
    printf("stringIdsOff  = %4d (0x%04x)\n", dex->stringIdsOff, dex->stringIdsOff);
    printf("typeIdsSize   = %4d (0x%04x)\n", dex->typeIdsSize, dex->typeIdsSize);
    printf("typeIdsOff    = %4d (0x%04x)\n", dex->typeIdsOff, dex->typeIdsOff);
    printf("protoIdsSize  = %4d (0x%04x)\n", dex->protoIdsSize, dex->protoIdsSize);
    printf("protoIdsOff   = %4d (0x%04x)\n", dex->protoIdsOff, dex->protoIdsOff);
    printf("fieldIdsSize  = %4d (0x%04x)\n", dex->fieldIdsSize, dex->fieldIdsSize);
    printf("fieldIdsOff   = %4d (0x%04x)\n", dex->fieldIdsOff, dex->fieldIdsOff);
    printf("methodIdsSize = %4d (0x%04x)\n", dex->methodIdsSize, dex->methodIdsSize);
    printf("methodIdsOff  = %4d (0x%04x)\n", dex->methodIdsOff, dex->methodIdsOff);
    printf("classDefsSize = %4d (0x%04x)\n", dex->classDefsSize, dex->classDefsSize);
    printf("classDefsOff  = %4d (0x%04x)\n", dex->classDefsOff, dex->classDefsOff);
    printf("dataSize      = %4d (0x%04x)\n", dex->dataSize, dex->dataSize);
    printf("dataOff       = %4d (0x%04x)\n", dex->dataOff, dex->dataOff);
}

void printDexFile(DexFileFormat *dex)
{
    printDexHeader(&dex->header);
}

/* Parse Dex File */
int parseDexFile(char *file, DexFileFormat *dex)
{
    FILE *fp = 0;
    unsigned char *buf = 0;
    size_t read_count;

    fp = fopen(file, "rb");
    if (fp == 0) {
        printf("Open file %s failed\n", file);
        return -1;
    }
    memset(dex, 0, sizeof(dex));
    read_count = fread(&dex->header, sizeof(DexHeader), 1, fp);
    if (read_count != 1) {
        printf("Reading dex header error (expect : %d, actual : %zd)\n",
               1, read_count);
        return -1;
    }

    /* NOTE! So buf doesn't contain dex header, so the all kind of offset value
     * should minus sizeof(DexHeader)
     */
    buf = (unsigned char *) malloc(
              sizeof(u1) * (dex->header.fileSize - sizeof(DexHeader)));

    /* read all value into buf */
    read_count = fread(buf, (dex->header.fileSize - sizeof(DexHeader)), 1, fp);
    if (read_count != 1) {
        printf("Reading whole dex file error (expect : %d, actual : %zd)\n",
               1, read_count);
        return -1;
    }

    fclose(fp);

    parse_map_list(dex, buf, dex->header.mapOff - sizeof(DexHeader));
    parse_string_ids(dex, buf, dex->header.stringIdsOff - sizeof(DexHeader));
    parse_type_ids(dex, buf, dex->header.typeIdsOff - sizeof(DexHeader));
    parse_proto_ids(dex, buf, dex->header.protoIdsOff - sizeof(DexHeader));
    parse_field_ids(dex, buf, dex->header.fieldIdsOff - sizeof(DexHeader));
    parse_method_ids(dex, buf, dex->header.methodIdsOff - sizeof(DexHeader));
    parse_class_defs(dex, buf, dex->header.classDefsOff - sizeof(DexHeader));

    if (dex->header.dataSize > 0) {
        assert(dex->header.dataSize == dex->header.fileSize - dex->header.dataOff);
        dex->data = malloc(sizeof(u1) * dex->header.dataSize);
        memcpy(dex->data, buf + dex->header.dataOff - sizeof(DexHeader),
               sizeof(u1) * dex->header.dataSize);

        if (is_verbose() > 3) {
            printf("copy data part, offset = 0x%x, size = %d\n",
                   dex->header.dataOff, dex->header.dataSize);
        }
    }

    free(buf);
    return 0;
}
