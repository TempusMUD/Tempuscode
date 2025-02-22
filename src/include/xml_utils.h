#ifndef _XML_UTILS_H_
#define _XML_UTILS_H_

void xml_boot(void);

/**
 * Parses a string from a named property in the given node
 **/
static inline char *
xmlGetStrProp(xmlNodePtr n, const char *name, const char *defValue)
{
    xmlChar *c = xmlGetProp(n, (const xmlChar *)(name));
    if (c == NULL) {
        if (defValue == NULL) {
            return NULL;
        }
        return strdup(defValue);
    }
    char *prop = strdup((char *)c);
    xmlFree(c);
    return prop;
}

/**
 * Parses an integer from a named property in the given node
 **/
static inline long
xmlGetLongProp(xmlNodePtr n, const char *name, long defValue)
{
    long prop = 0;
    xmlChar *c = xmlGetProp(n, (const xmlChar *)(name));
    if (c == NULL) {
        return defValue;
    }
    prop = atol((const char *)(c));
    xmlFree(c);
    return prop;
}

/**
 * Parses an integer from a named property in the given node
 **/
static inline int
xmlGetIntProp(xmlNodePtr n, const char *name, int defValue)
{
    int prop = 0;
    xmlChar *c = xmlGetProp(n, (const xmlChar *)(name));
    if (c == NULL) {
        return defValue;
    }
    prop = atoi((const char *)(c));
    xmlFree(c);
    return prop;
}

/**
 * Parses an integer from a named property in the given node
 **/
static inline float
xmlGetFloatProp(xmlNodePtr n, const char *name, float defValue)
{
    float prop = 0;
    xmlChar *c = xmlGetProp(n, (const xmlChar *)(name));
    if (c == NULL) {
        return defValue;
    }
    prop = (float)atof((const char *)(c));
    xmlFree(c);
    return prop;
}

/**
 * Parses a character from a named property in the given node
 **/
static inline char
xmlGetCharProp(xmlNodePtr n, const char *name)
{
    char prop = '\0';
    char *c = (char *)xmlGetProp(n, (const xmlChar *)(name));
    if (c == NULL) {
        return '\0';
    }
    prop = *c;
    xmlFree(c);
    return prop;
}

static inline bool
xmlMatches(const xmlChar *str_a, const char *str_b)
{
    return (strcmp((const char *)(str_a), str_b) == 0);
}

/**
 * Do a global encoding of a string, replacing the predefined entities and
 * non ASCII values with their entities and CharRef counterparts.
 * Contrary to xmlEncodeEntities, uses the tmpstr utility.
 **/
static inline char *
xmlEncodeTmp(const char *text)
{
    char *encoded = (char *)(xmlEncodeEntitiesReentrant(NULL, (const xmlChar *)(text)));
    char *tmp_encoded = tmp_strdup(encoded);
    free(encoded);
    return tmp_encoded;
}

/**
 * tmp_string wrapped xmlEncodeSpecialChars:
 *
 * Do a global encoding of a string, replacing the predefined entities
 * this routine is reentrant, and result must be deallocated.
 *
 * NOTE: This should be used for name="value" elements rather
 *       than <name>value</name>.
 **/
static inline char *
xmlEncodeSpecialTmp(const char *text)
{
    char *encoded = (char *)(xmlEncodeSpecialChars(NULL,
                                                   (const xmlChar *)(text)));
    char *tmp_encoded = tmp_gsub(encoded, "\"", "&quot;");
    free(encoded);
    return tmp_encoded;
}

static inline xmlDocPtr
xmlParseFileWithLog(const char *fname)
{
    xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        errlog("Can't allocate XML context!");
        return NULL;
    }

    xmlDocPtr doc = xmlCtxtReadFile(ctxt, fname, NULL, 0);
    if (doc == NULL) {
        const xmlError* err = xmlCtxtGetLastError(ctxt);
        errlog("Failed to parse %s: %s", fname, err->message);
    }

    xmlFreeParserCtxt(ctxt);
    return doc;
}

#endif                          // __TEMPUS_XML_UTILS_H
