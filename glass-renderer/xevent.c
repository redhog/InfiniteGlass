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

//typedef unsigned long Pixel;

const char *Yes = "YES";
const char *No = "NO";
const char *Unknown = "unknown";

// Display *dpy;
// int screen;

Bool initialized = False;

Bool have_rr;
int rr_event_base, rr_error_base;
Atom wm_protocols;

static void
prologue(XEvent *eventp, const char *event_name)
{
    XAnyEvent *e = (XAnyEvent *) eventp;

    printf("\n%s event, serial %ld, synthetic %s, window 0x%lx,\n",
           event_name, e->serial, e->send_event ? Yes : No, e->window);
}

static void
dump(char *str, int len)
{
    printf("(");
    len--;
    while (len-- > 0)
        printf("%02x ", (unsigned char) *str++);
    printf("%02x)", (unsigned char) *str++);
}

static void
do_KeyPress(Display *display, XEvent *eventp)
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

    printf("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf("    state 0x%x, keycode %u (keysym 0x%lx, %s), same_screen %s,\n",
           e->state, e->keycode, (unsigned long) ks, ksname,
           e->same_screen ? Yes : No);
    if (kc_set && e->keycode != kc)
        printf("    XKeysymToKeycode returns keycode: %u\n", kc);
    if (nbytes < 0)
        nbytes = 0;
    if (nbytes > 256)
        nbytes = 256;
    str[nbytes] = '\0';
    printf("    XLookupString gives %d bytes: ", nbytes);
    if (nbytes > 0) {
        dump(str, nbytes);
        printf(" \"%s\"\n", str);
    }
    else {
        printf("\n");
    }

    printf("    XFilterEvent returns: %s\n",
           XFilterEvent(eventp, e->window) ? "True" : "False");
}

static void
do_KeyRelease(Display *display, XEvent *eventp)
{
    do_KeyPress(display, eventp);        /* since it has the same info */
}

static void
do_ButtonPress(Display *display, XEvent *eventp)
{
    XButtonEvent *e = (XButtonEvent *) eventp;

    printf("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf("    state 0x%x, button %u, same_screen %s\n",
           e->state, e->button, e->same_screen ? Yes : No);
}

static void
do_ButtonRelease(Display *display, XEvent *eventp)
{
    do_ButtonPress(display, eventp);     /* since it has the same info */
}

static void
do_MotionNotify(Display *display, XEvent *eventp)
{
    XMotionEvent *e = (XMotionEvent *) eventp;

    printf("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf("    state 0x%x, is_hint %u, same_screen %s\n",
           e->state, e->is_hint, e->same_screen ? Yes : No);
}

static void
do_EnterNotify(Display *display, XEvent *eventp)
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

    printf("    root 0x%lx, subw 0x%lx, time %lu, (%d,%d), root:(%d,%d),\n",
           e->root, e->subwindow, e->time, e->x, e->y, e->x_root, e->y_root);
    printf("    mode %s, detail %s, same_screen %s,\n",
           mode, detail, e->same_screen ? Yes : No);
    printf("    focus %s, state %u\n", e->focus ? Yes : No, e->state);
}

static void
do_LeaveNotify(Display *display, XEvent *eventp)
{
    do_EnterNotify(display, eventp);     /* since it has same information */
}

static void
do_FocusIn(Display *display, XEvent *eventp)
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

    printf("    mode %s, detail %s\n", mode, detail);
}

static void
do_FocusOut(Display *display, XEvent *eventp)
{
    do_FocusIn(display, eventp);         /* since it has same information */
}

static void
do_KeymapNotify(Display *display, XEvent *eventp)
{
    XKeymapEvent *e = (XKeymapEvent *) eventp;
    int i;

    printf("    keys:  ");
    for (i = 0; i < 32; i++) {
        if (i == 16)
            printf("\n           ");
        printf("%-3u ", (unsigned int) e->key_vector[i]);
    }
    printf("\n");
}

static void
do_Expose(Display *display, XEvent *eventp)
{
    XExposeEvent *e = (XExposeEvent *) eventp;

    printf("    (%d,%d), width %d, height %d, count %d\n",
           e->x, e->y, e->width, e->height, e->count);
}

