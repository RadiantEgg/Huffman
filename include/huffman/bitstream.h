#ifndef HUFFMAN_BITSTREAM_H
#define HUFFMAN_BITSTREAM_H

/* 比特的顺序写入和读出。落盘形式是紧凑字节。不决定符号和码的对应。 */

#include "huffman/status.h"
#include <stdio.h>
#include <stdint.h>

typedef struct HuffBitWriter {
    FILE *file;
    unsigned char current;
    int filled;     // current 里面放了几位
    uint64_t count;
} HuffBitWriter;

typedef struct HuffBitReader {
    FILE *file;
    unsigned char current;
    int remaining;     // current里面还没读的字节
    uint64_t count;
    uint64_t done;
} HuffBitReader;

HuffStatus huff_bit_writer_open(HuffBitWriter *writer, const char *path);
HuffStatus huff_bit_writer_put(HuffBitWriter *writer, int bit);
HuffStatus huff_bit_writer_close(HuffBitWriter *writer);
HuffStatus huff_bit_reader_open(HuffBitReader *reader, const char *path);
HuffStatus huff_bit_reader_get(HuffBitReader *reader, int *bit, int *got);
HuffStatus huff_bit_reader_close(HuffBitReader *reader);

#endif
