/*
 * @(#)dump_offsets.c	1.3 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/* 
 * A simple program to generate C header files which contain
 * field offsets for KVM.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "classfile.h"

#ifdef NEED_GETOPT
#define getopt getOpt
static char *optarg = NULL;
static int optind = 1;
static int opterr = 1;
static int optopt = '?';

/*
 * Simple version of getopt
 */
static int
getOpt(int argc, char *argv[], char *options)
{
    static char *opt = NULL;

    if (optind >= argc) return EOF;
    if (strcmp(argv[optind], "--") == 0) return EOF;
    if (argv[optind][0] != '-') return '?';

    if (opt == NULL) {
        opt = argv[optind] + 1;
    }

    for (optopt = *opt++; *options; ++options) {
        if (*options == ':') continue;
        if (*options == optopt) {
            if (options[1] == ':') {
                if (*opt) {
                    optarg = opt;
                } else {
                    ++optind;
                    optarg = argv[optind];
                }
                opt = NULL;
                ++optind;
            } else {
                optarg = NULL;

                if (*opt == 0) {
                    opt = NULL;
                    ++optind;
                }
            }

            return optopt;
        }
    }

    if (opterr) {
        fprintf(stderr, "Unrecognized option %c\n", optopt);
        ++optind;
    }

    return '?';
}
#else
/* for use with getopt */
extern char *optarg;
extern int optind, opterr, optopt;
#endif

static char *spacer = "                                                  ";
static char pathsep = ';';
static char include_subclasses = 0;
static char package_prefix_levels = 0;

static int
isLongVariable(ClassFile *cf, field_info *fi)
{
    cp_info *c = cf->constant_pool + fi->descriptor_index;
    return (   (c->Utf8_info.bytes[0] == 'J')
            || (c->Utf8_info.bytes[0] == 'D')  );
}

static int
processFields(ClassFile *cf, int fieldCnt, FILE *output) {
    int i, j;
    cp_info *c;
    char className[200];
    char *indent;
    int first = 1;

    c = cf->constant_pool + cf->this_class;
    c = cf->constant_pool + c->Class_info.name_index;

    /*
     * Find the start of a (partially qualified) className.  
     * The number of path segments included is controlled 
     * by package_prefix_levels.
     */
    j = package_prefix_levels;
    for (i = c->Utf8_info.length - 1; i >= 0; --i) {
        if (c->Utf8_info.bytes[i] == '/') {
            --j;
            if (j < 0) {
                break;
            }
        }
    }

    /*
     * Copy.  Convert / characters to _ so that C 
     * preprocessor is happy.
     */
    for (j = 0, ++i; i < c->Utf8_info.length; ++i, ++j) {
        className[j] = c->Utf8_info.bytes[i];
        if (className[j] == '/') className[j] = '_';
    }
    className[j] = 0;

    if (output) {
        indent = spacer + strlen(className);
    }

    for (i = 0; i < cf->fields_count; ++i) {
        field_info *fi = cf->fields + i;

        if (fi->access_flags & ACC_STATIC) {
            continue;
        }

        if (output) {
            c = cf->constant_pool + fi->name_index;

            if (first) {
                fprintf(output, "#ifndef %s_", className);
                for (j = 0; j < c->Utf8_info.length; ++j) {
                    putc(c->Utf8_info.bytes[j], output);
                }
                fprintf(output, "\n");
                first = 0;
            }

            fprintf(output, "#define %s_", className);
            for (j = 0; j < c->Utf8_info.length; ++j) {
                putc(c->Utf8_info.bytes[j], output);
            }
            fprintf(output, "%s %d\n", 
                   indent + c->Utf8_info.length - ((fieldCnt < 10) ? 1 : 0),
                   fieldCnt);
        }

        ++fieldCnt;
        if (isLongVariable(cf, fi)) {
            ++fieldCnt;
        }
    }

    if (output && !first) {
        fprintf(output, "#endif\n\n", className);
    }

    return fieldCnt;
}


