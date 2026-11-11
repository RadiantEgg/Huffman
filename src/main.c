#include "huffman/codec.h"
#include "huffman/listing.h"
#include "huffman/model.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    HUFF_PATH_SIZE = 1024,
    HUFF_LINE_SIZE = 256,
    MENU_CONTINUE = 1,
    MENU_EOF = 0
};

static const char *const k_model_path = "model.huf";
static const char *const k_encoded_path = "encoded.huf";
static const char *const k_decoded_path = "decoded.bin";
static const char *const k_codes_path = "codes.txt";
static const char *const k_tree_path = "tree.txt";

static void report_error(HuffStatus st)
{
    printf("失败：%s\n", huff_status_message(st));
}

static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n')
        buf[--n] = '\0';
    if (n > 0 && buf[n - 1] == '\r')
        buf[--n] = '\0';
    if (n + 1 < size || feof(stdin))
        return 1;

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
    return 1;
}

/* 直接回车用 fallback。返回 0 表示输入结束。 */
static int ask_path(const char *prompt, const char *fallback, char *out, size_t size)
{
    printf("%s [%s]: ", prompt, fallback);
    fflush(stdout);
    if (!read_line(out, size))
        return 0;
    if (out[0] == '\0')
        snprintf(out, size, "%s", fallback);
    return 1;
}

/* 没有默认值，空行要求重来。返回 0 表示输入结束。 */
static int ask_required(const char *prompt, char *out, size_t size)
{
    for (;;) {
        printf("%s: ", prompt);
        fflush(stdout);
        if (!read_line(out, size))
            return 0;
        if (out[0] != '\0')
            return 1;
        printf("需要文件路径。\n");
    }
}

static int tree_in_memory(const HuffTree *tree)
{
    return tree->nodes != NULL && tree->node_count > 0;
}

static int parse_symbol_weight(const char *line, unsigned char *symbol, uint64_t *weight)
{
    const char *p = line;
    if (p[0] == '\\' && p[1] == 'x') {
        char *end = NULL;
        unsigned long value = strtoul(p + 2, &end, 16);
        if (end == p + 2 || value > 255)
            return 0;
        *symbol = (unsigned char)value;
        p = end;
    } else {
        if (*p == '\0')
            return 0;
        *symbol = (unsigned char)*p;
        p++;
    }

    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0' || *p == '-' || *p == '+')
        return 0;

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(p, &end, 10);
    if (end == p || errno == ERANGE || value == 0)
        return 0;
    while (*end == ' ' || *end == '\t')
        end++;
    if (*end != '\0')
        return 0;

    *weight = (uint64_t)value;
    return 1;
}

static int install_tree(HuffTree *tree, HuffTree *next, const char *model_path)
{
    HuffStatus st = huff_model_save(next, model_path);
    if (st != HUFF_OK) {
        huff_tree_destroy(next);
        report_error(st);
        return MENU_CONTINUE;
    }

    huff_tree_destroy(tree);
    *tree = *next;
    printf("完成，模型已写入 %s。\n", model_path);
    return MENU_CONTINUE;
}

static int init_from_file(HuffTree *tree)
{
    char source[HUFF_PATH_SIZE];
    char model[HUFF_PATH_SIZE];
    if (!ask_required("源文件", source, sizeof source))
        return MENU_EOF;
    if (!ask_path("模型文件", k_model_path, model, sizeof model))
        return MENU_EOF;

    unsigned char symbols[256];
    uint64_t weights[256];
    int leaf_count = 0;
    HuffStatus st = huff_codec_count(source, symbols, weights, &leaf_count);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    if (leaf_count <= 1) {
        report_error(HUFF_ERR_INVAL);
        return MENU_CONTINUE;
    }

    HuffTree next;
    huff_tree_init(&next);
    st = huff_tree_build(&next, symbols, weights, leaf_count);
    if (st != HUFF_OK) {
        huff_tree_destroy(&next);
        report_error(st);
        return MENU_CONTINUE;
    }
    return install_tree(tree, &next, model);
}

static int init_from_terminal(HuffTree *tree)
{
    char model[HUFF_PATH_SIZE];
    if (!ask_path("模型文件", k_model_path, model, sizeof model))
        return MENU_EOF;

    printf("符号个数: ");
    fflush(stdout);
    char line[HUFF_LINE_SIZE];
    if (!read_line(line, sizeof line))
        return MENU_EOF;

    char *end = NULL;
    long count = strtol(line, &end, 10);
    while (*end == ' ' || *end == '\t')
        end++;
    if (end == line || *end != '\0' || count < 2 || count > 256) {
        printf("符号个数要在 2 和 256 之间。\n");
        return MENU_CONTINUE;
    }

    unsigned char symbols[256];
    uint64_t weights[256];
    printf("每行一个符号和一个权值。符号是该行第一个字符；空格写成 \\x20 3。\n");
    for (long i = 0; i < count; i++) {
        for (;;) {
            printf("符号 %ld: ", i + 1);
            fflush(stdout);
            if (!read_line(line, sizeof line))
                return MENU_EOF;

            unsigned char symbol = 0;
            uint64_t weight = 0;
            if (!parse_symbol_weight(line, &symbol, &weight)) {
                printf("这一行无法识别，请再输入一次。\n");
                continue;
            }

            int duplicated = 0;
            for (long j = 0; j < i; j++) {
                if (symbols[j] == symbol)
                    duplicated = 1;
            }
            if (duplicated) {
                printf("这个符号已经有了，请再输入一次。\n");
                continue;
            }

            symbols[i] = symbol;
            weights[i] = weight;
            break;
        }
    }

    HuffTree next;
    huff_tree_init(&next);
    HuffStatus st = huff_tree_build(&next, symbols, weights, (int)count);
    if (st != HUFF_OK) {
        huff_tree_destroy(&next);
        report_error(st);
        return MENU_CONTINUE;
    }
    return install_tree(tree, &next, model);
}

