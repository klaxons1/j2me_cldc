#!/usr/bin/env python3
#
# gen_native_table.py
#
# Generates nativeFunctionTable.c for the KVM "no-ROM" (classpath) build.
#
# It scans a directory tree of prebuilt .class files (the MIDP 2.0 FCS
# classes) and, for every class that declares native methods, emits a
# NativeImplementationType table plus a ClassNativeImplementationType
# array named `nativeImplementations` -- the exact format expected by
# kvm/VmCommon/src/native.c when compiled with -DROMIZING=0, matching
# the output of the original jcc KVM_NATIVEWriter (see
# tools/jcc/src/runtime/KVM_NATIVEWriter.java).
#
# Naming rules replicated from jcc:
#   * class table name:  className with '/', '.', '$' replaced by '_'
#   * C function name:   Java_ + jni(class) + '_' + jni(method)
#       (+ '__' + jni(params) when the native method name is overloaded
#        among the class' native methods; params = the descriptor
#        text between '(' and ')' without parentheses)
#   * jni(s): per char:
#       - ASCII letter/digit: kept
#       - '/' -> '_',  '_' -> '_1',  ';' -> '_2',  '[' -> '_3'
#       - anything else -> '_' + hex(ord(ch)+0x100000) without the
#         leading '1'
#
# Usage:
#   python3 gen_native_table.py <classes-dir> <output.c> \
#       [--csrc <dir-or-file> ...] [--report-only]
#
# With one or more --csrc arguments the generator verifies that every
# native it emits has a matching `void Java_...` definition in those C
# sources, and prints the natives that are NOT found (candidates to
# implement or to remove from the classpath).

import os
import re
import sys
import struct

ACC_NATIVE = 0x100


def jni_escape(s):
    out = []
    for ch in s:
        o = ord(ch)
        if o <= 0x7F and (ch.isascii() and ch.isalnum()):
            out.append(ch)
        else:
            if ch == '/':
                out.append('_')
            elif ch == '_':
                out.append('_1')
            elif ch == ';':
                out.append('_2')
            elif ch == '[':
                out.append('_3')
            else:
                out.append('_' + format(o + 0x100000, 'x')[1:])
    return ''.join(out)


def class_table_name(class_name):
    return class_name.replace('/', '_').replace('.', '_').replace('$', '_')


