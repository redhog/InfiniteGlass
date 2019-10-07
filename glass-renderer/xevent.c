/*
XEvent printing library

Copyright (c) 2019 RedHog (Egil Moeller)

Based on https://xorg.freedesktop.org/releases/individual/app/xev-1.2.3.tar.gz:xev.c

Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * Author:  Jim Fulton, MIT X Consortium
 */

#include "xevent.h"
#include <sys/time.h>

static unsigned long get_timestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return 1000000 * tv.tv_sec + tv.tv_usec;
}

//typedef unsigned long Pixel;

const char *Yes = "true";
const char *No = "false";
const char *Unknown = "undefined";

// Display *dpy;
// int screen;

Bool initialized = False;

Bool have_rr;
Bool have_damage;
Bool have_shape;
int rr_event_base, rr_error_base;
int damage_event_base, damage_error_base;
int shape_event_base, shape_error_base;

Atom wm_protocols;


static void
prologue(FILE *stream, XEvent *eventp, const char *event_name)
{
    XAnyEvent *e = (XAnyEvent *) eventp;

    fprintf(stream, "\"type\":\"%s\", \"serial\":%ld, \"synthetic\":%s, \"window\":%lu, \"print_time\": %lu",
            event_name, e->serial, e->send_event ? Yes : No, e->window, get_timestamp());
}

static void
dump(FILE *stream, char *str, int len)
{
    printf("(");
    len--;
    while (len-- > 0)
        fprintf(stream, "%02x ", (unsigned char) *str++);
    fprintf(stream, "%02x)", (unsigned char) *str++);
}

