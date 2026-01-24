/*
 * @(#)classfile.c	1.2 02/07/25 @(#)
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * Simple representation of a single class file.  Does not (currently)
 * include support for the defined attributes, but represents them as
 * unformatted bytes instead.
 */
#include <stdio.h>
#include <stdlib.h>
#include "classfile.h"

static u1 read1(FILE *f);
static u2 read2(FILE *f);
static u4 read4(FILE *f);
static cp_info *readConstantPool(int cnt, FILE *f);
static u2 *readInterfaces(int cnt, FILE *f);
static attribute_info * readAttributes(int cnt, FILE *f);
static field_info *readFieldsAndMethods(int cnt, FILE *f);

/*
 * Read a class into memory from an open file pointer.
 */
ClassFile *readClass(FILE *f)
{
    ClassFile *cf;
    u4 magic = read4(f);

    if (magic != 0xCAFEBABE) {
        return NULL;
    }

    cf = malloc(sizeof(ClassFile));

    cf->magic               = magic;
    cf->minor_version       = read2(f);
    cf->major_version       = read2(f);
    cf->constant_pool_count = read2(f);
    cf->constant_pool       = readConstantPool(cf->constant_pool_count, f);
    cf->access_flags        = read2(f);
    cf->this_class          = read2(f);
    cf->super_class         = read2(f);
    cf->interfaces_count    = read2(f);
    cf->interfaces          = readInterfaces(cf->interfaces_count, f);
    cf->fields_count        = read2(f);
    cf->fields              = readFieldsAndMethods(cf->fields_count, f);
    cf->methods_count       = read2(f);
    cf->methods             = readFieldsAndMethods(cf->methods_count, f);
    cf->attributes_count    = read2(f);
    cf->attributes          = readAttributes(cf->attributes_count, f);

    return cf;
}

static u1 read1(FILE *f)
{
    return (u1) (getc(f) & 0xff);
}

static u2 read2(FILE *f)
{
    u2 hi = (u2) read1(f);
    u2 lo = (u2) read1(f);
    return (hi << 8) | lo;
}

static u4 read4(FILE *f)
{
    u4 hi = (u4) read2(f);
    u4 lo = (u4) read2(f);
    return (hi << 16) | lo;
}

static cp_info *readConstantPool(int cnt, FILE *f)
{
    if (cnt == 0) {
        return NULL;
    } else {
        cp_info *cp = malloc(sizeof(cp_info) * cnt);
        cp_info *this = cp + 1; /* zeroth constant unused */
        int nbytes;

        while (--cnt > 0) {
            u1 tag = read1(f);
            switch (tag) {
            case CONSTANT_Class:
                this->Class_info.tag = tag;
                this->Class_info.name_index = read2(f);
                break;

            case CONSTANT_Fieldref:
            case CONSTANT_Methodref:
            case CONSTANT_InterfaceMethodref:
                this->Fieldref_info.tag = tag;
                this->Fieldref_info.class_index = read2(f);
                this->Fieldref_info.name_and_type_index = read2(f);
                break;

            case CONSTANT_String:
                this->String_info.tag = tag;
                this->String_info.string_index = read2(f);
                break;

            case CONSTANT_Integer:
            case CONSTANT_Float:
                this->Integer_info.tag = tag;
                this->Integer_info.bytes = read4(f);
                break;

            case CONSTANT_Long:
            case CONSTANT_Double:
                this->Long_info.tag = tag;
                this->Long_info.high_bytes = read4(f);
                this->Long_info.low_bytes = read4(f);

                ++this;         /* these take two slots.  Sigh. */
                --cnt;
                break;

            case CONSTANT_NameAndType:
                this->NameAndType_info.tag = tag;
                this->NameAndType_info.name_index = read2(f);
                this->NameAndType_info.descriptor_index = read2(f);
                break;

            case CONSTANT_Utf8:
                this->Utf8_info.tag = tag;
                this->Utf8_info.length = read2(f);
                if (this->Utf8_info.length == 0) {
                  this->Utf8_info.bytes = NULL;
                } else {
                  this->Utf8_info.bytes = malloc(this->Utf8_info.length);
                  /* REMIND: theoretically we could get a short read! */
                  nbytes = fread(this->Utf8_info.bytes, 1,
                                 this->Utf8_info.length, f);
                }
                break;

            default:
                fprintf(stderr, "Bad constant tag value %d\n", tag);
            }

            ++this;
        }

        return cp;
    }
}

static u2 *
readInterfaces(int cnt, FILE *f)
{
    if (cnt == 0) {
        return NULL;
    } else {
        u2 *data = malloc(sizeof(u2) * cnt);
        u2 *this = data;
        while (--cnt >= 0) {
            *this++ = read2(f);
        }

        return data;
    }
}

static attribute_info *
readAttributes(int cnt, FILE *f)
{
    if (cnt == 0) {
        return NULL;
    } else {
        attribute_info *ai = malloc(sizeof(attribute_info) * cnt);
        attribute_info *this = ai;

        while (--cnt >= 0) {
            this->attribute_name_index = read2(f);
            this->attribute_length = read4(f);
            this->info = malloc(this->attribute_length);
            /* REMIND: beware of short read! */
            fread(this->info, 1, this->attribute_length, f);

            ++this;
        }

        return ai;
    }
}

static field_info *
readFieldsAndMethods(int cnt, FILE *f)
{
    if (cnt == 0) {
        return NULL;
    } else {
        field_info *fi = malloc(sizeof(field_info) * cnt);
        field_info *this = fi;

        while (--cnt >= 0) {
            this->access_flags = read2(f);
            this->name_index = read2(f);
            this->descriptor_index = read2(f);
            this->attributes_count = read2(f);
            this->attributes = readAttributes(this->attributes_count, f);

            ++this;
        }

        return fi;
    }
}
