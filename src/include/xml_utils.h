#ifndef __TEMPUS_XML_UTILS_H
#define __TEMPUS_XML_UTILS_H


/** 
 * Parses an integer from a named property in the given node
 **/
static inline int xmlGetIntProp(xmlNodePtr n, const char* name) {
    int prop = 0;
    xmlChar *c = xmlGetProp(n, (const xmlChar *)name);
    if( c == NULL ) return 0;
    prop = atoi((const char*)c);
    free(c);
    return prop;
}
/** 
 * Parses a character from a named property in the given node
 **/
static inline char xmlGetCharProp(xmlNodePtr n, const char* name) {
    char prop = 0;
    xmlChar *c = xmlGetProp(n, (const xmlChar *)name);
    if( c == NULL ) return 0;
    prop = *c;
    free(c);
    return prop;
}





#endif// __TEMPUS_XML_UTILS_H
