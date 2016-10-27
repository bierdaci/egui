#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "xml.h"

char *strndup(const char *s, size_t n);
static int xml_parse_element(Xml *xml, XmlElementNode *parent, XmlElementNode *elm);
static void release_element(XmlElement *elm);
#define MAX_LINE_BUF		512

#define LOG_MSG(a, b, ...)	fprintf(stderr, __VA_ARGS__)
#define STR_PATTERN			"_-."
#define STRING_SPACE		" \t"
#define STRING_LF			"\n\r"
#define STRING_QUOT			"\'\""
#define STRING_ILLEGAL		"&<"

#if 0
#define IS_ALPHA(c)		(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c)		((c) >= '0' && (c) <= '9')
#else
#define IS_ALPHA(c)		isalpha(c)
#define IS_DIGIT(c)		isdigit(c)
#endif

#define IS_XML_C(c) 	(IS_ALPHA(c) || IS_DIGIT(c) || is_matching(c, STR_PATTERN))
#define IS_SPACE(c)		is_matching(c, STRING_SPACE)
#define IS_BREAK(c)		is_matching(c, " \t\r\n")
#define IS_LF(c)		is_matching(c, "\r\n")
#define IS_QUOT(c)		is_matching(c, STRING_QUOT)
#define IS_ILLEGAL(c)	is_matching(c, STRING_ILLEGAL)

#define TRIM_LF() \
	while (*xml->p && is_matching(*xml->p, "\n")) { \
		xml->p++; xml->row++; xml->column = 1; \
	}

#define TRIM_SPACE() \
	while (is_matching(*xml->p, STRING_SPACE)) { \
		xml->p++; xml->column++; \
	}

#define TRIM_BREAK() \
	do { \
		TRIM_SPACE(); \
		if (is_matching(*xml->p, "\n\r")) { \
			xml_get_line(xml); \
		} \
	} while (0)

#define TRIM(n) \
	do { \
		xml->p += n; xml->column += n; \
	} while (0)

static struct {
	char *word;
	char ch;
} _predefine[5] = {
	{"&lt;",	'<'},
	{"&gt;",	'>'},
	{"&amp;",	'&'},
	{"&apos;",	'\''},
	{"&quot;",	'\"'}
};

static inline ebool is_matching(char c, const char *pattern)
{
	char b;
	int i = 0;

	while ((b = pattern[i++])) {
		if (b == c) 
			return etrue;
	}
	return efalse;
}

static int m_strcmp(char *src, char *dst, const char *br)
{
	char *s = src, *d = dst;

	while (*s && *d && *s == *d) {
		s++; d++;
	}

	if (br && !is_matching(*s, br))
		return 0;
	if (*d == '\0')
		return d - dst;
	return 0;
}

static int match_all(char c)
{
	return 1;
}
static int match_xml_c(char c)
{
	return IS_XML_C(c);
}
static int match_alpha(char c)
{
	return IS_ALPHA(c);
}
static int match_no_quot(char c)
{
	return (c != '"');
}

static char *m_strdup(const char *src, int (*match)(char))
{
	char buf[MAX_LINE_BUF];
	char *p = buf;

	if (!match) match = match_all;

	while (*src && match(*src))
		*p++ = *src++; 

	if (*src) return NULL;

	*p = 0;
	return strdup(buf);
}

static inline void root_add_element(Xml *xml, XmlElement *elm)
{
	if (XML_IS_NODE(elm))
		xml->root = elm;
	list_add_tail(&elm->list, &xml->root_head);
}

static inline void add_element(XmlElementNode *parent, XmlElement *base)
{
	if (base->host)
		list_del(&base->list);
	base->host = (XmlElement *)parent;
	list_add_tail(&base->list, &parent->child);
}

static inline void element_add_declare(XmlElementNode *parent, XmlElementDeclare *elm)
{
	add_element(parent, &elm->base);
}

static inline void element_add_comment(XmlElementNode *elm, XmlElementComment *com)
{
	add_element(elm, &com->base);
}

static inline void element_add_node(XmlElementNode *parent, XmlElementNode *elm)
{
	add_element(parent, &elm->base);
}

static inline void element_add_leaf(XmlElementNode *elm, XmlElementLeaf *eval)
{
	add_element(elm, &eval->base);
}

