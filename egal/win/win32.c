#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <egal/egal.h>
#include "win32.h"
#include "xcursors.h"

static GalWindow   w32_wait_event(GalEvent *, bool);
static GalWindow   w32_window_new(GalWindowAttr *);
static GalDrawable w32_drawable_new(eint, eint, bool);
static GalImage   *w32_image_new(eint, eint, bool);
static GalPB w32_pb_new(GalDrawable, GalPBAttr *);
static GalCursor w32_cursor_new(GalCursorType);
static GalCursor w32_cursor_new_name(const echar *);
static GalCursor w32_cursor_new_pixbuf(GalPixbuf *, eint, eint);
static void w32_get_event(GalEvent *gent);
static void w32_image_free(GalImage *);
static void w32_composite(GalDrawable, GalPB, int, int, GalDrawable, GalPB, int, int, int, int);
static void w32_composite_image(GalDrawable, GalPB, int, int, GalImage *, int, int, int, int);
static void w32_draw_drawable(GalDrawable, GalPB, int, int, GalDrawable, GalPB, int, int, int, int);
static bool w32_create_child_window(GalWindow32 *);

static HMODULE hmodule;
static HDC display_hdc;
static HDC comp_hdc;
static DWORD win_version; 
static const char *CLASS_NAME = "TestClass";

static GalWindow      root_window;
static GalWindow32   *w32_root;
static GalWindow32   *top_enter = NULL;
static eTree         *w32_tree;
static e_thread_mutex_t tree_lock;
static e_thread_mutex_t enter_create_lock;
static e_thread_mutex_t wait_create_lock;
static HWND create_hwnd;
static GalWindow w_grab_mouse = 0;
static GalWindow w_grab_key = 0;
static HWND hwnd_cursor = 0;

static struct {
	GalWindow32 *win;
	HWND  hwnd;
	HDC   hdc;
} currentGL = {NULL, 0, 0};

static eint w32_compare_func(eConstPointer a, eConstPointer b)
{
	if ((elong)a > (elong)b) return  1;
	if ((elong)a < (elong)b) return -1;
	return 0;
}

HDC w32_create_compatible_DC(void)
{
	return CreateCompatibleDC(display_hdc);
}

static eint w32_window_get_attr(GalWindow, GalWindowAttr *);
int w32_wmananger_init(GalWindowManager *wm)
{
	GalWindowAttr attributes;
	GalDrawable32 *xdraw;

	//GdiSetBatchLimit(1);

	hmodule = GetModuleHandle(NULL);
	display_hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
	comp_hdc = CreateCompatibleDC(display_hdc);
	win_version = GetVersion();

	//SetWindowsHookEx(WH_GETMESSAGE, WinProc, hmodule, 0);
	//CoInitialize(NULL);

	w32_tree = e_tree_new(w32_compare_func);
	e_thread_mutex_init(&tree_lock, NULL);
	e_thread_mutex_init(&enter_create_lock, NULL);
	e_thread_mutex_init(&wait_create_lock, NULL);
	e_thread_mutex_lock(&wait_create_lock);

	attributes.type = GalWindowRoot;
	root_window     = w32_window_new(&attributes);
	w32_root        = W32_WINDOW_DATA(root_window);
	w32_root->hwnd  = GetDesktopWindow();
	xdraw           = W32_DRAWABLE_DATA(root_window);

	w32_window_get_attr(root_window, &attributes);
	w32_root->w = attributes.width;
	w32_root->h = attributes.height;
	xdraw->w = attributes.width;
	xdraw->h = attributes.height;

	wm->wait_event   = NULL;
	wm->get_event    = w32_get_event;
	wm->window_new   = w32_window_new;
	wm->image_new    = w32_image_new;
	wm->image_free   = w32_image_free;
	wm->drawable_new = w32_drawable_new;
	wm->pb_new       = w32_pb_new;
	wm->create_font  = w32_create_font;
	wm->cursor_new   = w32_cursor_new;
	wm->cursor_new_name   = w32_cursor_new_name;
	wm->cursor_new_pixbuf = w32_cursor_new_pixbuf;

	wm->root_window  = root_window;

	return 0;
}

static GalWindow find_window(HWND hwnd)
{
	GalWindow window;

	e_thread_mutex_lock(&tree_lock);
	window = (GalWindow)e_tree_lookup(w32_tree, (eConstPointer)hwnd);
	e_thread_mutex_unlock(&tree_lock);

	return window;
}

static void check_mouse_leave(void)
{
	HWND hwnd;
	POINT point;
	GalEvent gent;

	GetCursorPos(&point);
	hwnd = WindowFromPoint(point);
	if (hwnd) {
		RECT rc1, rc2, wrc;
		int dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		int dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		GetClientRect(hwnd, &rc1);
		rc2 = rc1;
		AdjustWindowRectEx(&rc2, dwStyle, FALSE, dwExStyle);
		GetWindowRect(hwnd, &wrc);
		wrc.top -= rc2.top;
		wrc.left -= rc2.left;
		wrc.right -= rc2.right - rc1.right;
		wrc.bottom -= rc2.bottom - rc1.bottom;
		if (!PtInRect(&wrc, point))
			hwnd = 0;
	}

	if (top_enter) {
		if (top_enter->hwnd == hwnd)
			return;
		else {
			gent.type = GAL_ET_LEAVE;
			gent.window = OBJECT_OFFSET(top_enter);
			top_enter = NULL;
			egal_add_event_to_queue(&gent);
		}
	}

	if (hwnd) {
		gent.type = GAL_ET_ENTER;
		gent.window = find_window(hwnd);
		if (gent.window) {
			top_enter = W32_WINDOW_DATA(gent.window);
			egal_add_event_to_queue(&gent);
		}
	}
}

static void w32_get_event(GalEvent *gent)
{
	MSG msg;

	check_mouse_leave();
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else {
		//Sleep(1);
		gent->type = GAL_ET_TIMEOUT;
		egal_add_event_to_queue(gent);
	}
}

static void w32_list_add(GalWindow32 *parent, GalWindow32 *child)
{
	e_thread_mutex_lock(&parent->lock);
	list_add_tail(&child->list, &parent->child_head);
	e_thread_mutex_unlock(&parent->lock);
}

static void w32_list_del(GalWindow32 *xwin)
{
	GalWindow32 *parent = xwin->parent;
	if (parent) {
		e_thread_mutex_lock(&parent->lock);
		list_del(&xwin->list);
		e_thread_mutex_unlock(&parent->lock);
	}
}

static GalKeyState build_key_state(WPARAM wParam)
{
	GalKeyState key_state = 0;

	if (GetKeyState(VK_CONTROL) < 0)
		key_state |= GAL_KS_CTRL;
	if (GetKeyState(VK_MENU) < 0)
		key_state |= GAL_KS_ALT;
	if (GetKeyState(VK_SHIFT) < 0)
		key_state |= GAL_KS_SHIFT;
	if (GetKeyState(VK_CAPITAL) & 0x1)
		key_state |= GAL_KS_NUMLOCK;

	return key_state;
}

static GalKeyCode shift_digit(WPARAM wParam)
{
	switch (wParam) {
		case '0': return GAL_KC_parenright;
		case '1': return GAL_KC_exclam;
		case '2': return GAL_KC_at;
		case '3': return GAL_KC_numbersign;
		case '4': return GAL_KC_dollar;  
		case '5': return GAL_KC_percent; 
		case '6': return GAL_KC_asciicircum;
		case '7': return GAL_KC_ampersand; 
		case '8': return GAL_KC_asterisk;
		case '9': return GAL_KC_parenleft;
			return 0;
	}
	return 0;
}