static int command_init(HuffTree *tree)
{
    printf("来源，f 文件 / t 终端 [f]: ");
    fflush(stdout);
    char line[HUFF_LINE_SIZE];
    if (!read_line(line, sizeof line))
        return MENU_EOF;

    if (line[0] == '\0' || line[0] == 'f' || line[0] == 'F')
        return init_from_file(tree);
    if (line[0] == 't' || line[0] == 'T')
        return init_from_terminal(tree);

    printf("请输入 f 或 t。\n");
    return MENU_CONTINUE;
}

/* 内存里没有树时，从模型文件读入一次。 */
static int ensure_model(HuffTree *tree)
{
    if (tree_in_memory(tree))
        return MENU_CONTINUE;

    char model[HUFF_PATH_SIZE];
    if (!ask_path("模型文件", k_model_path, model, sizeof model))
        return MENU_EOF;

    HuffStatus st = huff_model_load(tree, model);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    return MENU_CONTINUE;
}

static int command_encode(HuffTree *tree)
{
    int ready = ensure_model(tree);
    if (ready == MENU_EOF || !tree_in_memory(tree))
        return ready;

    char source[HUFF_PATH_SIZE];
    char encoded[HUFF_PATH_SIZE];
    if (!ask_required("源文件", source, sizeof source))
        return MENU_EOF;
    if (!ask_path("编码文件", k_encoded_path, encoded, sizeof encoded))
        return MENU_EOF;

    HuffStatus st = huff_codec_encode(tree, source, encoded);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    printf("完成，已写入 %s。\n", encoded);
    return MENU_CONTINUE;
}

static int command_decode(HuffTree *tree)
{
    int ready = ensure_model(tree);
    if (ready == MENU_EOF || !tree_in_memory(tree))
        return ready;

    char encoded[HUFF_PATH_SIZE];
    char decoded[HUFF_PATH_SIZE];
    if (!ask_path("编码文件", k_encoded_path, encoded, sizeof encoded))
        return MENU_EOF;
    if (!ask_path("译码文件", k_decoded_path, decoded, sizeof decoded))
        return MENU_EOF;

    HuffStatus st = huff_codec_decode(tree, encoded, decoded);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    printf("完成，已写入 %s。\n", decoded);
    return MENU_CONTINUE;
}

static int command_codes(void)
{
    char encoded[HUFF_PATH_SIZE];
    char listing[HUFF_PATH_SIZE];
    if (!ask_path("编码文件", k_encoded_path, encoded, sizeof encoded))
        return MENU_EOF;
    if (!ask_path("代码清单", k_codes_path, listing, sizeof listing))
        return MENU_EOF;

    HuffStatus st = huff_listing_codes(encoded, listing);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    printf("完成，清单已写入 %s。\n", listing);
    return MENU_CONTINUE;
}

static int command_tree(const HuffTree *tree)
{
    if (!tree_in_memory(tree)) {
        printf("请先建立模型。\n");
        return MENU_CONTINUE;
    }

    char listing[HUFF_PATH_SIZE];
    if (!ask_path("树清单", k_tree_path, listing, sizeof listing))
        return MENU_EOF;

    HuffStatus st = huff_listing_tree(tree, listing);
    if (st != HUFF_OK) {
        report_error(st);
        return MENU_CONTINUE;
    }
    printf("完成，清单已写入 %s。\n", listing);
    return MENU_CONTINUE;
}

int main(void)
{
    HuffTree tree;
    if (huff_tree_init(&tree) != HUFF_OK)
        return 1;

    int running = MENU_CONTINUE;
    while (running == MENU_CONTINUE) {
        printf("\nI 初始化  E 编码  D 译码  P 印代码  T 印树  Q 退出\n> ");
        fflush(stdout);

        char line[HUFF_LINE_SIZE];
        if (!read_line(line, sizeof line))
            break;

        const char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            continue;

        switch (toupper((unsigned char)*p)) {
        case 'I':
            running = command_init(&tree);
            break;
        case 'E':
            running = command_encode(&tree);
            break;
        case 'D':
            running = command_decode(&tree);
            break;
        case 'P':
            running = command_codes();
            break;
        case 'T':
            running = command_tree(&tree);
            break;
        case 'Q':
            running = MENU_EOF;
            break;
        default:
            printf("未知命令。\n");
            break;
        }
    }

    huff_tree_destroy(&tree);
    return 0;
}
