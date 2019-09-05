#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "item_shader.h"
#include "wm.h"
#include <limits.h>

void item_type_base_constructor(Item *item, void *args) {}
void item_type_base_destructor(Item *item) {}
void item_type_base_draw(View *view, Item *item) {
  if (item->is_mapped) {
    ItemShader *shader = (ItemShader *) item->type->get_shader(item);

    glUniform1i(shader->picking_mode_attr, view->picking);
    glUniform4fv(shader->screen_attr, 1, view->screen);
    
    glUniform1f(shader->window_id_attr, (float) item->id / (float) INT_MAX);
    
    glEnableVertexAttribArray(shader->coords_attr);
    glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
    glVertexAttribPointer(shader->coords_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_POINTS, 0, 1);
  }
}
void item_type_base_update(Item *item) {
  if (item->coords_vbo == -1) {
    glGenBuffers(1, &item->coords_vbo);
  }
  glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(item->coords), item->coords, GL_STATIC_DRAW);

  item->_width = item->width;
  item->_height = item->height;
  item->_coords[0] = item->coords[0];
  item->_coords[1] = item->coords[1];
  item->_coords[2] = item->coords[2];
  item->_coords[3] = item->coords[3];
  item->_is_mapped = item->is_mapped;
}

Shader *item_type_base_get_shader(Item *item) {
  return NULL;
}

void item_type_base_print(Item *item) {
  printf("%s(%d):%s [%d,%d] @ %f,%f,%f,%f\n",
         item->type->name,
         item->id,
         item->is_mapped ? "" : " invisible",
         item->width,
         item->height,
         item->coords[0],
         item->coords[1],
         item->coords[2],
         item->coords[3]);
}

ItemType item_type_base = {
  NULL,
  sizeof(Item),
  "ItemBase",
  &item_type_base_constructor,
  &item_type_base_destructor,
  &item_type_base_draw,
  &item_type_base_update,
  &item_type_base_get_shader,
  &item_type_base_print
};

List *items_all = NULL;
size_t items_all_id = 0;

Bool item_isinstance(Item *item, ItemType *type) {
  ItemType *item_type;
  for (item_type = item->type; item_type && item_type != type; item_type = item_type->base);
  return !!item_type;
}

Item *item_create(ItemType *type, void *args) {
  Item *item = (Item *) malloc(type->size);

  item->_width = 0;
  item->_height = 0;
  item->width = 0;
  item->height = 0;
  item->_coords[0] = 0.0;
  item->_coords[1] = 0.0;
  item->_coords[2] = 0.0;
  item->_coords[3] = 0.0;
  item->coords[0] = 0.0;
  item->coords[1] = 0.0;
  item->coords[2] = 0.0;
  item->coords[3] = 0.0;
  item->coords_vbo = -1;
  item->is_mapped = False;
  item->_is_mapped = False;
  item->type = type;
  item->type->init(item, args);
  item_add(item);
  item->type->update(item);
  return item;
}

Item *item_get(int id) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      if (item->id == id) return item;
    }
  }
  return NULL;
}

void item_add(Item *item) {
  if (!items_all) items_all = list_create();
  item->id = ++items_all_id;  
  list_append(items_all, (void *) item);
}

void item_remove(Item *item) {
  list_remove(items_all, (void *) item);
  item->type->destroy(item);
}
