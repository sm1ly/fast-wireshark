/*
* This file is part of FAST Wireshark.
*
* FAST Wireshark is free software: you can redistribute it and/or modify
* it under the terms of the Lesser GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* FAST Wireshark is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* Lesser GNU General Public License for more details.
*
* You should have received a copy of the Lesser GNU General Public License
* along with FAST Wireshark.  If not, see 
* <http://www.gnu.org/licenses/lgpl.txt>.
*/

/*!
 * \file template.c
 * \brief  Handle template storage/lookups.
 */

#include "debug.h"
#include "decode.h"
#include "template.h"

static GHashTable* template_table = 0;
static GNode* template_tree = 0;

static gboolean requires_pmap_bit (const FieldType* ftype);
static void fixup_walk_template (FieldType* parent, GNode* parent_node);


void add_templates (GNode* templates)
{
  GNode* tmpl;  

  /* TODO: Clear hash table and free old templates. */

  if (!template_table) {
    template_table = g_hash_table_new (&g_int_hash, &g_int_equal);
  }
  
  if(!templates){
    return;
  }
  
  template_tree = templates;

  if (!template_table)  BAILOUT(;,"Template lookup table not created.");

  /* Loop thru templates, add each to lookup table. */
  for (tmpl = templates->children;  tmpl;  tmpl = tmpl->next) {
    FieldType* tfield;
    tfield = (FieldType*) tmpl->data;
    tfield->value.pmap_exists = TRUE;
    fixup_walk_template (tfield, tmpl);
    g_hash_table_insert (template_table, &tfield->id, tmpl);
  }
}


const gchar* field_typename (FieldTypeIdentifier type)
{
  static const gchar* names[] =
  {
    "uInt32", "uInt64", "int32", "int64",
    "decimal", "ascii", "unicode", "byteVector",
    "group", "sequence"
  };
  if (0 <= type && type < FieldTypeEnumLimit) {
    return names[type];
  }
  else {
    DBG1("Unknown type %d", type);
    return "";
  }
}


const gchar* operator_typename (FieldOperatorIdentifier type)
{
  static const gchar* names[] =
  {
    "no_operator", "constant", "default", "copy",
    "increment", "delta", "tail"
  };
  if(0<= type && type < FieldOperatorEnumLimit) {
    return names[type];
  }
  else {
    DBG1("Unknown type %d", type);
    return "";
  }
}


GNode* create_field (FieldTypeIdentifier type,
                     FieldOperatorIdentifier op)
{
  FieldType* field;
  GNode* node;
  field = g_malloc (sizeof (FieldType));
  if (!field)  BAILOUT(0, "Error g_malloc().");

  node = g_node_new (field);
  if (!node) {
    g_free (field);
    BAILOUT(0, "Error g_new_node().");
  }

  field->name       = 0;
  field->id         = 0;
  field->key        = 0;
  field->mandatory  = TRUE;
  field->type       = type;
  field->op         = op;
  field->hasDefault = FALSE;
  init_field_value(&field->value);
  field->dictionary = 0;

  return node;
}


GNode* find_template (guint32 id)
{
  gint key;
  key = (gint) id;
  return (GNode*) g_hash_table_lookup (template_table, &key);
}

/*! \brief  Check if a field type needs a bit in the PMAP. */
gboolean requires_pmap_bit (const FieldType* ftype)
{
  if (ftype->type == FieldTypeGroup) {
    return !ftype->mandatory;
  }
  switch (ftype->op) {
    case FieldOperatorConstant:
      return !ftype->mandatory;
    case FieldOperatorDefault:
    case FieldOperatorCopy:
    case FieldOperatorIncrement:
    case FieldOperatorTail:
      return TRUE;
    default:
      return FALSE;
  }
}

/*! \brief  Propagate data down and up the type tree.
 *
 * \param parent  The parent group. Not necessarily contained in parent_node.
 * \param parent_node  The node on whose children this function will operate.
 */
void fixup_walk_template (FieldType* parent, GNode* parent_node)
{
  GNode* tnode;
  for (tnode = parent_node->children;  tnode;  tnode = tnode->next) {
    FieldType* ftype;
    ftype = (FieldType*) tnode->data;
    if (!ftype) {
      DBG0("Null field type.");
      continue;
    }
    if (!parent->value.pmap_exists) {
      if (requires_pmap_bit (ftype)) {
        parent->value.pmap_exists = TRUE;
      }
    }
    /* Only have this field as a parent to recursion if it is a group
     * as only then will it be able to contain a PMAP.
     */
    if (FieldTypeGroup == ftype->type) {
      fixup_walk_template (ftype, tnode);
    }
    else {
      fixup_walk_template (parent, tnode);
    }
  }
}


GNode* full_templates_tree ()
{
  return template_tree;
}