static inline void node_add_attribute(XmlElement *elm, XmlAttribute *attrib)
{
	XmlElementNode *node = (XmlElementNode *)elm;
	list_add_tail(&attrib->list, &node->attribute);
}

static inline void declare_add_attribute(XmlElement *elm, XmlAttribute *attrib)
{
	list_add_tail(&attrib->list, &((XmlElementDeclare *)elm)->attribute);
}

static inline XmlAttribute *create_attribute(void)
{
	return (XmlAttribute *)calloc(1, sizeof(XmlAttribute));
}

static inline void release_attribute(XmlAttribute *attrib)
{
	if (!attrib) return;
	if (attrib->type) free(attrib->type);
	if (attrib->val) free(attrib->val);
	free(attrib);
}

static XmlElementDeclare *create_element_declare(void)
{
	XmlElementDeclare *elm;
	if (!(elm = calloc(1, sizeof(XmlElementDeclare))))
		return NULL;
	elm->base.type = TypeDeclare;
	return elm;
}

static void release_element_declare(XmlElementDeclare *elm)
{
	if (!elm) return;
	if (elm->string) free(elm->string);
	free(elm);
}

static XmlElementComment *create_element_comment(void)
{
	XmlElementComment *elm;
	if (!(elm = calloc(1, sizeof(XmlElementLeaf))))
		return NULL;
	elm->base.type = TypeComment;
	return elm;
}

static void release_element_comment(XmlElementComment *elm)
{
	if (!elm) return;
	if (elm->comment) free(elm->comment);
	free(elm);
}

static XmlElementLeaf *create_element_leaf(char *val)
{
	XmlElementLeaf *eval;
	if (!(eval = calloc(1, sizeof(XmlElementLeaf))))
		return NULL;
	eval->base.type = TypeLeaf;
	eval->val = val;
	return eval;
}

static void release_element_leaf(XmlElementLeaf *eval)
{
	if (!eval) return;
	if (eval->val) free(eval->val);
	free(eval);
}

static XmlElementNode *create_element_node(void)
{
	XmlElementNode *elm;
	if (!(elm = malloc(sizeof(XmlElementNode))))
		return NULL;
	elm->base.host = NULL;
	elm->base.type = TypeNode;
	INIT_LIST_HEAD(&elm->attribute);
	INIT_LIST_HEAD(&elm->child);
	return elm;
}

static void release_element_node(XmlElementNode *elm)
{
	list_t *pos, *save;

	if (!elm) return;
	if (elm->name) free(elm->name);

	list_for_each_safe(pos, save, &elm->attribute) {
		list_del(pos);
		release_attribute(list_entry(pos, XmlAttribute, list));
	}

	list_for_each_safe(pos, save, &elm->child) {
		XmlElement *base = list_entry(pos, XmlElement, list);
		release_element(base);
	}
	free(elm);
}

int xml_del_element(XmlElement *elm)
{
	if (!elm || !elm->host)
		return -1;
	list_del(&elm->list);
	elm->host = NULL;
	return 0;
}

static void release_element(XmlElement *elm)
{
	if (!elm) return;

	xml_del_element(elm);

	if (XML_IS_LEAF(elm))
		release_element_leaf((XmlElementLeaf *)elm);
	else if (XML_IS_NODE(elm))
		release_element_node((XmlElementNode *)elm);
	else if (XML_IS_COMMENT(elm))
		release_element_comment((XmlElementComment *)elm);
	else if (XML_IS_DECLARE(elm))
		release_element_declare((XmlElementDeclare *)elm);
}

static int xml_get_line(Xml *xml)
{
	if (!(xml->p = fgets(xml->buf, xml->blen, xml->fp)))
		return -1;

	xml->row++;
	xml->column = 1;
	TRIM_BREAK();
	if (!xml->p) return -1;
	return 0;
}

static char *xml_get_value(Xml *xml, char quot)
{
	char buf[MAX_LINE_BUF];
	char *p = buf;

	while (*xml->p != quot) {
		switch (*xml->p)
		{
			case '<': return NULL;
			case '&':
			{
				int i, n;
				for (i = 0; i < 5; i++) {
					if ((n = m_strcmp(xml->p, _predefine[i].word, NULL))) {
						*p++ = _predefine[i].ch;
						TRIM(n);
						break;
					}
				}
				if (i == 5) return NULL;
				continue;
			}
			case '\0':
			{
				if (xml_get_line(xml) < 0)
					return NULL;
				continue;
			}
			default:
			{
				*p++ = *xml->p;
				TRIM(1);
			}
		}
	}

	if (IS_QUOT(*xml->p)) TRIM(1);
	return strndup(buf, p - buf);
}

