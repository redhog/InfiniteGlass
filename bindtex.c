#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>

Display                 *dpy;
Window                  root;
GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *vi;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              glc;
Pixmap                  pixmap;
int                     pixmap_width = 128, pixmap_height = 128;
GC                      gc;
GLuint                  texture_id;

typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);

t_glx_bind glXBindTexImageEXT = 0;
t_glx_release glXReleaseTexImageEXT = 0;

GLXFBConfig * configs = 0;

const int pixmap_config[] = {
    GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
    GLX_DOUBLEBUFFER, False,
    GLX_Y_INVERTED_EXT, GLX_DONT_CARE,
    None
};

const int pixmap_attribs[] = {
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
    None
};

GLXPixmap glxpixmap = 0;

void Redraw() {
    XWindowAttributes  gwa;

    XGetWindowAttributes(dpy, win, &gwa);
    glViewport(0, 0, gwa.width, gwa.height);
    glClearColor(0.3, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.25, 1.25, -1.25, 1.25, 1., 20.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

    glColor3f(1.0, 1.0, 1.0);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,  1.0, 0.0);
    glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, 0.0);
    glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, -1.0, 0.0);
    glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 0.0);
    glEnd(); 

    glXSwapBuffers(dpy, win);
}

/*                */
/*  MAIN PROGRAM  */
/*                */
int main(int argc, char *argv[]) {
    XEvent     xev;

    dpy = XOpenDisplay(NULL);

    if(dpy == NULL) {
        printf("\n\tcannot open display\n\n");
        exit(0); 
    }

    root = DefaultRootWindow(dpy);

    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(0);
    }

    swa.event_mask = ExposureMask | KeyPressMask;
    swa.colormap   = XCreateColormap(dpy, root, vi->visual, AllocNone);

    win = XCreateWindow(dpy, root, 0, 0, 600, 600, 0, vi->depth, InputOutput, vi->visual, CWEventMask  | CWColormap, &swa);
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "PIXMAP TO TEXTURE");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);

    if(glc == NULL) {
        printf("\n\tcannot create gl context\n\n");
        exit(0);
    }

    glXMakeCurrent(dpy, win, glc);
    glEnable(GL_DEPTH_TEST);

    /* CREATE A PIXMAP AND DRAW SOMETHING */

    pixmap = XCreatePixmap(dpy, root, pixmap_width, pixmap_height, vi->depth);
    gc = DefaultGC(dpy, 0);

    XSetForeground(dpy, gc, 0x00c0c0);
    XFillRectangle(dpy, pixmap, gc, 0, 0, pixmap_width, pixmap_height);

    XSetForeground(dpy, gc, 0x000000);
    XFillArc(dpy, pixmap, gc, 15, 25, 50, 50, 0, 360*64);

    XSetForeground(dpy, gc, 0x0000ff);
    XDrawString(dpy, pixmap, gc, 10, 15, "PIXMAP TO TEXTURE", strlen("PIXMAP TO TEXTURE"));

    XSetForeground(dpy, gc, 0xff0000);
    XFillRectangle(dpy, pixmap, gc, 75, 75, 45, 35);

    XFlush(dpy);

    // Create a texture with GLX_texture_from_pixmap

    const char * exts = glXQueryExtensionsString(dpy, 0);

    printf("Extensions: %s\n", exts);
    if(! strstr(exts, "GLX_EXT_texture_from_pixmap"))
    {
        fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported!\n");
        return 1;
    }

    glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
    glXReleaseTexImageEXT = (t_glx_release) glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");

    if(!glXBindTexImageEXT || !glXReleaseTexImageEXT)
    {
        fprintf(stderr, "Some extension functions missing!");
        return 1;
    }

    int c=0;

    configs = glXChooseFBConfig(dpy, 0, pixmap_config, &c);
    if(!configs)
    {
        fprintf(stderr, "No appropriate GLX FBConfig available!\n");
        return 1;
    }

    glxpixmap = glXCreatePixmap(dpy, configs[0], pixmap, pixmap_attribs);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glXBindTexImageEXT(dpy, glxpixmap, GLX_FRONT_EXT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    while(1) {
        XNextEvent(dpy, &xev);

        if(xev.type == Expose) {
            Redraw();
        }

        else if(xev.type == KeyPress) 
        {
            glXReleaseTexImageEXT(dpy, glxpixmap, GLX_FRONT_EXT);
            XFree(configs);
            XFreePixmap(dpy, pixmap);

            glXMakeCurrent(dpy, None, NULL);
            glXDestroyContext(dpy, glc);
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
            exit(0);
        } 

    } /* while(1) */

} /* int main(...) */