static GalKeyCode shift_symbol(WPARAM wParam)
{
	switch (wParam) {
		case 186: return GAL_KC_colon;
		case 187: return GAL_KC_plus;
		case 188: return GAL_KC_less;
		case 189: return GAL_KC_underscore;
		case 190: return GAL_KC_greater;  
		case 191: return GAL_KC_question; 
		case 192: return GAL_KC_asciitilde;
		case 219: return GAL_KC_braceleft; 
		case 220: return GAL_KC_bar;
		case 221: return GAL_KC_braceright;
		case 222: return GAL_KC_quotedbl;
			return 0;
	}
	return 0;
}

static GalKeyCode noshift_symbol(WPARAM wParam)
{
	switch (wParam) {
		case 186: return GAL_KC_semicolon;
		case 187: return GAL_KC_equal;
		case 188: return GAL_KC_comma;
		case 189: return GAL_KC_minus;
		case 190: return GAL_KC_period;  
		case 191: return GAL_KC_slash; 
		case 192: return GAL_KC_grave;
		case 219: return GAL_KC_bracketleft; 
		case 220: return GAL_KC_backslash;
		case 221: return GAL_KC_bracketright;
		case 222: return GAL_KC_apostrophe;
			return 0;
	}
	return 0;
}

static GalKeyCode translate_keysym(WPARAM wParam)
{
	GalKeyCode keycode = 0;

	if (wParam >= 0x41 && wParam <= 0x5A) {
		if (GetKeyState(VK_CAPITAL) & 0x1) {
			if (GetKeyState(VK_SHIFT) < 0)
				return wParam + 0x20;
			else
				return wParam;
		}
		else {
			if (GetKeyState(VK_SHIFT) < 0)
				return wParam;
			else
				return wParam + 0x20;
		}
	}

	if (wParam >= '0' && wParam <= '9') {
		if (GetKeyState(VK_CAPITAL) & 0x1) {
			if (GetKeyState(VK_SHIFT) < 0)
				return wParam;
			else
				return shift_digit(wParam);
		}
		else {
			if (GetKeyState(VK_SHIFT) < 0)
				return shift_digit(wParam);
			else
				return wParam;
		}
	}

	if (wParam >= 186 && wParam <= 192 || wParam >= 219 && wParam <= 222) {
		if (GetKeyState(VK_CAPITAL) & 0x1) {
			if (GetKeyState(VK_SHIFT) < 0)
				return noshift_symbol(wParam);
			else
				return shift_symbol(wParam);
		}
		else {
			if (GetKeyState(VK_SHIFT) < 0)
				return shift_symbol(wParam);
			else
				return noshift_symbol(wParam);
		}
	}

	switch (wParam)
	{
		case VK_SPACE: keycode = GAL_KC_space; break;
		case VK_RETURN: keycode = GAL_KC_Enter; break;
		case VK_LBUTTON: break;
		case VK_RBUTTON: break;
		case VK_MBUTTON: break;
		case VK_CANCEL: break;
		case VK_BACK: keycode = GAL_KC_BackSpace; break;
		case VK_TAB: keycode  = GAL_KC_Tab; break;
		case VK_CLEAR: break;
		case VK_SHIFT:  keycode  = GAL_KC_Shift_L; break;
		case VK_CONTROL: keycode  = GAL_KC_Control_L; break;
		case VK_MENU: break;
		case VK_PAUSE: break;
		case VK_CAPITAL: break;
		case VK_ESCAPE: keycode  = GAL_KC_Escape; break;
		case VK_PRIOR:
		case VK_NEXT:
		case VK_END: keycode  = GAL_KC_End; break;
		case VK_HOME: keycode  = GAL_KC_Home; break;
		case VK_LEFT: keycode  = GAL_KC_Left; break;
		case VK_UP: keycode  = GAL_KC_Up; break;
		case VK_RIGHT: keycode  = GAL_KC_Right; break;
		case VK_DOWN: keycode  = GAL_KC_Down; break;
		case VK_SELECT: break;
		case VK_PRINT: break;
		case VK_EXECUTE: break;
		case VK_INSERT: keycode = GAL_KC_KP_Insert; break;
		case VK_DELETE: keycode  = GAL_KC_Delete; break;
		case VK_HELP: break;
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9: break;
		case VK_MULTIPLY: break;
		case VK_ADD: break;
		case VK_SEPARATOR: break;
		case VK_SUBTRACT: break;
		case VK_DECIMAL: break;
		case VK_DIVIDE: break;
		case VK_F1: keycode  = GAL_KC_F1; break;
		case VK_F2: keycode  = GAL_KC_F2; break;
		case VK_F3: keycode  = GAL_KC_F3; break;
		case VK_F4: keycode  = GAL_KC_F4; break;
		case VK_F5: keycode  = GAL_KC_F5; break;
		case VK_F6: keycode  = GAL_KC_F6; break;
		case VK_F7: keycode  = GAL_KC_F7; break;
		case VK_F8: keycode  = GAL_KC_F8; break;
		case VK_F9: keycode  = GAL_KC_F9; break;
		case VK_F10: keycode  = GAL_KC_F10; break;
		case VK_F11: keycode  = GAL_KC_F11; break;
		case VK_F12: keycode  = GAL_KC_F12; break;
		default:
				break;
	}
	return keycode;
}

static int mouse_event_proc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GalEventType etype;

	switch (msg) {
	case WM_LBUTTONDOWN:
		etype = GAL_ET_LBUTTONDOWN;
		break;

	case WM_MBUTTONDOWN:
		etype = GAL_ET_MBUTTONDOWN;
		break;

	case WM_RBUTTONDOWN:
		etype = GAL_ET_RBUTTONDOWN;
		break;

	case WM_LBUTTONUP:
		etype = GAL_ET_LBUTTONUP;
		break;

	case WM_MBUTTONUP:
		etype = GAL_ET_MBUTTONUP;
		break;

	case WM_RBUTTONUP:
		etype = GAL_ET_RBUTTONUP;
		break;

	case WM_MOUSEMOVE:
		etype = GAL_ET_MOUSEMOVE;
		break;

	case WM_MOUSEWHEEL:
	{
		short zDelta = (short)HIWORD(wParam);
		if (zDelta > 0)
			etype = GAL_ET_WHEELFORWARD;
		else
			etype = GAL_ET_WHEELBACKWARD;
		break;
	}

	default:
		etype = 0;
		break;
	}

	if (etype > 0) {
		POINT pt;
		GalEvent gent;
		gent.type = etype;
		GetCursorPos(&pt);
		gent.e.mouse.root_x  = pt.x;
		gent.e.mouse.root_y  = pt.y;
		if (msg == WM_MOUSEWHEEL) {
			ScreenToClient(wnd, &pt);
			gent.e.mouse.point.x = pt.x;
			gent.e.mouse.point.y = pt.y;
		}
		else {
			gent.e.mouse.point.x = LOWORD(lParam);
			gent.e.mouse.point.y = HIWORD(lParam);
		}
		gent.e.mouse.state   = build_key_state(wParam);
		if (w_grab_mouse)
			gent.window = w_grab_mouse;
		else
			gent.window = find_window(wnd);
		egal_add_event_to_queue(&gent);
		return 1;
	}

	return 0;
}