static int xml_parse_attribute(Xml *xml, XmlElement *elm)
{
	XmlAttribute *attrib = NULL;

	TRIM_BREAK();
	while (xml->p && *xml->p != '/' && *xml->p != '>') {
		char *start, quot;

		if (!IS_ALPHA(*xml->p))
			goto err;

		if (!(attrib = create_attribute()))
			goto err;

		start = xml->p;
		while (IS_XML_C(*xml->p))
			TRIM(1);

		if (!(attrib->type = strndup(start, xml->p - start)))
			goto err;

		TRIM_BREAK();
		if (!xml->p || *xml->p != '=')
			goto err;
		TRIM(1);

		TRIM_BREAK();
		if (!xml->p) goto err;

		quot = *xml->p;
		if (!IS_QUOT(quot)) goto err;
		TRIM(1);

		if (!(attrib->val = xml_get_value(xml, quot)))
			goto err;

		node_add_attribute(elm, attrib);

		TRIM_BREAK();
	}

	if (!xml->p) goto err;

	if (m_strcmp(xml->p, "/>", NULL) == 2) {
		TRIM(2);
		return 1;
	}
	if (*xml->p == '>') {
		TRIM(1);
		return 0;
	}
err:
	release_attribute(attrib);
	return -1;
}

static int xml_parse_node_name(Xml *xml, XmlElementNode *elm)
{
	char *start = xml->p;

	if (!IS_ALPHA(*xml->p) || !strncasecmp(xml->p, "xml", 3))
		return -1;

	while (IS_XML_C(*xml->p)) TRIM(1);

	if (!(elm->name = strndup(start, xml->p - start)))
		return -1;

	return xml_parse_attribute(xml, (XmlElement *)elm);
}

static int xml_parse_element_node(Xml *xml, XmlElementNode *parent)
{
	XmlElementNode *elm;
	int ret;

	if (xml_get_root_node(xml) && !parent)
		return -1;

	if (!(elm = create_element_node()))
		return -1;

	if ((ret = xml_parse_node_name(xml, elm)) < 0)
		goto err;
	else if (ret == 0) {
		if (xml_parse_element(xml, parent, elm) < 0)
			goto err;
	}
	else if (parent)
		element_add_node(parent, elm);
	else
		root_add_element(xml, (XmlElement *)elm);

	return 0;
err:
	release_element_node(elm);
	return -1;
}

static int xml_parse_element_leaf(Xml *xml, XmlElementNode *elm)
{
	char *val;
	XmlElementLeaf *eval;

	if (!(val = xml_get_value(xml, '<')))
		return -1;
	if (!(eval = create_element_leaf(val)))
		return -1;
	element_add_leaf(elm, eval);

	return 0;
}

static int xml_element_node_end(Xml *xml, XmlElementNode *elm)
{
	int n;

	if (!(n = m_strcmp(xml->p, elm->name, NULL)))
		return -1;
	TRIM(n);
	TRIM_BREAK();
	if (*xml->p != '>')
		return -1;
	TRIM(1);

	return 0;
}

static char *xml_parse_attribute_value(Xml *xml)
{
	char quot;
	char *start;

	TRIM_BREAK();
	if (!xml->p || !m_strcmp(xml->p, "=", NULL))
		return NULL;
	TRIM(1);
	TRIM_BREAK();
	if (!xml->p || !IS_QUOT(*xml->p))
		return NULL;
	quot = *xml->p;
	TRIM(1);

	start = xml->p;
	while (IS_XML_C(*xml->p))
		TRIM(1);
	if (*xml->p != quot)
		return NULL;
	TRIM(1);

	return strndup(start, xml->p - start -1);
}

