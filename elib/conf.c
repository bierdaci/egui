#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"

static int pattern_string(const echar *str)
{
#if 0
	int i, j;
	int l = e_strlen(str);

	for (j = 0; j < sizeof(DIGIT_STRING); j++) {
		if (str[0] == DIGIT_STRING[j])
			return -1;
	}

	for (j = 0; j < sizeof(ASCII_STRING); j++) {
		if (str[0] == ASCII_STRING[j])
			break;
	}
	if (j == sizeof(ASCII_STRING))
		return -1;

	for (i = 1; i < l; i++) {
		for (j = 0; j < sizeof(PATTERN_STRING); j++) {
			if (str[i] == PATTERN_STRING[j])
				break;
		}
		if (j == sizeof(PATTERN_STRING))
			return -1;
	}
#endif

	return 0;
}

static const echar *get_section(const echar *str)
{
	echar *s = (echar *)str;
	int l = e_strlen(str);

	if (s[0] != '[' || s[l-1] != ']') {
		printf("section format error\n");
		return NULL;
	}
	s++;
	l--;
	s[l-1] = 0;

	if (pattern_string(s) < 0) {
		printf("pattern section error\n");
		return NULL;
	}
	return s;
}

static eConfigSec *add_section(eConfig *conf, const echar *str)
{
	eConfigSec *sec;
	const echar *s;

	if (!(s = get_section(str)))
		return NULL;

	sec = (eConfigSec *)e_malloc(sizeof(eConfigSec));

	sec->count = 0;
	e_strcpy(sec->sec, s);
	INIT_LIST_HEAD(&sec->head);
	list_add_tail(&sec->list, &conf->head);
	conf->count++;

	return sec;
}

static int add_var(eConfigSec *sec, echar *str)
{
	eConfigVar *var;
	echar *c;
	int n;

	if (!(c = e_strchr(str, '=')))
		return -1;

	var = (eConfigVar *)e_malloc(sizeof(eConfigVar));

	*c = '\0';
	n = c - str;
	if (n >= VAR_SIZE - 1) {
		e_strncpy(var->var, str, VAR_SIZE - 1);
		var->var[VAR_SIZE - 1] = 0;
	}
	else
		e_strcpy(var->var, str);

	e_strncpy(var->val, c + 1, VAL_SIZE - 1);

	if (pattern_string(var->var) < 0) {
		e_free(var);
		return -1;
	}

	list_add_tail(&var->list, &sec->head); 
	sec->count++;

	return 0;
}

const echar *e_conf_get_val(eConfig *conf, const echar *sec, const echar *var)
{
	eConfigSec *csec;
	eConfigVar *cvar;
	list_t *s_pos, *v_pos;

	list_for_each(s_pos, &conf->head) {
		csec = list_entry(s_pos, eConfigSec, list);
		if (!e_strcmp(csec->sec, sec)) {
			list_for_each(v_pos, &csec->head) {
				cvar = list_entry(v_pos, eConfigVar, list);
				if (!e_strcmp(cvar->var, var))
					return cvar->val;
			}
		}
	}

	return NULL;
}

void e_conf_empty(eConfig *conf)
{
	eConfigSec *sec;
	eConfigVar *var;
	list_t *pos, *vpos;

	while (!list_empty(&conf->head)) {
		pos = conf->head.next;
		list_del(pos);
		sec = list_entry(pos, eConfigSec, list);
		while (!list_empty(&sec->head)) {
			vpos = sec->head.next;
			list_del(vpos);
			var = list_entry(vpos, eConfigVar, list);
			e_free(var);
		}
		e_free(sec);
	}
}

void e_conf_destroy(eConfig *conf)
{
	e_conf_empty(conf);
	e_free(conf);
}

