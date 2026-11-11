#include "huffman/model.h"
#include <stdio.h>
#include <stdlib.h>

// 这个写weight
static HuffStatus write_u64(FILE *fp, uint64_t value)
{
    unsigned char bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = (unsigned char)(value & 0xFFu);
        value >>= 8;
    }
    if (fwrite(bytes, 1, 8, fp) != 8)
        return HUFF_ERR_IO;
    return HUFF_OK;
}

// 这个leaf_count和节点其他的成员
static HuffStatus write_i32(FILE *fp, int value)
{
    uint32_t bits = (uint32_t)value;
    unsigned char bytes[4];
    for (int i = 0; i < 4; i++) {
        bytes[i] = (unsigned char)(bits & 0xFFu);
        bits >>= 8;
    }
    if (fwrite(bytes, 1, 4, fp) != 4)
        return HUFF_ERR_IO;
    return HUFF_OK;
}
// 先规定为：树统一先写4字节leaf_count， 然后每个节点 8字节weight， 4字节left， 4字节right，1字节symbol
HuffStatus huff_model_save(const HuffTree *tree, const char *path)
{
    if (tree == NULL || path == NULL)
        return HUFF_ERR_NULL;
    if (tree->nodes == NULL || tree->leaf_count <= 1)
        return HUFF_ERR_INVAL;

    FILE *fp = fopen(path, "wb");
    if (fp == NULL)
        return HUFF_ERR_IO;

    HuffStatus st = write_i32(fp, tree->leaf_count);
    for (int i = 0; st == HUFF_OK && i < tree->node_count; i++) {
        const HuffNode *node = &tree->nodes[i];
        st = write_u64(fp, node->weight);
        if (st == HUFF_OK)
            st = write_i32(fp, node->parent);
        if (st == HUFF_OK)
            st = write_i32(fp, node->left);
        if (st == HUFF_OK)
            st = write_i32(fp, node->right);
        if (st == HUFF_OK && fputc(node->symbol, fp) == EOF)
            st = HUFF_ERR_IO;
    }

    fclose(fp);
    return st;
}


static HuffStatus read_u64(FILE *fp, uint64_t *value)
{
    unsigned char bytes[8];
    if (fread(bytes, 1, 8, fp) != 8)
        return HUFF_ERR_FORMAT;

    uint64_t result = 0;
    for (int i = 0; i < 8; i++)
        result |= (uint64_t)bytes[i] << (8 * i);
    *value = result;
    return HUFF_OK;
}

static HuffStatus read_i32(FILE *fp, int *value)
{
    unsigned char bytes[4];
    if (fread(bytes, 1, 4, fp) != 4)
        return HUFF_ERR_FORMAT;

    uint32_t bits = 0;
    for (int i = 0; i < 4; i++)
        bits |= (uint32_t)bytes[i] << (8 * i);
    *value = (int)bits;
    return HUFF_OK;
}

HuffStatus huff_model_load(HuffTree *tree, const char *path)
{
    if (tree == NULL || path == NULL)
        return HUFF_ERR_NULL;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return HUFF_ERR_IO;

    int leaf_count = 0;
    HuffStatus st = read_i32(fp, &leaf_count);
    if (st != HUFF_OK) {
        fclose(fp);
        return st;
    }
    if (leaf_count <= 1) {
        fclose(fp);
        return HUFF_ERR_FORMAT;
    }

    int node_count = 2 * leaf_count - 1;
    HuffNode *nodes = calloc((size_t)node_count, sizeof(HuffNode));
    if (nodes == NULL) {
        fclose(fp);
        return HUFF_ERR_NOMEM;
    }

    for (int i = 0; st == HUFF_OK && i < node_count; i++) {
        st = read_u64(fp, &nodes[i].weight);
        if (st == HUFF_OK)
            st = read_i32(fp, &nodes[i].parent);
        if (st == HUFF_OK)
            st = read_i32(fp, &nodes[i].left);
        if (st == HUFF_OK)
            st = read_i32(fp, &nodes[i].right);
        if (st != HUFF_OK)
            break;

        int symbol = fgetc(fp);
        if (symbol == EOF)
            st = HUFF_ERR_FORMAT;
        else
            nodes[i].symbol = (unsigned char)symbol;
    }
    fclose(fp);
    if (st != HUFF_OK) {
        free(nodes);
        return st;
    }

    HuffTree loaded;
    huff_tree_init(&loaded);
    st = huff_tree_restore(&loaded, nodes, leaf_count);
    free(nodes);    // resotre 里面已经深拷贝了
    if (st != HUFF_OK)
        return st;

    // 防止泄漏
    huff_tree_destroy(tree);
    *tree = loaded;
    return HUFF_OK;
}