static void
do_GraphicsExpose(Display *display, XEvent *eventp)
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

    printf("    (%d,%d), width %d, height %d, count %d,\n",
           e->x, e->y, e->width, e->height, e->count);
    printf("    major %s, minor %d\n", m, e->minor_code);
}

static void
do_NoExpose(Display *display, XEvent *eventp)
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

    printf("    major %s, minor %d\n", m, e->minor_code);
    return;
}

static void
do_VisibilityNotify(Display *display, XEvent *eventp)
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

    printf("    state %s\n", v);
}

static void
do_CreateNotify(Display *display, XEvent *eventp)
{
    XCreateWindowEvent *e = (XCreateWindowEvent *) eventp;

    printf("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d\n",
           e->parent, e->window, e->x, e->y, e->width, e->height);
    printf("border_width %d, override %s\n",
           e->border_width, e->override_redirect ? Yes : No);
}

static void
do_DestroyNotify(Display *display, XEvent *eventp)
{
    XDestroyWindowEvent *e = (XDestroyWindowEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx\n", e->event, e->window);
}

static void
do_UnmapNotify(Display *display, XEvent *eventp)
{
    XUnmapEvent *e = (XUnmapEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx, from_configure %s\n",
           e->event, e->window, e->from_configure ? Yes : No);
}

static void
do_MapNotify(Display *display, XEvent *eventp)
{
    XMapEvent *e = (XMapEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx, override %s\n",
           e->event, e->window, e->override_redirect ? Yes : No);
}

static void
do_MapRequest(Display *display, XEvent *eventp)
{
    XMapRequestEvent *e = (XMapRequestEvent *) eventp;

    printf("    parent 0x%lx, window 0x%lx\n", e->parent, e->window);
}

static void
do_ReparentNotify(Display *display, XEvent *eventp)
{
    XReparentEvent *e = (XReparentEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx, parent 0x%lx,\n",
           e->event, e->window, e->parent);
    printf("    (%d,%d), override %s\n", e->x, e->y,
           e->override_redirect ? Yes : No);
}

static void
do_ConfigureNotify(Display *display, XEvent *eventp)
{
    XConfigureEvent *e = (XConfigureEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
           e->event, e->window, e->x, e->y, e->width, e->height);
    printf("    border_width %d, above 0x%lx, override %s\n",
           e->border_width, e->above, e->override_redirect ? Yes : No);
}

static void
do_ConfigureRequest(Display *display, XEvent *eventp)
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

    printf("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
           e->parent, e->window, e->x, e->y, e->width, e->height);
    printf("    border_width %d, above 0x%lx, detail %s, value 0x%lx\n",
           e->border_width, e->above, detail, e->value_mask);
}

static void
do_GravityNotify(Display *display, XEvent *eventp)
{
    XGravityEvent *e = (XGravityEvent *) eventp;

    printf("    event 0x%lx, window 0x%lx, (%d,%d)\n",
           e->event, e->window, e->x, e->y);
}

static void
do_ResizeRequest(Display *display, XEvent *eventp)
{
    XResizeRequestEvent *e = (XResizeRequestEvent *) eventp;

    printf("    width %d, height %d\n", e->width, e->height);
}

static void
do_CirculateNotify(Display *display, XEvent *eventp)
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

    printf("    event 0x%lx, window 0x%lx, place %s\n", e->event, e->window, p);
}

static void
do_CirculateRequest(Display *display, XEvent *eventp)
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

    printf("    parent 0x%lx, window 0x%lx, place %s\n",
           e->parent, e->window, p);
}

static void
do_PropertyNotify(Display *display, XEvent *eventp)
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

    printf("    atom 0x%lx (%s), time %lu, state %s\n",
           e->atom, aname ? aname : Unknown, e->time, s);

    XFree(aname);
}

static void
do_SelectionClear(Display *display, XEvent *eventp)
{
    XSelectionClearEvent *e = (XSelectionClearEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);

    printf("    selection 0x%lx (%s), time %lu\n",
           e->selection, sname ? sname : Unknown, e->time);

    XFree(sname);
}

