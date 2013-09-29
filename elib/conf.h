#ifndef _ELIB_CONF_H
#define _ELIB_CONF_H

#include "std.h"
#include "list.h"

#define PATTERN_STRING		"ABCDEFGHIFKLMNOPQRSTUVWXYZabcdefghifklmnopqrstuvwxyz1234567890"
#define DIGIT_STRING		"1234567890"
#define ASCII_STRING		"ABCDEFGHIFKLMNOPQRSTUVWXYZabcdefghifklmnopqrstuvwxyz"
#define SEC_SIZE			50
#define VAR_SIZE			50
#define VAL_SIZE			100

#define IS_SECTION(s)		((s[0] == '[') ? 1 : 0)
#define IS_HIDE(s)			((s[0] == '#') ? 1 : 0)

typedef struct {
	echar var[VAR_SIZE];
	echar val[VAL_SIZE];
	list_t list;
} eConfigVar;

typedef struct {
	echar sec[SEC_SIZE];
	eint count;
	list_t head;
	list_t list;
} eConfigSec;

typedef struct {
	eint count;
	list_t head;
	list_t h_list;
} eConfig;

eConfig *e_conf_new    (void);
eConfig *e_conf_open   (const echar *);
void     e_conf_destroy(eConfig *);
void     e_conf_empty  (eConfig *);
eint     e_conf_save   (eConfig *, const echar *);
eint     e_conf_set    (eConfig *, const echar *, const echar *, const echar *);
void     e_conf_print  (eConfig *);
const echar *e_conf_get_val(eConfig *, const echar *, const echar *);

#endif