static void
do_KeyPress(FILE *stream, Display *display, XEvent *eventp)
{
    XKeyEvent *e = (XKeyEvent *) eventp;
    KeySym ks;
    KeyCode kc = 0;
    Bool kc_set = False;
    const char *ksname;
    int nbytes = 0;
    char str[256 + 1];
    static char *buf = NULL;
    static int bsize = 8;

    if (buf == NULL)
        buf = malloc(bsize);

    nbytes = XLookupString(e, str, 256, &ks, NULL);

    if (ks == NoSymbol)
        ksname = "NoSymbol";
    else {
        if (!(ksname = XKeysymToString(ks)))
            ksname = "(no name)";
        kc = XKeysymToKeycode(display, ks);
        kc_set = True;
    }

    fprintf(stream, ", \"root\":%lu, \"subw\":%lu, \"time\":%lu, \"x\":%d, \"y\":%d, \"rootx\":%d, \"rooty\":%d",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    fprintf(stream, ", \"state\":0x%x, \"keycode\":%u, \"keysym\":%lu, \"keysymname\":\"%s\", \"same_screen\":%s",
           e->state, e->keycode, (unsigned long) ks, ksname,
           e->same_screen ? Yes : No);
    if (kc_set && e->keycode != kc)
        fprintf(stream, ", \"XKeysymToKeycode\":%u", kc);
    if (nbytes < 0)
        nbytes = 0;
    if (nbytes > 256)
        nbytes = 256;
    str[nbytes] = '\0';
    if (nbytes > 0) {
        dump(stream, str, nbytes);
        fprintf(stream, ", \"XLookupString\":\"%s\"", str);
    }

    fprintf(stream, ", \"XFilterEvent\":%s",
            XFilterEvent(eventp, e->window) ? "true" : "false");
}

static void
do_KeyRelease(FILE *stream, Display *display, XEvent *eventp)
{
 do_KeyPress(stream, display, eventp);        /* since it has the same info */
}

static void
do_ButtonPress(FILE *stream, Display *display, XEvent *eventp)
{
    XButtonEvent *e = (XButtonEvent *) eventp;

    fprintf(stream, ", \"root\":%lu, \"subw\":%lu, \"time\":%lu, \"x\":%d, \"y\":%d, \"rootx\":%d, \"rooty\":%d",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    fprintf(stream, ", \"state\":0x%x, \"button\":%u, \"same_screen\":%s",
           e->state, e->button, e->same_screen ? Yes : No);
}

static void
do_ButtonRelease(FILE *stream, Display *display, XEvent *eventp)
{
    do_ButtonPress(stream, display, eventp);     /* since it has the same info */
}

static void
do_MotionNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XMotionEvent *e = (XMotionEvent *) eventp;

    fprintf(stream, ", \"root\":%lu, \"subw\":%lu, \"time\":%lu, \"x\":%d, \"y\":%d, \"rootx\":%d, \"rooty\":%d",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    fprintf(stream, ", \"state\":0x%x, \"is_hint\":%u, \"same_screen\":%s",
           e->state, e->is_hint, e->same_screen ? Yes : No);
}

static void
do_EnterNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XCrossingEvent *e = (XCrossingEvent *) eventp;
    const char *mode, *detail;
    char dmode[10], ddetail[10];

    switch (e->mode) {
    case NotifyNormal:
        mode = "NotifyNormal";
        break;
    case NotifyGrab:
        mode = "NotifyGrab";
        break;
    case NotifyUngrab:
        mode = "NotifyUngrab";
        break;
    case NotifyWhileGrabbed:
        mode = "NotifyWhileGrabbed";
        break;
    default:
        mode = dmode;
        snprintf(dmode, sizeof(dmode), "%u", e->mode);
        break;
    }

    switch (e->detail) {
    case NotifyAncestor:
        detail = "NotifyAncestor";
        break;
    case NotifyVirtual:
        detail = "NotifyVirtual";
        break;
    case NotifyInferior:
        detail = "NotifyInferior";
        break;
    case NotifyNonlinear:
        detail = "NotifyNonlinear";
        break;
    case NotifyNonlinearVirtual:
        detail = "NotifyNonlinearVirtual";
        break;
    case NotifyPointer:
        detail = "NotifyPointer";
        break;
    case NotifyPointerRoot:
        detail = "NotifyPointerRoot";
        break;
    case NotifyDetailNone:
        detail = "NotifyDetailNone";
        break;
    default:
        detail = ddetail;
        snprintf(ddetail, sizeof(ddetail), "%u", e->detail);
        break;
    }

    fprintf(stream, ", \"root\":%lu, \"subw\":%lu, \"time\":%lu, \"x\":%d, \"y\":%d, \"rootx\":%d, \"rooty\":%d",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    fprintf(stream, ", \"mode\":\"%s\", \"detail\":\"%s\", \"same_screen\":%s",
           mode, detail, e->same_screen ? Yes : No);
    fprintf(stream, ", \"focus\":%s, \"state\":%u", e->focus ? Yes : No, e->state);
}

static void
do_LeaveNotify(FILE *stream, Display *display, XEvent *eventp)
{
    do_EnterNotify(stream, display, eventp);     /* since it has same information */
}

static void
do_FocusIn(FILE *stream, Display *display, XEvent *eventp)
{
    XFocusChangeEvent *e = (XFocusChangeEvent *) eventp;
    const char *mode, *detail;
    char dmode[10], ddetail[10];

    switch (e->mode) {
    case NotifyNormal:
        mode = "NotifyNormal";
        break;
    case NotifyGrab:
        mode = "NotifyGrab";
        break;
    case NotifyUngrab:
        mode = "NotifyUngrab";
        break;
    case NotifyWhileGrabbed:
        mode = "NotifyWhileGrabbed";
        break;
    default:
        mode = dmode;
        snprintf(dmode, sizeof(dmode), "%u", e->mode);
        break;
    }

    switch (e->detail) {
    case NotifyAncestor:
        detail = "NotifyAncestor";
        break;
    case NotifyVirtual:
        detail = "NotifyVirtual";
        break;
    case NotifyInferior:
        detail = "NotifyInferior";
        break;
    case NotifyNonlinear:
        detail = "NotifyNonlinear";
        break;
    case NotifyNonlinearVirtual:
        detail = "NotifyNonlinearVirtual";
        break;
    case NotifyPointer:
        detail = "NotifyPointer";
        break;
    case NotifyPointerRoot:
        detail = "NotifyPointerRoot";
        break;
    case NotifyDetailNone:
        detail = "NotifyDetailNone";
        break;
    default:
        detail = ddetail;
        snprintf(ddetail, sizeof(ddetail), "%u", e->detail);
        break;
    }

    fprintf(stream, ", \"mode\":\"%s\", \"detail\":\"%s\"", mode, detail);
}

static void
do_FocusOut(FILE *stream, Display *display, XEvent *eventp)
{
    do_FocusIn(stream, display, eventp);         /* since it has same information */
}

static void
do_KeymapNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XKeymapEvent *e = (XKeymapEvent *) eventp;
    int i;

    fprintf(stream, ", \"keys\":[[");
    for (i = 0; i < 32; i++) {
        if (i == 16)
            fprintf(stream, "], [");
        fprintf(stream, "%-3u, ", (unsigned int) e->key_vector[i]);
    }
    fprintf(stream, "]]");
}

static void
do_Expose(FILE *stream, Display *display, XEvent *eventp)
{
    XExposeEvent *e = (XExposeEvent *) eventp;

    fprintf(stream, ", \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d, \"count\":%d",
           e->x, e->y, e->width, e->height, e->count);
}

static void
do_GraphicsExpose(FILE *stream, Display *display, XEvent *eventp)
{
    XGraphicsExposeEvent *e = (XGraphicsExposeEvent *) eventp;
    const char *m;
    char mdummy[10];

    switch (e->major_code) {
    case X_CopyArea:
        m = "CopyArea";
        break;
    case X_CopyPlane:
        m = "CopyPlane";
        break;
    default:
        m = mdummy;
        snprintf(mdummy, sizeof(mdummy), "%d", e->major_code);
        break;
    }

    fprintf(stream, ", \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d, \"count\":%d",
           e->x, e->y, e->width, e->height, e->count);
    fprintf(stream, ", \"major\":\"%s\", \"minor\":%d", m, e->minor_code);
}

static void
do_NoExpose(FILE *stream, Display *display, XEvent *eventp)
{
    XNoExposeEvent *e = (XNoExposeEvent *) eventp;
    const char *m;
    char mdummy[10];

    switch (e->major_code) {
    case X_CopyArea:
        m = "CopyArea";
        break;
    case X_CopyPlane:
        m = "CopyPlane";
        break;
    default:
        m = mdummy;
        snprintf(mdummy, sizeof(mdummy), "%d", e->major_code);
        break;
    }

    fprintf(stream, ", \"major\":\"%s\", \"minor\":%d", m, e->minor_code);
    return;
}

static void
do_VisibilityNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XVisibilityEvent *e = (XVisibilityEvent *) eventp;
    const char *v;
    char vdummy[10];

    switch (e->state) {
    case VisibilityUnobscured:
        v = "VisibilityUnobscured";
        break;
    case VisibilityPartiallyObscured:
        v = "VisibilityPartiallyObscured";
        break;
    case VisibilityFullyObscured:
        v = "VisibilityFullyObscured";
        break;
    default:
        v = vdummy;
        snprintf(vdummy, sizeof(vdummy), "%d", e->state);
        break;
    }

    fprintf(stream, ", \"state\":\"%s\"", v);
}

