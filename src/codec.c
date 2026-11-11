#include "huffman/codec.h"
#include <stdio.h>
#include <stdlib.h>

HuffStatus huff_codec_count(const char *path, unsigned char *symbols,
 uint64_t *weights, int *leaf_count)
{
    if (path == NULL || symbols == NULL || weights == NULL || leaf_count == NULL) {
        return HUFF_ERR_NULL;
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return HUFF_ERR_IO;
    }

    uint64_t freqency[256] = {0};
    int byte;
    while ((byte = fgetc(fp)) != EOF) {
        freqency[byte]++;
    }

    if (ferror(fp)) {
        fclose(fp);
        return HUFF_ERR_IO;
    }
    fclose(fp);

    int count = 0;
    for (int i = 0; i < 256; i++) {
        if (freqency[i] == 0)
            continue;
        symbols[count] = (unsigned char)i;
        weights[count] = freqency[i];
        count++;
    }

    *leaf_count = count;
    return HUFF_OK;
}

HuffStatus huff_codec_encode(const HuffTree *tree, const char *source_path,
                             const char *encoded_path)
{
    if (tree == NULL || tree->nodes == NULL || source_path == NULL || encoded_path == NULL)
        return HUFF_ERR_NULL;

    FILE *src = fopen(source_path, "rb");
    if (src == NULL)
        return HUFF_ERR_IO;

    HuffBitWriter writer;
    HuffStatus st = huff_bit_writer_open(&writer, encoded_path);
    if (st != HUFF_OK) {
        fclose(src);
        return st;
    }

    unsigned char *bits = malloc(tree->leaf_count);
    if (bits == NULL) {
        fclose(src);
        huff_bit_writer_close(&writer);
        return HUFF_ERR_NOMEM;
    }

    int byte;
    while (st == HUFF_OK && (byte = fgetc(src)) != EOF) {
        int length = 0;
        st = huff_tree_encode(tree, (unsigned char)byte, bits, tree->leaf_count, &length);
        for (int i = 0; st == HUFF_OK && i < length; i++)
            st = huff_bit_writer_put(&writer, bits[i]);
    }
    if (st == HUFF_OK && ferror(src))
        st = HUFF_ERR_IO;

    fclose(src);
    free(bits);
    HuffStatus close_st = huff_bit_writer_close(&writer);
    if (st != HUFF_OK)
        return st;
    return close_st;
}

HuffStatus huff_codec_decode(const HuffTree *tree, const char *encoded_path,
                             const char *decoded_path)
{
    if (tree == NULL || tree->nodes == NULL || encoded_path == NULL || decoded_path == NULL)
        return HUFF_ERR_NULL;

    HuffBitReader reader;
    HuffStatus st = huff_bit_reader_open(&reader, encoded_path);
    if (st != HUFF_OK)
        return st;

    FILE *out = fopen(decoded_path, "wb");
    if (out == NULL) {
        huff_bit_reader_close(&reader);
        return HUFF_ERR_IO;
    }

    HuffCursor cursor;
    huff_tree_cursor_init(tree, &cursor);

    while (st == HUFF_OK) {
        int bit = 0;
        int got = 0;
        st = huff_bit_reader_get(&reader, &bit, &got);
        if (st != HUFF_OK || !got)
            break;

        int emitted = 0;
        unsigned char symbol = 0;
        st = huff_tree_cursor_step(tree, &cursor, bit, &emitted, &symbol);
        if (st != HUFF_OK)
            break;
        if (emitted && fputc(symbol, out) == EOF) {
            st = HUFF_ERR_IO;
            break;
        }
    }

    if (st == HUFF_OK && cursor.index != tree->node_count - 1)
        st = HUFF_ERR_FORMAT;

    fclose(out);
    HuffStatus close_st = huff_bit_reader_close(&reader);
    if (st != HUFF_OK)
        return st;
    return close_st;
}