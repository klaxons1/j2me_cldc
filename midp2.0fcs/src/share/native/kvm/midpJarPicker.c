/*
 * midpJarPicker.c
 *
 * JAR/JAD picker for the desktop MIDP 2.0 emulator.
 *
 * When the emulator is started without a usable command line target
 * (or with a bare .jar/.jad path), the user is asked to pick a
 * MIDlet suite file. The suite is then run in "RUN_CLASS" mode: the
 * suite jar is added to the VM classpath and the MIDlet class (taken
 * from the JAR manifest, or selected from the JAD) is launched.
 *
 * For a bare .jar (no .jad next to it) a temporary JAD is generated
 * from META-INF/MANIFEST.MF so that the MIDlet suite still carries
 * its name, vendor and version metadata.
 *
 * Portable C89. The file dialog uses the Win32 common dialog on
 * Windows; on other platforms the path is read from stdin (used by
 * the headless test harness).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#ifndef OFN_HIDEPICKER
#define OFN_HIDEPICKER 0x00000080
#endif
#endif

#include "commandState.h"
#include "midpMalloc.h"

#include <sys/stat.h>

/* from midpZipRead.c */
extern int midpZipReadEntry(const char *jarPath, const char *entryName,
                            unsigned char **dataOut, unsigned long *sizeOut);

/*====================================================================
 * portability helpers
 *==================================================================*/

static int
midpPickerCaseCmpN(const char *a, const char *b, int n) {
    int i;
    for (i = 0; i < n; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca == '\0' || cb == '\0') {
            break;
        }
        if (ca >= 'A' && ca <= 'Z') {
            ca += 32;
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb += 32;
        }
        if (ca != cb) {
            return ca - cb;
        }
    }
    return 0;
}

static int
midpPickerFileExists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/*
 * Convert a possibly relative path to an absolute one so that the
 * VM classpath does not depend on the current working directory.
 * Returns a pointer to a static buffer (valid until the next call).
 */
static const char *
midpPickerMakeAbsolute(const char *path) {
    static char absBuf[4096];
#ifdef WIN32
    DWORD n = GetFullPathNameA(path, (DWORD)sizeof(absBuf),
                               absBuf, NULL);
    if (n == 0 || n >= (DWORD)sizeof(absBuf)) {
        return path;
    }
    return absBuf;
#else
    if (path[0] == '/') {
        return path;
    }
    {
        const char *cwd = getenv("PWD");
        int n;
        if (cwd == NULL || *cwd == '\0') {
            return path;
        }
        n = snprintf(absBuf, (size_t)sizeof(absBuf), "%s/%s", cwd, path);
        if (n < 0 || (size_t)n >= sizeof(absBuf)) {
            return path;
        }
        return absBuf;
    }
#endif
}

static void
midpPickerCopyStr(char *dst, int dstLen, const char *src) {
    int n = 0;
    while (src[n] != '\0' && n < dstLen - 1) {
        n++;
    }
    memcpy(dst, src, (size_t)n);
    dst[n] = '\0';
}

/*====================================================================
 * small string utilities
 *==================================================================*/