static int  CALLBACK WinProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GalEvent gent;

	if (mouse_event_proc(wnd, msg, wParam, lParam))
		return 0;

	switch (msg)
	{
		case WM_INPUTLANGCHANGE:
			break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			break;

		case WM_KEYUP:
			gent.type = GAL_ET_KEYUP;
			gent.e.key.code  = translate_keysym(wParam);
			gent.e.key.state = build_key_state(wParam);
			if (w_grab_key)
				gent.window = w_grab_key;
			else
				gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;

		case WM_KEYDOWN:
			gent.type = GAL_ET_KEYDOWN;
			gent.e.key.code  = translate_keysym(wParam);
			gent.e.key.state = build_key_state(wParam);
			if (w_grab_key)
				gent.window = w_grab_key;
			else
				gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;

		case WM_CHAR:
			break;

		case WM_SYSCHAR:
			break;

		case WM_NCMOUSEMOVE:
			break;

		case WM_QUERYNEWPALETTE:
			break;

		case WM_PALETTECHANGED:
			break;

		case WM_SETFOCUS:
			gent.type = GAL_ET_FOCUS_IN;
			gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;

		case WM_KILLFOCUS:
			gent.type = GAL_ET_FOCUS_OUT;
			gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(wnd, &ps);
			EndPaint(wnd, &ps);
			gent.type = GAL_ET_EXPOSE;
			gent.e.expose.rect.x = ps.rcPaint.left;
			gent.e.expose.rect.y = ps.rcPaint.top;
			gent.e.expose.rect.w = ps.rcPaint.right - ps.rcPaint.left;
			gent.e.expose.rect.h = ps.rcPaint.bottom - ps.rcPaint.top;
			gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;
		}

		case WM_ERASEBKGND:	
			break;

		case WM_SIZE:
			gent.window = find_window(wnd);
			if (gent.window) {
				GalWindow32 *xwin = W32_WINDOW_DATA(gent.window);
				int w = LOWORD(lParam);
				int h = HIWORD(lParam);
				if (xwin->is_configure < 1) {
					xwin->ntype = wParam;
					xwin->is_configure++;
					gent.type = GAL_ET_CONFIGURE;
					gent.e.configure.rect.x = 0;
					gent.e.configure.rect.y = 0;
					gent.e.configure.rect.w = w;
					gent.e.configure.rect.h = h;
					egal_add_event_to_queue(&gent);
				}
				else if (xwin->ntype != wParam) {
					xwin->ntype = wParam;
					if (w > 0 && h > 0) {
						gent.type = GAL_ET_RESIZE;
						gent.e.resize.w = w;
						gent.e.resize.h = h;
						egal_add_event_to_queue(&gent);
					}
				}
			}
		break;

		case WM_SIZING:
			gent.window = find_window(wnd);
			if (gent.window) {
				RECT rect;
				GetClientRect(wnd, &rect);
				gent.type = GAL_ET_RESIZE;
				gent.e.resize.w = rect.right - rect.left + 1;
				gent.e.resize.h = rect.bottom - rect.top + 1;
				egal_add_event_to_queue(&gent);
			}
		break;

		case WM_CLOSE:
			gent.window = find_window(wnd);
			if (gent.window) {
				gent.type = GAL_ET_DESTROY;
				egal_add_event_to_queue(&gent);
			}
		break;

		case WM_GETMINMAXINFO:
		{
			PMINMAXINFO minfo = (PMINMAXINFO)lParam;
			eHandle window = find_window(wnd);
			if (minfo && window) {
				GalWindow32 *xwin = W32_WINDOW_DATA(window);
				minfo->ptMinTrackSize.x = xwin->min_w;
				minfo->ptMinTrackSize.y = xwin->min_h;
			}
			break;
		}

		case WM_IME_COMPOSITION:
		{
			HIMC himc = ImmGetContext(wnd);
			gent.window = find_window(wnd);
			if (gent.window && himc && (lParam & GCS_RESULTSTR)) {
				euchar buf[128];
				int len = ImmGetCompositionString(himc, GCS_RESULTSTR, NULL, 0);
				ImmGetCompositionString(himc, GCS_RESULTSTR, buf, len);
				gent.type = GAL_ET_IME_INPUT;
				while (len  > sizeof(eHandle) * 7) {
					len -= sizeof(eHandle) * 7;
					gent.e.imeinput.len = sizeof(eHandle) * 7;
					memcpy(gent.e.imeinput.data, buf, sizeof(eHandle) * 7);
					egal_add_event_to_queue(&gent);
				}
				gent.e.imeinput.len = len;
				memcpy(gent.e.imeinput.data, buf, len);
				egal_add_event_to_queue(&gent);
			}
			break;
		}
		case WM_SETCURSOR:
			if (wnd == hwnd_cursor)
				break;
			return DefWindowProc(wnd, msg, wParam, lParam);
/*
		case WM_IME_KEYDOWN:
		case WM_INPUTLANGCHANGEREQUEST:
		case WM_IME_SETCONTEXT:
		case WM_IME_STARTCOMPOSITION: 
		case WM_IME_COMPOSITIONFULL:
		case WM_IME_CHAR:
		case WM_IME_CONTROL:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_SELECT:
		case WM_IME_NOTIFY:
		case WM_NCCALCSIZE:
		case WM_GETICON:
		case WM_SETCURSOR:
		case WM_SHOWWINDOW:
		case WM_MOVE:
		case WM_DESTROY:
*/
		case WM_NCACTIVATE:
			if (w_grab_mouse) {
				if (W32_WINDOW_DATA(w_grab_mouse)->hwnd == wnd)
					return 1;
			}

		default:
			return DefWindowProc(wnd, msg, wParam, lParam);
	}

	return 0;
}

static int  CALLBACK WinProcChild(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GalEvent gent;

	switch (msg)
	{
		case WM_SIZE:
			gent.window = find_window(wnd);
			if (gent.window) {
				GalWindow32 *xwin = W32_WINDOW_DATA(gent.window);
				int w = LOWORD(lParam);
				int h = HIWORD(lParam);
				if (!xwin->is_configure) {
					xwin->is_configure = TRUE;
					gent.type = GAL_ET_CONFIGURE;
					gent.e.configure.rect.x = 0;
					gent.e.configure.rect.y = 0;
					gent.e.configure.rect.w = w;
					gent.e.configure.rect.h = h;
					egal_add_event_to_queue(&gent);
				}
			}
			break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(wnd, &ps);
			EndPaint(wnd, &ps);
			gent.type = GAL_ET_EXPOSE;
			gent.e.expose.rect.x = ps.rcPaint.left;
			gent.e.expose.rect.y = ps.rcPaint.top;
			gent.e.expose.rect.w = ps.rcPaint.right - ps.rcPaint.left;
			gent.e.expose.rect.h = ps.rcPaint.bottom - ps.rcPaint.top;
			gent.window = find_window(wnd);
			egal_add_event_to_queue(&gent);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(wnd, msg, wParam, lParam);
	}

	return 0;
}