static int xml_parse_main_declare(Xml *xml)
{
	int n;
	char quot;

	TRIM_BREAK();
	if (!xml->p || !(n = m_strcmp(xml->p, "version", " \t\n\r=")))
		return -1;
	TRIM(n);
	TRIM_BREAK();
	if (!xml->p || !m_strcmp(xml->p, "=", NULL))
		return -1;
	TRIM(1);
	TRIM_BREAK();
	if (!xml->p || !IS_QUOT(*xml->p))
		return -1;
	quot = *xml->p;
	TRIM(1);
	if (!(n = m_strcmp(xml->p, xml->version, NULL)))
		return -1;
	TRIM(n);
	if (*xml->p != quot)
		return -1;
	TRIM(1);
	TRIM_BREAK();
	if (!xml->p) return -1;
	if ((n = m_strcmp(xml->p, "encoding", " \t\n\r="))) {
		TRIM(n);
		if (!(xml->encoding = xml_parse_attribute_value(xml)))
			return -1;
	}
	TRIM_BREAK();
	if (!xml->p) return -1;
	if ((n = m_strcmp(xml->p, "standalone", " \t\n\r="))) {
		TRIM(n);
		if (!(xml->standalone = xml_parse_attribute_value(xml)))
			return -1;
		if (strcmp(xml->standalone, "yes") && strcmp(xml->standalone, "no")) {
			free(xml->standalone);
			return -1;
		}
	}

	TRIM_BREAK();
	if (!xml->p || !m_strcmp(xml->p, "?>", NULL))
		return -1;
	TRIM(2);

	return 0;
}

static int xml_parse_question(Xml *xml, XmlElement *parent)
{
	XmlElementDeclare *elm;
	char buf[MAX_LINE_BUF], *p = buf;

	if (!m_strcmp(xml->p, "xml", NULL))
		return -1;

	if (IS_BREAK(xml->p[3])) {
		TRIM(3);
		return xml_parse_main_declare(xml);
	}

	while (xml->p[0]) {
		if (m_strcmp(xml->p, "?>", NULL) == 2) {
			TRIM(2);
			break;
		}
		*p++ = xml->p[0];
		TRIM(1);
		if (!xml->p[0] && (xml_get_line(xml) < 0))
			return -1;
	}

	if (!(elm = create_element_declare()))
		return -1;

	if (!(elm->string = strndup(buf, p - buf)))
		goto err;

	if (parent)
		element_add_declare((XmlElementNode *)parent, elm);
	else
		root_add_element(xml, (XmlElement *)elm);

	return 0;
err:
	release_element_declare(elm);
	return -1;
}

static int xml_parse_exclamation(Xml *xml, XmlElementNode *parent)
{
	XmlElementComment *com;
	char buf[MAX_LINE_BUF];
	char *p = buf;

	if (*xml->p != '-') {
		//other process
		return -1;
	}

	if (!m_strcmp(xml->p, "--", NULL))
		return -1;

	TRIM(2);
	if (!*xml->p) xml_get_line(xml);
	if (!xml->p) return -1;
	while (!m_strcmp(xml->p, "--", NULL)) {
		*p++ = *xml->p;
		TRIM(1);
		if (!*xml->p) xml_get_line(xml);
		if (!xml->p) return -1;
	}
	TRIM(2);

	if (*xml->p != '>')
		return -1;

	TRIM(1);

	if (!(com = create_element_comment()))
		return -1;

	if (!(com->comment = strndup(buf, p - buf))) {
		free(com);
		return -1;
	}

	if (parent)
		element_add_comment(parent, com);
	else
		root_add_element(xml, (XmlElement *)com);

	return 0;
}

static int xml_parse_element(Xml *xml, XmlElementNode *parent, XmlElementNode *elm)
{
	TRIM_BREAK();

	while (xml->p) {
		if (*xml->p == '<') {
			TRIM(1);

			if (*xml->p == '/') {
				TRIM(1);
				if (xml_element_node_end(xml, elm) < 0)
					break;

				if (parent)
					element_add_node(parent, elm);
				else
					root_add_element(xml, (XmlElement *)elm);

				return 0;
			}

			if (*xml->p == '?') {
				TRIM(1);
				if (xml_parse_question(xml, (XmlElement *)elm) < 0)
					break;
			}
			else if (*xml->p == '!') {
				TRIM(1);
				if (xml_parse_exclamation(xml, elm) < 0)
					break;
			}
			else if (xml_parse_element_node(xml, elm) < 0)
				break;
		}
		else if (!elm) {
			TRIM_BREAK();
			if (xml->p) break;
			return 0;
		}
		else if ((xml_parse_element_leaf(xml, elm) < 0))
			break;

		TRIM_BREAK();
	}

	if (!elm && !xml->p) return 0;
	return -1;
}

