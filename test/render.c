#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>

#include <egal/egal.h>
 
static Display *x11_dpy;
static Window root_xid;
static int event_base;
static int error_base;
static Window wId;
static XRenderPictFormat *format;
static XVisualInfo *x11_vinfo;
static Pixmap  pixmap;
static Colormap  x11_cmap;
static Picture root_picture;
static int hasAlpha;
static int damage_event;
static int damage_error;
static GC root_gc;
static XImage *back;

#define MOUSE_MASK (GalButtonPressMask | GalButtonReleaseMask | GalPointerMotionMask | GalButtonMotionMask | GalEnterWindowMask | GalLeaveWindowMask)
#define DSP_MASK (GalVisibilityChangeMask | GalStructureNotifyMask | GalFocusChangeMask)

static Window x11_window_new(Window pxid, int x, int y, int w, int h)
{
	XSetWindowAttributes attribs;
	Window xid;

	memset(&attribs, 0, sizeof(attribs));

	attribs.event_mask        = MOUSE_MASK | SubstructureRedirectMask;
	attribs.border_pixel      = XWhitePixel(x11_dpy, x11_vinfo->screen);
	attribs.background_pixel  = XBlackPixel(x11_dpy, x11_vinfo->screen);
	attribs.override_redirect = True;
	attribs.colormap          = x11_cmap;
	//attribs.bit_gravity       = NorthWestGravity;
	//attribs.bit_gravity       = StaticGravity;
	xid = XCreateWindow(x11_dpy,
			pxid, x, y, w, h,
			0,
			x11_vinfo->depth,
			InputOutput,
			x11_vinfo->visual,
			CWEventMask | CWColormap | CWBorderPixel | CWOverrideRedirect,
			&attribs);

	XMapWindow(x11_dpy, xid);
	XMoveWindow(x11_dpy, xid, x, y);

	return xid;
}

static euint32 pixbuf_read_pixel(GalPixbuf *pixbuf, eint x, eint y)
{
	char *line = pixbuf->pixels + y * pixbuf->rowbytes;

	if (pixbuf->pixelbytes == 3) {
		char pixel[4]; 
		pixel[0] = (line + x * 3)[0];
		pixel[1] = (line + x * 3)[1];
		pixel[2] = (line + x * 3)[2];
		pixel[3] = 0;
		return ((long *)pixel)[0];
	}

	return ((long *)line)[x];
}

static void image32_copy_from_pixbuf(XImage *ximg, GalPixbuf *pixbuf)
{
    int x, y;
    long *p = (long *)ximg->data;

    for (y = 0; y < pixbuf->h; y++) {
        for (x = 0; x < pixbuf->w; x++)
            p[x] = pixbuf_read_pixel(pixbuf, x, y);
        p += ximg->width;
    }
}

static XImage *x11_image_new(const char *picname)
{
	XImage *ximg;
	int depth;

	GalPixbuf *pixbuf = egal_pixbuf_new_from_file(picname, 0, 0);

	if (pixbuf->pixelbytes == 4)
		depth = 32;
	else
		depth = x11_vinfo->depth;

	ximg = XCreateImage(x11_dpy, x11_vinfo->visual, depth,
			ZPixmap, 0, NULL, pixbuf->w, pixbuf->h, 32, 0);

	ximg->data = malloc(pixbuf->rowbytes * pixbuf->h);
	image32_copy_from_pixbuf(ximg, pixbuf);

	return ximg;
}

static GC x11_create_gc(Drawable xid)
{
	XGCValues xgcvalues;
	xgcvalues.function = GXcopy;
	xgcvalues.graphics_exposures = False;
	return XCreateGC(x11_dpy, xid, GCGraphicsExposures | GCFunction, &xgcvalues);
}

static Picture x11_window_picture(Window xid)
{
	Picture pic;
	XRenderPictFormat *format;
	XRenderPictureAttributes pa;

	pa.subwindow_mode = IncludeInferiors;
	format = XRenderFindVisualFormat(x11_dpy, x11_vinfo->visual);
	pic    = XRenderCreatePicture(x11_dpy, xid, format, CPSubwindowMode, &pa);

	return pic;
}