static ATOM  RegisterGalClass(GalWindowType type)
{
	static ATOM topClass    = 0;
	static ATOM tempClass   = 0;
	static ATOM childClass  = 0;
	static ATOM dialogClass = 0;
	ATOM kclass;
	WNDCLASSEX  wce = {0};

	if (type == GalWindowTop) {
		if (topClass == 0) {
			wce.cbSize = sizeof(wce);
			wce.style  = 0;
			wce.lpfnWndProc = WinProc;
			wce.hInstance = GetModuleHandle(0);
			wce.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_WINLOGO));
			wce.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
			wce.hbrBackground = (HBRUSH)GetStockObject(COLOR_APPWORKSPACE);
			wce.lpszClassName = "GalWindowTop";
			topClass = RegisterClassEx(&wce);
		}
		kclass = topClass;
	}
	else if (type == GalWindowChild) {
		if (childClass == 0) {
			wce.cbSize = sizeof(wce);
			wce.style  = CS_PARENTDC;
			wce.lpfnWndProc = WinProcChild;
			wce.hInstance = GetModuleHandle(0);
			wce.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_WINLOGO));
			wce.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
			wce.hbrBackground = (HBRUSH)GetStockObject(COLOR_APPWORKSPACE);
			wce.lpszClassName = "GalWindowChild";
			childClass = RegisterClassEx(&wce);
		}
		kclass = childClass;
	}
	else if (type == GalWindowDialog) {
		if (dialogClass == 0) {
			wce.cbSize = sizeof(wce);
			wce.style |= CS_SAVEBITS;
			wce.lpfnWndProc = WinProc;
			wce.hInstance = GetModuleHandle(0);
			wce.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_WINLOGO));
			wce.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
			wce.hbrBackground = (HBRUSH)GetStockObject(COLOR_APPWORKSPACE);
			wce.lpszClassName = "GalWindowDialog";
			dialogClass = RegisterClassEx(&wce);
		}
		kclass = dialogClass;
	}
	else if (type == GalWindowTemp) {
		if (tempClass == 0) {
			wce.cbSize = sizeof(wce);
			wce.style |= CS_SAVEBITS;
			wce.lpfnWndProc = WinProc;
			wce.hInstance = GetModuleHandle(0);
			wce.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_WINLOGO));
			wce.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
			wce.hbrBackground = (HBRUSH)GetStockObject(COLOR_APPWORKSPACE);
			wce.lpszClassName = "GalWindowTemp";
			tempClass = RegisterClassEx(&wce);
		}
		kclass = tempClass;
	}
	return kclass;
}

#ifdef _GAL_SUPPORT_OPENGL
static void makeCurrent32(GalWindow32 *win32)
{
	if (win32->hwnd == 0)
		return;
	if (win32->hwnd == currentGL.hwnd)
		return;

	if (win32->hrc == 0) {
		static PIXELFORMATDESCRIPTOR pixelFormateDes;
		static BOOL is_init = FALSE;
		if (!is_init) {
			is_init = TRUE;
			ZeroMemory(&pixelFormateDes,sizeof(pixelFormateDes));
			pixelFormateDes.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pixelFormateDes.nVersion = 1;
			pixelFormateDes.cColorBits = 32;
			pixelFormateDes.cDepthBits = 32;
			pixelFormateDes.iPixelType = PFD_TYPE_RGBA;
			pixelFormateDes.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL| PFD_DOUBLEBUFFER;
		}
		win32->hdc = GetDC(win32->hwnd);
		SetPixelFormat(win32->hdc, ChoosePixelFormat(win32->hdc, &pixelFormateDes), &pixelFormateDes);
		win32->hrc = wglCreateContext(win32->hdc);
	}
	currentGL.hdc  = win32->hdc;
	currentGL.hwnd = win32->hwnd;
	wglMakeCurrent(currentGL.hdc, win32->hrc);
}
#endif

static bool w32_create_window(GalWindow32 *parent, GalWindow32 *child)
{
	GalWindow  window = OBJECT_OFFSET(child);
	eint   depth  = 0;
	DWORD  dwStyle, dwExStyle;
	ATOM kclass;

	if (child->attr.wclass == GAL_INPUT_OUTPUT)
		dwExStyle = 0;
	else if (child->attr.wclass == GAL_INPUT_ONLY)
		dwExStyle = WS_EX_TRANSPARENT;

	if (child->attr.wa_mask & GAL_WA_TOPMOST)
		dwExStyle = WS_EX_TOPMOST;

	switch (child->attr.type)
	{
		case GalWindowTop:
			dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
			break;

		case GalWindowChild:
			dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
			break;

		case GalWindowDialog:
			dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_CLIPCHILDREN;
			break;

		case GalWindowTemp:
			dwStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
			dwExStyle |= WS_EX_TOOLWINDOW;
			break;
	}

	kclass = RegisterGalClass(child->attr.type);

	child->hwnd = CreateWindowEx(dwExStyle,
		    MAKEINTRESOURCE(kclass),
		    "egui",
		    dwStyle,
		    child->x, child->y,
		    child->w, child->h,
		    parent->hwnd,
		    NULL,
		    hmodule,
		    (void *)window);

#ifdef _GAL_SUPPORT_OPENGL
	if (currentGL.win == child)
		makeCurrent32(child);
#endif

	e_thread_mutex_lock(&tree_lock);
	e_tree_insert(w32_tree, (ePointer)child->hwnd, (ePointer)window);
	e_thread_mutex_unlock(&tree_lock);

	if (child->attr.type == GalWindowChild)
		EnableWindow(child->hwnd, FALSE);

	w32_create_child_window(child);

	if (child->attr.visible)
		ShowWindow(child->hwnd, SW_SHOWNORMAL);
	else
		ShowWindow(child->hwnd, SW_HIDE);

	if (child->attr.wclass == GAL_INPUT_OUTPUT) {
		GalDrawable32 *xdraw = W32_DRAWABLE_DATA(window);
		xdraw->w        = child->w;
		xdraw->h        = child->h;
		xdraw->isdev	= true;
		xdraw->handle   = child->hwnd;
		xdraw->depth    = depth;
	}

	return true;
}

static bool w32_create_child_window(GalWindow32 *parent)
{
	list_t *pos;

	list_for_each(pos, &parent->child_head) {
		GalWindow32 *child = list_entry(pos, GalWindow32, list);
		w32_create_window(parent, child);
	}
	return true;
}

static void w32_destory_window(GalWindow32 *xwin)
{
	w32_list_del(xwin);

	e_thread_mutex_lock(&tree_lock);
	e_tree_remove(w32_tree, (ePointer)xwin->hwnd);
	e_thread_mutex_unlock(&tree_lock);
#ifdef _GAL_SUPPORT_OPENGL
	if (xwin->hdc) {
		wglDeleteContext(xwin->hrc);
		ReleaseDC(xwin->hwnd, xwin->hdc);
	}
#endif
	DestroyWindow(xwin->hwnd);
}

static eint w32_window_put(GalWindow win1, GalWindow win2, eint x, eint y)
{
	GalWindow32 *parent = W32_WINDOW_DATA(win1);
	GalWindow32 *child  = W32_WINDOW_DATA(win2);

	if (child->parent)
		return 0;

	child->x = x;
	child->y = y;
	child->parent = parent;
	w32_list_add(parent, child);
	if (parent->hwnd && !child->hwnd)
		w32_create_window(parent, child);
	else if (parent->hwnd && child->hwnd && parent->hwnd != child->hwnd) {
		SetParent(child->hwnd, parent->hwnd);
		MoveWindow(child->hwnd, child->x, child->y, child->w, child->h, TRUE);
	}
	else if (!parent->hwnd && child->hwnd) {
		int ox = child->x;
		int oy = child->y;
		while (parent->parent) {
			ox += parent->x;
			oy += parent->y;
			parent = parent->parent;
			if (parent->hwnd)
				break;
		}
		if (parent->hwnd) {
			SetParent(child->hwnd, parent->hwnd);
			MoveWindow(child->hwnd, ox, oy, child->w, child->h, TRUE);
		}
	}
	return 0;
}

