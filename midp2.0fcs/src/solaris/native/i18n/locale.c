/*
 * @(#)locale.c	1.12 02/07/24 @(#)
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*
 * The following code are borrowed from JDK1.3.
 */

#include <stdlib.h>
#include <strings.h>
#include <locale.h>
#include <midpMalloc.h>
#include <langinfo.h>           /* For nl_langinfo */

#include "configuration.h"

/*
 * Mappings from partial locale names to full locale names
 */
static char *locale_aliases[] = {
#ifdef NOT_SUPPORTED
    "ar", "ar_EG",
    "be", "be_BY",
    "bg", "bg_BG",
    "ca", "ca_ES",
    "cs", "cs_CZ",
    "cz", "cs_CZ",
    "da", "da_DK",
    "de", "de_DE",
    "el", "el_GR",
    "en", "en_US",
    "es", "es_ES",
    "et", "et_EE",
    "fi", "fi_FI",
    "fr", "fr_FR",
    "iw", "iw_IL",
    "hr", "hr_HR",
    "hu", "hu_HU",
    "is", "is_IS",
    "it", "it_IT",
#endif
    "ja", "ja_JP",
#ifdef NOT_SUPPORTED
    "ko", "ko_KR",
    "lt", "lt_LT",
    "lv", "lv_LV",
    "mk", "mk_MK",
    "nl", "nl_NL",
    "no", "no_NO",
    "pl", "pl_PL",
    "pt", "pt_PT",
    "ro", "ro_RO",
    "ru", "ru_RU",
    "sh", "sh_YU",
    "sk", "sk_SK",
    "sl", "sl_SI",
    "sq", "sq_AL",
    "sr", "sr_YU",
    "su", "fi_FI",
    "sv", "sv_SE",
    "th", "th_TH",
    "tr", "tr_TR",
    "uk", "uk_UA",
    "zh", "zh_CN",
    "tchinese", "zh_TW",
    "big5", "zh_TW.Big5",
#endif
    "japanese", "ja_JP",
    ""
 };

/*
 * Solaris language string to ISO639 string mapping table.
 */
static char *language_names[] = {
#ifdef NOT_SUPPORTED
    "C", "en",
    "POSIX", "en",
    "ar", "ar",
    "be", "be",
    "bg", "bg",
    "ca", "ca",
    "chinese", "zh",
    "cs", "cs",
    "cz", "cs",
    "da", "da",
    "de", "de",
    "el", "el",
    "en", "en",
    "es", "es",
    "et", "et",
    "fi", "fi",
    "su", "fi",
    "fr", "fr",
    "he", "iw",
    "hr", "hr",
    "hu", "hu",
    "is", "is",
    "it", "it",
    "iw", "iw",
#endif
    "ja", "ja",
    "japanese", "ja",
#ifdef NOT_SUPPORTED
    "ko", "ko",
    "korean", "ko",
    "lt", "lt",
    "lv", "lv",
    "mk", "mk",
    "nl", "nl",
    "no", "no",
    "nr", "nr",
    "pl", "pl",
    "pt", "pt",
    "ro", "ro",
    "ru", "ru",
    "sh", "sh",
    "sk", "sk",
    "sl", "sl",
    "sq", "sq",
    "sr", "sr",
    "sv", "sv",
    "th", "th",
    "tr", "tr",
    "uk", "uk",
    "zh", "zh",
#endif
    "",
};

/*
 * Solaris country string to ISO3166 string mapping table.
 * Currently only different string is UK/GB.
 */
static char *region_names[] = {
#ifdef NOT_SUPPORTED
    "AT", "AT",
    "AU", "AU",
    "AR", "AR",
    "BE", "BE",
    "BR", "BR",
    "BO", "BO",
    "CA", "CA",
    "CH", "CH",
    "CL", "CL",
    "CN", "CN",
    "CO", "CO",
    "CR", "CR",
    "EC", "EC",
    "GT", "GT",
    "IE", "IE",
    "IL", "IL",
#endif
    "JP", "JP",
#ifdef NOT_SUPPORTED
    "KR", "KR",
    "MX", "MX",
    "NI", "NI",
    "NZ", "NZ",
    "PA", "PA",
    "PE", "PE",
    "PY", "PY",
    "SV", "SV",
    "TH", "TH",
    "UK", "GB",
    "US", "US",
    "UY", "UY",
    "VE", "VE",
    "TW", "TW",
#endif
    "",
};

