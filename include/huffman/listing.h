#ifndef HUFFMAN_LISTING_H
#define HUFFMAN_LISTING_H

#include "huffman/bitstream.h"
#include "huffman/tree.h"

HuffStatus huff_listing_codes(const char *encoded_path, const char *listing_path);

HuffStatus huff_listing_tree(const HuffTree *tree, const char *listing_path);

#endif