static void
do_CreateNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XCreateWindowEvent *e = (XCreateWindowEvent *) eventp;

    fprintf(stream, ", \"parent\":%lu, \"window\":%lu, \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d",
           e->parent, e->window, e->x, e->y, e->width, e->height);
    fprintf(stream, ", \"border_width\":%d, \"override\":%s",
           e->border_width, e->override_redirect ? Yes : No);
}

static void
do_DestroyNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XDestroyWindowEvent *e = (XDestroyWindowEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu", e->event, e->window);
}

static void
do_UnmapNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XUnmapEvent *e = (XUnmapEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"from_configure\":%s",
           e->event, e->window, e->from_configure ? Yes : No);
}

static void
do_MapNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XMapEvent *e = (XMapEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"override\":%s",
           e->event, e->window, e->override_redirect ? Yes : No);
}

static void
do_MapRequest(FILE *stream, Display *display, XEvent *eventp)
{
    XMapRequestEvent *e = (XMapRequestEvent *) eventp;

    fprintf(stream, ", \"parent\":%lu, \"window\":%lu", e->parent, e->window);
}

static void
do_ReparentNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XReparentEvent *e = (XReparentEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"parent\":%lu",
           e->event, e->window, e->parent);
    fprintf(stream, ", \"x\":%d, \"y\":%d, \"override\":%s", e->x, e->y,
           e->override_redirect ? Yes : No);
}

static void
do_ConfigureNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XConfigureEvent *e = (XConfigureEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d",
           e->event, e->window, e->x, e->y, e->width, e->height);
    fprintf(stream, ", \"border_width\":%d, \"above\":%lu, \"override\":%s",
           e->border_width, e->above, e->override_redirect ? Yes : No);
}

