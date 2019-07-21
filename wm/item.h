#ifndef ITEM
#define ITEM

#include "xapi.h"
#include "glapi.h"
#include "shader.h"
#include "view_type.h"

struct ItemTypeStruct;
struct ItemStruct;

typedef struct ItemTypeStruct ItemType;
typedef struct ItemStruct Item;

typedef void ItemTypeConstructor(Item *item, void *args);
typedef void ItemTypeDestructor(Item *item);
typedef void ItemTypeDraw(View *view, Item *item);
typedef void ItemTypeUpdate(Item *item);
typedef Shader *ItemTypeGetShader(Item *);

struct ItemTypeStruct {
  ItemType *base;
  size_t size;
  ItemTypeConstructor *init;
  ItemTypeDestructor *destroy;
  ItemTypeDraw *draw;
  ItemTypeUpdate *update;
  ItemTypeGetShader *get_shader;
};

struct ItemStruct {
  ItemType *type;

  int id;
 
  int width;
  int height;

  float coords[4];
  GLuint coords_vbo;

  uint is_mapped; 

  int _width;
  int _height;

  float _coords[4];
  uint _is_mapped; 
};

extern ItemType item_type_base;

extern Item **items_all;
extern size_t items_all_usage;

Bool item_isinstance(Item *item, ItemType *type);
Item *item_create(ItemType *type, void *args);
Item *item_get(int id);
void item_add(Item *item);
void item_remove(Item *item);

#endif