static void
do_SelectionRequest(Display *display, XEvent *eventp)
{
    XSelectionRequestEvent *e = (XSelectionRequestEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);
    char *tname = XGetAtomName(display, e->target);
    char *pname = XGetAtomName(display, e->property);

    printf("    owner 0x%lx, requestor 0x%lx, selection 0x%lx (%s),\n",
           e->owner, e->requestor, e->selection, sname ? sname : Unknown);
    printf("    target 0x%lx (%s), property 0x%lx (%s), time %lu\n",
           e->target, tname ? tname : Unknown, e->property,
           pname ? pname : Unknown, e->time);

    XFree(sname);
    XFree(tname);
    XFree(pname);
}

static void
do_SelectionNotify(Display *display, XEvent *eventp)
{
    XSelectionEvent *e = (XSelectionEvent *) eventp;
    char *sname = XGetAtomName(display, e->selection);
    char *tname = XGetAtomName(display, e->target);
    char *pname = XGetAtomName(display, e->property);

    printf("    selection 0x%lx (%s), target 0x%lx (%s),\n",
           e->selection, sname ? sname : Unknown, e->target,
           tname ? tname : Unknown);
    printf("    property 0x%lx (%s), time %lu\n",
           e->property, pname ? pname : Unknown, e->time);

    XFree(sname);
    XFree(tname);
    XFree(pname);
}

static void
do_ColormapNotify(Display *display, XEvent *eventp)
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

    printf("    colormap 0x%lx, new %s, state %s\n",
           e->colormap, e->new ? Yes : No, s);
}

static void
do_ClientMessage(Display *display, XEvent *eventp)
{
    XClientMessageEvent *e = (XClientMessageEvent *) eventp;

    char *mname = XGetAtomName(display, e->message_type);

    if (e->message_type == wm_protocols) {
        char *message = XGetAtomName(display, e->data.l[0]);

        printf("    message_type 0x%lx (%s), format %d, message 0x%lx (%s)\n",
               e->message_type, mname ? mname : Unknown, e->format,
               e->data.l[0], message);
        XFree(message);
    }
    else {
        printf("    message_type 0x%lx (%s), format %d\n",
               e->message_type, mname ? mname : Unknown, e->format);
    }

    XFree(mname);
}

static void
do_MappingNotify(Display *display, XEvent *eventp)
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

    printf("    request %s, first_keycode %d, count %d\n",
           r, e->first_keycode, e->count);
    XRefreshKeyboardMapping(e);
}

static void
print_SubPixelOrder(SubpixelOrder subpixel_order)
{
    switch (subpixel_order) {
    case SubPixelUnknown:
        printf("SubPixelUnknown");
        return;
    case SubPixelHorizontalRGB:
        printf("SubPixelHorizontalRGB");
        return;
    case SubPixelHorizontalBGR:
        printf("SubPixelHorizontalBGR");
        return;
    case SubPixelVerticalRGB:
        printf("SubPixelVerticalRGB");
        return;
    case SubPixelVerticalBGR:
        printf("SubPixelVerticalBGR");
        return;
    case SubPixelNone:
        printf("SubPixelNone");
        return;
    default:
        printf("%d", subpixel_order);
    }
}

static void
print_Rotation(Rotation rotation)
{
    if (rotation & RR_Rotate_0)
        printf("RR_Rotate_0");
    else if (rotation & RR_Rotate_90)
        printf("RR_Rotate_90");
    else if (rotation & RR_Rotate_180)
        printf("RR_Rotate_180");
    else if (rotation & RR_Rotate_270)
        printf("RR_Rotate_270");
    else {
        printf("%d", rotation);
        return;
    }
    if (rotation & RR_Reflect_X)
        printf(", RR_Reflect_X");
    if (rotation & RR_Reflect_Y)
        printf(", RR_Reflect_Y");
}

static void
print_Connection(Connection connection)
{
    switch (connection) {
    case RR_Connected:
        printf("RR_Connected");
        return;
    case RR_Disconnected:
        printf("RR_Disconnected");
        return;
    case RR_UnknownConnection:
        printf("RR_UnknownConnection");
        return;
    default:
        printf("%d", connection);
    }
}

