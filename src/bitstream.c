#include "huffman/bitstream.h"

HuffStatus huff_bit_writer_open(HuffBitWriter *writer, const char *path)
{
    if (writer == NULL || path == NULL) {
        return HUFF_ERR_NULL;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return HUFF_ERR_IO;
    }

    // 头八个字节清零，最后再写入有效比特数
    unsigned char header[8] = {0};
    if (fwrite(header, 1, 8, file) != 8) {
        fclose(file);
        return HUFF_ERR_IO;
    }

    writer->file = file;
    writer->current = 0;
    writer->filled = 0;
    writer->count = 0;
    return HUFF_OK;
}
HuffStatus huff_bit_writer_put(HuffBitWriter *writer, int bit)
{
    if (writer == NULL || writer->file == NULL) {
        return HUFF_ERR_NULL;
    }
    if (bit != 0 && bit != 1) {
        return HUFF_ERR_INVAL;
    }

    if (bit == 1) {
        writer->current |= (unsigned char)(1u << (7 - writer->filled));
    }

    writer->filled++;
    writer->count++;

    if (writer->filled == 8) {
        if (fputc(writer->current, writer->file) == EOF) {
            return HUFF_ERR_IO;
        }
        writer->current = 0;
        writer->filled = 0;
    }
    return HUFF_OK;
}
HuffStatus huff_bit_writer_close(HuffBitWriter *writer)
{
    if (writer == NULL || writer->file == NULL) {
        return HUFF_ERR_IO;
    }

    if (writer->filled > 0) {
        if (fputc(writer->current, writer->file) == EOF) {
            fclose(writer->file);
            writer->file = NULL;
            return HUFF_ERR_IO;
        }
    }

    if (fseek(writer->file, 0, SEEK_SET) != 0) {
        fclose(writer->file);
        writer->file = NULL;
        return HUFF_ERR_IO;
    }

    unsigned char header[8];
    uint64_t count = writer->count;
    for (int i = 0; i < 8; i++) {
        header[i] = (unsigned char)(count & 0xFFu);
        count >>= 8;
    }
    
    if (fwrite(header, 1, 8, writer->file) != 8) {
        fclose(writer->file);
        writer->file = NULL;
        return HUFF_ERR_IO;
    }

    fclose(writer->file);
    writer->file = NULL;
    writer->current = 0;
    writer->filled = 0;
    return HUFF_OK;
}
HuffStatus huff_bit_reader_open(HuffBitReader *reader, const char *path)
{
    if (reader == NULL || path == NULL) {
        return HUFF_ERR_NULL;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return HUFF_ERR_IO;
    }
    reader->file = file;

    unsigned char header[8];
    if (fread(header, 1, 8, reader->file) != 8) {
        fclose(file);
        file = NULL;
        return HUFF_ERR_IO;
    }

    uint64_t count = 0;
    for (int i = 0; i < 8; i++) {
        count |= (uint64_t)header[i] << (8 * i);
    }

    reader->count = count;
    reader->current = 0;
    reader->done = 0;
    reader->remaining = 0;
    return HUFF_OK;
}
HuffStatus huff_bit_reader_get(HuffBitReader *reader, int *bit, int *got)
{
    if (reader == NULL || reader->file == NULL || bit == NULL || got == NULL) {
        return HUFF_ERR_NULL;
    }
    
    if (reader->done == reader->count) {
        *got = 0;
        return HUFF_OK;
    }

    if (reader->remaining == 0) {
        int next = fgetc(reader->file);
        if (next == EOF) {
            return HUFF_ERR_IO;
        }
        reader->current = (unsigned char)next;
        reader->remaining = 8;
    }

    *bit = (reader->current >> (reader->remaining - 1)) & 1;
    reader->remaining--;
    reader->done++;
    *got = 1;
    return HUFF_OK;
}
HuffStatus huff_bit_reader_close(HuffBitReader *reader)
{
    if (reader == NULL || reader->file == NULL) {
        return HUFF_ERR_IO;
    }
    fclose(reader->file);
    reader->file = NULL;
    reader->current = 0;
    reader->remaining = 0;
    reader->count = 0;
    reader->done = 0;
    return HUFF_OK;   
}