eConfig *e_conf_open(const echar *file)
{
	FILE *fp;
	echar tmp[200];
	echar *s;
	eConfigSec *cur_sec = NULL;

	eConfig *conf = e_conf_new();

	if (!(fp = fopen((char *)file, "r"))) {
		printf("open file \"%s\" failed\n", file);
		return NULL;
	}

	while ((s = (echar *)fgets((char *)tmp, sizeof(tmp), fp))) {
		while ((*s == ' ' || *s == '\t') && *s != '\n' && s++);

		if (*s == '\n')
			continue;

		s[e_strlen(s)-1] = 0;

		if (IS_SECTION(s)) {
			if (!(cur_sec = add_section(conf, s)))
				goto reterr;
		}
		else if (IS_HIDE(s)) {
			//add_hide_item
		}
		else if (!cur_sec || add_var(cur_sec, s) < 0) {
			goto reterr;
		}
	}

	fclose(fp);
	return conf;

reterr:
	fclose(fp);
	e_conf_destroy(conf);

	return NULL;
}

int e_conf_save(eConfig *conf, const echar *file)
{
	list_t * spos;
	eConfigSec *sec;
	echar buf[100];
	FILE *fp;

	if (!(fp = fopen((char *)file, "w+")))
		return -1;

	list_for_each(spos, &conf->head) {
		list_t * vpos;

		sec = list_entry(spos, eConfigSec, list);
		e_strcpy(buf, _("["));
		e_strcat(buf, sec->sec);
		e_strcat(buf, _("]"));
		e_strcat(buf, _("\n"));
		fwrite(buf, e_strlen(buf), 1, fp);

		list_for_each(vpos, &sec->head) {
			eConfigVar *var;

			var = list_entry(vpos, eConfigVar, list);
			e_strcpy(buf, var->var);
			e_strcat(buf, _("="));
			if (var->val)
				e_strcat(buf, var->val);
			e_strcat(buf, _("\n"));
			fwrite(buf, e_strlen(buf), 1, fp);
		}
	}

	fclose(fp);
	return 0;
}

eConfig* e_conf_new(void)
{
	eConfig *conf;

	conf = e_calloc(1, sizeof(eConfig));
	INIT_LIST_HEAD(&conf->head);
	INIT_LIST_HEAD(&conf->h_list);

	return conf;
}

int e_conf_set(eConfig *conf,
		const echar * sec_name,
		const echar * var_name,
		const echar * val_name)
{
	list_t *pos;
	eConfigSec *sec = NULL;
	eConfigVar *var = NULL;

	if (!sec_name) 
		return -1;
	if (!var_name)
		return -1;

	list_for_each(pos, &conf->head) {
		eConfigSec *ts;
		ts = list_entry(pos, eConfigSec, list);
		if (!e_strcmp(ts->sec, sec_name))
			sec = ts;
	}

	if (!sec) {
		if (!(sec = e_malloc(sizeof(eConfigSec))))
			return -1;
		conf->count++;
		e_strncpy(sec->sec, sec_name, SEC_SIZE);
		INIT_LIST_HEAD(&sec->head);
		list_add_tail(&sec->list, &conf->head);
	}

	list_for_each(pos, &sec->head) {
		eConfigVar *tv;
		tv = list_entry(pos, eConfigVar, list);
		if (!e_strcmp(tv->var, var_name))
			var = tv;
	}

	if (!var) {
		if (!(var = calloc(1, sizeof(eConfigVar))))
			return -1;
		e_strncpy(var->var, var_name, VAR_SIZE);
		list_add_tail(&var->list, &sec->head);
	}

	if (val_name)
		e_strncpy(var->val, val_name, VAL_SIZE);

	return 0;
}

void e_conf_print(eConfig *conf)
{
	eConfigSec *sec;
	eConfigVar *var;
	list_t *s_pos, *v_pos;

	list_for_each(s_pos, &conf->head) {
		sec = list_entry(s_pos, eConfigSec, list);
		printf("[%s]\n", sec->sec);
		list_for_each(v_pos, &sec->head) {
			var = list_entry(v_pos, eConfigVar, list);
			printf("%s=%s\n", var->var, var->val);
		}
	}
}