static void
do_ConfigureRequest(FILE *stream, Display *display, XEvent *eventp)
{
    XConfigureRequestEvent *e = (XConfigureRequestEvent *) eventp;
    const char *detail;
    char ddummy[10];

    switch (e->detail) {
    case Above:
        detail = "Above";
        break;
    case Below:
        detail = "Below";
        break;
    case TopIf:
        detail = "TopIf";
        break;
    case BottomIf:
        detail = "BottomIf";
        break;
    case Opposite:
        detail = "Opposite";
        break;
    default:
        detail = ddummy;
        snprintf(ddummy, sizeof(ddummy), "%d", e->detail);
        break;
    }

    fprintf(stream, ", \"parent\":%lu, \"window\":%lu, \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d",
           e->parent, e->window, e->x, e->y, e->width, e->height);
    fprintf(stream, ", \"border_width\":%d, \"above\":%lu, \"detail\":\"%s\", \"value\":%lu",
           e->border_width, e->above, detail, e->value_mask);
}

static void
do_GravityNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XGravityEvent *e = (XGravityEvent *) eventp;

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"x\":%d, \"y\":%d",
           e->event, e->window, e->x, e->y);
}

static void
do_ResizeRequest(FILE *stream, Display *display, XEvent *eventp)
{
    XResizeRequestEvent *e = (XResizeRequestEvent *) eventp;

    fprintf(stream, ", \"width\":%d, \"height\":%d", e->width, e->height);
}

static void
do_CirculateNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XCirculateEvent *e = (XCirculateEvent *) eventp;
    const char *p;
    char pdummy[10];

    switch (e->place) {
    case PlaceOnTop:
        p = "PlaceOnTop";
        break;
    case PlaceOnBottom:
        p = "PlaceOnBottom";
        break;
    default:
        p = pdummy;
        snprintf(pdummy, sizeof(pdummy), "%d", e->place);
        break;
    }

    fprintf(stream, ", \"event\":%lu, \"window\":%lu, \"place\":\"%s\"", e->event, e->window, p);
}

static void
do_CirculateRequest(FILE *stream, Display *display, XEvent *eventp)
{
    XCirculateRequestEvent *e = (XCirculateRequestEvent *) eventp;
    const char *p;
    char pdummy[10];

    switch (e->place) {
    case PlaceOnTop:
        p = "PlaceOnTop";
        break;
    case PlaceOnBottom:
        p = "PlaceOnBottom";
        break;
    default:
        p = pdummy;
        snprintf(pdummy, sizeof(pdummy), "%d", e->place);
        break;
    }

    fprintf(stream, ", \"parent\":%lu, \"window\":%lu, \"place\":\"%s\"",
           e->parent, e->window, p);
}

static void
do_PropertyNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XPropertyEvent *e = (XPropertyEvent *) eventp;
    char *aname = XGetAtomName(display, e->atom);
    const char *s;
    char sdummy[10];

    switch (e->state) {
    case PropertyNewValue:
        s = "PropertyNewValue";
        break;
    case PropertyDelete:
        s = "PropertyDelete";
        break;
    default:
        s = sdummy;
        snprintf(sdummy, sizeof(sdummy), "%d", e->state);
        break;
    }

    fprintf(stream, ", \"atom\":%lu, \"name\":\"%s\", \"time\":%lu, \"state\":\"%s\"",
           e->atom, aname ? aname : Unknown, e->time, s);

    XFree(aname);
}

static void
do_SelectionClear(FILE *stream, Display *display, XEvent *eventp)
{
    XSelectionClearEvent *e = (XSelectionClearEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);

    fprintf(stream, ", \"selection\":%lu, \"name\":\"%s\", \"time\":%lu",
           e->selection, sname ? sname : Unknown, e->time);

    XFree(sname);
}

static void
do_SelectionRequest(FILE *stream, Display *display, XEvent *eventp)
{
    XSelectionRequestEvent *e = (XSelectionRequestEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);
    char *tname = XGetAtomName(display, e->target);
    char *pname = XGetAtomName(display, e->property);

    fprintf(stream, ", \"owner\":%lu, \"requestor\":%lu, \"selection\":%lu, \"name\":\"%s\"",
           e->owner, e->requestor, e->selection, sname ? sname : Unknown);
    fprintf(stream, ", \"target\":%lu, \"target_name\":\"%s\", \"property\":%lu, \"property_name\":\"%s\", \"time\":%lu",
           e->target, tname ? tname : Unknown, e->property,
           pname ? pname : Unknown, e->time);

    XFree(sname);
    XFree(tname);
    XFree(pname);
}

