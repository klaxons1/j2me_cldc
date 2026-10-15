/*
 * midpZipRead.c
 *
 * A tiny, self-contained ZIP (JAR) entry reader used by the MIDlet
 * suite picker to read META-INF/MANIFEST.MF from a .jar file before
 * the virtual machine has started (so KVM/JNI facilities cannot be
 * used at that point).
 *
 * It understands the ZIP local file headers, the central directory
 * and both "stored" and "deflate" (raw DEFLATE as in ZIP) entries.
 *
 * This file is portable C89; it compiles for the Windows emulator
 * and is also used by the headless test harness on Linux.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*--------------------------------------------------------------------
 * Little-endian read helpers
 *------------------------------------------------------------------*/
static unsigned int
zipU16(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned long
zipU32(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned long)p[3] << 24);
}

/*--------------------------------------------------------------------
 * Raw DEFLATE decoder (RFC 1951), sufficient for ZIP entries.
 *------------------------------------------------------------------*/

#define INFLATE_MAXBITS 15

typedef struct {
    unsigned short count[INFLATE_MAXBITS + 1]; /* codes per length */
    unsigned short offset[INFLATE_MAXBITS + 1]; /* symbol[] start per length */
    unsigned short symbol[320];
} HuffmanCode;

typedef struct {
    const unsigned char *in;
    unsigned long inPos;
    unsigned long inLen;
    unsigned long bitBuf;
    int bitCnt;

    unsigned char *out;
    unsigned long outPos;
    unsigned long outLen;

    int fail;
} InflateCtx;

static int
getBit(InflateCtx *c) {
    if (c->bitCnt == 0) {
        if (c->inPos >= c->inLen) {
            c->fail = 1;
            return 0;
        }
        c->bitBuf = c->in[c->inPos++];
        c->bitCnt = 8;
    }
    c->bitCnt--;
    {
        int b = (int)(c->bitBuf & 1);
        c->bitBuf >>= 1;
        return b;
    }
}

static int
getBits(InflateCtx *c, int n) {
    int v = 0;
    int i;
    for (i = 0; i < n; i++) {
        v |= getBit(c) << i;
    }
    return v;
}

/*
 * Build a canonical Huffman decoding table from code lengths.
 * Returns 0 on success, -1 if the code is invalid.
 */
static int
makeCode(HuffmanCode *code, const unsigned char *lengths, int count) {
    int bits, sym;
    int left;

    memset(code, 0, sizeof(*code));
    for (sym = 0; sym < count; sym++) {
        code->count[lengths[sym]]++;
    }
    if (code->count[0] == count) {
        /* no codes at all: valid (unused code) */
        return 0;
    }

    /* check for over-subscription */
    left = 1;
    for (bits = 1; bits <= INFLATE_MAXBITS; bits++) {
        left <<= 1;
        left -= code->count[bits];
        if (left < 0) {
            return -1;
        }
    }
    if (left > 0 && count < 2) {
        /* single-symbol code: DEFLATE allows this (length 1) */
        if (count != 1) {
            return -1;
        }
    }

    /* assign canonical symbols, grouped by code length */
    {
        unsigned short nextFree[INFLATE_MAXBITS + 1];
        int offset = 0;
        for (bits = 1; bits <= INFLATE_MAXBITS; bits++) {
            code->offset[bits] = (unsigned short)offset;
            nextFree[bits] = 0;
            offset += code->count[bits];
        }
        for (sym = 0; sym < count; sym++) {
            int len = lengths[sym];
            if (len != 0) {
                code->symbol[code->offset[len] + nextFree[len]++] =
                    (unsigned short)sym;
            }
        }
    }
    return 0;
}

/*
 * Decode one symbol from the stream using a canonical table.
 */
static int
decodeSymbol(InflateCtx *c, const HuffmanCode *code) {
    int codeValue = 0;
    int first = 0;
    int index = 0;
    int bits;

    for (bits = 1; bits <= INFLATE_MAXBITS; bits++) {
        codeValue |= getBit(c);
        if (c->fail) {
            return -1;
        }
        if (codeValue - first < code->count[bits]) {
            return code->symbol[index + (codeValue - first)];
        }
        index += code->count[bits];
        first += code->count[bits];
        first <<= 1;
        codeValue <<= 1;
    }
    c->fail = 1;
    return -1;
}