static int
midpPickerIStrEndsWith(const char *s, const char *suffix) {
    int slen = (int)strlen(s);
    int xlen = (int)strlen(suffix);
    int i;
    if (xlen > slen) {
        return 0;
    }
    for (i = 0; i < xlen; i++) {
        char a = s[slen - xlen + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a += 32;
        }
        if (b >= 'A' && b <= 'Z') {
            b += 32;
        }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

static char *
midpPickerTrim(char *s) {
    char *end;
    while (*s && (isspace((unsigned char)*s))) {
        s++;
    }
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

int
midpPickerIsMIDletFilePublic(const char *path) {
    if (path == NULL || *path == '\0') {
        return 0;
    }
    return midpPickerIStrEndsWith(path, ".jar") ||
           midpPickerIStrEndsWith(path, ".jad");
}

/*====================================================================
 * manifest / JAD property list
 *
 * Both META-INF/MANIFEST.MF and .jad files use the same line-based
 * property format:  "Name: value"  with continuation lines starting
 * with a single space.
 *==================================================================*/

#define PICKER_MAX_PROPS   128
#define PICKER_MAX_NAME    256
#define PICKER_MAX_VALUE   2048

typedef struct {
    char name[PICKER_MAX_NAME];
    char value[PICKER_MAX_VALUE];
} PickerProp;

static int
midpPickerParseProps(const char *text, PickerProp *props, int maxProps) {
    int count = 0;
    int haveCur = 0;
    char curName[PICKER_MAX_NAME];
    char curValue[PICKER_MAX_VALUE];
    const char *p = text;

    curName[0] = '\0';
    curValue[0] = '\0';
    while (*p != '\0') {
        const char *eol = p;
        int len;
        char line[PICKER_MAX_VALUE];
        while (*eol != '\0' && *eol != '\n') {
            eol++;
        }
        len = (int)(eol - p);
        if (len >= (int)sizeof(line)) {
            len = (int)sizeof(line) - 1;
        }
        while (len > 0 && (p[len - 1] == '\r' || p[len - 1] == '\n')) {
            len--;
        }
        if (len < 0) {
            len = 0;
        }
        memcpy(line, p, (size_t)len);
        line[len] = '\0';
        p = eol + (*eol != '\0');

        if (len > 0 && line[0] == ' ') {
            /* continuation of the previous value */
            int off;
            int i;
            if (!haveCur) {
                continue;
            }
            off = (int)strlen(curValue);
            for (i = 1; i < len; i++) {
                if (off < PICKER_MAX_VALUE - 1) {
                    curValue[off++] = line[i];
                }
            }
            curValue[off] = '\0';
            continue;
        }
        if (len == 0) {
            continue;
        }
        /* flush the previous property, if any */
        if (haveCur) {
            if (count < maxProps) {
                midpPickerCopyStr(props[count].name, PICKER_MAX_NAME,
                                  curName);
                midpPickerCopyStr(props[count].value, PICKER_MAX_VALUE,
                                  curValue);
                count++;
            }
            haveCur = 0;
        }
        {
            const char *colon = strchr(line, ':');
            if (colon != NULL) {
                int nlen = (int)(colon - line);
                char *v;
                if (nlen >= PICKER_MAX_NAME) {
                    nlen = PICKER_MAX_NAME - 1;
                }
                memcpy(curName, line, (size_t)nlen);
                curName[nlen] = '\0';
                v = midpPickerTrim((char *)(colon + 1));
                midpPickerCopyStr(curValue, PICKER_MAX_VALUE, v);
                haveCur = 1;
            }
        }
    }
    if (haveCur && count < maxProps) {
        midpPickerCopyStr(props[count].name, PICKER_MAX_NAME, curName);
        midpPickerCopyStr(props[count].value, PICKER_MAX_VALUE, curValue);
        count++;
    }
    return count;
}

static const char *
midpPickerGetProp(const PickerProp *props, int count, const char *name) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(props[i].name, name) == 0) {
            return props[i].value;
        }
    }
    return NULL;
}

/*
 * Find the value of "MIDlet-<n>" with the smallest n.
 */
