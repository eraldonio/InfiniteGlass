#ifndef ITEM_WIDGET
#define ITEM_WIDGET

#include "item.h"
#include "view.h"
#include "texture.h"
#include <cairo.h>

typedef struct {
  int x;
  int y;
  int width;
  int height;
  int itemwidth;
  int itemheight;
  cairo_surface_t *surface;
  Texture texture;
} WidgetItemTile;

#define WIDGET_ITEM_TILE_CACHE_SIZE 5

typedef struct {
  Item base;
  char *label;
  WidgetItemTile tiles[WIDGET_ITEM_TILE_CACHE_SIZE];
} WidgetItem;

extern ItemType item_type_widget;

extern Item *item_get_widget(char *label);

#endif