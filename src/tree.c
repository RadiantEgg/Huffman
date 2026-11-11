#include "huffman/tree.h"

#include <stdlib.h>
#include <string.h>

// 建树和释放

HuffStatus huff_tree_init(HuffTree *tree)
{
    if (tree == NULL) {
        return HUFF_ERR_NULL;
    }

    tree->nodes = NULL;
    tree->leaf_count = 0;
    tree->node_count = 0;
    return HUFF_OK;
}

HuffStatus huff_tree_destroy(HuffTree *tree)
{
    if (tree == NULL) {
        return HUFF_ERR_NULL;
    }
    free(tree->nodes);
    tree->nodes = NULL;
    tree->leaf_count = 0;
    tree->node_count = 0;
    return HUFF_OK;
}

static HuffStatus huff_select_min_pair(const HuffNode *nodes, int count, int *first, int *second)
{
    if (nodes == NULL || first == NULL || second == NULL) {
        return HUFF_ERR_NULL;
    }
    if (count <= 1) {
        return HUFF_ERR_INVAL;
    }

    int left = -1;
    int right = -1;
    for (int i = 0; i < count; i++) {
        if (nodes[i].parent >= 0) {
            continue;
        }

        if (left == -1) {
            left = i;
        } else if (nodes[i].weight < nodes[left].weight) {
            right = left;
            left = i;
        } else if (right == -1 || nodes[i].weight < nodes[right].weight) {
            right = i;
        }
    }

    *first = left;
    *second = right;
    return HUFF_OK;
}

HuffStatus huff_tree_build(HuffTree *tree, const unsigned char *symbols, const uint64_t *weights, int leaf_count)
{
    if (tree == NULL || symbols == NULL || weights == NULL) {
        return HUFF_ERR_NULL;
    }
    if (leaf_count <= 1) {
        return HUFF_ERR_INVAL;
    }

    int node_count = 2 *leaf_count - 1;
    HuffNode *nodes = calloc(node_count, sizeof(HuffNode));
    if (nodes == NULL) {
        return HUFF_ERR_NOMEM;
    }

    // 初始化
    for (int i = 0; i < node_count; i++) {
        nodes[i].left = -1;
        nodes[i].right = -1;
        nodes[i].parent = -1;
        if (i < leaf_count) {
            nodes[i].symbol = symbols[i];
            nodes[i].weight = weights[i];
        }
    }

    // 构建
    for (int i = leaf_count; i < node_count; i++) {
        int left;
        int right;
        HuffStatus st = huff_select_min_pair(nodes, i, &left, &right);
        if (st != HUFF_OK) {
            free(nodes);
            return st;
        }

        nodes[left].parent = i;
        nodes[right].parent = i;
        nodes[i].left = left;
        nodes[i].right = right;
        nodes[i].weight = nodes[left].weight + nodes[right].weight;
    }

    tree->nodes = nodes;
    tree->leaf_count = leaf_count;
    tree->node_count = node_count;
    return HUFF_OK;
} 
/* 把文件里的树装回来 */
HuffStatus huff_tree_restore(HuffTree *tree, const HuffNode *nodes, int leaf_count)
{
    if (tree == NULL || nodes == NULL) {
        return HUFF_ERR_NULL;
    }
    if (leaf_count <= 1) {
        return HUFF_ERR_INVAL;
    }

    int node_count = 2 * leaf_count - 1;
    HuffNode *copy = calloc(node_count, sizeof(HuffNode));
    if (copy == NULL) {
        return HUFF_ERR_NOMEM;
    }

    memcpy(copy, nodes, node_count * sizeof(HuffNode));
    tree->nodes = copy;
    tree->leaf_count = leaf_count;
    tree->node_count = node_count;
    
    return HUFF_OK;
}

// 编码
/* 根据symbol获得01编码字符串，写入bits里面，长度是length */
static HuffStatus huff_index_of_symbol(const HuffNode *nodes, int leaf_count, const unsigned char symbol, int *index)
{
    if (nodes == NULL || index == NULL) {
        return HUFF_ERR_NULL;
    }
    *index = -1;
    if (leaf_count <= 1) {
        return HUFF_ERR_INVAL;
    }

    for (int i = 0; i < leaf_count; i++) {
        if (nodes[i].symbol == symbol) {
            *index = i;
            return HUFF_OK;
        }
    }
    return HUFF_ERR_SYMBOL;
}

// bits 是调用方准备好的数组，capacity是能放多少bit
HuffStatus huff_tree_encode(const HuffTree *tree, unsigned char symbol, unsigned char *bits, int capacity, int *length)
{
    if (tree == NULL || bits == NULL || length == NULL) {
        return HUFF_ERR_NULL;
    }
    if (capacity < tree->leaf_count) {
        return HUFF_ERR_INVAL;
    }

    int index;
    const HuffNode *nodes = tree->nodes;
    HuffStatus st = huff_index_of_symbol(nodes, tree->leaf_count, symbol, &index);
    if (st != HUFF_OK) {
        return st;
    }

    int start = capacity;
    int child = index;
    int parent = nodes[index].parent;
    while (parent >= 0) {
        if (start == 0) {
            return HUFF_ERR_INVAL;
        }
        bits[--start] = (nodes[parent].left == child) ? 0 : 1;
        child = parent;
        parent = nodes[parent].parent;
    }

    *length = capacity - start;
    memmove(bits, bits + start, *length);
    return HUFF_OK;
}


void huff_tree_cursor_init(const HuffTree *tree, HuffCursor *cursor)
{
    // 这边保证了树一已经建好，cursor正常
    cursor->index = tree->node_count - 1;
}

HuffStatus huff_tree_cursor_step(const HuffTree *tree, HuffCursor *cursor, int bit, int *emitted, unsigned char *symbol)
{
    if (tree == NULL || emitted == NULL || symbol == NULL) {
        return HUFF_ERR_NULL;
    }
    if (bit != 0 && bit != 1) {
        return HUFF_ERR_INVAL;
    }

    const HuffNode *nodes = tree->nodes;
    int next = (bit == 0) ? nodes[cursor->index].left : nodes[cursor->index].right;

    if (next == -1) {
        return HUFF_ERR_INVAL;
    }

    cursor->index = next;
    if (nodes[next].left == -1) {
        *emitted = 1;
        *symbol = nodes[next].symbol;
        cursor->index = tree->node_count - 1;
    } else {
        *emitted = 0;
    }
    return HUFF_OK;
}