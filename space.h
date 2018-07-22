#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>


typedef struct BBox {
 float x1, y1, x2, y2;
};

Bool bbox_nonzero_intersection(BBox b1, BBox b2);


typedef struct PixelSize {
 uint w, h;
};

typedef struct Item {
 BBox bbox;
 PixelSize size;
 Window window;
 Pixmap pixmap;
 GLXPixmap glxpixmap;
 GLuint texture_id;
};

Item **items_all;

Item *item_get(Window window);
void item_remove(Item *item);
void item_update_texture(Item *item);