int xml_load(Xml *xml, const char *path)
{
	int ret = -1;

	if (!(xml->fp = fopen(path, "r"))) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	if (!(xml->buf = calloc(1, xml->blen))) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	if (xml_get_line(xml) < 0) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	if (xml_parse_element(xml, NULL, NULL) < 0) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	ret = 0;
err:
	if (xml->fp) {
		fclose(xml->fp);
		xml->fp = NULL;
	}
	if (xml->buf) {
		free(xml->buf);
		xml->buf = NULL;
	}

	return ret;
}

static void xml_write_val(Xml *xml, char *val)
{
	char *p = val;

	while (*p) {
		int i;
		for (i = 0; i < 5; i++) {
			if (*p == _predefine[i].ch) {
				fprintf(xml->fp, "%s", _predefine[i].word);
				break;
			}
		}
		if (i == 5) fwrite(p, 1, 1, xml->fp);
		p++;
	}
}

static void xml_write_leaf(Xml *xml, XmlElement *leaf)
{
	xml_write_val(xml, xml_get_leaf_val(leaf));
}

static void xml_write_declare(Xml *xml, XmlElement *elm)
{
	XmlElementDeclare *dec = (XmlElementDeclare *)elm;
	fprintf(xml->fp, "<?%s?>", dec->string);
}

static void xml_write_comment(Xml *xml, XmlElement *elm)
{
	XmlElementComment *com = (XmlElementComment *)elm;
	fprintf(xml->fp, "<!--%s-->", com->comment);
}

static void xml_write_attribute(Xml *xml, XmlAttribute *attrib)
{
	fprintf(xml->fp, " %s=\"", xml_get_attribute_type(attrib));
	xml_write_val(xml, xml_get_attribute_val(attrib));
	fprintf(xml->fp, "\"");
}

static int xml_write_node(Xml *xml, XmlElement *node)
{
	XmlAttribute *attrib = NULL;
	int ret;

	fprintf(xml->fp, "<%s", xml_get_node_name(node));

	while ((attrib = xml_next_attribute(node, attrib)))
		xml_write_attribute(xml, attrib);

	if (!xml_next_element(xml, node, NULL)) {
		fprintf(xml->fp, "/>\n");
		ret = 0;
	}
	else {
		fprintf(xml->fp, ">");
		ret = 1;
	}
	return ret;
}

static int xml_save_node(Xml *xml, XmlElement *parent, int step)
{
	int i; 
	int ret = 0;
	ebool is_lf = etrue;
	XmlElement *elm = NULL;

	while ((elm = xml_next_element(xml, parent, elm))) {
		if (XML_IS_NODE(elm)) {
			int b;
			if (is_lf) {
				fprintf(xml->fp, "\n");
				is_lf = efalse;
			}
			for (i = 0; i < step; i++) fprintf(xml->fp, "\t");
			if (xml_write_node(xml, elm)) {
				if ((b = xml_save_node(xml, elm, step + 1)) < 0)
					return -1;
				if (!b) {
					for (i = 0; i < step; i++)
						fprintf(xml->fp, "\t");
				}
				fprintf(xml->fp, "</%s>\n", xml_get_node_name(elm));
			}
			ret = 0;
		}
		else if (XML_IS_LEAF(elm)) {
			xml_write_leaf(xml, elm);
			ret = 1;
		}
		else if (XML_IS_COMMENT(elm)) {
			if (is_lf) {
				fprintf(xml->fp, "\n");
				is_lf = efalse;
			}
			for (i = 0; i < step; i++) fprintf(xml->fp, "\t");
			xml_write_comment(xml, elm);
			fprintf(xml->fp, "\n");
			ret = 0;
		}
		else if (XML_IS_DECLARE(elm)) {
			if (is_lf) {
				fprintf(xml->fp, "\n");
				is_lf = efalse;
			}
			for (i = 0; i < step; i++) fprintf(xml->fp, "\t");
			xml_write_declare(xml, elm);
			fprintf(xml->fp, "\n");
			ret = 0;
		}
	}

	return ret;
}