static void
do_SelectionNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XSelectionEvent *e = (XSelectionEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);
    char *tname = XGetAtomName(display, e->target);
    char *pname = XGetAtomName(display, e->property);

    fprintf(stream, ", \"selection\":%lu, \"name\":\"%s\", \"target\":%lu, \"target_name\":\"%s\"",
           e->selection, sname ? sname : Unknown, e->target,
           tname ? tname : Unknown);
    fprintf(stream, ", \"property\":%lu, \"property_name\":\"%s\", \"time\":%lu",
           e->property, pname ? pname : Unknown, e->time);

    XFree(sname);
    XFree(tname);
    XFree(pname);
}

static void
do_ColormapNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XColormapEvent *e = (XColormapEvent *) eventp;
    const char *s;
    char sdummy[10];

    switch (e->state) {
    case ColormapInstalled:
        s = "ColormapInstalled";
        break;
    case ColormapUninstalled:
        s = "ColormapUninstalled";
        break;
    default:
        s = sdummy;
        snprintf(sdummy, sizeof(sdummy), "%d", e->state);
        break;
    }

    fprintf(stream, ", \"colormap\":%lu, \"new\":%s, \"state\":\"%s\"",
           e->colormap, e->new ? Yes : No, s);
}

static void
do_ClientMessage(FILE *stream, Display *display, XEvent *eventp)
{
    XClientMessageEvent *e = (XClientMessageEvent *) eventp;

    char *mname = XGetAtomName(display, e->message_type);

    if (e->message_type == wm_protocols) {
        char *message = XGetAtomName(display, e->data.l[0]);

        fprintf(stream, ", \"message_type\":%lu, \"message_type_name\":\"%s\", \"format\":%d, \"message\":%lu, \"message_name\":\"%s\"",
               e->message_type, mname ? mname : Unknown, e->format,
               e->data.l[0], message);
        XFree(message);
    }
    else {
     fprintf(stream, ", \"message_type\":%lu, \"message_type_name\":\"%s\", \"format\":%d",
               e->message_type, mname ? mname : Unknown, e->format);
    }

    XFree(mname);
}

static void
do_MappingNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XMappingEvent *e = (XMappingEvent *) eventp;
    const char *r;
    char rdummy[10];

    switch (e->request) {
    case MappingModifier:
        r = "MappingModifier";
        break;
    case MappingKeyboard:
        r = "MappingKeyboard";
        break;
    case MappingPointer:
        r = "MappingPointer";
        break;
    default:
        r = rdummy;
        snprintf(rdummy, sizeof(rdummy), "%d", e->request);
        break;
    }

    fprintf(stream, ", \"request\":\"%s\", \"first_keycode\":%d, \"count\":%d",
           r, e->first_keycode, e->count);
    XRefreshKeyboardMapping(e);
}

static void
print_SubPixelOrder(FILE *stream, SubpixelOrder subpixel_order)
{
    switch (subpixel_order) {
    case SubPixelUnknown:
        fprintf(stream, "SubPixelUnknown");
        return;
    case SubPixelHorizontalRGB:
        fprintf(stream, "SubPixelHorizontalRGB");
        return;
    case SubPixelHorizontalBGR:
        fprintf(stream, "SubPixelHorizontalBGR");
        return;
    case SubPixelVerticalRGB:
        fprintf(stream, "SubPixelVerticalRGB");
        return;
    case SubPixelVerticalBGR:
        fprintf(stream, "SubPixelVerticalBGR");
        return;
    case SubPixelNone:
        fprintf(stream, "SubPixelNone");
        return;
    default:
        fprintf(stream, "%d", subpixel_order);
    }
}

static void
print_Rotation(FILE *stream, Rotation rotation)
{
    if (rotation & RR_Rotate_0)
        fprintf(stream, "RR_Rotate_0");
    else if (rotation & RR_Rotate_90)
        fprintf(stream, "RR_Rotate_90");
    else if (rotation & RR_Rotate_180)
        fprintf(stream, "RR_Rotate_180");
    else if (rotation & RR_Rotate_270)
        fprintf(stream, "RR_Rotate_270");
    else {
        fprintf(stream, "%d", rotation);
        return;
    }
    if (rotation & RR_Reflect_X)
        fprintf(stream, ", RR_Reflect_X");
    if (rotation & RR_Reflect_Y)
        fprintf(stream, ", RR_Reflect_Y");
}

