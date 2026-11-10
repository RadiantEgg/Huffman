#ifndef HUFFMAN_STATUS_H
#define HUFFMAN_STATUS_H

typedef enum {
    HUFF_OK = 0,
    HUFF_ERR_NULL,
    HUFF_ERR_NOMEM,
    HUFF_ERR_INVAL,
    HUFF_ERR_IO,
    HUFF_ERR_FORMAT,    // 系统读写失败
    HUFF_ERR_SYMBOL,
    HUFF_STATUS_COUNT     // 文件内容与格式不符
} HuffStatus;

#endif