static void write_main_declare(Xml *xml)
{
	if (xml->encoding)
		fprintf(xml->fp, "<?xml version=\"%s\" encoding=\"%s\"?>", xml->version, xml->encoding);
	else
		fprintf(xml->fp, "<?xml version=\"%s\"?>", xml->version);
}

int xml_save(Xml *xml, const char *path)
{
	int ret = -1;

	if (!(xml->fp = fopen(path, "w"))) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	write_main_declare(xml);

	if (xml_save_node(xml, NULL, 0) < 0) {
		xml->_errno = ERROR_ERR;
		goto err;
	}

	ret = 0;
err:
	if (xml->fp) {
		fclose(xml->fp);
		xml->fp = NULL;
	}
	return ret;
}

Xml *xml_create(int maxbuf)
{
	Xml *xml;

	if (!(xml = (Xml *)calloc(1, sizeof(Xml))))
		return NULL;

	xml->blen = maxbuf;
	xml->_errno = ERROR_OK;
	xml->row = 0;
	xml->column = 1;
	xml->root = NULL;
	strcpy(xml->version, XML_VERSION);
	INIT_LIST_HEAD(&xml->root_head);

	return xml;
}

void xml_release(Xml *xml)
{
	list_t *pos, *save;
	list_for_each_safe(pos, save, &xml->root_head) {
		XmlElement *base = list_entry(pos, XmlElement, list);
		release_element(base);
	}

	if (xml->encoding) free(xml->encoding);
	if (xml->standalone) free(xml->standalone);

	free(xml);
}

char *xml_get_declare(XmlElement *elm)
{
	if (XML_IS_DECLARE(elm))
		return ((XmlElementDeclare *)elm)->string;
	return NULL;
}

char *xml_get_node_name(XmlElement *elm)
{
	if (XML_IS_NODE(elm))
		return ((XmlElementNode *)elm)->name;
	return NULL;
}

char *xml_get_comment(XmlElement *elm)
{
	if (XML_IS_COMMENT(elm))
		return ((XmlElementComment *)elm)->comment;
	return NULL;
}

char *xml_get_leaf_val(XmlElement *elm)
{
	if (XML_IS_LEAF(elm))
		return ((XmlElementLeaf *)elm)->val;
	return NULL;
}

char *xml_get_attribute_type(XmlAttribute *attrib)
{
	return attrib->type;
}

char *xml_get_attribute_val(XmlAttribute *attrib)
{
	return attrib->val;
}

char *xml_get_attribute_val_by_type(XmlElement *elm, char *type)
{
	if (XML_IS_NODE(elm)) {
		list_t *pos;
		XmlElementNode *node = (XmlElementNode *)elm;
		list_for_each(pos, &node->attribute) {
			XmlAttribute *attrib = list_entry(pos, XmlAttribute, list);
			if (!strcmp(attrib->type, type))
				return attrib->val;
		}
	}
	return NULL;
}

XmlAttribute *xml_next_attribute(XmlElement *elm, XmlAttribute *cur)
{
	list_t *head, *next;
	XmlElementNode *p;

	if (XML_IS_NODE(elm)) {
		p = (XmlElementNode *)elm;
		head = &p->attribute;
	}
	else
		return NULL;

	if (list_empty(head))
		return NULL;

	if (cur == NULL)
		next = head->next;
	else
		next = cur->list.next;

	if (next == head)
		return NULL;

	return list_entry(next, XmlAttribute, list);
}

XmlAttribute *xml_prev_attribute(XmlElement *elm, XmlAttribute *cur)
{
	list_t *head, *prev;
	XmlElementNode *p;

	if (XML_IS_NODE(elm)) {
		p = (XmlElementNode *)elm;
		head = &p->attribute;
	}
	else
		return NULL;

	if (list_empty(head))
		return NULL;

	if (cur == NULL)
		prev = head->prev;
	else
		prev = cur->list.prev;

	if (prev == head)
		return NULL;

	return list_entry(prev, XmlAttribute, list);
}

XmlElement *xml_next_element(Xml *xml, XmlElement *parent, XmlElement *cur)
{
	list_t *head, *next;

	if (!parent)
		head = &xml->root_head;
	else if (XML_IS_NODE(parent))
		head = &((XmlElementNode *)parent)->child;
	else
		return NULL;

	if (list_empty(head))
		return NULL;

	if (cur == NULL)
		next = head->next;
	else
		next = cur->list.next;

	if (next == head)
		return NULL;

	return list_entry(next, XmlElement, list);
}