static int
midpPickerFindMIDlet(const PickerProp *props, int count,
                     char *nameOut, int nameLen,
                     char *iconOut, int iconLen,
                     char *classOut, int classLen) {
    int best = 0x7FFFFFFF;
    int i;
    for (i = 0; i < count; i++) {
        const char *n = props[i].name;
        int num;
        if (strncmp(n, "MIDlet-", 7) != 0) {
            continue;
        }
        {
            const char *d = n + 7;
            if (*d < '1' || *d > '9') {
                continue;
            }
            num = 0;
            while (*d >= '0' && *d <= '9') {
                num = num * 10 + (*d - '0');
                d++;
            }
            if (*d != '\0' || num <= 0) {
                continue;
            }
            if (num < best) {
                best = num;
            }
        }
    }
    if (best == 0x7FFFFFFF) {
        return -1;
    }
    /* now parse  "Name, Icon, Class" */
    {
        char key[32];
        int klen = 0;
        key[klen++] = 'M'; key[klen++] = 'I'; key[klen++] = 'D';
        key[klen++] = 'l'; key[klen++] = 'e'; key[klen++] = 't';
        key[klen++] = '-';
        {
            int v = best;
            char digits[12];
            int dl = 0;
            if (v == 0) {
                digits[dl++] = '0';
            } else {
                while (v > 0) {
                    digits[dl++] = (char)('0' + (v % 10));
                    v /= 10;
                }
            }
            while (dl > 0) {
                key[klen++] = digits[--dl];
            }
        }
        key[klen] = '\0';
        for (i = 0; i < count; i++) {
            if (strcmp(props[i].name, key) == 0) {
                char buf[PICKER_MAX_VALUE];
                char *c1;
                char *c2;
                char *name;
                char *icon;
                char *cls;
                int blen;
                strcpy(buf, props[i].value);
                c1 = strchr(buf, ',');
                if (c1 == NULL) {
                    return -1;
                }
                *c1 = '\0';
                c2 = strchr(c1 + 1, ',');
                if (c2 == NULL) {
                    return -1;
                }
                *c2 = '\0';
                name = midpPickerTrim(buf);
                icon = midpPickerTrim(c1 + 1);
                cls = midpPickerTrim(c2 + 1);
                blen = (int)strlen(name);
                if (blen >= nameLen) {
                    blen = nameLen - 1;
                }
                memcpy(nameOut, name, (size_t)blen);
                nameOut[blen] = '\0';
                blen = (int)strlen(icon);
                if (blen >= iconLen) {
                    blen = iconLen - 1;
                }
                memcpy(iconOut, icon, (size_t)blen);
                iconOut[blen] = '\0';
                blen = (int)strlen(cls);
                if (blen >= classLen) {
                    blen = classLen - 1;
                }
                memcpy(classOut, cls, (size_t)blen);
                classOut[blen] = '\0';
                return 0;
            }
        }
    }
    return -1;
}

/*====================================================================
 * file dialog
 *==================================================================*/

static void
midpPickerInitialDir(char *dir, int dirLen) {
#ifdef WIN32
    char exePath[MAX_PATH];
    char *slash;
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0) {
        slash = strrchr(exePath, '\\');
        if (slash != NULL) {
            *slash = '\0';
        }
        {
            char probe[MAX_PATH + 16];
            snprintf(probe, sizeof(probe), "%s\\games", exePath);
            if (GetFileAttributesA(probe) != INVALID_FILE_ATTRIBUTES) {
                strncpy(dir, probe, (size_t)(dirLen - 1));
                dir[dirLen - 1] = '\0';
                return;
            }
        }
        strncpy(dir, exePath, (size_t)(dirLen - 1));
        dir[dirLen - 1] = '\0';
        return;
    }
#endif
    strncpy(dir, ".", (size_t)(dirLen - 1));
    dir[dirLen - 1] = '\0';
}

/*
 * Ask the user for a .jar/.jad file.
 * Returns 1 if a file was selected (copied to pathBuf/pathLen),
 * 0 if the user cancelled.
 */
static int
midpPickerChooseFile(char *pathBuf, int pathLen) {
#ifdef WIN32
    {
        OPENFILENAMEA ofn;
        char initialDir[MAX_PATH + 16];
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = pathBuf;
        ofn.nMaxFile = (DWORD)pathLen;
        ofn.lpstrFilter = "J2ME MIDlets (*.jar, *.jad)\0*.jar;*.jad\0"
                          "MIDlet JAR files (*.jar)\0*.jar\0"
                          "MIDlet descriptors (*.jad)\0*.jad\0"
                          "All files (*.*)\0*.*\0";
        ofn.lpstrTitle = "Select a J2ME MIDlet suite (.jar or .jad)";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEPICKER;
        midpPickerInitialDir(initialDir, (int)sizeof(initialDir));
        ofn.lpstrInitialDir = initialDir;
        if (GetOpenFileNameA(&ofn) != 0) {
            return 1;
        }
        return 0;
    }
#else
    /* non-Windows: read from stdin (headless test path) */
    {
        size_t n = 0;
        int ch;
        printf("Enter path to .jar or .jad (empty to cancel): ");
        fflush(stdout);
        while ((ch = getchar()) != EOF && ch != '\n') {
            if (n + 1 < (size_t)pathLen) {
                pathBuf[n++] = (char)ch;
            }
        }
        pathBuf[n] = '\0';
        pathBuf[n + 1] = '\0';
        return (*pathBuf != '\0');
    }
#endif
}

