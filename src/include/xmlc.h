struct xmlc_node {
    enum { XMLC_NULL, XMLC_NODE, XMLC_ATTR, XMLC_TEXT, XMLC_SPLICE } kind;
    struct xmlc_node *next;
    struct xmlc_node *attrs;
    struct xmlc_node *children;
    const char *text;
    const char *tag;            /* NULL tag means ignore node entirely */
    const char *val;
};

struct xmlc_node *xml_node(const char *tag, ...)
    __attribute__((sentinel));
struct xmlc_node *xml_null_node();
struct xmlc_node *xml_append_node(struct xmlc_node *head, struct xmlc_node *node);
struct xmlc_node *xml_int_attr(const char *attr, int val);
struct xmlc_node *xml_hex_attr(const char *attr, int val);
struct xmlc_node *xml_float_attr(const char *attr, float val);
struct xmlc_node *xml_str_attr(const char *attr, const char *val);
struct xmlc_node *xml_bool_attr(const char *attr, bool val);
struct xmlc_node *xml_if(bool condition, struct xmlc_node *node);
struct xmlc_node *xml_text(const char *text);
struct xmlc_node *xml_splice(struct xmlc_node *node);

int xml_output(const char *path, struct xmlc_node *root);
