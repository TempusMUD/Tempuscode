#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <libxml/parser.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "tmpstr.h"
#include "utils.h"
#include "xml.h"

struct xmlc_node *
xml_null_node(void)
{
    struct xmlc_node *result;
    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_NULL;

    return result;
}

struct xmlc_node *
xml_node(const char *tag, ...)
{
    va_list args;
    struct xmlc_node *result;
    struct xmlc_node *last_attr = NULL;
    struct xmlc_node *last_child = NULL;

    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_NODE;
    result->tag = tag;

    va_start(args, tag);
    struct xmlc_node *sn;
    while ((sn = va_arg(args, struct xmlc_node *)) != NULL) {
        switch (sn->kind) {
        case XMLC_NULL:
            // false xml_if condition -- ignore.
            break;
        case XMLC_ATTR:
            // add to attrs
            if (last_attr) {
                last_attr->next = sn;
                last_attr = sn;
            } else {
                result->attrs = last_attr = sn;
            }
            break;
        case XMLC_NODE:
            // append to children
            if (last_child) {
                last_child->next = sn;
                last_child = sn;
            } else {
                result->children = last_child = sn;
            }
            break;
        case XMLC_SPLICE:
            // splice children of node to children
            if (last_child) {
                last_child->next = sn->children;
            } else {
                result->children = last_child = sn->children;
            }
            // advance to last child
            while (last_child->next != NULL) {
                last_child = last_child->next;
            }
            break;
        case XMLC_TEXT:
            result->text = sn->text;
            free(sn);
            break;
        }

    }
    va_end(args);

    return result;
}

struct xmlc_node *
xml_int_attr(const char *attr, int val)
{
    struct xmlc_node *result;
    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_ATTR;
    result->tag = attr;
    result->val = tmp_sprintf("%d", val);
    return result;
}

struct xmlc_node *
xml_hex_attr(const char *attr, int val)
{
    struct xmlc_node *result;
    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_ATTR;
    result->tag = attr;
    result->val = tmp_sprintf("%x", val);
    return result;
}

struct xmlc_node *
xml_float_attr(const char *attr, float val)
{
    struct xmlc_node *result;
    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_ATTR;
    result->tag = attr;
    result->val = tmp_sprintf("%f", val);
    return result;
}

struct xmlc_node *
xml_str_attr(const char *attr, const char *val)
{
    struct xmlc_node *result;

    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_ATTR;
    result->tag = attr;
    result->val = val;
    return result;
}

struct xmlc_node *
xml_bool_attr(const char *attr, bool val)
{
    struct xmlc_node *result;

    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_ATTR;
    result->tag = attr;
    result->val = val ? "yes":"no";
    return result;
}

struct xmlc_node *
xml_text(const char *text)
{
    if (!text) {
        return xml_null_node();
    }

    struct xmlc_node *result;
    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_TEXT;
    result->text = tmp_gsub(text, "\r", "");
    return result;
}

struct xmlc_node *
xml_if(bool condition, struct xmlc_node *node)
{
    if (condition) {
        return node;
    }
    return xml_null_node();
}

struct xmlc_node *
xml_splice(struct xmlc_node *node)
{
    struct xmlc_node *result;

    CREATE(result, struct xmlc_node, 1);
    result->kind = XMLC_SPLICE;
    result->children = node;
    return result;
}

xmlNodePtr
xmlc_to_node(struct xmlc_node *cn)
{
    xmlNodePtr n = xmlNewNode(NULL, (const xmlChar *)cn->tag);

    for (struct xmlc_node *sn = cn->attrs;sn;sn = sn->next) {
        if (sn->tag) {
            xmlSetProp(n, (const xmlChar *)sn->tag, (const xmlChar *)sn->val);
        }
    }
    for (struct xmlc_node *sn = cn->children;sn;sn = sn->next) {
        if (sn->tag) {
            xmlAddChild(n, xmlc_to_node(sn));
        }
    }
    if (cn->text) {
        xmlNodeAddContent(n, (const xmlChar *)cn->text);
    }

    return n;
}

// Convert our xmlc_node tree into a libxml2 xmlDoc.
xmlDocPtr
xml_create_doc(struct xmlc_node *root)
{
    xmlDocPtr doc = xmlNewDoc((xmlChar *)"1.0");
    xmlNodePtr node = xmlc_to_node(root);

    xmlDocSetRootElement(doc, node);

    return doc;
}

int
xml_output(const char *path, struct xmlc_node *root)
{
    xmlDocPtr doc = xml_create_doc(root);
    const char *tmp_path = tmp_sprintf("%s.tmp", path);
    FILE *ouf = fopen(tmp_path, "w");

    if (!ouf) {
        errlog("Unable to open XML file for save.[%s] (%s)",
               path, strerror(errno));
        xmlFreeDoc(doc);
        return errno;
    }
    if (xmlDocFormatDump(ouf, doc, true) < 0) {
        errlog("Error while writing XML file [%s] (%s)", path, strerror(errno));
        fclose(ouf);
        unlink(tmp_path);
        xmlFreeDoc(doc);
        return errno;
    }

    if (fclose(ouf) < 0) {
        errlog("Error while closing XML file [%s] (%s)", path, strerror(errno));
        unlink(tmp_path);
        xmlFreeDoc(doc);
        return errno;
    }

    rename(tmp_path, path);
    xmlFreeDoc(doc);
    return 0;
}