static int
inflateBlock(InflateCtx *c) {
    static const unsigned char clclOrder[19] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
        11, 4, 12, 3, 13, 2, 14, 1, 15
    };
    int last;
    for (;;) {
        last = getBit(c);
        if (c->fail) {
            return -1;
        }
        {
            int type = getBits(c, 2);
            if (c->fail) {
                return -1;
            }
            if (type == 0) {
                /* stored block */
                int discard = c->bitCnt;
                (void)discard;
                c->bitCnt = 0;
                c->bitBuf = 0;
                if (c->inPos + 4 > c->inLen) {
                    c->fail = 1;
                    return -1;
                }
                {
                    unsigned long len = (unsigned int)c->in[c->inPos] |
                                        ((unsigned int)c->in[c->inPos + 1] << 8);
                    unsigned long nlen = (unsigned int)c->in[c->inPos + 2] |
                                         ((unsigned int)c->in[c->inPos + 3] << 8);
                    int i;
                    c->inPos += 4;
                    if ((len ^ 0xFFFF) != nlen) {
                        c->fail = 1;
                        return -1;
                    }
                    if (c->inPos + len > c->inLen) {
                        c->fail = 1;
                        return -1;
                    }
                    for (i = 0; i < (int)len; i++) {
                        if (c->outPos < c->outLen) {
                            c->out[c->outPos++] = c->in[c->inPos++];
                        }
                    }
                }
            } else {
                HuffmanCode litLen;
                HuffmanCode dist;
                const unsigned char *litLens;
                const unsigned char *distLens;
                int litCount, distCount;
                if (type == 1) {
                    /* fixed Huffman codes */
                    int i;
                    static unsigned char l_lengths[288];
                    static unsigned char d_lengths[30];
                    static int init = 0;
                    if (!init) {
                        for (i = 0; i < 144; i++) l_lengths[i] = 8;
                        for (i = 144; i < 256; i++) l_lengths[i] = 9;
                        for (i = 256; i < 280; i++) l_lengths[i] = 7;
                        for (i = 280; i < 288; i++) l_lengths[i] = 8;
                        for (i = 0; i < 30; i++) d_lengths[i] = 5;
                        init = 1;
                    }
                    litLens = l_lengths;
                    distLens = d_lengths;
                    litCount = 288;
                    distCount = 30;
                } else {
                    /* dynamic Huffman codes */
                    static unsigned char d_lengths[288];
                    unsigned char dlens[19];
                    int hlit, hdist, hclen;
                    int i, sym;
                    HuffmanCode codeLen;
                    hlit = getBits(c, 5) + 257;
                    hdist = getBits(c, 5) + 1;
                    hclen = getBits(c, 4) + 4;
                    if (c->fail || hlit > 286 || hdist > 30) {
                        c->fail = 1;
                        return -1;
                    }
                    for (i = 0; i < 19; i++) {
                        dlens[i] = 0;
                    }
                    for (i = 0; i < hclen; i++) {
                        dlens[clclOrder[i]] = (unsigned char)getBits(c, 3);
                    }
                    if (c->fail) {
                        return -1;
                    }
                    if (makeCode(&codeLen, dlens, 19) != 0) {
                        c->fail = 1;
                        return -1;
                    }
                    i = 0;
                    while (i < hlit + hdist) {
                                                sym = decodeSymbol(c, &codeLen);
                        if (c->fail || sym < 0) {
                            c->fail = 1;
                            return -1;
                        }
                        if (sym < 16) {
                            d_lengths[i++] = (unsigned char)sym;
                        } else {
                            int repeatLen = 0;
                            int repeatVal = 0;
                            if (sym == 16) {
                                if (i == 0) {
                                    c->fail = 1;
                                    return -1;
                                }
                                repeatVal = d_lengths[i - 1];
                                repeatLen = getBits(c, 2) + 3;
                            } else if (sym == 17) {
                                repeatLen = getBits(c, 3) + 3;
                            } else { /* 18 */
                                repeatLen = getBits(c, 7) + 11;
                            }
                            if (c->fail || i + repeatLen > hlit + hdist) {
                                c->fail = 1;
                                return -1;
                            }
                            while (repeatLen-- > 0) {
                                d_lengths[i++] = (unsigned char)repeatVal;
                            }
                        }
                    }
                    litLens = d_lengths;
                    distLens = d_lengths + hlit;
                    litCount = hlit;
                    distCount = hdist;
                }
                if (makeCode(&litLen, litLens, litCount) != 0 ||
                    makeCode(&dist, distLens, distCount) != 0) {
                    c->fail = 1;
                    return -1;
                }
                for (;;) {
                    int sym = decodeSymbol(c, &litLen);
                    if (c->fail || sym < 0) {
                        return -1;
                    }
                    if (sym < 256) {
                        if (c->outPos < c->outLen) {
                            c->out[c->outPos++] = (unsigned char)sym;
                        }
                    } else if (sym == 256) {
                        break;
                    } else {
                        static const int
                            lenBase[29] = {
                            3, 4, 5, 6, 7, 8, 9, 10,
                            11, 13, 15, 17, 19, 23, 27, 31,
                            35, 43, 51, 59, 67, 83, 99, 115,
                            131, 163, 195, 227, 258
                        };
                        static const unsigned char
                            lenExtra[29] = {
                            0, 0, 0, 0, 0, 0, 0, 0,
                            1, 1, 1, 1, 2, 2, 2, 2,
                            3, 3, 3, 3, 4, 4, 4, 4,
                            5, 5, 5, 5, 0
                        };
                        static const int
                            distBase[30] = {
                            1, 2, 3, 4, 5, 7, 9, 13,
                            17, 25, 33, 49, 65, 97, 129, 193,
                            257, 385, 513, 769, 1025, 1537, 2049,
                            3073, 4097, 6145, 8193, 12289, 16385,
                            24577
                        };
                        static const unsigned char
                            distExtra[30] = {
                            0, 0, 0, 0, 1, 1, 2, 2,
                            3, 3, 4, 4, 5, 5, 6, 6,
                            7, 7, 8, 8, 9, 9, 10, 10,
                            11, 11, 12, 12, 13, 13
                        };
                        int li = sym - 257;
                        int di;
                        unsigned long len;
                        unsigned long distance;
                        if (li >= 29) {
                            c->fail = 1;
                            return -1;
                        }
                        len = lenBase[li] + getBits(c, lenExtra[li]);
                        di = decodeSymbol(c, &dist);
                        if (c->fail || di < 0 || di >= 30) {
                            return -1;
                        }
                        distance = distBase[di] + getBits(c, distExtra[di]);
                        if (c->fail || distance > c->outPos) {
                            c->fail = 1;
                            return -1;
                        }
                        while (len-- > 0) {
                            unsigned char ch =
                                c->out[c->outPos - distance];
                            if (c->outPos < c->outLen) {
                                c->out[c->outPos++] = ch;
                            }
                        }
                    }
                }
            }
        }
        if (last) {
            break;
        }
    }
    return 0;
}

