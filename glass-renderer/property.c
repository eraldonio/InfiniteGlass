#include "property.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"

Property *property_allocate(Atom name) {
  Property *prop = malloc(sizeof(Property));
  prop->name = name;
  prop->program = -1;
  prop->name_str = NULL;
  prop->values.bytes = NULL;
  prop->data = NULL;
  return prop;
}

Bool property_load(Property *prop, Window window) {
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  prop->window = window;
  
  if (!prop->name_str) prop->name_str = XGetAtomName(display, prop->name);

  unsigned char *old = prop->values.bytes;
  int old_nitems = prop->nitems;
  int old_format = prop->format;
  prop->values.bytes = NULL;

  XGetWindowProperty(display, window, prop->name, 0, 0, 0, AnyPropertyType,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (prop->type == None) {
    if (old) { XFree(old); return True; }
    return False;
  }
  XGetWindowProperty(display, window, prop->name, 0, bytes_after_return, 0, prop->type,
                     &prop->type, &prop->format, &prop->nitems, &bytes_after_return, &prop->values.bytes);
  Bool changed = !old || old_nitems != prop->nitems || old_format != prop->format || memcmp(old, prop->values.bytes, prop->nitems * prop->format) != 0;

  if (old) XFree(old);
  if (!changed) return False;
  
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) type->load(prop);
  if (DEBUG_ENABLED("prop.changed")) {
    DEBUG("prop.changed", "");
    property_print(prop, stderr);
  }
  
  return True;
}

void property_free(Property *prop) {
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if(type) type->free(prop);
  if (prop->name_str) XFree(prop->name_str);
  if (prop->values.bytes) XFree(prop->values.bytes);
  free(prop);
}

void property_to_gl(Property *prop, Rendering *rendering) {
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) type->to_gl(prop, rendering);
}

void property_print(Property *prop, FILE *fp) {
  PropertyTypeHandler *type = property_type_get(prop->type, prop->name);
  if (type) {
    type->print(prop, fp);
  } else {
    char *type_name = XGetAtomName(display, prop->type);
    fprintf(fp, "%s=<%s>\n", prop->name_str, type_name);
    XFree(type_name);
  }
}

List *properties_load(Window window) {
  List *prop_list = list_create();
  int nr_props;
  Atom *prop_names = XListProperties(display, window, &nr_props);
  for (int i = 0; i < nr_props; i++) {
    Property *prop = property_allocate(prop_names[i]);
    property_load(prop, window);
    list_append(prop_list, (void *) prop);
  }
  return prop_list;
}

Bool properties_update(List *properties, Window window, Atom name) {
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    if (prop->name == name) {
      return property_load(prop, window);
    }   
  }
  Property *prop = property_allocate(name);
  property_load(prop, window);
  list_append(properties, (void *) prop);
  return True;
}

void properties_free(List *properties) {
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    property_free(prop);
  }
  list_destroy(properties);
}

void properties_to_gl(List *properties, Rendering *rendering) {
  gl_check_error("properties_to_gl");
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    property_to_gl(prop, rendering);
    gl_check_error(prop->name_str);
  }
}

void properties_print(List *properties, FILE *fp) {
  fprintf(fp, "Properties:\n");
  for (size_t i = 0; i < properties->count; i++) {
    Property *prop = (Property *) properties->entries[i];
    fprintf(fp, "  ");
    property_print(prop, fp);
  }
  fprintf(fp, "\n");
}

Property *properties_find(List *properties, Atom name) {
  for (size_t idx = 0; idx < properties->count; idx++) {
    Property *p = (Property *) properties->entries[idx];   
    if (p->name == name) {
      return p;
    }
  }
  return NULL;
}

List *property_types = NULL;

void property_type_register(PropertyTypeHandler *handler) {
  if (!property_types) {
    property_types = list_create();
  }
  handler->init(handler);
  list_append(property_types, (void *) handler);
}


PropertyTypeHandler *property_type_get(Atom type, Atom name) {
  if (!property_types) return NULL;
  int best_level = -1;
  PropertyTypeHandler *best_type_handler = NULL;
  for (size_t i = 0; i < property_types->count; i++) {
    PropertyTypeHandler *type_handler = property_types->entries[i];
    if (   (type_handler->type == AnyPropertyType || type_handler->type == type)
        && (type_handler->name == AnyPropertyType || type_handler->name == name)) {
      int level = (  (type_handler->name != AnyPropertyType ? 2 : 0)
                   + (type_handler->type != AnyPropertyType ? 1 : 0));
      if (level > best_level) {
        best_type_handler = type_handler;
      }
    }
  }
  return best_type_handler;
}
