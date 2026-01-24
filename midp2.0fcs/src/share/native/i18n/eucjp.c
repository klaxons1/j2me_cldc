/*
 * @(#)eucjp.c	1.7 02/07/24 @(#)
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

#include <kni.h>
#include "conv.h"

/*
 * This EUC converter utilizes the SJIS converter defined in sjis.c
 * since the conversion from SJIS to EUC and vice versa can be done
 * mathematically.  Note that this does not support the CS3 range.  
 */

extern LcConvMethodsRec SJISConvRec;
static LcConvMethods sjis = &SJISConvRec;

static int
EUCJPMaxLen(void)
{
    return 2;
}

static int
EUCJPLen(const unsigned char *c, int len)
{
    if (*c < 0x7f) {
        return 1;
    } 
    return 2;
}


static void
sjis2euc(unsigned char *h, unsigned char *l)
{
    unsigned char hib = *h;
    unsigned char lob = *l;
    
    hib -= (hib <= 0x9f) ? 0x71 : 0xb1;
    hib = (hib << 1) + 1;
    if (lob >= 0x9e) {
        lob -= 0x7e;
        hib++;
    } else if (lob > 0x7f) {
        lob -= 0x20;
    } else {
        lob -= 0x1f;
    }

    *h = hib | 0x80;
    *l = lob | 0x80;
}

static int
unicodeToEUCJP(const unicode *ustr, int ulen, unsigned char *estr, int elen)
{
    int i = 0, cnt = 0;

    for (; i < ulen && cnt < elen; i++) {
        unicode uc = ustr[i];

        if (0x00a5 == uc) {
            estr[cnt++] = 0x5c;
        } else if (0x203e == uc) {
            estr[cnt++] = 0x7e;
        } else if (uc <= 0x007f) {
            estr[cnt++] = (uc & 0xff);
        } else if ((0xff61 <= uc) && (uc <= 0xff9f)) {
            estr[cnt++] = 0x8e;
            estr[cnt++] = ((0x00a1 + uc - 0xff61) & 0xff);
        } else {
            unsigned char buf[2];
            int len = sjis->unicodeToNative(&uc, 1, buf, 2);
            if (len == 2) {
                sjis2euc(&buf[0], &buf[1]);
                estr[cnt++] = buf[0];
                estr[cnt++] = buf[1];
            } else {
                estr[cnt++] = '?';
            }
        }
    }

    return cnt;
}

static void
euc2sjis(unsigned char *h, unsigned char *l)
{
    unsigned char hib = (*h & ~0x80);
    unsigned char lob = (*l & ~0x80);

    lob += (hib & 1) ? 0x1f : 0x7d;
    if (lob >= 0x7f) lob++;
    hib = ((hib - 0x21) >> 1) + 0x81;
    if (hib > 0x9f) hib += 0x40;

    *h = hib;
    *l = lob;
}

static int
EUCJPToUnicode(const unsigned char *estr, int elen, unicode *ustr, int ulen)
{
    int i = 0, cnt = 0;

    for (; i < elen && cnt < ulen; i++) {
        unsigned char sc = estr[i];
        if (sc <= 0x7f) {
            ustr[cnt] = (unicode) sc;
        } else if (sc == 0x8e) {
            unsigned char sc2;
            i++;
            if (i == elen) {
                break;
            }
            sc2 = estr[i];
            if (((0xa0 <= sc2) && (sc2 <= 0xff))) {
                ustr[cnt] = 0xff61 + sc2 - 0x00a1;
            } else {
                ustr[cnt] = (unicode) 0xfffd;
            }
        } else {
            unsigned char sc2;
            i++;
            if (i == elen) {
                break;
            }
            sc2 = estr[i];
            if ((0xa0 <= sc) && (sc <= 0xff)) {
                if ((0xa0 <= sc2) && (sc2 <= 0xff)) {
                    unsigned char buf[2];
                    unicode uc;
                    int len;
                    euc2sjis(&sc, &sc2);
                    buf[0] = sc;
                    buf[1] = sc2;
                    len = sjis->nativeToUnicode(buf, 2, &uc, 1);
                    if (len == 1) {
                        ustr[cnt] = uc;
                    } else {
                        ustr[cnt] = (unicode) 0xfffd;
                    }
                } else {
                    ustr[cnt] = (unicode) 0xfffd;
                }
            } else {
                continue;
            }
        }
        cnt++;
    }

    return cnt;
}

static int
sizeOfEUCJPInUnicode(const unsigned char *b, int offset, int len)
{
    int numChars = 0;

    if (len > 0) {
        int pos = offset;
        int end = offset + len;
        char bPos = 1;
        for (; pos < end; pos++) {
            unsigned char t = (unsigned char) b[pos];
            if (bPos == 2) {
                bPos = 1;
            } else if ((t > 0xa0  &&  t < 0xff)  ||  t == 0x8e) {
                bPos = 2;
                continue;
            }
            numChars++;
        }
    }

    return numChars;
}


static int
sizeOfUnicodeInEUCJP(const unicode *c, int offset, int len)
{
    int numBytes = 0;

    if (len > 0) {
        int pos = offset;
        int end = offset + len;
        for (; pos < end; pos++) {
            unicode u = (unicode) c[pos];
            if (u < 0x81  || u == 0x203E) {
                numBytes++;
            } else {
                numBytes += 2;
            }
        }
    }

    return numBytes;
}

LcConvMethodsRec EUCJPConvRec = {
    "EUC_JP",
    EUCJPMaxLen,
    EUCJPLen,
    unicodeToEUCJP,
    EUCJPToUnicode,
    sizeOfEUCJPInUnicode,
    sizeOfUnicodeInEUCJP
};