static void
do_RRScreenChangeNotify(Display *display, XEvent *eventp)
{
    XRRScreenChangeNotifyEvent *e = (XRRScreenChangeNotifyEvent *) eventp;

    XRRUpdateConfiguration(eventp);
    printf("    root 0x%lx, timestamp %lu, config_timestamp %lu\n",
           e->root, e->timestamp, e->config_timestamp);
    printf("    size_index %hu", e->size_index);
    printf(", subpixel_order ");
    print_SubPixelOrder(e->subpixel_order);
    printf("\n    rotation ");
    print_Rotation(e->rotation);
    printf("\n    width %d, height %d, mwidth %d, mheight %d\n",
           e->width, e->height, e->mwidth, e->mheight);
}

static void
do_RRNotify_OutputChange(Display *display, XEvent *eventp, XRRScreenResources *screen_resources)
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
    printf("    subtype XRROutputChangeNotifyEvent\n");
    if (output_info)
        printf("    output %s, ", output_info->name);
    else
        printf("    output %lu, ", e->output);
    if (e->crtc)
        printf("crtc %lu, ", e->crtc);
    else
        printf("crtc None, ");
    if (mode_info)
        printf("mode %s (%dx%d)\n", mode_info->name, mode_info->width,
               mode_info->height);
    else if (e->mode)
        printf("mode %lu\n", e->mode);
    else
        printf("mode None\n");
    printf("    rotation ");
    print_Rotation(e->rotation);
    printf("\n    connection ");
    print_Connection(e->connection);
    printf(", subpixel_order ");
    print_SubPixelOrder(e->subpixel_order);
    printf("\n");
    XRRFreeOutputInfo(output_info);
}

static void
do_RRNotify_CrtcChange(Display *display, XEvent *eventp, XRRScreenResources *screen_resources)
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
    printf("    subtype XRRCrtcChangeNotifyEvent\n");
    if (e->crtc)
        printf("    crtc %lu, ", e->crtc);
    else
        printf("    crtc None, ");
    if (mode_info)
        printf("mode %s, ", mode_info->name);
    else if (e->mode)
        printf("mode %lu, ", e->mode);
    else
        printf("mode None, ");
    printf("rotation ");
    print_Rotation(e->rotation);
    printf("\n    x %d, y %d, width %d, height %d\n",
           e->x, e->y, e->width, e->height);
}

static void
do_RRNotify_OutputProperty(Display *display, XEvent *eventp,
                           XRRScreenResources *screen_resources)
{
    XRROutputPropertyNotifyEvent *e = (XRROutputPropertyNotifyEvent *) eventp;
    XRROutputInfo *output_info = NULL;
    char *property = XGetAtomName(display, e->property);

    if (screen_resources)
        output_info = XRRGetOutputInfo(display, screen_resources, e->output);
    printf("    subtype XRROutputPropertyChangeNotifyEvent\n");
    if (output_info)
        printf("    output %s, ", output_info->name);
    else
        printf("    output %lu, ", e->output);
    printf("property %s, timestamp %lu, state ", property, e->timestamp);
    if (e->state == PropertyNewValue)
        printf("NewValue\n");
    else if (e->state == PropertyDelete)
        printf("Delete\n");
    else
        printf("%d\n", e->state);
    XRRFreeOutputInfo(output_info);
    XFree(property);
}

static void
do_RRNotify(Display *display, XEvent *eventp)
{
    XRRNotifyEvent *e = (XRRNotifyEvent *) eventp;
    XRRScreenResources *screen_resources;

    XRRUpdateConfiguration(eventp);
    screen_resources = XRRGetScreenResources(display, e->window);
    prologue(eventp, "RRNotify");
    switch (e->subtype) {
    case RRNotify_OutputChange:
        do_RRNotify_OutputChange(display, eventp, screen_resources);
        break;
    case RRNotify_CrtcChange:
        do_RRNotify_CrtcChange(display, eventp, screen_resources);
        break;
    case RRNotify_OutputProperty:
        do_RRNotify_OutputProperty(display, eventp, screen_resources);
        break;
    default:
        printf("    subtype %d\n", e->subtype);
    }
    XRRFreeScreenResources(screen_resources);
}

