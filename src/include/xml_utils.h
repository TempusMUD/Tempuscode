#ifndef __TEMPUS_XML_UTILS_H
#define __TEMPUS_XML_UTILS_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>

void xml_boot(void);

/** 
 * Parses an integer from a named property in the given node
 **/
static inline long
xmlGetLongProp(xmlNodePtr n, const char *name)
{
	long prop = 0;
	xmlChar *c = xmlGetProp(n, (const xmlChar *)name);
	if (c == NULL)
		return 0;
	prop = atol((const char *)c);
	free(c);
	return prop;
}

/** 
 * Parses an integer from a named property in the given node
 **/
static inline int
xmlGetIntProp(xmlNodePtr n, const char *name, int defValue = 0)
{
	int prop = 0;
	xmlChar *c = xmlGetProp(n, (const xmlChar *)name);
	if (c == NULL)
		return defValue;
	prop = atoi((const char *)c);
	free(c);
	return prop;
}

/** 
 * Parses a character from a named property in the given node
 **/
static inline char
xmlGetCharProp(xmlNodePtr n, const char *name)
{
	char prop = 0;
	xmlChar *c = xmlGetProp(n, (const xmlChar *)name);
	if (c == NULL)
		return 0;
	prop = *c;
	free(c);
	return prop;
}

static inline char *
xmlGetProp(xmlNodePtr node, const char *name)
{
	return (char *)(xmlGetProp(node, (const xmlChar *)name));
}

static inline xmlAttrPtr
xmlSetProp(xmlNodePtr node, const char *name, const char *value)
{
	return xmlSetProp(node, (const xmlChar *)name, (const xmlChar *)value);
}

static inline xmlAttrPtr
xmlSetProp(xmlNodePtr node, const char *name, const int value)
{
	char xmlSetPropBuf[80];
	sprintf(xmlSetPropBuf, "%d", value);
	return xmlSetProp(node, (const xmlChar *)name,
		(const xmlChar *)xmlSetPropBuf);
}

static inline bool
xmlMatches(const xmlChar *str_a, const char *str_b)
{
	return !strcmp((const char *)str_a, str_b);
}

#endif							// __TEMPUS_XML_UTILS_H
