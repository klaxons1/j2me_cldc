/*
 * @(#)classfile.h	1.2 02/07/25 @(#)
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
#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_SUPER        0x0020
#define ACC_VOLATILE     0x0040
#define ACC_TRANSIENT    0x0080
#define ACC_NATIVE       0x0100
#define ACC_INTERFACE    0x0200
#define ACC_ABSTRACT     0x0400
#define ACC_STRICT       0x0800

#define CONSTANT_Class              7
#define CONSTANT_Fieldref           9
#define CONSTANT_Methodref          10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String             8
#define CONSTANT_Integer            3
#define CONSTANT_Float              4
#define CONSTANT_Long               5
#define CONSTANT_Double             6
#define CONSTANT_NameAndType        12
#define CONSTANT_Utf8               1

typedef unsigned long  u4;
typedef unsigned short u2;
typedef unsigned char  u1;

typedef union {
    struct {
        u1 tag;
        u2 name_index;
    } Class_info;
    struct {
        u1 tag;
        u2 class_index;
        u2 name_and_type_index;
    } Fieldref_info;
    struct {
        u1 tag;
        u2 class_index;
        u2 name_and_type_index;
    } Methodref_info;
    struct {
        u1 tag;
        u2 class_index;
        u2 name_and_type_index;
    } InterfaceMethodref_info;
    struct {
        u1 tag;
        u2 string_index;
    } String_info;
    struct {
        u1 tag;
        u4 bytes;
    } Integer_info;
    struct {
        u1 tag;
        u4 bytes;
    } Float_info;
    struct {
        u1 tag;
        u4 high_bytes;
        u4 low_bytes;
    } Long_info;
    struct {
        u1 tag;
        u4 high_bytes;
        u4 low_bytes;
    } Double_info;
    struct {
        u1 tag;
        u2 name_index;
        u2 descriptor_index;
    } NameAndType_info;
    struct {
        u1 tag;
        u2 length;
        u1 *bytes;
    } Utf8_info;
} cp_info;

struct exception {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
};

struct inner_class {
    u2 inner_class_info_index;
    u2 outer_class_info_index;
    u2 inner_name_index;
    u2 inner_class_access_flags;
};

struct line_number {
    u2 start_pc;
    u2 line_number;
};

struct local_variable {
    u2 start_pc;
    u2 length;
    u2 name_index;
    u2 descriptor_index;
    u2 index;
};

typedef struct _att_info {
    u2 attribute_name_index;
    u4 attribute_length;
    u1 *info;
} attribute_info;

#ifdef NOTYET
typedef union _known_attribute {
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 constantvalue_index;
    } ConstantValue_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 max_stack;
        u2 max_locals;
        u4 code_length;
        u1 *code;
        u2 exception_table_length;
        struct exception *exception_table;
        u2 attributes_count;
        attribute_info *attributes;
    } Code_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 number_of_exceptions;
        u2 *exception_index_table;
    } Exceptions_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 number_of_classes;
        struct inner_class *classes;
    } InnerClasses_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
    } Synthetic_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 sourcefile_index;
    } Sourcefile_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 line_number_table_length;
        struct line_number *line_number_table;
    } LineNumberTable_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
        u2 local_variable_table_length;
        struct local_variable *local_variable_table;
    } LocalVariableTable_attribute;
    struct {
        u2 attribute_name_index;
        u4 attribute_length;
    } Deprecated_attribute;
} known_attribute;
#endif

typedef struct _fm_info {
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
    attribute_info *attributes;
} field_info, method_info;

typedef struct _ClassFile {
    u4 magic;
    u2 minor_version;
    u2 major_version;
    u2 constant_pool_count;
    cp_info *constant_pool;
    u2 access_flags;
    u2 this_class;
    u2 super_class;
    u2 interfaces_count;
    u2 *interfaces;
    u2 fields_count;
    field_info *fields;
    u2 methods_count;
    method_info *methods;
    u2 attributes_count;
    attribute_info *attributes;
} ClassFile;

extern ClassFile *readClass(FILE *f);