/*
 * Solaris converter string to supported conveter string.
 */
static char *encoding_names[] = {
#ifdef NOT_SUPPORTED
    "646",               "ISO8859_1",
    "8859-1",            "ISO8859_1",
    "ISO8859-1",         "ISO8859_1",

    "8859-2",            "ISO8859_2",
    "ISO8859-2",         "ISO8859_2",

    "8859-3",            "ISO8859_3",
    "ISO8859-3",         "ISO8859_3",

    "8859-4",            "ISO8859_4",
    "ISO8859-4",         "ISO8859_4",

    "8859-5",            "ISO8859_5",
    "ISO8859-5",         "ISO8859_5",

    "8859-6",            "ISO8859_6",
    "ISO8859-6",         "ISO8859_6",

    "8859-7",            "ISO8859_7",
    "ISO8859-7",         "ISO8859_7",

    "8859-8",            "ISO8859_8",
    "ISO8859-8",         "ISO8859_8",

    "8859-9",            "ISO8859_9",
    "ISO8859-9",         "ISO8859_9",

    "8859-13",           "ISO8859_13",
    "ISO8859-13",        "ISO8859_13",

    "8859-15",           "ISO8859_15_FDIS",
    "ISO8859-15",        "ISO8859_15_FDIS",

    "KOI8-R",            "KOI8_R",

    "5601",              "EUC_KR",
    "EUC-KR",            "EUC_KR",
    "KSC5601",           "EUC_KR",
#endif

    "EUCJP",             "EUC_JP",

#ifdef NOT_SUPPORTED
    "BIG5",              "Big5",

    "GB2312",            "EUC_CN",
    "EUCCN",             "EUC_CN",

    "TIS620.2533",       "TIS620",

    "EUCTW",             "EUC_TW",
#endif

    "",
};

/*
 * Solaris variant string to Java variant name mapping table.
 */
static char *variant_names[] = {
#ifdef NOT_SUPPORTED
    "euro", "EURO",
#endif
    "",
};

/* Take an array of string pairs (map of key->value) and a string (key).
 * Examine each pair in the map to see if the first string (key) matches the
 * string.  If so, store the second string of the pair (value) in the value and
 * return 1.  Otherwise do nothing and return 0.  The end of the map is
 * indicated by an empty string at the start of a pair (key of "").
 */
static int
mapLookup(char* map[], const char* key, char** value) {
    int i;
    for (i = 0; strcmp(map[i], ""); i += 2){
        if (!strcmp(key, map[i])){
            *value = map[i + 1];
            return 1;
        }
    }
    return 0;
}