static void
print_Connection(FILE *stream, Connection connection)
{
    switch (connection) {
    case RR_Connected:
        fprintf(stream, "RR_Connected");
        return;
    case RR_Disconnected:
        fprintf(stream, "RR_Disconnected");
        return;
    case RR_UnknownConnection:
        fprintf(stream, "RR_UnknownConnection");
        return;
    default:
        fprintf(stream, "%d", connection);
    }
}

static void
do_RRScreenChangeNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XRRScreenChangeNotifyEvent *e = (XRRScreenChangeNotifyEvent *) eventp;

    XRRUpdateConfiguration(eventp);
    fprintf(stream, ", \"root\":%lu, \"timestamp\":%lu, \"config_timestamp\":%lu",
           e->root, e->timestamp, e->config_timestamp);
    fprintf(stream, ", \"size_index\":%hu", e->size_index);
    fprintf(stream, ", \"subpixel_order\":\"");
    print_SubPixelOrder(stream, e->subpixel_order);
    fprintf(stream, "\", \"rotation\":\"");
    print_Rotation(stream, e->rotation);
    fprintf(stream, "\", \"width\":%d, \"height\":%d, \"mwidth\":%d, \"mheight\":%d",
           e->width, e->height, e->mwidth, e->mheight);
}

static void
do_RRNotify_OutputChange(FILE *stream, Display *display, XEvent *eventp, XRRScreenResources *screen_resources)
{
    XRROutputChangeNotifyEvent *e = (XRROutputChangeNotifyEvent *) eventp;
    XRROutputInfo *output_info = NULL;
    XRRModeInfo *mode_info = NULL;

    if (screen_resources) {
        int i;

        output_info = XRRGetOutputInfo(display, screen_resources, e->output);
        for (i = 0; i < screen_resources->nmode; i++)
            if (screen_resources->modes[i].id == e->mode) {
                mode_info = &screen_resources->modes[i];
                break;
            }
    }
    fprintf(stream, ", \"subtype\":\"XRROutputChangeNotifyEvent\"");
    if (output_info)
        fprintf(stream, ", \"output_name\":\"%s\"", output_info->name);
    else
        fprintf(stream, ", \"output\":%lu", e->output);
    if (e->crtc)
        fprintf(stream, ", \"crtc\":%lu", e->crtc);
    else
        fprintf(stream, ", \"crtc\":undefined");
    if (mode_info)
        fprintf(stream, ", \"mode\":\"%s\", \"width\":%d, \"height\":%d", mode_info->name, mode_info->width,
               mode_info->height);
    else if (e->mode)
        fprintf(stream, ", \"mode\":%lu", e->mode);
    else
        fprintf(stream, ", \"mode\":undefined");
    fprintf(stream, ", \"rotation\":\"");
    print_Rotation(stream, e->rotation);
    fprintf(stream, "\", \"connection\":\"");
    print_Connection(stream, e->connection);
    fprintf(stream, "\", \"subpixel_order\":\"");
    print_SubPixelOrder(stream, e->subpixel_order);
    fprintf(stream, "\"");
    XRRFreeOutputInfo(output_info);
}

static void
do_RRNotify_CrtcChange(FILE *stream, Display *display, XEvent *eventp, XRRScreenResources *screen_resources)
{
    XRRCrtcChangeNotifyEvent *e = (XRRCrtcChangeNotifyEvent *) eventp;
    XRRModeInfo *mode_info = NULL;

    if (screen_resources) {
        int i;

        for (i = 0; i < screen_resources->nmode; i++)
            if (screen_resources->modes[i].id == e->mode) {
                mode_info = &screen_resources->modes[i];
                break;
            }
    }
    fprintf(stream, ", \"subtype\":\"XRRCrtcChangeNotifyEvent\"");
    if (e->crtc)
        fprintf(stream, ", \"crtc\":%lu", e->crtc);
    else
        fprintf(stream, ", \"crtc\":undefined");
    if (mode_info)
        fprintf(stream, ", \"mode\":\"%s\"", mode_info->name);
    else if (e->mode)
        fprintf(stream, ", \"mode\":%lu", e->mode);
    else
        fprintf(stream, ", \"mode\":undefined");
    fprintf(stream, ", \"rotation\":\"");
    print_Rotation(stream, e->rotation);
    fprintf(stream, "\", \"x\":%d, \"y\":%d, \"width\":%d, \"height\":%d",
           e->x, e->y, e->width, e->height);
}