static eint w32_window_show(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	ShowWindow(xwin->hwnd, SW_SHOWNORMAL);
	return 0;
}

static eint w32_window_hide(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	ShowWindow(xwin->hwnd, SW_HIDE);
	return 0;
}

static eint w32_window_move(GalWindow window, eint x, eint y)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (xwin->attr.type == GalWindowChild)
		MoveWindow(xwin->hwnd, x, y, xwin->w, xwin->h, true);
	else {
		RECT rect = {0, 0, xwin->w-1, xwin->h-1};
		int dwStyle = GetWindowLong(xwin->hwnd, GWL_STYLE);
		int dwExStyle = GetWindowLong(xwin->hwnd, GWL_EXSTYLE);
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
		MoveWindow(xwin->hwnd, x, y, rect.right - rect.left + 1, rect.bottom - rect.top + 1, true);
	}
	xwin->x = x;
	xwin->y = y;
	return -1;
}

static eint w32_window_rasie(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	return 0;
}

static eint w32_window_lower(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	return 0;
}

static eint w32_window_remove(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (xwin->hwnd) {
		ShowWindow(xwin->hwnd, SW_HIDE);
		SetParent(xwin->hwnd, w32_root->hwnd);
	}
	return 0;
}

static eint w32_window_configure(GalWindow window, GalRect *rect)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	return 0;
}

static eint w32_window_resize(GalWindow window, eint w, eint h)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (xwin->attr.type == GalWindowChild)
		MoveWindow(xwin->hwnd, xwin->x, xwin->y, w, h, true);
	else {
		POINT pt;
		RECT rect;
		DWORD dwStyle;
		DWORD dwExStyle;

		pt.x = 0;
		pt.y = 0; 
		ClientToScreen(xwin->hwnd, &pt);
		rect.left = pt.x;
		rect.top = pt.y;
		rect.right = pt.x + w - 1;
		rect.bottom = pt.y + h - 1;

		dwStyle = GetWindowLong(xwin->hwnd, GWL_STYLE);
		dwExStyle = GetWindowLong(xwin->hwnd, GWL_EXSTYLE);
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

		SetWindowPos(xwin->hwnd, NULL,
			rect.left, rect.top,
			rect.right - rect.left + 1, rect.bottom - rect.top + 1,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}
	xwin->w = w;
	xwin->h = h;

	return 0;
}

static eint w32_window_move_resize(GalWindow window, eint x, eint y, eint w, eint h)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (xwin->attr.type == GalWindowChild)
		MoveWindow(xwin->hwnd, x, y, w, h, true);
	else {
		DWORD dwStyle;
		DWORD dwExStyle;
		RECT rect;
		rect.left   = x;
		rect.top    = y;
		rect.right  = x + w - 1;
		rect.bottom = y + h - 1;

		dwStyle = GetWindowLong(xwin->hwnd, GWL_STYLE);
		dwExStyle = GetWindowLong(xwin->hwnd, GWL_EXSTYLE);
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

		SetWindowPos(xwin->hwnd, NULL,
			rect.left, rect.top,
			rect.right - rect.left + 1, rect.bottom - rect.top + 1,
			SWP_NOACTIVATE | SWP_NOZORDER);
	}
	xwin->x = x;
	xwin->y = y;
	xwin->w = w;
	xwin->h = h;
	return 0;
}

static void w32_window_get_geometry_hints(GalWindow window, GalGeometry *geometry, GalWindowHints *geom_mask)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
}

static eint w32_get_origin(GalWindow window, eint *x, eint *y)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	POINT pt = {0, 0};
	ClientToScreen(xwin->hwnd, &pt);
	if (x) *x = pt.x;
	if (y) *y = pt.y;
	return 0;
}

static void w32_window_set_geometry_hints(GalWindow window, GalGeometry *geometry, GalWindowHints geom_mask)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	WINDOWPLACEMENT size_hints;

	if (xwin->hwnd == 0)
		return;

	size_hints.flags = 0;

	if (geom_mask & GAL_HINT_MIN_SIZE) {
		RECT rect = {0, 0, geometry->min_width - 1, geometry->min_height - 1};
		int dwStyle  = GetWindowLong(xwin->hwnd, GWL_STYLE);
		int dwExStyle = GetWindowLong(xwin->hwnd, GWL_EXSTYLE);
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
		xwin->min_w  = rect.right - rect.left + 1;
		xwin->min_h = rect.bottom - rect.top + 1;
	}

	if (geom_mask & GAL_HINT_MAX_SIZE) {
	}

	if (geom_mask & GAL_HINT_BASE_SIZE) {
	}
/*
	if (GetWindowPlacement(xwin->hwnd, &size_hints)) {
		size_hints.rcNormalPosition.right = size_hints.rcNormalPosition.left + xwin->min_w;
		size_hints.rcNormalPosition.bottom = size_hints.rcNormalPosition.top + xwin->min_h;
		SetWindowPlacement(xwin->hwnd, &size_hints);
	}
	*/
}

static void w32_warp_pointer(GalWindow window, int sx, int sy, int sw, int sh, int dx, int dy)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
}

static void w32_get_pointer(GalWindow window,
		eint *root_x, eint *root_y,
		eint *win_x, eint *win_y, GalModifierType *mask)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	POINT point;

	GetCursorPos(&point);
	if (root_x) *root_x = point.x;
	if (root_y) *root_y = point.y;
	ScreenToClient(xwin->hwnd, &point);
	if (win_x) *win_x = point.x;
	if (win_y) *win_y = point.y;
}

static GalGrabStatus w32_grab_pointer(GalWindow window, bool owner_events, GalCursor cursor)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (w_grab_mouse) {
		ReleaseCapture();
	}
	while (xwin->parent)
		xwin = xwin->parent;
	SetCapture(xwin->hwnd);
	w_grab_mouse = OBJECT_OFFSET(xwin);

	if (cursor) {
		xwin->cursor = cursor;
		SetCursor(W32_CURSOR_DATA(cursor)->hcursor);
		hwnd_cursor = xwin->hwnd;
	}

	return 0;
}

static GalGrabStatus w32_ungrab_pointer(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (w_grab_mouse) {
		while (xwin->parent)
			xwin = xwin->parent;
		if (w_grab_mouse == OBJECT_OFFSET(xwin)) {
			w_grab_mouse = 0;
			ReleaseCapture();
			if (xwin->cursor) {
				hwnd_cursor = 0;
				xwin->cursor = 0;
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
		}
	}
	return 0;
}

static GalGrabStatus w32_grab_keyboard(GalWindow window, bool owner_events)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	while (xwin->parent)
		xwin = xwin->parent;
	w_grab_key = OBJECT_OFFSET(xwin);
	return 0;
}

static GalGrabStatus w32_ungrab_keyboard(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	if (w_grab_key) {
		while (xwin->parent)
			xwin = xwin->parent;
		if (w_grab_key == OBJECT_OFFSET(xwin))
			w_grab_key = 0;
	}
	return 0;
}

static INLINE bool xpb_init_gc(GalPB32 *xpb)
{
	if (xpb->draw->handle == 0)
		return false;

	if (xpb->hdc == 0) {
		xpb->hdc = GetDC(xpb->draw->handle);
		if (xpb->attr.func >= 0)
			SetROP2(xpb->hdc, xpb->attr.func);
		if (xpb->colorbrush)
			SelectObject(xpb->hdc, xpb->colorbrush);
	}
	return true;
}