void print_xevent(Display *display, XEvent *event) {
    if (!initialized) {
        initialized = True;
        have_rr = XRRQueryExtension(display, &rr_event_base, &rr_error_base);
        wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    }
    switch (event->type) {
        case KeyPress:
            prologue(event, "KeyPress");
            do_KeyPress(display, event);
            break;
        case KeyRelease:
            prologue(event, "KeyRelease");
            do_KeyRelease(display, event);
            break;
        case ButtonPress:
            prologue(event, "ButtonPress");
            do_ButtonPress(display, event);
            break;
        case ButtonRelease:
            prologue(event, "ButtonRelease");
            do_ButtonRelease(display, event);
            break;
        case MotionNotify:
            prologue(event, "MotionNotify");
            do_MotionNotify(display, event);
            break;
        case EnterNotify:
            prologue(event, "EnterNotify");
            do_EnterNotify(display, event);
            break;
        case LeaveNotify:
            prologue(event, "LeaveNotify");
            do_LeaveNotify(display, event);
            break;
        case FocusIn:
            prologue(event, "FocusIn");
            do_FocusIn(display, event);
            break;
        case FocusOut:
            prologue(event, "FocusOut");
            do_FocusOut(display, event);
            break;
        case KeymapNotify:
            prologue(event, "KeymapNotify");
            do_KeymapNotify(display, event);
            break;
        case Expose:
            prologue(event, "Expose");
            do_Expose(display, event);
            break;
        case GraphicsExpose:
            prologue(event, "GraphicsExpose");
            do_GraphicsExpose(display, event);
            break;
        case NoExpose:
            prologue(event, "NoExpose");
            do_NoExpose(display, event);
            break;
        case VisibilityNotify:
            prologue(event, "VisibilityNotify");
            do_VisibilityNotify(display, event);
            break;
        case CreateNotify:
            prologue(event, "CreateNotify");
            do_CreateNotify(display, event);
            break;
        case DestroyNotify:
            prologue(event, "DestroyNotify");
            do_DestroyNotify(display, event);
            break;
        case UnmapNotify:
            prologue(event, "UnmapNotify");
            do_UnmapNotify(display, event);
            break;
        case MapNotify:
            prologue(event, "MapNotify");
            do_MapNotify(display, event);
            break;
        case MapRequest:
            prologue(event, "MapRequest");
            do_MapRequest(display, event);
            break;
        case ReparentNotify:
            prologue(event, "ReparentNotify");
            do_ReparentNotify(display, event);
            break;
        case ConfigureNotify:
            prologue(event, "ConfigureNotify");
            do_ConfigureNotify(display, event);
            break;
        case ConfigureRequest:
            prologue(event, "ConfigureRequest");
            do_ConfigureRequest(display, event);
            break;
        case GravityNotify:
            prologue(event, "GravityNotify");
            do_GravityNotify(display, event);
            break;
        case ResizeRequest:
            prologue(event, "ResizeRequest");
            do_ResizeRequest(display, event);
            break;
        case CirculateNotify:
            prologue(event, "CirculateNotify");
            do_CirculateNotify(display, event);
            break;
        case CirculateRequest:
            prologue(event, "CirculateRequest");
            do_CirculateRequest(display, event);
            break;
        case PropertyNotify:
            prologue(event, "PropertyNotify");
            do_PropertyNotify(display, event);
            break;
        case SelectionClear:
            prologue(event, "SelectionClear");
            do_SelectionClear(display, event);
            break;
        case SelectionRequest:
            prologue(event, "SelectionRequest");
            do_SelectionRequest(display, event);
            break;
        case SelectionNotify:
            prologue(event, "SelectionNotify");
            do_SelectionNotify(display, event);
            break;
        case ColormapNotify:
            prologue(event, "ColormapNotify");
            do_ColormapNotify(display, event);
            break;
        case ClientMessage:
            prologue(event, "ClientMessage");
            do_ClientMessage(display, event);
            break;
        case MappingNotify:
            prologue(event, "MappingNotify");
            do_MappingNotify(display, event);
            break;
        default:
            if (have_rr) {
                if (event->type == rr_event_base + RRScreenChangeNotify) {
                    prologue(event, "RRScreenChangeNotify");
                    do_RRScreenChangeNotify(display, event);
                    break;
                }
                if (event->type == rr_event_base + RRNotify) {
                    do_RRNotify(display, event);
                    break;
                }
            }
            printf("Unknown event type %d\n", event->type);
            break;
    }
}