/*====================================================================
 * suite selection
 *
 * Given a .jar or .jad path, prepare the RUN_CLASS command state:
 *   - classNameOut:      MIDlet class to run (NULL for .jad with a
 *                        MIDlet list -> the in-VM selector is shown)
 *   - descriptorOut:     JAD to use for suite metadata (may point to
 *                        the picked .jad, or a generated temp .jad)
 *   - classpathEntryOut: jar file to add to the VM classpath
 *
 * Returns 0 on success, -1 on failure (a message is printed).
 *==================================================================*/

static char *
midpPickerTempJadPath(const char *jarPath, char *buf, int bufLen) {
    const char *slash;
    const char *base;
    const char *dot;
    int baseLen;
#ifdef WIN32
    char tmpDir[MAX_PATH];
    (void)GetTempPathA(MAX_PATH, tmpDir);
#endif

    slash = strrchr(jarPath, '/');
#ifdef WIN32
    {
        const char *slash2 = strrchr(jarPath, '\\');
        if (slash2 != NULL && (slash == NULL || slash2 > slash)) {
            slash = slash2;
        }
    }
#endif
    base = (slash != NULL) ? slash + 1 : jarPath;
    dot = strrchr(base, '.');
    baseLen = (dot != NULL) ? (int)(dot - base) : (int)strlen(base);
    if (baseLen <= 0) {
        static const char *fallbackBase = "suite";
        base = fallbackBase;
        baseLen = (int)strlen(base);
    }
#ifdef WIN32
    snprintf(buf, (size_t)bufLen, "%s%.*s.midp-jad", tmpDir, baseLen, base);
#else
    snprintf(buf, (size_t)bufLen, "/tmp/%.*s.midp-jad", baseLen, base);
#endif
    return buf;
}

/*
 * Generate a JAD from the jar's manifest. Returns the generated JAD
 * path (malloc'ed) or NULL on error.
 */
static char *
midpPickerGenerateJad(const char *jarPath, const PickerProp *props,
                      int propCount, int firstMIDletOk) {
    static PickerProp entries[64];
    int entryCount = 0;
    int i;
    char jadPathBuf[1024];
    char *jadPath;
    FILE *f;
    const char *name;
    const char *vendor;
    const char *version;

    /* collect all MIDlet-N entries in order */
    for (i = 0; i < propCount && entryCount < 64; i++) {
        const char *n = props[i].name;
        if (strncmp(n, "MIDlet-", 7) == 0 && n[7] >= '1' && n[7] <= '9' &&
            n[8] == '\0') {
            entries[entryCount++] = props[i];
        }
    }
    if (entryCount == 0 && firstMIDletOk) {
        return NULL;
    }

    midpPickerTempJadPath(jarPath, jadPathBuf, (int)sizeof(jadPathBuf));
    jadPath = (char *)midpStrdup(jadPathBuf);
    if (jadPath == NULL) {
        return NULL;
    }
    f = fopen(jadPath, "w");
    if (f == NULL) {
        midpFree(jadPath);
        return NULL;
    }
    name = midpPickerGetProp(props, propCount, "MIDlet-Name");
    if (name == NULL || *name == '\0') {
        name = "MIDlet";
    }
    vendor = midpPickerGetProp(props, propCount, "MIDlet-Vendor");
    if (vendor == NULL || *vendor == '\0') {
        vendor = "unknown";
    }
    version = midpPickerGetProp(props, propCount, "MIDlet-Version");
    if (version == NULL || *version == '\0') {
        version = "1.0.0";
    }
    fprintf(f, "MIDlet-Name: %s\n", name);
    fprintf(f, "MIDlet-Vendor: %s\n", vendor);
    fprintf(f, "MIDlet-Version: %s\n", version);
    for (i = 0; i < entryCount; i++) {
        fprintf(f, "%s: %s\n", entries[i].name, entries[i].value);
    }
    fclose(f);
    return jadPath;
}

