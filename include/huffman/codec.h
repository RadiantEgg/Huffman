#ifndef HUFFMAN_CODEC_H
#define HUFFMAN_CODEC_H

#include "huffman/bitstream.h"
#include "huffman/tree.h"

HuffStatus huff_codec_count(const char *path, unsigned char *symbols, uint64_t *weights, int *leaf_count);

HuffStatus huff_codec_encode(const HuffTree *tree, const char *source_path, const char *encoded_path);

HuffStatus huff_codec_decode(const HuffTree *tree, const char *encoded_path, const char *decoded_path);

#endif