static Picture x11_pixmap_picture(XImage *ximg)
{
	XRenderPictFormat *format;
	Picture pic;
	Pixmap xid;

	xid = XCreatePixmap(x11_dpy, root_xid, ximg->width, ximg->height, 32);

	format = XRenderFindStandardFormat(x11_dpy, PictStandardARGB32);
	pic    = XRenderCreatePicture(x11_dpy, xid, format, 0, NULL);
	XPutImage(x11_dpy, xid, x11_create_gc(xid), ximg, 0, 0, 0, 0, ximg->width, ximg->height);

	return pic;
}

static void x11_test_composit(Picture src, Picture dst, int w, int h)
{
	XRenderComposite(x11_dpy, PictOpOver, src, None, dst, 0, 0, 0, 0, 200, 200, w, h);

	//Pixmap fromX11Pixmap(root_picture).save("xx.png");
	//XserverRegion region = XFixesCreateRegionFromWindow(x11_dpy, wId, WindowRegionBounding);
	//XFixesTranslateRegion(x11_dpy, region, -x, -y);
	//XFixesSetPictureClipRegion(x11_dpy, root_picture, 0, 0, region);
	//XFixesDestroyRegion(x11_dpy, region);
}

static void x11_wait_event(Window xid, Picture src, Picture dst, XImage *ximg)
{
	XEvent xent;
	Damage damage;

	XDamageQueryExtension(x11_dpy, &damage_event, &damage_error);
	damage = XDamageCreate(x11_dpy, root_xid, XDamageReportNonEmpty);

	while (1) {
		while (!XPending(x11_dpy))
			usleep(1000);

		XNextEvent(x11_dpy, &xent);

		if (xent.type == damage_event + XDamageNotify) {
			XDamageNotifyEvent *e = (XDamageNotifyEvent *)xent.pad;
			if (e->drawable == root_xid) {
				XDamageSubtract(x11_dpy, e->damage, None, None);
				x11_test_composit(src, root_picture, ximg->width, ximg->height);
			}
		}
		else if (xent.type == ConfigureRequest) {
		}
		else if (xent.type == ConfigureNotify) {
			XConfigureEvent *e = &xent.xconfigure;
		}
		else if (xent.type == VisibilityNotify) {
			printf("visibility %d\n", xent.type);
		}
		else if (xent.type == MapNotify) {
			XMapEvent *e = &xent.xmap;
			//XMapWindow(x11_dpy, e->window);
		}
		else if (xent.type == Expose) {
			XExposeEvent *e = &xent.xexpose;
			if (e->window == xid) {
				//XPutImage(x11_dpy, xid, root_gc, back, 0, 0, 0, 0, back->width, back->height);
				//x11_test_composit(src, dst, ximg->width, ximg->height);

				XSync(x11_dpy, False);
			}
		}
		else if (xent.type == ButtonPress) {
			XButtonEvent *e = &xent.xbutton;
			printf("button %x   %x   x:%d  y:%d\n", e->window, xid, e->x, e->y);
		}
		else if (xent.type == KeyPress) {
			XKeyEvent *e = &xent.xkey;
			printf("key   %x   %x\n", e->window, xid);
		}
	}
}

int main(int argc, char *argv[])
{
	XVisualInfo vinfo;
	Window xid;
	Picture src, dst;
	XImage *ximg;
	int vnum;

	if (!(x11_dpy = XOpenDisplay(0))) {
		printf("Could not open display [%s]\n", getenv("DISPLAY"));
		return -1;
	}

	XSynchronize(x11_dpy, False);

	vinfo.visualid = XVisualIDFromVisual(XDefaultVisual(x11_dpy, XDefaultScreen(x11_dpy)));
	x11_vinfo      = XGetVisualInfo(x11_dpy, VisualIDMask, &vinfo, &vnum);
	if (vnum == 0) {
		printf("Bad visual id %d\n", (int)vinfo.visualid);
		return -1;
	}

	root_xid = XRootWindow(x11_dpy, x11_vinfo->screen);
	x11_cmap = XCreateColormap(x11_dpy, root_xid, x11_vinfo->visual, AllocNone);
	root_gc  = x11_create_gc(root_xid);
	root_picture = x11_window_picture(root_xid);

	XCompositeRedirectSubwindows(x11_dpy, root_xid, CompositeRedirectAutomatic);

	back = x11_image_new("1.jpg");

	ximg = x11_image_new("tt.png");
	src  = x11_pixmap_picture(ximg);

	xid = x11_window_new(root_xid, 200, 200, ximg->width, ximg->height);
	dst = x11_window_picture(xid);

	x11_wait_event(xid, src, dst, ximg);
 
    return 0;
}