class ClassFile:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def u1(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def u2(self):
        v = struct.unpack_from('>H', self.data, self.pos)[0]
        self.pos += 2
        return v

    def u4(self):
        v = struct.unpack_from('>I', self.data, self.pos)[0]
        self.pos += 4
        return v


def parse_class(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] != b'\xca\xfe\xba\xbe':
        return None
    cf = ClassFile(data)
    cf.pos = 4
    cf.u2()  # minor
    cf.u2()  # major
    cp_count = cf.u2()
    cp = [None] * cp_count
    i = 1
    while i < cp_count:
        tag = cf.u1()
        if tag == 1:
            ln = cf.u2()
            cp[i] = ('utf8', data[cf.pos:cf.pos + ln].decode('utf-8', 'replace'))
            cf.pos += ln
        elif tag in (7, 8, 16):
            cp[i] = (tag, cf.u2())
        elif tag in (3, 4, 9, 10, 11, 12):
            cf.u4()
        elif tag in (5, 6):
            cf.u4()
            cf.u4()
            i += 1
        elif tag == 15:
            cf.u1()
            cf.u2()
        elif tag == 17:
            cf.u4()
        else:
            raise ValueError('bad constant pool tag %d in %s' % (tag, path))
        i += 1

    cf.u2()  # access_flags
    this_class = cf.u2()
    # class name
    _, idx = cp[this_class]
    _, name = cp[idx]

    cf.u2()  # super_class
    ifc = cf.u2()
    cf.pos += 2 * ifc

    fcount = cf.u2()
    for _ in range(fcount):
        cf.u2()
        cf.u2()
        cf.u2()
        ac = cf.u2()
        for _ in range(ac):
            cf.u2()          # attribute name
            alen = cf.u4()   # attribute length
            cf.pos += alen

    mcount = cf.u2()
    methods = []
    for _ in range(mcount):
        access = cf.u2()
        name_i = cf.u2()
        desc_i = cf.u2()
        ac = cf.u2()
        for _ in range(ac):
            cf.u2()          # attribute name
            alen = cf.u4()   # attribute length
            cf.pos += alen
        methods.append((access, cp[name_i][1], cp[desc_i][1]))
    return name, methods


def collect_natives(classes_dir):
    """Return {class_name: [(method_name, descriptor), ...]} for classes
    having at least one native method."""
    result = {}
    for root, _dirs, files in os.walk(classes_dir):
        for fn in files:
            if not fn.endswith('.class'):
                continue
            path = os.path.join(root, fn)
            try:
                parsed = parse_class(path)
            except Exception as e:
                print('WARN: failed to parse %s: %s' % (path, e), file=sys.stderr)
                continue
            if parsed is None:
                continue
            name, methods = parsed
            natives = [(mn, d) for (acc, mn, d) in methods if acc & ACC_NATIVE]
            if natives:
                # de-dup (keep first) but keep all overloads
                seen = set()
                uniq = []
                for mn, d in natives:
                    if (mn, d) not in seen:
                        seen.add((mn, d))
                        uniq.append((mn, d))
                result[name] = uniq
    return result


def find_c_symbols(csrc_list):
    """Find all `Java_...` function names defined (or referenced) in the
    given C files or directories. The KVM codebase declares natives with
    macro return types, e.g.

        KNIEXPORT KNI_RETURNTYPE_INT
        Java_com_sun_cldc_i18n_j2me_Conv_byteToChar() {

    so we match any Java_ identifier immediately followed by '('
    (allowing arbitrary whitespace / newlines in between)."""
    syms = set()
    pat = re.compile(r'\b(Java_[A-Za-z0-9_]+)\s*\(')
    files = []
    for entry in csrc_list:
        if os.path.isdir(entry):
            for root, _dirs, fs in os.walk(entry):
                for f in fs:
                    if f.endswith(('.c', '.h')):
                        files.append(os.path.join(root, f))
        else:
            files.append(entry)
    for f in files:
        try:
            with open(f, errors='replace') as fh:
                t = fh.read()
            for m in pat.finditer(t):
                syms.add(m.group(1))
        except OSError:
            pass
    return syms


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        print('usage: gen_native_table.py <classes-dir> <output.c> '
              '[--csrc <dir|file> ...]', file=sys.stderr)
        return 1
    classes_dir = args[0]
    out_path = args[1]
    csrc = []
    if '--csrc' in args:
        i = args.index('--csrc') + 1
        while i < len(args) and not args[i].startswith('--'):
            csrc.append(args[i])
            i += 1

    natives = collect_natives(classes_dir)
    c_syms = find_c_symbols(csrc) if csrc else None

    # order classes the way the writer sees them (sorted for stability)
    classes = sorted(natives)

    missing = []
    if c_syms is not None:
        for cn in classes:
            for mn, d in natives[cn]:
                names = native_names(cn, mn, d, natives[cn])
                if names[0] not in c_syms:
                    missing.append(names[0])

    lines = []
    lines.append('/* This is a generated file.  Do not modify.')
    lines.append(' * Generated by tools/gen_native_table/gen_native_table.py')
    lines.append(' * from the prebuilt MIDP 2.0 FCS class files. */')
    lines.append('')
    lines.append('#include <global.h>')
    lines.append('')
    lines.append('#if !ROMIZING')
    lines.append('')
    # externs
    for cn in classes:
        for mn, d in natives[cn]:
            lines.append('extern void %s(void);' % native_names(cn, mn, d, natives[cn])[0])
    lines.append('')
    lines.append('')
    for cn in classes:
        table = class_table_name(cn) + '_natives'
        lines.append('const NativeImplementationType %s[] = {' % table)
        for mn, d in natives[cn]:
            sig = descriptor_sig(cn, mn, d, natives[cn])
            nm = native_names(cn, mn, d, natives[cn])[0]
            if sig:
                lines.append('\t{ "%s", "%s", %s},' % (mn, sig, nm))
            else:
                lines.append('\t{ "%s", NULL, %s},' % (mn, nm))
        lines.append('\tNATIVE_END_OF_LIST')
        lines.append('};')
        lines.append('')
    lines.append('const ClassNativeImplementationType nativeImplementations[] = {')
    for cn in classes:
        table = class_table_name(cn) + '_natives'
        if '/' in cn:
            pkg, base = cn.rsplit('/', 1)
            lines.append('\t{ "%s", "%s", %s },' % (pkg, base, table))
        else:
            lines.append('\t{ 0, "%s", %s },' % (cn, table))
    lines.append('NATIVE_END_OF_LIST')
    lines.append('};')
    lines.append('')
    lines.append('#endif')
    lines.append('')
    with open(out_path, 'w') as f:
        f.write('\n'.join(lines))

    total = sum(len(v) for v in natives.values())
    print('wrote %s: %d classes, %d native methods' % (out_path, len(classes), total))
    if c_syms is not None:
        if missing:
            print('WARNING: %d natives have no matching C definition:' % len(missing))
            for m in sorted(missing):
                print('   ', m)
        else:
            print('all native C symbols found in provided C sources')
    return 0


def native_names(class_name, method_name, descriptor, all_natives):
    """Return (jnic_name, params_part) -- jnic_name is the full C symbol."""
    base = 'Java_' + jni_escape(class_name) + '_' + jni_escape(method_name)
    overloaded = any(mn == method_name for (mn, d) in all_natives if d != descriptor)
    if overloaded:
        p = descriptor.find('(')
        q = descriptor.find(')')
        params = descriptor[p + 1:q]
        return (base + '__' + jni_escape(params), descriptor)
    return (base, None)


def descriptor_sig(class_name, method_name, descriptor, all_natives):
    return native_names(class_name, method_name, descriptor, all_natives)[1]


if __name__ == '__main__':
    sys.exit(main())