/*
 * Find the suite jar referenced by a JAD (MIDlet-JAR-URL), or a
 * same-named .jar next to the JAD. Returns malloc'ed path or NULL.
 */
static char *
midpPickerFindJarForJad(const char *jadPath,
                        const PickerProp *props, int propCount) {
    const char *jarUrl = midpPickerGetProp(props, propCount,
                                           "MIDlet-JAR-URL");
    char *result = NULL;

    if (jarUrl != NULL && *jarUrl != '\0') {
        if (midpPickerCaseCmpN(jarUrl, "http://", 7) == 0 ||
            midpPickerCaseCmpN(jarUrl, "https://", 8) == 0) {
            /* remote jar: cannot be added to the local classpath */
            return NULL;
        }
        {
            char dir[1024];
            char probe[2048];
            const char *slash = strrchr(jadPath, '/');
#ifdef WIN32
            const char *slash2 = strrchr(jadPath, '\\');
            if (slash2 != NULL && (slash == NULL || slash2 > slash)) {
                slash = slash2;
            }
#endif
            if (slash != NULL) {
                int dlen = (int)(slash - jadPath);
                if (dlen >= (int)sizeof(dir)) {
                    dlen = (int)sizeof(dir) - 1;
                }
                memcpy(dir, jadPath, (size_t)dlen);
                dir[dlen] = '\0';
            } else {
                strcpy(dir, ".");
            }
            /* plain relative filename */
            snprintf(probe, sizeof(probe), "%s/%s", dir, jarUrl);
#ifdef WIN32
            /* try back-slash form too */
            {
                char probe2[2048];
                int plen = (int)strlen(dir);
                if (plen < (int)sizeof(probe2) - 2 - (int)strlen(jarUrl)) {
                    memcpy(probe2, dir, (size_t)plen);
                    probe2[plen] = '\\';
                    midpPickerCopyStr(probe2 + plen + 1,
                                      (int)sizeof(probe2) - plen - 1,
                                      jarUrl);
                    if (!midpPickerFileExists(probe)) {
                        strcpy(probe, probe2);
                    }
                }
            }
#endif
            if (midpPickerFileExists(probe)) {
                result = (char *)midpStrdup(probe);
            }
        }
    }
    if (result == NULL) {
        /* fallback: <jad base>.jar next to the JAD */
        char probe[2048];
        strncpy(probe, jadPath, sizeof(probe) - 5);
        probe[sizeof(probe) - 5] = '\0';
        {
            char *dot = strrchr(probe, '.');
            if (dot != NULL) {
                *dot = '\0';
            }
            strcat(probe, ".jar");
            if (midpPickerFileExists(probe)) {
                result = (char *)midpStrdup(probe);
            }
        }
    }
    return result;
}

static int
midpPickerReadFile(const char *path, char **textOut) {
    FILE *f = fopen(path, "rb");
    long size;
    char *buf;
    if (f == NULL) {
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    if (size < 0 || size > (1024L * 1024L)) {
        fclose(f);
        return -1;
    }
    buf = (char *)midpMalloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(f);
        return -1;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        midpFree(buf);
        fclose(f);
        return -1;
    }
    buf[size] = '\0';
    fclose(f);
    *textOut = buf;
    return 0;
}

/*
 * Main entry: handle a picked/typed .jar or .jad file.
 */
