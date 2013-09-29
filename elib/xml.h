#ifndef __XML_H
#define __XML_H

#include <elib/types.h>
#include <elib/list.h>

#define 	XML_VERSION			_("1.0")

#define		MAX_VERSION		8
#define		MAX_ENCODING	40
#define		MAX_ID			256
#define		MAX_TYPE		256
#define		MAX_VAR			256
#define		MAX_BUF			1024

typedef enum {TypeLeaf, TypeNode, TypeComment, TypeDeclare} XmlElementType;
typedef enum {ERROR_OK, ERROR_ERR} XmlErrorType;

#define XML_IS_NODE(elm) 		(elm->type == TypeNode)
#define XML_IS_LEAF(elm) 		(elm->type == TypeLeaf)
#define XML_IS_COMMENT(elm)		(elm->type == TypeComment)
#define XML_IS_DECLARE(elm)		(elm->type == TypeDeclare)

typedef struct _XmlAttribute {
	echar *type;
	echar *val;
	list_t list;
} XmlAttribute;

typedef struct _XmlElement {
	XmlElementType type;
	struct _XmlElement *host;
	list_t list;
} XmlElement;

typedef struct _XmlElementDeclare {
	XmlElement base;
	echar *string;
	list_t attribute;
} XmlElementDeclare;

typedef struct _XmlElementComment {
	XmlElement base;
	echar *comment;
} XmlElementComment;

typedef struct _XmlElementLeaf {
	XmlElement base;
	echar *val;
} XmlElementLeaf;

typedef struct _XmlElementNode {
	XmlElement base;
	echar *name;
	list_t attribute;
	list_t child;
} XmlElementNode;

typedef struct _Xml {
	XmlErrorType _errno;
	FILE *fp;
	int row, column;
	echar version[MAX_VERSION];
	echar *encoding;
	echar *standalone;
	XmlElement *root;
	list_t root_head;

	int blen;
	echar *buf, *p;
} Xml;

int xml_load(Xml *xml, const echar *path);
Xml *xml_create(int maxbuf);
void xml_release(Xml *xml);
static inline XmlElement *xml_get_root_node(Xml *xml) {return xml->root;}

XmlElement *xml_create_comment(const echar *comment);
XmlElement *xml_create_node(const echar *name);
XmlElement *xml_create_leaf(const echar *val);
XmlElement *xml_next_element(Xml *xml, XmlElement *parent, XmlElement *elm);
XmlElement *xml_prev_element(Xml *xml, XmlElement *parent, XmlElement *cur);

XmlElement *xml_find_node_by_name(XmlElement *parent, const echar *name);
XmlElement *xml_find_node_by_attribute(XmlElement *parent, const echar *name, const echar *type, const echar *val);
XmlElement *xml_next_node_by_name(Xml *xml, XmlElement *parent, XmlElement *elm, const echar *name);
XmlElement *xml_next_node_by_attribute(Xml *xml, XmlElement *parent, XmlElement *elm, const echar *name, const echar *type, const echar *val);

XmlAttribute *xml_next_attribute(XmlElement *elm, XmlAttribute *cur);
XmlAttribute *xml_prev_attribute(XmlElement *elm, XmlAttribute *cur);

XmlElement *xml_next_leaf(Xml *xml, XmlElement *node, XmlElement *leaf);
const echar *xml_get_val_by_node_name(Xml *xml, XmlElement *parent, const echar *name, echar *buf, int size);

echar *xml_get_node_name(XmlElement *elm);
echar *xml_get_declare(XmlElement *elm);
echar *xml_get_comment(XmlElement *elm);
echar *xml_get_leaf_val(XmlElement *elm);
echar *xml_get_attribute_type(XmlAttribute *ppy);
echar *xml_get_attribute_val(XmlAttribute *ppy);
echar *xml_get_attribute_val_by_type(XmlElement *elm, echar *type);

int xml_root_add_element(Xml *xml, XmlElement *node);
int xml_add_element(XmlElement *parent, XmlElement *child);
int xml_set_leaf(XmlElement *elm, const echar *val);
int xml_set_node_name(XmlElement *elm, const echar *name);
int xml_del_element(XmlElement *elm);
int xml_del_node_by_name(XmlElement *parent, const echar *name);
int xml_del_node_by_attribute(XmlElement *parent, const echar *name, const echar *type, const echar *val);

int xml_set_encoding(Xml *xml, const echar *encoding);
int xml_set_attribute(XmlElement *node, const echar *type, const echar *val);
void xml_del_attribute(XmlElement *node, const echar *type);

void xml_release_element(XmlElement *elm);
int xml_save(Xml *xml, const echar *path);

#endif