static void
do_RRNotify_OutputProperty(FILE *stream, Display *display, XEvent *eventp,
                           XRRScreenResources *screen_resources)
{
    XRROutputPropertyNotifyEvent *e = (XRROutputPropertyNotifyEvent *) eventp;
    XRROutputInfo *output_info = NULL;
    char *property = XGetAtomName(display, e->property);

    if (screen_resources)
        output_info = XRRGetOutputInfo(display, screen_resources, e->output);
    fprintf(stream, ", \"subtype\":\"XRROutputPropertyChangeNotifyEvent\"");
    if (output_info)
        fprintf(stream, ", output \"%s\"", output_info->name);
    else
        fprintf(stream, ", \"output\":%lu", e->output);
    fprintf(stream, ", \"property\":\"%s\", \"timestamp\":%lu, \"state\":", property, e->timestamp);
    if (e->state == PropertyNewValue)
        fprintf(stream, "\"NewValue\"");
    else if (e->state == PropertyDelete)
        fprintf(stream, "\"Delete\"");
    else
        fprintf(stream, "%d", e->state);
    XRRFreeOutputInfo(output_info);
    XFree(property);
}

static void
do_RRNotify(FILE *stream, Display *display, XEvent *eventp)
{
    XRRNotifyEvent *e = (XRRNotifyEvent *) eventp;
    XRRScreenResources *screen_resources;

    XRRUpdateConfiguration(eventp);
    screen_resources = XRRGetScreenResources(display, e->window);
    prologue(stream, eventp, "RRNotify");
    switch (e->subtype) {
    case RRNotify_OutputChange:
        do_RRNotify_OutputChange(stream, display, eventp, screen_resources);
        break;
    case RRNotify_CrtcChange:
        do_RRNotify_CrtcChange(stream, display, eventp, screen_resources);
        break;
    case RRNotify_OutputProperty:
        do_RRNotify_OutputProperty(stream, display, eventp, screen_resources);
        break;
    default:
        fprintf(stream, ", \"subtype\":%d", e->subtype);
    }
    XRRFreeScreenResources(screen_resources);
}