int
midpPickerHandleSuiteFile(const char *path,
                          char **classNameOut,
                          char **descriptorOut,
                          char **classpathEntryOut) {
    static PickerProp props[PICKER_MAX_PROPS];
    const char *absPath;
    int propCount = 0;
    char *text = NULL;
    int isJar;
    int isJad;

    *classNameOut = NULL;
    *descriptorOut = NULL;
    *classpathEntryOut = NULL;

    absPath = midpPickerMakeAbsolute(path);
    isJar = midpPickerIStrEndsWith(absPath, ".jar");
    isJad = midpPickerIStrEndsWith(absPath, ".jad");

    if (!isJar && !isJad) {
        fprintf(stderr, "midp: \"%s\" is not a .jar or .jad file\n", path);
        return -1;
    }

    if (isJar) {
        unsigned char *manifest = NULL;
        unsigned long manifestSize = 0;
        char manifestText[PICKER_MAX_VALUE * 4];
        int rc;

        *classpathEntryOut = (char *)midpStrdup(absPath);
        if (*classpathEntryOut == NULL) {
            fprintf(stderr, "midp: out of memory\n");
            return -1;
        }
        rc = midpZipReadEntry(absPath, "META-INF/MANIFEST.MF",
                              &manifest, &manifestSize);
        if (rc != 0 || manifest == NULL) {
            fprintf(stderr,
                    "midp: cannot read META-INF/MANIFEST.MF from \"%s\"\n",
                    path);
            midpFree(*classpathEntryOut);
            *classpathEntryOut = NULL;
            return -1;
        }
        if (manifestSize >= sizeof(manifestText)) {
            manifestSize = sizeof(manifestText) - 1;
        }
        memcpy(manifestText, manifest, manifestSize);
        manifestText[manifestSize] = '\0';
        free(manifest);
        propCount = midpPickerParseProps(manifestText, props,
                                         PICKER_MAX_PROPS);
        {
            char name[512], icon[512], cls[1024];
            int firstOk =
                (midpPickerFindMIDlet(props, propCount, name,
                                      (int)sizeof(name), icon,
                                      (int)sizeof(icon), cls,
                                      (int)sizeof(cls)) == 0);
            if (firstOk) {
                *classNameOut = (char *)midpStrdup(cls);
                *descriptorOut =
                    midpPickerGenerateJad(absPath, props, propCount, 1);
                if (*descriptorOut == NULL) {
                    /* metadata-only failure: still runnable */
                    *descriptorOut = NULL;
                }
                if (*classNameOut == NULL) {
                    fprintf(stderr, "midp: out of memory\n");
                    midpFree(*classpathEntryOut);
                    *classpathEntryOut = NULL;
                    return -1;
                }
                return 0;
            }
            fprintf(stderr,
                "midp: no MIDlet-N entry found in the manifest of \"%s\"\n",
                path);
            midpFree(*classpathEntryOut);
            *classpathEntryOut = NULL;
            return -1;
        }
    } else {
        /* .jad */
        if (midpPickerReadFile(absPath, &text) != 0) {
            fprintf(stderr, "midp: cannot read \"%s\"\n", path);
            return -1;
        }
        propCount = midpPickerParseProps(text, props, PICKER_MAX_PROPS);
        midpFree(text);
        *descriptorOut = (char *)midpStrdup(absPath);
        {
            char *jarPath = midpPickerFindJarForJad(absPath, props, propCount);
            if (jarPath != NULL) {
                *classpathEntryOut = jarPath;
            } else {
                fprintf(stderr,
                    "midp: warning: cannot locate the suite JAR for "
                    "\"%s\" (MIDlet-JAR-URL); the suite may not run\n",
                    path);
            }
        }
        /* midletClassName stays NULL -> in-VM MIDlet selector */
        return 0;
    }
}

/*
 * Ask the user (file dialog) to pick a suite, then process it.
 * Returns 0 if a suite was selected and prepared, -1 if cancelled
 * or failed.
 */
int
midpPickerShowAndSelect(char **classNameOut, char **descriptorOut,
                        char **classpathEntryOut) {
    char path[4096];
    *classNameOut = NULL;
    *descriptorOut = NULL;
    *classpathEntryOut = NULL;
    if (!midpPickerChooseFile(path, (int)sizeof(path))) {
        return -1;
    }
    return midpPickerHandleSuiteFile(path, classNameOut, descriptorOut,
                                     classpathEntryOut);
}