XmlElement *xml_prev_element(Xml *xml, XmlElement *parent, XmlElement *cur)
{
	list_t *head, *prev;

	if (!parent)
		head = &xml->root_head;
	else if (XML_IS_NODE(parent))
		head = &((XmlElementNode *)parent)->child;
	else
		return NULL;

	if (list_empty(head))
		return NULL;

	if (cur == NULL)
		prev = head->prev;
	else
		prev = cur->list.prev;

	if (prev == head)
		return NULL;

	return list_entry(prev, XmlElement, list);
}

XmlElement *xml_create_comment(const char *comment)
{
	XmlElementComment *com;

	if (!comment) return NULL;

	if (!(com = create_element_comment()))
		return NULL;

	if (!(com->comment = strdup(comment))) {
		release_element_comment(com);
		com = NULL;
	}

	return (XmlElement *)com;
}

XmlElement *xml_create_node(const char *name)
{
	XmlElementNode *node;

	if (!name) return NULL;

	if (!(node = create_element_node())) 
		return NULL;

	if (!(node->name = m_strdup(name, match_alpha))) {
		release_element_node(node);
		node = NULL;
	}

	return (XmlElement *)node;
}

XmlElement *xml_create_leaf(const char *val)
{
	XmlElementLeaf *leaf;
	char *p;

	if (!val) return NULL;

	if (!(p = strdup(val)))
		return NULL;

	if (!(leaf = create_element_leaf((char *)p)))
		free(p);
	
	return (XmlElement *)leaf;
}

int xml_add_element(XmlElement *parent, XmlElement *child)
{
	if (XML_IS_NODE(parent)) {
		add_element((XmlElementNode *)parent, child);
		return 0;
	}
	return -1;
}

static int set_val(char **p, const char *val, int (*match)(char))
{
	char *t = m_strdup(val, match);
	if (!t)
		return -1;
	if (*p) free(*p);
	*p = t;
	return 0;
}

int xml_set_encoding(Xml *xml, const char *encoding)
{
	return set_val(&xml->encoding, encoding, match_xml_c);
}

int xml_set_leaf(XmlElement *elm, const char *val)
{
	if (XML_IS_LEAF(elm)) {
		XmlElementLeaf *leaf = (XmlElementLeaf *)elm;
		return set_val(&leaf->val, val, match_all);
	}
	return -1;
}

static XmlAttribute *find_attribute(XmlElement *elm, const char *type)
{
	if (XML_IS_NODE(elm)) {
		XmlElementNode *node = (XmlElementNode *)elm;
		list_t *pos;

		list_for_each(pos, &node->attribute) {
			XmlAttribute *attrib = list_entry(pos, XmlAttribute, list);
			if (!strcmp(attrib->type, type))
				return attrib;
		}
	}

	return NULL;
}

static XmlAttribute* xml_add_attribute(XmlElement *elm, const char *type, const char *val)
{
	XmlAttribute *attrib;

	attrib = create_attribute();

	if (!IS_ALPHA(*type))
		goto err;
	if (set_val(&attrib->type, type, match_xml_c) < 0)
		goto err;

	if (set_val(&attrib->val, val, match_no_quot) < 0)
		goto err;

	node_add_attribute(elm, attrib);
	return attrib;
err:
	release_attribute(attrib);
	return NULL;
}

int xml_set_attribute(XmlElement *node, const char *type, const char *val)
{
	XmlAttribute *attrib;

	if (!XML_IS_NODE(node))
		return -1;

	if (!(attrib = find_attribute(node, type)))
		attrib = xml_add_attribute(node, type, val);

	if (attrib)
		return set_val(&attrib->val, val, match_no_quot);
	return -1;
}

void xml_del_attribute(XmlElement *node, const char *type)
{
	XmlAttribute *attrib = find_attribute(node, type);
	list_del(&attrib->list);
	release_attribute(attrib);
}

int xml_set_node_name(XmlElement *elm, const char *name)
{
	XmlElementNode *node = (XmlElementNode *)elm;
	if (!XML_IS_NODE(elm))
		return -1;

	if (set_val(&node->name, name, match_alpha) < 0)
		return -1;

	return 0;
}