void print_xevent_fragment(FILE *stream, Display *display, XEvent *event) {
    if (!initialized) {
        initialized = True;
        have_rr = XRRQueryExtension(display, &rr_event_base, &rr_error_base);
        have_damage = XDamageQueryExtension(display, &damage_event_base, &damage_error_base);
        have_shape = XShapeQueryExtension(display, &shape_event_base, &shape_error_base);

        wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    }
    switch (event->type) {
        case KeyPress:
            prologue(stream, event, "KeyPress");
            do_KeyPress(stream, display, event);
            break;
        case KeyRelease:
            prologue(stream, event, "KeyRelease");
            do_KeyRelease(stream, display, event);
            break;
        case ButtonPress:
            prologue(stream, event, "ButtonPress");
            do_ButtonPress(stream, display, event);
            break;
        case ButtonRelease:
            prologue(stream, event, "ButtonRelease");
            do_ButtonRelease(stream, display, event);
            break;
        case MotionNotify:
            prologue(stream, event, "MotionNotify");
            do_MotionNotify(stream, display, event);
            break;
        case EnterNotify:
            prologue(stream, event, "EnterNotify");
            do_EnterNotify(stream, display, event);
            break;
        case LeaveNotify:
            prologue(stream, event, "LeaveNotify");
            do_LeaveNotify(stream, display, event);
            break;
        case FocusIn:
            prologue(stream, event, "FocusIn");
            do_FocusIn(stream, display, event);
            break;
        case FocusOut:
            prologue(stream, event, "FocusOut");
            do_FocusOut(stream, display, event);
            break;
        case KeymapNotify:
            prologue(stream, event, "KeymapNotify");
            do_KeymapNotify(stream, display, event);
            break;
        case Expose:
            prologue(stream, event, "Expose");
            do_Expose(stream, display, event);
            break;
        case GraphicsExpose:
            prologue(stream, event, "GraphicsExpose");
            do_GraphicsExpose(stream, display, event);
            break;
        case NoExpose:
            prologue(stream, event, "NoExpose");
            do_NoExpose(stream, display, event);
            break;
        case VisibilityNotify:
            prologue(stream, event, "VisibilityNotify");
            do_VisibilityNotify(stream, display, event);
            break;
        case CreateNotify:
            prologue(stream, event, "CreateNotify");
            do_CreateNotify(stream, display, event);
            break;
        case DestroyNotify:
            prologue(stream, event, "DestroyNotify");
            do_DestroyNotify(stream, display, event);
            break;
        case UnmapNotify:
            prologue(stream, event, "UnmapNotify");
            do_UnmapNotify(stream, display, event);
            break;
        case MapNotify:
            prologue(stream, event, "MapNotify");
            do_MapNotify(stream, display, event);
            break;
        case MapRequest:
            prologue(stream, event, "MapRequest");
            do_MapRequest(stream, display, event);
            break;
        case ReparentNotify:
            prologue(stream, event, "ReparentNotify");
            do_ReparentNotify(stream, display, event);
            break;
        case ConfigureNotify:
            prologue(stream, event, "ConfigureNotify");
            do_ConfigureNotify(stream, display, event);
            break;
        case ConfigureRequest:
            prologue(stream, event, "ConfigureRequest");
            do_ConfigureRequest(stream, display, event);
            break;
        case GravityNotify:
            prologue(stream, event, "GravityNotify");
            do_GravityNotify(stream, display, event);
            break;
        case ResizeRequest:
            prologue(stream, event, "ResizeRequest");
            do_ResizeRequest(stream, display, event);
            break;
        case CirculateNotify:
            prologue(stream, event, "CirculateNotify");
            do_CirculateNotify(stream, display, event);
            break;
        case CirculateRequest:
            prologue(stream, event, "CirculateRequest");
            do_CirculateRequest(stream, display, event);
            break;
        case PropertyNotify:
            prologue(stream, event, "PropertyNotify");
            do_PropertyNotify(stream, display, event);
            break;
        case SelectionClear:
            prologue(stream, event, "SelectionClear");
            do_SelectionClear(stream, display, event);
            break;
        case SelectionRequest:
            prologue(stream, event, "SelectionRequest");
            do_SelectionRequest(stream, display, event);
            break;
        case SelectionNotify:
            prologue(stream, event, "SelectionNotify");
            do_SelectionNotify(stream, display, event);
            break;
        case ColormapNotify:
            prologue(stream, event, "ColormapNotify");
            do_ColormapNotify(stream, display, event);
            break;
        case ClientMessage:
            prologue(stream, event, "ClientMessage");
            do_ClientMessage(stream, display, event);
            break;
        case MappingNotify:
            prologue(stream, event, "MappingNotify");
            do_MappingNotify(stream, display, event);
            break;
        case GenericEvent:
            switch (event->xcookie.evtype) {
                case XI_RawMotion:
                    prologue(stream, event, "XI_RawMotion");
                    break;
                default:  
                    fprintf(stream, "\"type\":\"GenericEvent\", \"error\":\"Unknown GenericEvent\", \"evtype\":%d", event->xcookie.evtype);
                    break;
            }
            break;
        default:
            if (have_rr) {
                if (event->type == rr_event_base + RRScreenChangeNotify) {
                    prologue(stream, event, "RRScreenChangeNotify");
                    do_RRScreenChangeNotify(stream, display, event);
                    break;
                }
                if (event->type == rr_event_base + RRNotify) {
                    do_RRNotify(stream, display, event);
                    break;
                }
            }
            if (have_damage) {
                if (event->type == damage_event_base + XDamageNotify) {
                    prologue(stream, event, "XDamageNotify");
                    break;
                }
            }
            if (have_shape) {
                if (event->type == damage_event_base + ShapeNotify) {
                    prologue(stream, event, "ShapeNotify");
                    break;
                }
            }
            fprintf(stream, "\"error\":\"Unknown event\", \"type\":%d", event->type);
            break;
     }
}

void print_xevent(FILE *stream, Display *display, XEvent *event) {
     fprintf(stream, "{");
     print_xevent_fragment(stream, display, event);
     fprintf(stream, "}\n");
}
