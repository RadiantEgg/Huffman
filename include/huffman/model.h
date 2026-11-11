#ifndef HUFFMAN_MODEL_H
#define HUFFMAN_MODEL_H

#include "huffman/tree.h"

HuffStatus huff_model_save(const HuffTree *tree, const char *path);
HuffStatus huff_model_load(HuffTree *tree, const char *path);

#endif
