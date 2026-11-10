#include "huffman/status.h"

static const char *const messages[HUFF_STATUS_COUNT] = {
    [HUFF_OK]         = "",
    [HUFF_ERR_NULL]   = "调用参数是空指针",
    [HUFF_ERR_NOMEM]  = "内存分配失败",
    [HUFF_ERR_INVAL]  = "参数不合法",
    [HUFF_ERR_IO]     = "读写文件失败",
    [HUFF_ERR_FORMAT] = "文件内容与格式不符",
    [HUFF_ERR_SYMBOL] = "符号不在当前模型中",
};