static void w32_draw_drawable(GalDrawable dst,
		GalPB dpb, int x, int y,
		GalDrawable src, GalPB spb,
		int sx, int sy, int w, int h)
{
	GalDrawable32 *xsrc = W32_DRAWABLE_DATA(src);
	GalDrawable32 *xdst = W32_DRAWABLE_DATA(dst);
	GalPB32       *xpb  = W32_PB_DATA(dpb);
	eint  src_depth = egal_get_depth(src);
	eint dest_depth = egal_get_depth(dst);
	HDC  hdc;

	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, xdst->handle);
	if (spb)
		hdc = W32_PB_DATA(spb)->hdc;
	else
		hdc = comp_hdc;
	SelectObject(hdc, xsrc->handle);
	BitBlt(xpb->hdc, x, y, w, h, hdc, sx, sy, SRCCOPY);
}

static void w32_composite(GalDrawable dst, GalPB pb1, int dx, int dy,
		GalDrawable src, GalPB pb2, int sx, int sy, int w, int h)
{
	GalDrawable32 *draw1 = W32_DRAWABLE_DATA(dst);
	GalDrawable32 *draw2 = W32_DRAWABLE_DATA(src);
	GalPB32       *xpb1  = W32_PB_DATA(pb1);
	GalPB32       *xpb2  = W32_PB_DATA(pb2);
	BLENDFUNCTION blend;

	if (!xpb_init_gc(xpb1))
		return;

	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = 1;
	SelectObject(xpb1->hdc, draw1->handle);
	SelectObject(xpb2->hdc, draw2->handle);
	AlphaBlend(xpb1->hdc, dx, dy, w, h, xpb2->hdc, sx, sy, w, h, blend);
}

static void w32_composite_image(GalDrawable dst, GalPB pb,
		int dx, int dy,
		GalImage *img, int sx, int sy, int w, int h)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(dst);
	GalPB32       *xpb  = W32_PB_DATA(pb);
	GalImage32    *ximg = (GalImage32 *)img;
	BLENDFUNCTION blend;

	if (!xpb_init_gc(xpb))
		return;

	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = 1;
	SelectObject(xpb->hdc, draw->handle);
	SelectObject(comp_hdc, ximg->hbitmap);
	AlphaBlend(xpb->hdc, dx, dy, w, h, comp_hdc, sx, sy, w, h, blend);
}

static void w32_draw_image(GalDrawable drawable,
		GalPB pb, int x, int y,
		GalImage *gimg, int bx, int by, int w, int h)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
	GalImage32    *ximg = (GalImage32 *)gimg;

	if (!xpb_init_gc(xpb))
		return;

	SelectObject(comp_hdc, ximg->hbitmap);
	SelectObject(xpb->hdc, draw->handle);
	BitBlt(xpb->hdc, x, y, w, h, comp_hdc, bx, by, SRCCOPY);
}

static void w32_draw_point(GalDrawable drawable, GalPB pb, int x, int y)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
}

static void w32_draw_line(GalDrawable drawable, GalPB pb, int x1, int y1, int x2, int y2)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
		
	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, draw->handle);
	SelectObject(xpb->hdc, xpb->pen);
	MoveToEx(xpb->hdc, x1, y1, NULL);
	LineTo(xpb->hdc, x2, y2);
}

static void w32_draw_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
	int x2 = x + w - 1;
	int y2 = y + h - 1;

	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, xpb->pen);
	MoveToEx(xpb->hdc, x, y, NULL);
	LineTo(xpb->hdc, x2, y);
	LineTo(xpb->hdc, x2, y2);
	LineTo(xpb->hdc, x, y2);
	LineTo(xpb->hdc, x, y);
}

static void w32_draw_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);

	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, xpb->pen);
	//Arc();
}

static void w32_fill_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);

	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, draw->handle);
	PatBlt(xpb->hdc, x, y, w, h, PATCOPY);
}

static void w32_fill_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
	
	if (!xpb_init_gc(xpb))
		return;

	SelectObject(xpb->hdc, draw->handle);
	//Pie();
}

/*
static int w32_fill_polygon(GalDrawable drawable, GapPB pb, GalPoint *points, int npoints, int shape, int mode)
{
	GalDrawable32 *draw = W32_DRAWABLE_DATA(drawable);
	GalPB32       *xpb  = W32_PB_DATA(pb);
	XFillPolygon(draw->xid, xpb->gc, points, npoints, shape, mode);
}
*/

static void w32_set_attachmen(GalWindow window, eHandle obj)
{
	W32_WINDOW_DATA(window)->attachmen = obj;
}

static eHandle w32_get_attachmen(GalWindow window)
{
	return W32_WINDOW_DATA(window)->attachmen;
}

static eint w32_window_set_cursor(GalWindow window, GalCursor cursor)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);

	xwin->cursor = cursor;

	if (cursor) {
		SetCursor(W32_CURSOR_DATA(cursor)->hcursor);
		while (xwin->parent)
			xwin = xwin->parent;
		hwnd_cursor = xwin->hwnd;
	}
	else {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		hwnd_cursor = 0;
	}

	return 0;
}

static GalCursor w32_window_get_cursor(GalWindow window)
{
	return W32_WINDOW_DATA(window)->cursor;
}

static eint w32_window_set_attr(GalWindow window, GalWindowAttr *attr)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	return 0;
}

static eint w32_window_get_attr(GalWindow window, GalWindowAttr *attr)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	RECT rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	*attr = xwin->attr;
	attr->x = rect.left;
	attr->y = rect.top;
	attr->width  = rect.right - rect.left + 1;
	attr->height = rect.bottom - rect.top + 1;
	return 0;
}

static euint w32_set_font_color(GalPB pb, euint color)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	int r = (0xff0000 & color) >> 16;
	int g = (0xff00 & color) >> 8;
	int b = (0xff & color);
	euint old = xpb->font_color;
	xpb->font_color = RGB(r, g, b);
	b = (0xff0000 & old) >> 16;
	g = (0xff00 & old) >> 8;
	r = (0xff & old);
	return ((r << 0xff0000) | (g << 0xff00) | b);
}

static euint w32_get_font_color(GalPB pb)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	int b = (0xff0000 & xpb->font_color) >> 16;
	int g = (0xff00 & xpb->font_color) >> 8;
	int r = (0xff & xpb->font_color);
	return ((r << 0xff0000) | (g << 0xff00) | b);
}

static eint w32_set_foreground(GalPB pb, euint color)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	int r = (0xff0000 & color) >> 16;
	int g = (0xff00 & color) >> 8;
	int b = (0xff & color);
	euint old = xpb->attr.foreground;

	xpb->fg_color = RGB(r, g, b);
	xpb->font_color = xpb->fg_color;
	xpb->attr.foreground = color;
	if (xpb->colorbrush)
		DeleteObject(xpb->colorbrush);
	xpb->colorbrush = CreateSolidBrush(RGB(r, g, b));
	if (xpb->pen)
		DeleteObject(xpb->pen);
	xpb->pen = CreatePen(xpb->pen_style, xpb->pen_width, xpb->fg_color);

	SelectObject(xpb->hdc, xpb->colorbrush);
	return old;
}

static eint w32_set_background(GalPB pb, euint color)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	xpb->attr.background = color;
	return -1;
}

static eulong w32_get_foreground(GalPB pb)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	return xpb->attr.foreground;
}

static eulong w32_get_background(GalPB pb)
{
	GalPB32 *xpb = W32_PB_DATA(pb);
	return xpb->attr.background;
}