static XmlElementNode *find_node(XmlElementNode *parent, const char *name)
{
	list_t *pos;

	list_for_each(pos, &parent->child) {
		XmlElement *elm = list_entry(pos, XmlElement, list);
		if (XML_IS_NODE(elm)) {
			XmlElementNode *node = (XmlElementNode *)elm;
			if (!strcmp(node->name, name))
				return node;
		}
	}

	return NULL;
}

XmlElement *xml_find_node_by_name(XmlElement *parent, const char *name)
{
	if (!XML_IS_NODE(parent))
		return NULL;

	return (XmlElement *)find_node((XmlElementNode *)parent, name);
}

XmlElement *xml_find_node_by_attribute(XmlElement *parent, const char *name, const char *type, const char *val)
{
	XmlElementNode *node;
	list_t *pos;

	if (!XML_IS_NODE(parent))
		return NULL;

	node = (XmlElementNode *)parent;

	list_for_each(pos, &node->child) {
		XmlElement *elm = list_entry(pos, XmlElement, list);
		if (XML_IS_NODE(elm)) {
			XmlAttribute *attrib = find_attribute(elm, type);
			if (attrib && !strcmp(attrib->val, val))
				return elm;
		}
	}

	return NULL;
}

XmlElement *xml_next_leaf(Xml *xml, XmlElement *node, XmlElement *leaf)
{
	XmlElement *cur = leaf;

	while ((cur = xml_next_element(xml, node, cur))) {
		if (XML_IS_LEAF(cur))
			return cur;
	}

	return NULL;
}

const char *xml_get_val_by_node_name(Xml *xml, XmlElement *parent, const char *name, char *buf, int size)
{
	XmlElement *node;

	if ((node = xml_find_node_by_name(parent, name))) {
		XmlElement *leaf = xml_next_leaf(xml, node, NULL);
		if (leaf != NULL) {
			strncpy(buf, xml_get_leaf_val(leaf), size);
			return buf;
		}
	}
	return NULL;
}

XmlElement *xml_next_node_by_name(Xml *xml, XmlElement *parent, XmlElement *elm, const char *name)
{
	XmlElement *cur = elm;

	while ((cur = xml_next_element(xml, parent, cur))) {
		XmlElementNode *node;

		if (!XML_IS_NODE(cur))
			continue;

		node = (XmlElementNode *)cur;
		if (!strcmp(node->name, name))
			return cur;
	}

	return NULL;
}

XmlElement *xml_next_node_by_attribute(Xml *xml, XmlElement *parent, XmlElement *elm,
						const char *name, const char *type, const char *val)
{
	XmlElement *cur = elm;

	while ((cur = xml_next_element(xml, parent, cur))) {
		if (!XML_IS_NODE(cur))
			continue;

		if (!strcmp(((XmlElementNode *)cur)->name, name)) {
			XmlAttribute *attrib = find_attribute(cur, type);
			if (attrib && !strcmp(attrib->val, val))
				return cur;
		}
	}

	return NULL;
}

int xml_del_node_by_name(XmlElement *parent, const char *name)
{
	XmlElementNode *node;

	if (!XML_IS_NODE(parent))
		return -1;

	if (!(node = find_node((XmlElementNode *)parent, name)))
		return -1;

	xml_del_element((XmlElement *)node);
	release_element_node(node);
	return 0;
}

int xml_del_node_by_attribute(XmlElement *parent, const char *name, const char *type, const char *val)
{
	XmlElementNode *node;
	list_t *pos;

	if (!XML_IS_NODE(parent))
		return -1;

	node = (XmlElementNode *)parent;

	list_for_each(pos, &node->child) {
		XmlElement *elm = list_entry(pos, XmlElement, list);
		if (XML_IS_NODE(elm)) {
			XmlAttribute *attrib = find_attribute(elm, type);
			if (!strcmp(attrib->type, type) && !strcmp(attrib->val, val)) {
				xml_del_element((XmlElement *)node);
				release_element_node(node);
			}
		}
	}

	return -1;
}

int xml_root_add_element(Xml *xml, XmlElement *node)
{
	if (XML_IS_LEAF(node) || (XML_IS_NODE(node) && xml_get_root_node(xml)))
		return -1;

	root_add_element(xml, node);
	return 0;
}

void xml_release_element(XmlElement *elm)
{
	release_element(elm);
}