void
getLocaleInfo(char **locale, char **encoding)
{
    static char *sprops_locale, *sprops_encoding;
    static int isValuesSet = 0;
    char *sprops_language, *sprops_region;
    char region_variant[64];

    /* return the cache values */
    if (isValuesSet) {
        goto return_values;
    }

    /* set the flag for the cache */
    isValuesSet = 1;

    /* see LANG and ENCODING available. */
    sprops_language = getInternalPropDefault("system.i18n.lang","en");
    sprops_encoding = getInternalPropDefault("system.i18n.encoding","ISO8859_1");
    sprops_region   = NULL;

    if ((sprops_language && *sprops_language) && 
        (sprops_encoding && *sprops_encoding)) {
        ; /* use the enviroment varables as is */
    } 
    /* Determing the language, country, and encoding from the host,
     * and store these in the user.language, user.region, and
     * file.encoding system properties. */
    else {
        char *lc;
        lc = setlocale(LC_CTYPE, "");
        if (lc == NULL) {
            /*
             * 'lc == null' means system doesn't support user's environment
             * variable's locale.
             */
            setlocale(LC_ALL, "C");
            goto return_values;
        } else {
            /*
             * locale string format in Solaris is
             * <language name>_<region name>.<encoding name>
             * <region name> and <encoding name> are optional.
             */
            char temp[64], *language = NULL, *region = NULL, *encoding = NULL;
            char *std_language = NULL, *std_region = NULL,
                *std_encoding = NULL;
            char *variant = NULL, *std_variant = NULL;
            char *p, encoding_variant[64];
            int len;

            strcpy(temp, lc);
            setlocale(LC_ALL, lc);

            /* Parse the language, region, encoding, and variant from
             * the locale.  Any of the elements may be missing, but
             * they must occur in the order
             * language_region.encoding@variant, and must be preceded
             * by their delimiter (except for language).
             *
             * If the locale name (without .encoding@variant, if any)
             * matches any of the names in the locale_aliases list,
             * map it to the corresponding full locale name.  Most of
             * the entries in the locale_aliases list are locales that
             * include a language name but no country name, and this
             * facility is used to map each language to a default
             * country if that's possible.  It's also used to map the
             * Solaris locale aliases to their proper Java locale IDs.
             */
            if ((p = strchr(temp, '.')) != NULL) {
                strcpy(encoding_variant, p); /* Copy the leading '.' */
                *p = '\0';
            } else if ((p = strchr(temp, '@')) != NULL) {
                strcpy(encoding_variant, p); /* Copy the leading '@' */
                *p = '\0';
            } else {
                *encoding_variant = '\0';
            }
            
            if (mapLookup(locale_aliases, temp, &p)) {
                strcpy(temp, p);
            }
            
            language = temp;
            if ((region = strchr(temp, '_')) != NULL) {
                *region++ = '\0';
            }
            
            p = encoding_variant;
            if ((encoding = strchr(p, '.')) != NULL) {
                p[encoding++ - p] = '\0';
                p = encoding;
            }
            if ((variant = strchr(p, '@')) != NULL) {
                p[variant++ - p] = '\0';
            }

            /* Normalize the language name */
            /* std_language = "en"; */
            if (language != NULL) {
                mapLookup(language_names, language, &std_language);
            }
            sprops_language = std_language;

            /* Normalize the variant name.  Do this BEFORE handling
             * the region name, since the variant name will be
             * incorporated into the user.region property.  */
            if (variant != NULL) {
                mapLookup(variant_names, variant, &std_variant);
            }

            /* Normalize the region name.  If there is a std_variant,
             * then append it to the region.  This applies even if
             * there is no region, in which case the empty string is
             * used for the region.  Note that we only use variants
             * listed in the mapping array; others are ignored.  */
            *region_variant = '\0';
            if (region != NULL) {
                std_region = region;
                mapLookup(region_names, region, &std_region);
                strcpy(region_variant, std_region);
            }
            if (std_variant != NULL) {
                strcat(region_variant, "_");
                strcat(region_variant, std_variant);
            }
            if ((*region_variant) != '\0') {
                sprops_region = &region_variant[0];
            }

            if (sprops_language && *sprops_language) {
                /* Normalize the encoding name.  Note that we IGNORE the
                 * string 'encoding' extracted from the locale name above.
                 * Instead, we use the more reliable method of calling
                 * nl_langinfo(CODESET).  This function returns an empty
                 * string if no encoding is set for the given locale
                 * (e.g., the C or POSIX locales); we use the default ISO
                 * 8859-1 converter for such locales.  We don't need to
                 * map from the Solaris string to the Java identifier at
                 * this point because that mapping is handled by the
                 * character converter alias table in
                 * CharacterEncoding.java.  */
                p = nl_langinfo(CODESET);
                if (strcmp(p, "ANSI_X3.4-1968") == 0 || *p == '\0') {
                    std_encoding = "8859-1";
                } else {
                    std_encoding = p;
                }

                len = strlen(std_encoding);
                encoding_variant[len] = 0;
                while (--len >= 0) {
                    encoding_variant[len] = toupper(std_encoding[len]);
                }

                mapLookup(encoding_names, encoding_variant, &sprops_encoding);
            }
        }
    }

    if (sprops_language && *sprops_language) {
        char buf[6];
        if (sprops_region && *sprops_region) {
            sprintf(buf, "%s_%s", sprops_language, sprops_region);
        } else {
            sprintf(buf, "%s", sprops_language);
        }
        sprops_locale = midpStrdup(buf);
    }

  return_values:
    *locale = sprops_locale;
    *encoding = sprops_encoding;
}
/*
main()
{
    char *locale;
    char *encoding;

    getLocaleInfo(&locale, &encoding);

    if (locale && *locale) {
        printf("locale:%s\n", locale);
    } else {
        printf("locale:null\n");
    }
    if (encoding && *encoding) {
        printf("encoding:%s\n", encoding);
    } else {
        printf("encoding:null\n");
    }
}
*/