static ClassFile *
findClass(char *classpath, char *classname)
{
    char buf[256];
    char *dirStart = classpath;
    char *dirEnd;

    do {
        int len;
        FILE *f;

        /* extract a single directory from the path */
        dirEnd = strchr(dirStart, pathsep);
        if ((dirEnd == dirStart) || (dirEnd == NULL)) {
            len = strlen(dirStart);
            dirEnd = NULL;
        } else {
            len = dirEnd - dirStart;
        }

        memcpy(buf, dirStart, len);
        sprintf(buf + len, "/%s.class", classname);

        f = fopen(buf, "rb");
        if (f != NULL) {
            ClassFile *cf = readClass(f);
            fclose(f);
            return cf;
        }

        dirStart = dirEnd + 1;
    } while (dirEnd != NULL);

    return NULL;
}

static char *
copyUtf8(unsigned char *utf8, int len)
{
    static char buf[1024];
    int i;

    /* REMIND: to be safe, check for len < 1024 */
    for (i = 0; i < len; ++i) {
        /* REMIND: to be safe, check for utf8[i] < 0x80? */
        buf[i] = (char)utf8[i];
    }
    buf[i] = 0;

    return buf;
}

static char *
convertPeriodToSlash(char *s) 
{
    static char buf[256];
    char *d = buf;

    while (*s) {
        *d++ = (*s == '.') ? '/' : *s;
        ++s;
    }
    *d = 0;

    return buf;
}

static char *usage = 
"usage: %s [-c path] [-i] [-o file] [-p number] [-s char] class\n";

int main(int argc, char *argv[]) {
    char *classname;
    char *classpath;
    char *cname;
    ClassFile *classes[16];
    int classdepth, fieldCnt;
    int opt;
    FILE *outFile = stdout;

    classpath = getenv("CLASSPATH");

    while (opt = getopt(argc, argv, "c:io:p:s:")) {
        switch (opt) {
        case 'c':
            classpath = optarg;
            break;
        case 'i':
            include_subclasses = 1;
            break;
        case 'o':
            outFile = fopen(optarg, "w");
            break;
        case 'p':
            package_prefix_levels = atoi(optarg);
            break;
        case 's':
            pathsep = optarg[0];
            break;
        default:
            if (optind == (argc - 1)) {
                goto done;
            } else {
                fprintf(stderr, usage, argv[0]);
                fprintf(stderr, "   -c: set classpath\n");
                fprintf(stderr, "   -i: include subclass fields\n");
                fprintf(stderr, "   -o: specify output filename\n");
                fprintf(stderr, "   -p: set package prefix levels\n");
                fprintf(stderr, "   -s: set path separator char\n");
                return 1;
            }
        }
    }    

 done:

    classname = convertPeriodToSlash(argv[argc - 1]);

    if ((classes[0] = findClass(classpath, classname)) == NULL) {
        fprintf(stderr, "couldn't find class %s\n", classname);
        return 2;
    }

    classdepth = 1;
    while (classes[classdepth - 1]->super_class) {
        cp_info *base = classes[classdepth - 1]->constant_pool;
        cp_info *ci = base + classes[classdepth - 1]->super_class;

        ci = base + ci->Class_info.name_index;

        cname = copyUtf8(ci->Utf8_info.bytes, ci->Utf8_info.length);
        if ((classes[classdepth] =  findClass(classpath, cname)) == NULL) {
            fprintf(stderr, "couldn't find superclass %s\n", cname);
            return 3;
        } else {
            ++classdepth;
        }
    }


    fprintf(outFile, "/* THIS FILE IS AUTOMATICALLY GENERATED */\n\n");

    fieldCnt = 0;
    while (--classdepth >= 0) {
        FILE *oFile = 
            include_subclasses || (classdepth == 0) ? outFile : NULL;

        fieldCnt = processFields(classes[classdepth], fieldCnt, oFile);
    }

    fclose(outFile);

    return 0;
}
