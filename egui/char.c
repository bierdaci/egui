#include "event.h"
#include "char.h"

static void char_init_orders(eGeneType, ePointer);
static void char_init_gene(eGeneType);
static eint char_keydown(eHandle, GalEventKey *ent);

esig_t char_signal_char = 0;

eGeneType egui_genetype_char(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiCharOrders),
			char_init_orders,
			0, NULL, NULL,
			char_init_gene,
		};

		gtype = e_register_genetype(&info, GTYPE_EVENT, NULL);
	}
	return gtype;
}

static void char_init_orders(eGeneType new, ePointer this)
{
	GuiEventOrders *orders = e_genetype_orders(new, GTYPE_EVENT);
	orders->keydown = char_keydown;
}

static void char_init_gene(eGeneType new)
{
	char_signal_char = e_signal_new("char",
			new,
			STRUCT_OFFSET(GuiCharOrders, achar),
			efalse, 0, "%p");
}

static euchar char_trans(GalKeyCode code)
{
	if (code > 0 && code <= GAL_KC_asciitilde)
		return (euchar)code;
	return 0;
}

static eint char_keydown(eHandle hobj, GalEventKey *ent)
{
	euchar c = char_trans(ent->code);
	if (c)
		e_signal_emit(hobj, SIG_CHAR, c);
	return -1;
}

#if 0
	switch (code) {
		case GAL_KC_space:			c = ' '; break;
		case GAL_KC_exclam:			c = '!'; break;
		case GAL_KC_quotedbl:		c = '"'; break;
		case GAL_KC_numbersign:		c = '#'; break;
		case GAL_KC_dollar:			c = '$'; break;
		case GAL_KC_percent:		c = '%'; break;
		case GAL_KC_ampersand:		c = '&'; break;
		case GAL_KC_apostrophe:		c = '\''; break;
		case GAL_KC_Tab:			c = '\t'; break;
		case GAL_KC_Enter:			c = '\n'; break;
		case GAL_KC_BackSpace:		c = '\b'; break;
		case GAL_KC_parenleft:		c = '('; break;
		case GAL_KC_parenright:		c = ')'; break;
		case GAL_KC_asterisk:		c = '*'; break;
		case GAL_KC_plus:			c = '+'; break;
		case GAL_KC_comma:			c = ','; break;
		case GAL_KC_minus:			c = '-'; break;
		case GAL_KC_period:			c = '.'; break;
		case GAL_KC_slash:			c = '/'; break;
		case GAL_KC_0:				c = '0'; break;
		case GAL_KC_1:				c = '1'; break;
		case GAL_KC_2:				c = '2'; break;
		case GAL_KC_3:				c = '3'; break;
		case GAL_KC_4:				c = '4'; break;
		case GAL_KC_5:				c = '5'; break;
		case GAL_KC_6:				c = '6'; break;
		case GAL_KC_7:				c = '7'; break;
		case GAL_KC_8:				c = '8'; break;
		case GAL_KC_9:				c = '9'; break;
		case GAL_KC_colon:			c = ':'; break;
		case GAL_KC_semicolon:		c = ';'; break;
		case GAL_KC_less:			c = '<'; break;
		case GAL_KC_equal:			c = '='; break;
		case GAL_KC_greater:		c = '>'; break;
		case GAL_KC_question:		c = '?'; break;
		case GAL_KC_at:				c = '@'; break;
		case GAL_KC_A:				c = 'A'; break;
		case GAL_KC_B:				c = 'B'; break;
		case GAL_KC_C:				c = 'C'; break;
		case GAL_KC_D:				c = 'D'; break;
		case GAL_KC_E:				c = 'E'; break;
		case GAL_KC_F:				c = 'F'; break;
		case GAL_KC_G:				c = 'G'; break;
		case GAL_KC_H:				c = 'H'; break;
		case GAL_KC_I:				c = 'I'; break;
		case GAL_KC_J:				c = 'J'; break;
		case GAL_KC_K:				c = 'K'; break;
		case GAL_KC_L:				c = 'L'; break;
		case GAL_KC_M:				c = 'M'; break;
		case GAL_KC_N:				c = 'N'; break;
		case GAL_KC_O:				c = 'O'; break;
		case GAL_KC_P:				c = 'P'; break;
		case GAL_KC_Q:				c = 'Q'; break;
		case GAL_KC_R:				c = 'R'; break;
		case GAL_KC_S:				c = 'S'; break;
		case GAL_KC_T:				c = 'T'; break;
		case GAL_KC_U:				c = 'U'; break;
		case GAL_KC_V:				c = 'V'; break;
		case GAL_KC_W:				c = 'W'; break;
		case GAL_KC_X:				c = 'X'; break;
		case GAL_KC_Y:				c = 'Y'; break;
		case GAL_KC_Z:				c = 'Z'; break;
		case GAL_KC_bracketleft:	c = '['; break;
		case GAL_KC_backslash:		c = '\\'; break;
		case GAL_KC_bracketright:	c = ']'; break;
		case GAL_KC_asciicircum:	c = '^'; break;
		case GAL_KC_underscore:		c = '_'; break;
		case GAL_KC_grave:			c = '`'; break;
		case GAL_KC_a:				c = 'a'; break;
		case GAL_KC_b:				c = 'b'; break;
		case GAL_KC_c:				c = 'c'; break;
		case GAL_KC_d:				c = 'd'; break;
		case GAL_KC_e:				c = 'e'; break;
		case GAL_KC_f:				c = 'f'; break;
		case GAL_KC_g:				c = 'g'; break;
		case GAL_KC_h:				c = 'h'; break;
		case GAL_KC_i:				c = 'i'; break;
		case GAL_KC_j:				c = 'j'; break;
		case GAL_KC_k:				c = 'k'; break;
		case GAL_KC_l:				c = 'l'; break;
		case GAL_KC_m:				c = 'm'; break;
		case GAL_KC_n:				c = 'n'; break;
		case GAL_KC_o:				c = 'o'; break;
		case GAL_KC_p:				c = 'p'; break;
		case GAL_KC_q:				c = 'q'; break;
		case GAL_KC_r:				c = 'r'; break;
		case GAL_KC_s:				c = 's'; break;
		case GAL_KC_t:				c = 't'; break;
		case GAL_KC_u:				c = 'u'; break;
		case GAL_KC_v:				c = 'v'; break;
		case GAL_KC_w:				c = 'w'; break;
		case GAL_KC_x:				c = 'x'; break;
		case GAL_KC_y:				c = 'y'; break;
		case GAL_KC_z:				c = 'z'; break;
		case GAL_KC_braceleft:		c = '{'; break;
		case GAL_KC_bar:			c = '|'; break;
		case GAL_KC_braceright:		c = '}'; break;
		case GAL_KC_asciitilde:		c = '~'; break;
		default: c = 0;
	}
#endif