/*
static int w32_set_font(GalPB *pb, GalFont *font)
{
	GalPB32 *xpb = (GalPB32 *)W32_PB_DATA(pb);
	return XSetFont(w32_dpy, xpb->gc, font);
}
*/


#ifdef _GAL_SUPPORT_OPENGL
static void w32_window_make_GL(GalWindow window)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);
	makeCurrent32(xwin);
	currentGL.win = xwin;
}
#endif

static eint w32_window_set_name(GalWindow window, const echar *name)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(window);

	return -1;
}

static void w32_window_init_orders(eGeneType new, ePointer this)
{
	GalWindowOrders *win = e_genetype_orders(new, GTYPE_GAL_WINDOW);

	win->window_put    = w32_window_put;
	win->window_show   = w32_window_show;
	win->window_hide   = w32_window_hide;
	win->window_move   = w32_window_move;
	win->window_raise  = w32_window_rasie;
	win->window_lower  = w32_window_lower;
	win->window_remove = w32_window_remove;
	win->window_resize = w32_window_resize;
	win->window_configure   = w32_window_configure;
	win->window_move_resize = w32_window_move_resize;
	win->set_geometry_hints = w32_window_set_geometry_hints;
	win->get_geometry_hints = w32_window_get_geometry_hints;

	win->set_attachment = w32_set_attachmen;
	win->get_attachment = w32_get_attachmen;

	win->set_cursor = w32_window_set_cursor;
	win->get_cursor = w32_window_get_cursor;

	win->set_attr = w32_window_set_attr;
	win->get_attr = w32_window_get_attr;

	win->grab_keyboard   = w32_grab_keyboard;
	win->ungrab_keyboard = w32_ungrab_keyboard;

	win->get_origin      = w32_get_origin;
	win->get_pointer     = w32_get_pointer;
	win->grab_pointer    = w32_grab_pointer;
	win->ungrab_pointer  = w32_ungrab_pointer;
	win->warp_pointer    = w32_warp_pointer;
	//win->set_font        = w32_set_font;

	win->set_name        = w32_window_set_name;
#ifdef _GAL_SUPPORT_OPENGL
	win->make_GL         = w32_window_make_GL;
#endif
}

static void w32_window_free(eHandle hobj, ePointer data)
{
	w32_destory_window(W32_WINDOW_DATA(hobj));
}

eGeneType w32_genetype_window(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			w32_window_init_orders,
			sizeof(GalWindow32),
			NULL,
			w32_window_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_WINDOW, w32_genetype_drawable(), NULL);
	}
	return gtype;
}

static eint w32_window_init(eHandle hobj, GalWindowAttr *attr)
{
	GalWindow32 *xwin = W32_WINDOW_DATA(hobj);

	xwin->attr   = *attr;
	xwin->hwnd   = 0;
	xwin->parent = NULL;
	if (xwin->attr.type != GalWindowRoot) {
		xwin->w  = xwin->attr.width;
		xwin->h  = xwin->attr.height;
	}
	INIT_LIST_HEAD(&xwin->child_head);
	e_thread_mutex_init(&xwin->lock, NULL);

	if (xwin->attr.type == GalWindowTop
			|| xwin->attr.type == GalWindowDialog
			|| xwin->attr.type == GalWindowTemp) {
		if (!w32_create_window(w32_root, xwin))
			return -1;
		w32_list_add(w32_root, xwin);
	}
	else {
		W32_DRAWABLE_DATA(hobj)->isdev = true;
	}

	return 0;
}

static GalWindow w32_window_new(GalWindowAttr *attr)
{
	GalWindow window = e_object_new(w32_genetype_window());
	w32_window_init(window, attr);
	return window;
}

static GalImage *w32_image_new(eint w, eint h, bool alpha)
{
	BITMAPINFO bitinfo;
	BITMAP b;
	int bits = alpha ? 32 : 24;

	GalImage32 *ximg = e_malloc(sizeof(GalImage32));
	GalImage   *gimg = (GalImage *)ximg;

	memset(&bitinfo, 0, sizeof(BITMAPINFO));

	bitinfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bitinfo.bmiHeader.biWidth       = w;
	bitinfo.bmiHeader.biHeight      = h;
	bitinfo.bmiHeader.biPlanes      = 1;
	bitinfo.bmiHeader.biBitCount    = bits;
	bitinfo.bmiHeader.biCompression = BI_RGB;
	bitinfo.bmiHeader.biSizeImage   = w * h * bits / 8;
	ximg->hbitmap = CreateDIBSection(display_hdc, &bitinfo, DIB_RGB_COLORS, &gimg->pixels, NULL, 0);
	GetObject(ximg->hbitmap, sizeof(b), &b);

	gimg->w = w;
	gimg->h = h;
	gimg->pixelbytes = b.bmBitsPixel / 8;
	ximg->pb         = 0;
	gimg->alpha      = alpha;
	gimg->depth      = b.bmBitsPixel;
	gimg->rowbytes   = b.bmWidthBytes;
	gimg->negative   = 1;

	return gimg;
}

static void w32_image_free(GalImage *image)
{
	GalImage32 *ximg = (GalImage32 *)image;
	DeleteObject(ximg->hbitmap);
	e_free(ximg);
}

static void w32_bitmap_free(eHandle bitmap, ePointer data)
{
	GalDrawable32 *xdraw = W32_DRAWABLE_DATA(bitmap);
}

eGeneType w32_genetype_bitmap(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			NULL,
			0,
			NULL,
			w32_bitmap_free,
			NULL,
		};

		gtype = e_register_genetype(&info, w32_genetype_drawable(), NULL);
	}
	return gtype;
}

static void w32_create_bitmap(GalDrawable drawable, eint w, eint h, bool alpha)
{
	GalDrawable32 *xdraw = W32_DRAWABLE_DATA(drawable);
	struct {
		BITMAPINFOHEADER bmiHeader;
		union {
			WORD bmiIndices[256];
			DWORD bmiMasks[3];
			RGBQUAD bmiColors[256];
	} u;
	} bmi;

	HWND hwnd;
	HBITMAP hbitmap;
	int depth;

	if (alpha)
		depth = 32;
	else
		depth = 24;

	hwnd = w32_root->hwnd;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth  = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = depth;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter =
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;

	hbitmap = CreateDIBSection(display_hdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, NULL, NULL, 0);

	xdraw->w        = w;
	xdraw->h        = h;
	xdraw->isdev	= false;
	xdraw->handle   = (HWND)hbitmap;
	xdraw->depth    = depth;
}

static GalDrawable w32_drawable_new(eint w, eint h, bool alpha)
{
	GalDrawable drawable = e_object_new(w32_genetype_bitmap());
	w32_create_bitmap(drawable, w, h, alpha);
	return drawable;
}

static int rop_convert(GalPBFunc func)
{
	switch (func) {
		case GalPBclear:		return -1;
		case GalPBand:			return -1;
		case GalPBandReverse:	return -1;
		case GalPBcopy:			return R2_COPYPEN;
		case GalPBandInverted:	return -1;
		case GalPBnoop:			return R2_NOP;
		case GalPBxor:			return R2_XORPEN;
		case GalPBor:			return -1;
		case GalPBnor:			return R2_NOT;
		case GalPBequiv:		return -1;
		case GalPBinvert:		return -1;
		case GalPBorReverse:	return -1;
		case GalPBcopyInverted:	return R2_NOTCOPYPEN;
		case GalPBorInverted:	return -1;
		case GalPBnand:			return -1;
		case GalPBset:			return -1;
	}
	return -1;
}

