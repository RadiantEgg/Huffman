#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

/* 内存中的树、建树、由叶子得到码、按比特走回符号。不打开文件。 */

#include "huffman/status.h"
#include <stdint.h>

typedef struct HuffNode {
    uint64_t weight;    // 文件里不知道如果字节出现很多次管不管用，后面可能要改
    int parent;
    int left;
    int right;
    unsigned char symbol;   // 存储叶子的符号
} HuffNode;

typedef struct HuffTree {
    HuffNode *nodes;
    int leaf_count;
    int node_count;
} HuffTree;


// 建树和释放

HuffStatus huff_tree_init(HuffTree *tree);
/* 建树，传入符号和权重数组 */

// 这里是直接覆盖，所以上次=层调用的时候要先free掉之前的树，防止泄漏
HuffStatus huff_tree_build(HuffTree *tree, const unsigned char *symbols, const uint64_t *weights, int leaf_count);  
/* 把文件里的树装回来 */
HuffStatus huff_tree_restore(HuffTree *tree, const HuffNode *nodes, int leaf_count);
HuffStatus huff_tree_destroy(HuffTree *tree);

// 编码
/* 根据symbol获得01编码字符串，写入bits里面，长度是length */
HuffStatus huff_tree_encode(const HuffTree *tree, unsigned char symbol, unsigned char *bits, int capacity, int *length);

// 译码
/* 用光标，一次一个bit */
typedef struct HuffCursor {
    int index;
} HuffCursor;

// 光标放在根上
void huff_tree_cursor_init(const HuffTree *tree, HuffCursor *cursor);
/* emitted 标记是不是叶子, symbol 记录叶子符号 */
// capacity 至少为leaf_count
HuffStatus huff_tree_cursor_step(const HuffTree *tree, HuffCursor *cursor, int bit, int *emitted, unsigned char *symbol);

#endif