int
midpInflateData(const unsigned char *in, unsigned long inLen,
                unsigned char *out, unsigned long outLen) {
    InflateCtx c;
    int rc;
    memset(&c, 0, sizeof(c));
    c.in = in;
    c.inLen = inLen;
    c.out = out;
    c.outLen = outLen;
    rc = inflateBlock(&c);
    return (rc == 0 && !c.fail) ? 1 : 0;
}

/*--------------------------------------------------------------------
 * ZIP (central directory) access
 *------------------------------------------------------------------*/

typedef struct {
    char name[1024];
    unsigned short method;
    unsigned long compLen;
    unsigned long uncompLen;
    unsigned long localOffset;
} ZipEntry;

/*
 * Load the whole file into memory (jar manifests are small; and we
 * only use this for reading one entry from a suite jar).
 */
static unsigned char *
zipReadFile(const char *path, unsigned long *sizeOut) {
    FILE *f = fopen(path, "rb");
    unsigned char *buf;
    long size;
    if (f == NULL) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    size = ftell(f);
    if (size < 0 || size > (1024L * 1024L * 512L)) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    buf = (unsigned char *)malloc((size_t)size + 8);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    /* inflate wants up to 4 extra bytes readable past the data */
    memset(buf + size, 0, 8);
    *sizeOut = (unsigned long)size;
    return buf;
}

