#include "huffman/tree.h"

#include <stdio.h>
#include <string.h>

static const char *st_name(HuffStatus s)
{
    switch (s) {
    case HUFF_OK:         return "OK";
    case HUFF_ERR_NULL:   return "NULL";
    case HUFF_ERR_NOMEM:  return "NOMEM";
    case HUFF_ERR_INVAL:  return "INVAL";
    case HUFF_ERR_IO:     return "IO";
    case HUFF_ERR_FORMAT: return "FORMAT";
    case HUFF_ERR_SYMBOL: return "SYMBOL";
    default:              return "?";
    }
}

static int fail_count = 0;

static void check(const char *what, int cond)
{
    if (!cond) {
        printf("[FAIL] %s\n", what);
        fail_count++;
    } else {
        printf("[PASS] %s\n", what);
    }
}

int main(void)
{
    HuffTree tree;
    huff_tree_init(&tree);

    unsigned char sym[] = {'a', 'b', 'c', 'd'};
    uint64_t w[] = {5, 3, 2, 1};
    HuffStatus st = huff_tree_build(&tree, sym, w, 4);
    check("build 4 leaves", st == HUFF_OK);
    printf("  node_count=%d leaf_count=%d\n", tree.node_count, tree.leaf_count);

    struct { unsigned char s; const char *expect; } cases[] = {
        {'a', "0"}, {'b', "10"}, {'c', "111"}, {'d', "110"},
    };
    for (int i = 0; i < 4; i++) {
        unsigned char bits[16];
        int len = -1;
        st = huff_tree_encode(&tree, cases[i].s, bits, 16, &len);
        int ok = (st == HUFF_OK && len == (int)strlen(cases[i].expect));
        if (ok) {
            for (int j = 0; j < len; j++) {
                if (bits[j] != (unsigned char)(cases[i].expect[j] - '0')) {
                    ok = 0;
                    break;
                }
            }
        }
        char label[64];
        snprintf(label, sizeof label, "encode '%c' -> %s", cases[i].s, cases[i].expect);
        check(label, ok);
    }

    unsigned char bits[16];
    int len;
    st = huff_tree_encode(&tree, 'z', bits, 16, &len);
    check("encode unknown symbol -> SYMBOL", st == HUFF_ERR_SYMBOL);

    st = huff_tree_encode(&tree, 'c', bits, 2, &len);   /* 'c' 需要 3 bit */
    check("encode capacity too small -> INVAL", st == HUFF_ERR_INVAL);

    const char *seq = "0" "10" "111" "110";
    const char *expect = "abcd";
    HuffCursor cursor;
    huff_tree_cursor_init(&tree, &cursor);
    char got[8] = {0};
    int got_n = 0;
    for (const char *p = seq; *p; p++) {
        int emitted = 0;
        unsigned char out = 0;
        st = huff_tree_cursor_step(&tree, &cursor, *p - '0', &emitted, &out);
        if (st != HUFF_OK) break;
        if (emitted) got[got_n++] = (char)out;
    }
    check("decode round-trip 'abcd'", strcmp(got, expect) == 0);
    printf("  decoded: \"%s\"\n", got);

    st = huff_tree_build(&tree, (unsigned char[]){'x'}, (uint64_t[]){5}, 1);
    check("build 1 leaf -> INVAL", st == HUFF_ERR_INVAL);

    huff_tree_destroy(&tree);
    check("destroy resets", tree.nodes == NULL);
    st = huff_tree_build(&tree, sym, w, 4);
    check("rebuild after destroy", st == HUFF_OK);

    huff_tree_destroy(&tree);
    printf("\n%d failure(s)\n", fail_count);
    return fail_count ? 1 : 0;
}