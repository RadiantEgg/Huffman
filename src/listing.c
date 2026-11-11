#include "huffman/listing.h"

#include <ctype.h>
#include <stdio.h>

enum { HUFF_CODES_PER_LINE = 50 };

static HuffStatus close_listing(FILE *copy)
{
    int failed = ferror(stdout) || ferror(copy);
    if (fclose(copy) == EOF)
        failed = 1;
    if (failed)
        return HUFF_ERR_IO;
    return HUFF_OK;
}

static void write_symbol(FILE *fp, unsigned char symbol)
{
    if (isprint(symbol))
        fprintf(fp, "'%c'", symbol);
    else
        fprintf(fp, "\\x%02x", symbol);
}

static void write_node(FILE *fp, const HuffTree *tree, int index, int depth)
{
    for (int i = 0; i < depth; i++)
        fputs("  ", fp);

    const HuffNode *node = &tree->nodes[index];
    if (node->left < 0) {
        write_symbol(fp, node->symbol);
        fprintf(fp, " %llu\n", (unsigned long long)node->weight);
        return;
    }

    fprintf(fp, "%llu\n", (unsigned long long)node->weight);
    write_node(fp, tree, node->left, depth + 1);
    write_node(fp, tree, node->right, depth + 1);
}

HuffStatus huff_listing_codes(const char *encoded_path, const char *listing_path)
{
    if (encoded_path == NULL || listing_path == NULL)
        return HUFF_ERR_NULL;

    HuffBitReader reader;
    HuffStatus st = huff_bit_reader_open(&reader, encoded_path);
    if (st != HUFF_OK)
        return st;

    FILE *copy = fopen(listing_path, "w");
    if (copy == NULL) {
        huff_bit_reader_close(&reader);
        return HUFF_ERR_IO;
    }

    int column = 0;
    int any = 0;
    while (st == HUFF_OK) {
        int bit = 0;
        int got = 0;
        st = huff_bit_reader_get(&reader, &bit, &got);
        if (st != HUFF_OK || !got)
            break;

        int ch = bit ? '1' : '0';
        fputc(ch, stdout);
        fputc(ch, copy);
        any = 1;
        column++;
        if (column == HUFF_CODES_PER_LINE) {
            fputc('\n', stdout);
            fputc('\n', copy);
            column = 0;
        }
    }

    if (st == HUFF_OK && any && column != 0) {
        fputc('\n', stdout);
        fputc('\n', copy);
    }

    huff_bit_reader_close(&reader);
    if (st != HUFF_OK) {
        fclose(copy);
        return st;
    }
    return close_listing(copy);
}

HuffStatus huff_listing_tree(const HuffTree *tree, const char *listing_path)
{
    if (tree == NULL || listing_path == NULL)
        return HUFF_ERR_NULL;
    if (tree->nodes == NULL || tree->node_count <= 0)
        return HUFF_ERR_INVAL;

    FILE *copy = fopen(listing_path, "w");
    if (copy == NULL)
        return HUFF_ERR_IO;

    int root = tree->node_count - 1;
    write_node(stdout, tree, root, 0);
    write_node(copy, tree, root, 0);
    return close_listing(copy);
}