/*
 * Find and parse the central directory. Returns number of entries
 * (up to maxEntries), or -1 on error.
 */
static int
zipOpenCentral(const unsigned char *buf, unsigned long size,
               ZipEntry *entries, int maxEntries) {
    long i;
    unsigned long eocd;
    int count;
    const unsigned char *p;

    if (size < 22) {
        return -1;
    }
    /* scan backwards for the end of central directory record */
    eocd = (unsigned long)size;
    for (i = (long)size - 22; i >= (long)(size - 65557) && i >= 0; i--) {
        if (buf[i] == 0x50 && buf[i + 1] == 0x4B &&
            buf[i + 2] == 0x05 && buf[i + 3] == 0x06) {
            eocd = (unsigned long)i;
            break;
        }
    }
    if (eocd >= (unsigned long)size - 4) {
        return -1;
    }
    count = (int)zipU16(buf + eocd + 10);
    p = buf + zipU32(buf + eocd + 16); /* offset of central directory */
    for (i = 0; i < count; i++) {
        unsigned long nameLen, extraLen, cmtLen;
        if (p[0] != 0x50 || p[1] != 0x4B ||
            p[2] != 0x01 || p[3] != 0x02) {
            return -1;
        }
        nameLen = zipU16(p + 28);
        extraLen = zipU16(p + 30);
        cmtLen = zipU16(p + 32);
        if (i < maxEntries) {
            unsigned long n = nameLen;
            if (n > sizeof(entries[i].name) - 1) {
                n = sizeof(entries[i].name) - 1;
            }
            memcpy(entries[i].name, p + 46, n);
            entries[i].name[n] = '\0';
            entries[i].method = (unsigned short)zipU16(p + 10);
            entries[i].compLen = zipU32(p + 20);
            entries[i].uncompLen = zipU32(p + 24);
            entries[i].localOffset = zipU32(p + 42);
        }
        p += 46 + nameLen + extraLen + cmtLen;
    }
    return count;
}

/*
 * Extract one entry (by name) from the given buffer. The result is
 * malloc'ed; the caller frees it. Returns 0 on success.
 */
static int
zipExtractEntry(const unsigned char *buf, unsigned long size,
                int entryCount, ZipEntry *entries,
                const char *name,
                unsigned char **dataOut, unsigned long *sizeOut) {
    int i;
    for (i = 0; i < entryCount; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            const unsigned char *lh;
            unsigned long nameLen, extraLen;
            unsigned char *out;
            int ok;

            lh = buf + entries[i].localOffset;
            if (lh[0] != 0x50 || lh[1] != 0x4B ||
                lh[2] != 0x03 || lh[3] != 0x04) {
                return -1;
            }
            nameLen = zipU16(lh + 26);
            extraLen = zipU16(lh + 28);
            lh += 30 + nameLen + extraLen;

            out = (unsigned char *)malloc(
                (size_t)(entries[i].uncompLen ? entries[i].uncompLen : 1) + 8);
            if (out == NULL) {
                return -1;
            }
            if (entries[i].method == 0) {
                /* stored */
                memcpy(out, lh, (size_t)entries[i].uncompLen);
                ok = 1;
            } else if (entries[i].method == 8) {
                /* deflate */
                ok = midpInflateData(lh, entries[i].compLen, out,
                                     entries[i].uncompLen);
            } else {
                free(out);
                return -1;
            }
            if (!ok) {
                free(out);
                return -1;
            }
            *dataOut = out;
            *sizeOut = entries[i].uncompLen;
            return 0;
        }
    }
    return -1;
}

/*
 * Public helper: read one entry from a jar/zip file.
 * Returns 0 on success; *dataOut is malloc'ed.
 */
int
midpZipReadEntry(const char *jarPath, const char *entryName,
                 unsigned char **dataOut, unsigned long *sizeOut) {
    static ZipEntry entries[1024];
    unsigned char *buf;
    unsigned long size;
    int count;
    int rc;

    buf = zipReadFile(jarPath, &size);
    if (buf == NULL) {
        return -1;
    }
    count = zipOpenCentral(buf, size, entries, 1024);
    if (count < 0) {
        free(buf);
        return -1;
    }
    rc = zipExtractEntry(buf, size, count, entries, entryName,
                         dataOut, sizeOut);
    free(buf);
    return rc;
}