static eint w32_pb_init(eHandle hobj, GalDrawable drawable, GalPBAttr *attr)
{
	GalPB32        *xpb = W32_PB_DATA(hobj);
	GalDrawable32 *draw;

	if (drawable == 0) {
		xpb->hdc = display_hdc;
		return 0;
	}

	if (attr)
		xpb->attr.func = rop_convert(attr->func);
	else
		xpb->attr.func = -1;

	xpb->pen_style = PS_SOLID;
	xpb->pen_width = 1;
	xpb->font_color = 0;

	draw = W32_DRAWABLE_DATA(drawable);
	xpb->draw = draw;
	if (draw->isdev) {
		if (draw->handle)
			xpb->hdc = GetDC(draw->handle);
		else
			xpb->hdc = 0;
		xpb->oldbmp = 0;
	}
	else {
		HBITMAP hbitmap = (HBITMAP)draw->handle;
		xpb->hdc = CreateCompatibleDC(display_hdc);
		xpb->oldbmp = SelectObject(xpb->hdc, hbitmap);
	}

	if (xpb->hdc && attr)
		SetROP2(xpb->hdc, rop_convert(attr->func));

	return 0;
}

static void w32_pb_free(eHandle hobj, ePointer data)
{
	GalPB32 *xpb = W32_PB_DATA(hobj);
	if (xpb->hdc == display_hdc) {
	}
	else if (xpb->oldbmp) {
		SelectObject(xpb->hdc, xpb->oldbmp);
		DeleteDC(xpb->hdc);
	}
	else {
		ReleaseDC(xpb->draw->handle, xpb->hdc);
	}
}

static void w32_pb_init_orders(eGeneType new, ePointer this)
{
	GalPBOrders *pb = e_genetype_orders(new, GTYPE_GAL_PB);
	pb->set_foreground = w32_set_foreground;
	pb->set_background = w32_set_background;
	pb->get_foreground = w32_get_foreground;
	pb->get_background = w32_get_background;
	pb->set_color = w32_set_font_color;
	pb->get_color = w32_get_font_color;
}

eGeneType w32_genetype_pb(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			w32_pb_init_orders,
			sizeof(GalPB32),
			NULL,
			w32_pb_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_PB, NULL);
	}
	return gtype;
}

static GalPB w32_pb_new(GalDrawable drawable, GalPBAttr *attr)
{
	GalPB pb = e_object_new(w32_genetype_pb()); 
	w32_pb_init(pb, drawable, attr);
	return pb;
}

static eint w32_get_mark(GalDrawable drawable)
{
	return 0;
}

static eint w32_get_depth(GalDrawable drawable)
{
	return W32_DRAWABLE_DATA(drawable)->depth;
}

static GalVisual *w32_get_visual(GalDrawable drawable)
{
	return 0;
}

static GalColormap *w32_get_colormap(GalDrawable drawable)
{
	return 0;
}

static int w32_get_visual_info(GalDrawable drawable, GalVisualInfo *vinfo)
{
	GalDrawable32 *xdraw = W32_DRAWABLE_DATA(drawable);
	vinfo->w     = xdraw->w;
	vinfo->h     = xdraw->h;
	vinfo->depth = xdraw->depth;
	return 0;
}

static void w32_drawable_init_orders(eGeneType new, ePointer this)
{
	GalDrawableOrders *draw = e_genetype_orders(new, GTYPE_GAL_DRAWABLE);

	draw->get_mark        = w32_get_mark;
	draw->get_depth       = w32_get_depth;
	draw->draw_image      = w32_draw_image;
	draw->draw_point      = w32_draw_point;
	draw->draw_line       = w32_draw_line;
	draw->draw_rect       = w32_draw_rect;
	draw->draw_arc        = w32_draw_arc;
	draw->fill_rect       = w32_fill_rect;
	draw->fill_arc        = w32_fill_arc;
	draw->get_visual      = w32_get_visual;
	draw->get_colormap    = w32_get_colormap;
	draw->get_visual_info = w32_get_visual_info;

	draw->composite           = w32_composite;
	draw->composite_image     = w32_composite_image;
	draw->draw_drawable       = w32_draw_drawable;
}

static void w32_drawable_free(eHandle hobj, ePointer data)
{
	GalDrawable32 *xdraw = data;
	if (xdraw->surface)
		e_object_unref(xdraw->surface);
}

eGeneType w32_genetype_drawable(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			w32_drawable_init_orders,
			sizeof(GalDrawable32),
			NULL, w32_drawable_free, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_DRAWABLE, NULL);
	}
	return gtype;
}

static void w32_cursor_free(eHandle hobj, ePointer data)
{
	GalCursor32 *cursor32 = W32_CURSOR_DATA(hobj);
	DestroyCursor(cursor32->hcursor);
}

eGeneType w32_genetype_cursor(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			NULL,
			sizeof(GalCursor32),
			NULL,
			w32_cursor_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_CURSOR, NULL);
	}
	return gtype;
}

static GalCursor w32_cursor_new_pixbuf(GalPixbuf *pixbuf, eint x, eint y)
{
	return 0;
}

static HCURSOR w32_data_to_wcursor(GalCursorType cursor_type)
{
	eint i, j, x, y, ofs;
	HCURSOR rv = NULL;
	eint w, h;
	euchar *and_plane, *xor_plane;

	for (i = 0; i < TABLES_SIZEOF(xcursors); i++)
		if (xcursors[i].type == cursor_type)
			break;

	if (i >= TABLES_SIZEOF(xcursors) || !xcursors[i].name)
		return NULL;

	w = GetSystemMetrics(SM_CXCURSOR);
	h = GetSystemMetrics(SM_CYCURSOR);

	and_plane = e_malloc((w/8) * h);
	e_memset(and_plane, 0xff, (w/8) * h);
	xor_plane = e_malloc((w/8) * h);
	e_memset(xor_plane, 0, (w/8) * h);

#define SET_BIT(v, b)	(v |= (1 << b))
#define RESET_BIT(v, b)	(v &= ~(1 << b))

	for (j = 0, y = 0; y < xcursors[i].height && y < h; y++) {
		ofs = (y * w) / 8;
		j = y * xcursors[i].width;

		for (x = 0; x < xcursors[i].width && x < w ; x++, j++) {
			eint pofs = ofs + x / 8;
			euchar data = (xcursors[i].data[j/4] & (0xc0 >> (2 * (j%4)))) >> (2 * (3 - (j%4)));
			eint bit = 7 - (j % xcursors[i].width) % 8;

			if (data) {
				RESET_BIT(and_plane[pofs], bit);
				if (data == 1)
					SET_BIT(xor_plane[pofs], bit);
			}
		}
	}

#undef SET_BIT
#undef RESET_BIT

	rv = CreateCursor(hmodule, xcursors[i].hotx, xcursors[i].hoty, w, h, and_plane, xor_plane);

	e_free(and_plane);
	e_free(xor_plane);

	return rv;
}

static GalCursor w32_cursor_new_name(const echar *name)
{
	return 0;
}

static GalCursor w32_cursor_new(GalCursorType type)
{
	GalCursor32 *cursor32;
	GalCursor cursor;

	HCURSOR hcursor = w32_data_to_wcursor(type);

	if (!hcursor) return 0;

	cursor  = e_object_new(w32_genetype_cursor());
	cursor32 = W32_CURSOR_DATA(cursor);
	cursor32->type = type;
	cursor32->hcursor = hcursor;

	return cursor;
}

void GalSwapBuffers(void)
{
	SwapBuffers(currentGL.hdc);
}
