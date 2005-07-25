/* GObject introspection: metadata validation, auxiliary functions 
 * related to the binary metadata format
 *
 * Copyright (C) 2005 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "gmetadata.h"


#define ALIGN_VALUE(this, boundary) \
  (( ((unsigned long)(this)) + (((unsigned long)(boundary)) -1)) & (~(((unsigned long)(boundary))-1)))


DirEntry *
g_metadata_get_dir_entry (const guchar *metadata,
			  guint16       index)
{
  Header *header = (Header *)metadata;

  return (DirEntry *)&metadata[header->directory + (index - 1) * header->entry_blob_size];
}

void    
g_metadata_check_sanity (void)
{
  /* Check that struct layout is as we expect */
  g_assert (sizeof (Header) == 84);
  g_assert (sizeof (DirEntry) == 12);
  g_assert (sizeof (SimpleTypeBlob) == 4);
  g_assert (sizeof (ArgBlob) == 12);
  g_assert (sizeof (SignatureBlob) == 8);
  g_assert (sizeof (CommonBlob) == 8);
  g_assert (sizeof (FunctionBlob) == 16);
  g_assert (sizeof (InterfaceTypeBlob) == 4);
  g_assert (sizeof (ArrayTypeBlob) == 8);
  g_assert (sizeof (ParamTypeBlob) == 4);
  g_assert (sizeof (ErrorTypeBlob) == 4);
  g_assert (sizeof (ErrorDomainBlob) == 16);
  g_assert (sizeof (ValueBlob) == 12);
  g_assert (sizeof (FieldBlob) == 12);
  g_assert (sizeof (RegisteredTypeBlob) == 16);
  g_assert (sizeof (StructBlob) == 20);
  g_assert (sizeof (EnumBlob) == 20);
  g_assert (sizeof (PropertyBlob) == 12);
  g_assert (sizeof (SignalBlob) == 12);
  g_assert (sizeof (VFuncBlob) == 16);
  g_assert (sizeof (ObjectBlob) == 32);
  g_assert (sizeof (InterfaceBlob) == 28);
  g_assert (sizeof (ConstantBlob) == 20);
  g_assert (sizeof (AnnotationBlob) == 12);
  g_assert (sizeof (UnionBlob) == 28);
}


static gboolean
is_aligned (guint32 offset)
{
  return offset == ALIGN_VALUE (offset, 4);
}

#define MAX_NAME_LEN 200

static gboolean
is_name (const guchar *data, guint32 offset)
{
  gchar *name;

  name = (gchar*)&data[offset];
  
  if (!memchr (name, '\0', MAX_NAME_LEN))
    return FALSE;
  
  if (strspn (name, G_CSET_a_2_z G_CSET_A_2_Z G_CSET_DIGITS "-_") < strlen (name))
    return FALSE;
  
  return TRUE;
}

static gboolean 
validate_header (const guchar  *data,
		 gint           len,
		 GError       **error)
{
  Header *header;

  if (len < sizeof (Header))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  header = (Header *)data;

  if (strncmp (header->magic, G_IDL_MAGIC, 16) != 0)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Magic string not found");
      return FALSE;
      
    }

  if (header->major_version != 1 || header->minor_version != 0)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Version mismatch");
      return FALSE;
      
    }

  if (header->n_entries < header->n_local_entries)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Inconsistent entry counts");
      return FALSE; 
    }

  if (header->size != len)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Metadata size mismatch");
      return FALSE; 
    }

  if (header->entry_blob_size != 12 ||
      header->function_blob_size != 16 ||
      header->callback_blob_size != 12 ||
      header->signal_blob_size != 12 ||
      header->vfunc_blob_size != 16 ||
      header->arg_blob_size != 12 ||
      header->property_blob_size != 12 ||
      header->field_blob_size != 12 ||
      header->value_blob_size != 12 ||
      header->constant_blob_size != 20 ||
      header->error_domain_blob_size != 16 ||
      header->annotation_blob_size != 12 ||
      header->signature_blob_size != 8 ||
      header->enum_blob_size != 20 ||
      header->struct_blob_size != 20 ||
      header->object_blob_size != 32 ||
      header->interface_blob_size != 28 ||
      header->union_blob_size != 28)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Blob size mismatch");
      return FALSE; 
    }

  if (!is_aligned (header->directory))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Misaligned directory");
      return FALSE; 
    }

  if (!is_aligned (header->annotations))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Misaligned annotations");
      return FALSE; 
    }

  if (header->annotations == 0 && header->n_annotations > 0)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Wrong number of annotations");
      return FALSE; 
    }

  if (!is_name (data, header->namespace))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_HEADER,
		   "Invalid namespace name");
      return FALSE; 
    }

  return TRUE;
}

static gboolean validate_type_blob (const guchar  *data,
				    guint          len,
				    guint32        offset,
				    guint32        signature_offset,
				    gboolean       return_type,
				    GError       **error);

static gboolean
validate_array_type_blob (const guchar  *data,
			  guint          len,
			  guint32        offset,
			  guint32        signature_offset,
			  gboolean       return_type,
			  GError       **error)
{
  ArrayTypeBlob *blob;

  blob = (ArrayTypeBlob*)&data[offset];

  if (!blob->pointer)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Pointer type exected for tag %d", blob->tag);
      return FALSE;	  
    }

  /* FIXME validate length */

  if (!validate_type_blob (data, len,
			   offset + G_STRUCT_OFFSET (ArrayTypeBlob, type),
			   0, FALSE, error))
    return FALSE;

  return TRUE;
}

static gboolean
validate_iface_type_blob (const guchar  *data,
			  guint          len,
			  guint32        offset,
			  guint32        signature_offset,
			  gboolean       return_type,
			  GError       **error)
{
  InterfaceTypeBlob *blob;
  Header *header;

  header = (Header *)data;

  blob = (InterfaceTypeBlob*)&data[offset];

  if (blob->interface == 0 || blob->interface > header->n_entries)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid directory index %d", blob->interface);
      return FALSE;	        
    }

  return TRUE;
}

static gboolean
validate_param_type_blob (const guchar  *data,
			  guint          len,
			  guint32        offset,
			  guint32        signature_offset,
			  gboolean       return_type,
			  gint           n_params,
			  GError       **error)
{
  ParamTypeBlob *blob;
  gint i;

  blob = (ParamTypeBlob*)&data[offset];

  if (!blob->pointer)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Pointer type exected for tag %d", blob->tag);
      return FALSE;	  
    }
  
  if (blob->n_types != n_params)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Parameter type number mismatch");
      return FALSE;	        
    }
  
  for (i = 0; i < n_params; i++)
    {
      if (!validate_type_blob (data, len,
			       offset + sizeof (ParamTypeBlob) +
			       i * sizeof (SimpleTypeBlob),
			       0, FALSE, error))
	return FALSE;
    }

  return TRUE;
}

static gboolean
validate_error_type_blob (const guchar  *data,
			  guint          len,
			  guint32        offset,
			  guint32        signature_offset,
			  gboolean       return_type,
			  GError       **error)
{
  ErrorTypeBlob *blob;
  Header *header;
  gint i;
  DirEntry *entry;

  blob = (ErrorTypeBlob*)&data[offset];

  header = (Header *)data;

  if (!blob->pointer)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Pointer type exected for tag %d", blob->tag);
      return FALSE;	  
    }
  
  for (i = 0; i < blob->n_domains; i++)
    {
      if (blob->domains[i] == 0 || blob->domains[i] > header->n_entries)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid directory index %d", blob->domains[i]);
	  return FALSE;	        
	}

      entry = g_metadata_get_dir_entry (data, blob->domains[i]);

      if (entry->blob_type != BLOB_TYPE_ERROR_DOMAIN &&
	  (entry->local || entry->blob_type != BLOB_TYPE_INVALID))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Wrong blob type");
	  return FALSE;	        
	}
    }

  return TRUE;
}

static gboolean
validate_type_blob (const guchar  *data,
		    guint          len,
		    guint32        offset,
		    guint32        signature_offset,
		    gboolean       return_type,
		    GError       **error)
{
  SimpleTypeBlob *simple;
  InterfaceTypeBlob *iface;
  
  simple = (SimpleTypeBlob *)&data[offset];

  if (simple->reserved == 0 && 
      simple->reserved2 == 0)
    {
      if (simple->tag >= TYPE_TAG_ARRAY)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Wrong tag in simple type");
	  return FALSE;
	}
      
      if (simple->tag >= TYPE_TAG_UTF8 &&
	  !simple->pointer)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Pointer type exected for tag %d", simple->tag);
	  return FALSE;	  
	}

      return TRUE;
    }

  iface = (InterfaceTypeBlob*)&data[simple->offset];

  switch (iface->tag)
    {
    case TYPE_TAG_ARRAY:
      if (!validate_array_type_blob (data, len, simple->offset, 
				     signature_offset, return_type, error))
	return FALSE;
      break;
    case TYPE_TAG_INTERFACE:
      if (!validate_iface_type_blob (data, len, simple->offset, 
				     signature_offset, return_type, error))
	return FALSE;
      break;
    case TYPE_TAG_LIST:
    case TYPE_TAG_SLIST:
      if (!validate_param_type_blob (data, len, simple->offset, 
				     signature_offset, return_type, 1, error))
	return FALSE;
      break;
    case TYPE_TAG_HASH:
      if (!validate_param_type_blob (data, len, simple->offset, 
				     signature_offset, return_type, 2, error))
	return FALSE;
      break;
    case TYPE_TAG_ERROR:
      if (!validate_error_type_blob (data, len, simple->offset, 
				     signature_offset, return_type, error))
	return FALSE;
      break;
    default:
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong tag in complex type");
      return FALSE;
    }

  return TRUE;
}

static gboolean
validate_arg_blob (const guchar  *data,
		   guint          len,
		   guint32        offset,
		   guint32        signature_offset,
		   GError       **error)
{
  ArgBlob *blob;

  if (len < offset + sizeof (ArgBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (ArgBlob*) &data[offset];

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid argument name");
      return FALSE; 
    }
 
  if (!validate_type_blob (data, len, 
			   offset + G_STRUCT_OFFSET (ArgBlob, arg_type), 
			   signature_offset, FALSE, error))
    return FALSE;

  return TRUE;
}

static gboolean
validate_signature_blob (const guchar  *data,
			 guint          len,
			 guint32        offset,
			 GError       **error)
{
  SignatureBlob *blob;
  gint i;

  if (len < offset + sizeof (SignatureBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (SignatureBlob*) &data[offset];

  if (blob->return_type.offset != 0)
    {
      if (!validate_type_blob (data, len, 
			       offset + G_STRUCT_OFFSET (SignatureBlob, return_type), 
			       offset, TRUE, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_arguments; i++)
    {
      if (!validate_arg_blob (data, len, 
			      offset + sizeof (SignatureBlob) + 
			      i * sizeof (ArgBlob), 
			      offset, 
			      error))
	return FALSE;
    }

  /* FIXME check constraints on return_value */
  /* FIXME check array-length pairs */
  return TRUE;
}

static gboolean
validate_function_blob (const guchar  *data,
			guint          len,
			guint32        offset,
			guint16        container_type,
			GError       **error)
{
  FunctionBlob *blob;

  if (len < offset + sizeof (FunctionBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (FunctionBlob*) &data[offset];

  if (blob->blob_type != BLOB_TYPE_FUNCTION)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid function name");
      return FALSE; 
    }
  
  if (!is_name (data, blob->symbol))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid function symbol");
      return FALSE; 
    }
  
  if (blob->constructor)
    {
      switch (container_type)
	{
	case BLOB_TYPE_BOXED:
	case BLOB_TYPE_OBJECT:
	case BLOB_TYPE_INTERFACE:
	  break;
	default:
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Constructor not allowed");
	  return FALSE;
	}
    }

  if (blob->setter || blob->getter || blob->wraps_vfunc)
    {
      switch (container_type)
	{
	case BLOB_TYPE_OBJECT:
	case BLOB_TYPE_INTERFACE:
	  break;
	default:
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Setter, getter or wrapper not allowed");
	  return FALSE;
	}
    }

  if (blob->index)
    {
      if (!(blob->setter || blob->getter || blob->wraps_vfunc))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Must be setter, getter or wrapper");
	  return FALSE;
	}
    }

  /* FIXME: validate index range */
  /* FIXME: validate "this" argument for methods */
  /* FIXME: validate return type for constructors */

  if (!validate_signature_blob (data, len, blob->signature, error))
    return FALSE;
	
  return TRUE;
}

static gboolean
validate_callback_blob (const guchar  *data,
			guint          len,
			guint32        offset,
			GError       **error)
{
  CallbackBlob *blob;

  if (len < offset + sizeof (CallbackBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (CallbackBlob*) &data[offset];

  if (blob->blob_type != BLOB_TYPE_CALLBACK)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid callback name");
      return FALSE; 
    }
  
  if (!validate_signature_blob (data, len, blob->signature, error))
    return FALSE;
	
  return TRUE;
}

static gboolean
validate_constant_blob (const guchar  *data,
			guint          len,
			guint32        offset,
			GError       **error)
{
  gint value_size[] = {
    0, 4, 1, 1, 2, 2, 4, 4, 8, 8, 
    sizeof (gint), sizeof (guint), 
    sizeof (glong), sizeof (gulong),
    sizeof (gssize), sizeof (gsize),
    sizeof (gfloat), sizeof (gdouble), 
    0, 0
  }; 
  ConstantBlob *blob;
  SimpleTypeBlob *type;

  if (len < offset + sizeof (ConstantBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (ConstantBlob*) &data[offset];

  if (blob->blob_type != BLOB_TYPE_CONSTANT)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid constant name");
      return FALSE; 
    }
  
  if (!validate_type_blob (data, len, offset + G_STRUCT_OFFSET (ConstantBlob, type), 
			   0, FALSE, error))
    return FALSE;

  if (!is_aligned (blob->offset))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Misaligned constant value");
      return FALSE;
    }
  
  type = (SimpleTypeBlob *)&data[offset + G_STRUCT_OFFSET (ConstantBlob, type)];
  if (type->reserved == 0)
    {
      if (type->tag == 0)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Constant value type void");
	  return FALSE;
	}

      if (value_size[type->tag] != 0 &&
	  blob->size != value_size[type->tag])
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Constant value size mismatch");
	  return FALSE;
	}
      /* FIXME check string values */
    }

  return TRUE;
}

static gboolean
validate_value_blob (const guchar  *data,
		     guint          len,
		     guint32        offset,
		     GError       **error)
{
  ValueBlob *blob;

  if (len < offset + sizeof (ValueBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (ValueBlob*) &data[offset];

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid value name");
      return FALSE; 
    }
  
  return TRUE;
}

static gboolean
validate_field_blob (const guchar  *data,
		     guint          len,
		     guint32        offset,
		     GError       **error)
{
  FieldBlob *blob;

  if (len < offset + sizeof (FieldBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (FieldBlob*) &data[offset];
  
  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid field name");
      return FALSE; 
    }
    
  if (!validate_type_blob (data, len, 
			   offset + G_STRUCT_OFFSET (FieldBlob, type), 
			   0, FALSE, error))
    return FALSE;

  return TRUE;
}

static gboolean
validate_property_blob (const guchar  *data,
			guint          len,
			guint32        offset,
			GError       **error)
{
  PropertyBlob *blob;

  if (len < offset + sizeof (PropertyBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (PropertyBlob*) &data[offset];
  
  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid property name");
      return FALSE; 
    }
    
  if (!validate_type_blob (data, len, 
			   offset + G_STRUCT_OFFSET (PropertyBlob, type), 
			   0, FALSE, error))
    return FALSE;

  return TRUE;
}

static gboolean
validate_signal_blob (const guchar  *data,
		      guint          len,
		      guint32        offset,
		      guint32        container_offset,
		      GError       **error)
{
  SignalBlob *blob;
  gint n_signals;

  if (len < offset + sizeof (SignalBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (SignalBlob*) &data[offset];

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid signal name");
      return FALSE; 
    }
  
  if ((blob->run_first != 0) + 
      (blob->run_last != 0) + 
      (blob->run_cleanup != 0) != 1)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid signal run flags");
      return FALSE; 
    }

  if (blob->has_class_closure)
    {
      if (((CommonBlob*)&data[container_offset])->blob_type == BLOB_TYPE_OBJECT)
	{
	  ObjectBlob *object;

	  object = (ObjectBlob*)&data[container_offset];
	  
	  n_signals = object->n_signals;
	}
      else
	{
	  InterfaceBlob *iface;
	  
	  iface = (InterfaceBlob*)&data[container_offset];
	  
	  n_signals = iface->n_signals;
	}

      if (blob->class_closure >= n_signals)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid class closure index");
	  return FALSE; 
	}
    }

  if (!validate_signature_blob (data, len, blob->signature, error))
    return FALSE;
  
  return TRUE;
}

static gboolean
validate_vfunc_blob (const guchar  *data,
		     guint          len,
		     guint32        offset,
		     guint32        container_offset,
		     GError       **error)
{
  VFuncBlob *blob;
  gint n_vfuncs;

  if (len < offset + sizeof (VFuncBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (VFuncBlob*) &data[offset];

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid vfunc name");
      return FALSE; 
    }
  
  if (blob->class_closure)
    {
      if (((CommonBlob*)&data[container_offset])->blob_type == BLOB_TYPE_OBJECT)
	{
	  ObjectBlob *object;

	  object = (ObjectBlob*)&data[container_offset];
	  
	  n_vfuncs = object->n_vfuncs;
	}
      else
	{
	  InterfaceBlob *iface;
	  
	  iface = (InterfaceBlob*)&data[container_offset];
	  
	  n_vfuncs = iface->n_vfuncs;
	}

      if (blob->class_closure >= n_vfuncs)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid class closure index");
	  return FALSE; 
	}
    }

  if (!validate_signature_blob (data, len, blob->signature, error))
    return FALSE;
  
  return TRUE;
}

static gboolean
validate_struct_blob (const guchar  *data,
		      guint          len,
		      guint32        offset,
		      guint16        blob_type,
		      GError       **error)
{
  StructBlob *blob;
  gint i;

  if (len < offset + sizeof (StructBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (StructBlob*) &data[offset];

  if (blob->blob_type != blob_type)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }
  
  if ((blob->blob_type == BLOB_TYPE_BOXED && blob->unregistered) ||
      (blob->blob_type == BLOB_TYPE_STRUCT && !blob->unregistered))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Registration/blob type mismatch");
      return FALSE;
    }

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid struct name");
      return FALSE; 
    }
  
  if (blob_type == BLOB_TYPE_BOXED)
    {
      if (!is_name (data, blob->gtype_name))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid boxed type name");
	  return FALSE; 
	}

      if (!is_name (data, blob->gtype_init))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid boxed type init");
	  return FALSE; 
	}
    }
  else
    {
      if (blob->gtype_name || blob->gtype_init)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Gtype data in struct");
	  return FALSE; 
	}
    }

  if (len < offset + sizeof (StructBlob) + 
            blob->n_fields * sizeof (FieldBlob) +
            blob->n_methods * sizeof (FunctionBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  for (i = 0; i < blob->n_fields; i++)
    {
      if (!validate_field_blob (data, len, 
				offset + sizeof (StructBlob) + 
				i * sizeof (FieldBlob), 
				error))
	return FALSE;
    }

  for (i = 0; i < blob->n_methods; i++)
    {
      if (!validate_function_blob (data, len, 
				   offset + sizeof (StructBlob) + 
				   blob->n_fields * sizeof (FieldBlob) + 
				   i * sizeof (FunctionBlob), 
				   BLOB_TYPE_STRUCT,
				   error))
	return FALSE;
    }

  return TRUE;
}

static gboolean
validate_enum_blob (const guchar  *data,
		    guint          len,
		    guint32        offset,
		    guint16        blob_type,
		    GError       **error)
{
  EnumBlob *blob;
  ValueBlob *v1, *v2;
  gint i, j; 

  if (len < offset + sizeof (EnumBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (EnumBlob*) &data[offset];

  if (blob->blob_type != blob_type)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }
  
  if (!blob->unregistered)
    {
      if (!is_name (data, blob->gtype_name))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid enum type name");
	  return FALSE; 
	}

      if (!is_name (data, blob->gtype_init))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid enum type init");
	  return FALSE; 
	}
    }
  else
    {
      if (blob->gtype_name || blob->gtype_init)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Gtype data in unregistered enum");
	  return FALSE; 
	}
    }

  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid enum name");
      return FALSE; 
    }
  
  if (len < offset + sizeof (EnumBlob) + 
      blob->n_values * sizeof (ValueBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }
  
  for (i = 0; i < blob->n_values; i++)
    {
      if (!validate_value_blob (data, len, 
				offset + sizeof (EnumBlob) + 
				i * sizeof (ValueBlob), 
				error))
	return FALSE;

      v1 = (ValueBlob *)&data[offset + sizeof (EnumBlob) + 
			      i * sizeof (ValueBlob)];
      for (j = 0; j < i; j++) 
	{
	  v2 = (ValueBlob *)&data[offset + sizeof (EnumBlob) + 
				  j * sizeof (ValueBlob)];

	  if (v1->value == v2->value)
	    {
	      /* FIXME should this be an error ? */
	      g_set_error (error,
			   G_METADATA_ERROR,
			   G_METADATA_ERROR_INVALID_BLOB,
			   "Duplicate enum value");
	      return FALSE;
	    }
	}
    }
  
  return TRUE;
}

static gboolean
validate_object_blob (const guchar  *data,
		      guint          len,
		      guint32        offset,
		      GError       **error)
{
  Header *header;
  ObjectBlob *blob;
  gint i;
  guint32 offset2;

  header = (Header *)data;

  if (len < offset + sizeof (ObjectBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (ObjectBlob*) &data[offset];

  if (blob->blob_type != BLOB_TYPE_OBJECT)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }
  
  if (!is_name (data, blob->gtype_name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid object type name");
      return FALSE; 
    }
  
  if (!is_name (data, blob->gtype_init))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid object type init");
      return FALSE; 
    }
  
  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid object name");
      return FALSE; 
    }
  
  if (blob->parent > header->n_entries)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid parent index");
      return FALSE; 
    }

  if (blob->parent != 0)
    {
      DirEntry *entry;

      entry = g_metadata_get_dir_entry (data, blob->parent);
      if (entry->blob_type != BLOB_TYPE_OBJECT &&
	  (entry->local || entry->blob_type != 0))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Parent not object");
	  return FALSE; 
	}
    }
  
  if (len < offset + sizeof (ObjectBlob) + 
            (blob->n_interfaces + blob->n_interfaces % 2) * 2 +
            blob->n_fields * sizeof (FieldBlob) +
            blob->n_properties * sizeof (PropertyBlob) +
            blob->n_methods * sizeof (FunctionBlob) +
            blob->n_signals * sizeof (SignalBlob) +
            blob->n_vfuncs * sizeof (VFuncBlob) +
            blob->n_constants * sizeof (ConstantBlob))
     
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  offset2 = offset + sizeof (ObjectBlob);

  for (i = 0; i < blob->n_interfaces; i++, offset2 += 2)
    {
      guint16 iface;
      DirEntry *entry;

      iface = *(guint16*)&data[offset2];
      if (iface == 0 || iface > header->n_entries)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid interface index");
	  return FALSE; 
	}
      
      entry = g_metadata_get_dir_entry (data, iface);

      if (entry->blob_type != BLOB_TYPE_INTERFACE &&
	  (entry->local || entry->blob_type != 0))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Not an interface");
	  return FALSE; 
	}
    }

  offset2 += 2 * (blob->n_interfaces %2);

  for (i = 0; i < blob->n_fields; i++, offset2 += sizeof (FieldBlob))
    {
      if (!validate_field_blob (data, len, offset2, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_properties; i++, offset2 += sizeof (PropertyBlob))
    {
      if (!validate_property_blob (data, len, offset2, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_methods; i++, offset2 += sizeof (FunctionBlob))
    {
      if (!validate_function_blob (data, len, offset2, BLOB_TYPE_OBJECT, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_signals; i++, offset2 += sizeof (SignalBlob))
    {
      if (!validate_signal_blob (data, len, offset2, offset, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_vfuncs; i++, offset2 += sizeof (VFuncBlob))
    {
      if (!validate_vfunc_blob (data, len, offset2, offset, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_constants; i++, offset2 += sizeof (ConstantBlob))
    {
      if (!validate_constant_blob (data, len, offset2, error))
	return FALSE;
    }

  return TRUE;
}

static gboolean
validate_interface_blob (const guchar  *data,
			 guint          len,
			 guint32        offset,
			 GError       **error)
{
  Header *header;
  InterfaceBlob *blob;
  gint i;
  guint32 offset2;
  
  header = (Header *)data;

  if (len < offset + sizeof (InterfaceBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  blob = (InterfaceBlob*) &data[offset];

  if (blob->blob_type != BLOB_TYPE_INTERFACE)
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Wrong blob type");
      return FALSE;
    }
  
  if (!is_name (data, blob->gtype_name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid interface type name");
      return FALSE; 
    }
  
  if (!is_name (data, blob->gtype_init))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid interface type init");
      return FALSE; 
    }
  
  if (!is_name (data, blob->name))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_BLOB,
		   "Invalid interface name");
      return FALSE; 
    }
  
  if (len < offset + sizeof (InterfaceBlob) + 
            (blob->n_prerequisites + blob->n_prerequisites % 2) * 2 +
            blob->n_properties * sizeof (PropertyBlob) +
            blob->n_methods * sizeof (FunctionBlob) +
            blob->n_signals * sizeof (SignalBlob) +
            blob->n_vfuncs * sizeof (VFuncBlob) +
            blob->n_constants * sizeof (ConstantBlob))
     
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  offset2 = offset + sizeof (InterfaceBlob);

  for (i = 0; i < blob->n_prerequisites; i++, offset2 += 2)
    {
      DirEntry *entry;
      guint16 req;

      req = *(guint16*)&data[offset2];
      if (req == 0 || req > header->n_entries)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Invalid prerequisite index");
	  return FALSE; 
	}

      entry = g_metadata_get_dir_entry (data, req);
      if (entry->blob_type != BLOB_TYPE_INTERFACE &&
	  entry->blob_type != BLOB_TYPE_OBJECT &&
	  (entry->local || entry->blob_type != 0))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_BLOB,
		       "Not an interface or object");
	  return FALSE; 
	}
    }

  offset2 += 2 * (blob->n_prerequisites % 2);

  for (i = 0; i < blob->n_properties; i++, offset2 += sizeof (PropertyBlob))
    {
      if (!validate_property_blob (data, len, offset2, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_methods; i++, offset2 += sizeof (FunctionBlob))
    {
      if (!validate_function_blob (data, len, offset2, BLOB_TYPE_INTERFACE, error))
	return FALSE;
    }
  
  for (i = 0; i < blob->n_signals; i++, offset2 += sizeof (SignalBlob))
    {
      if (!validate_signal_blob (data, len, offset2, offset, error))
	return FALSE;
    }
  
  for (i = 0; i < blob->n_vfuncs; i++, offset2 += sizeof (VFuncBlob))
    {
      if (!validate_vfunc_blob (data, len, offset2, offset, error))
	return FALSE;
    }

  for (i = 0; i < blob->n_constants; i++, offset2 += sizeof (ConstantBlob))
    {
      if (!validate_constant_blob (data, len, offset2, error))
	return FALSE;
    }

  return TRUE;
}

static gboolean
validate_errordomain_blob (const guchar  *data,
			   guint          len,
			   guint32        offset,
			   GError       **error)
{
  return TRUE;
}

static gboolean
validate_union_blob (const guchar  *data,
		     guint          len,
		     guint32        offset,
		     GError       **error)
{
  return TRUE;
}

static gboolean
validate_blob (const guchar  *data,
	       guint          len,
	       guint32        offset,
	       GError       **error)
{
  CommonBlob *common;

  if (len < offset + sizeof (CommonBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  common = (CommonBlob*)&data[offset];
  
  switch (common->blob_type)
    {
    case BLOB_TYPE_FUNCTION:
      if (!validate_function_blob (data, len, offset, 0, error))
	return FALSE;
      break;
    case BLOB_TYPE_CALLBACK:
      if (!validate_callback_blob (data, len, offset, error))
	return FALSE;
      break;
    case BLOB_TYPE_STRUCT:
    case BLOB_TYPE_BOXED:
      if (!validate_struct_blob (data, len, offset, common->blob_type, error))
	return FALSE;
      break;
    case BLOB_TYPE_ENUM:
    case BLOB_TYPE_FLAGS:
      if (!validate_enum_blob (data, len, offset, common->blob_type, error))
	return FALSE;
      break;
    case BLOB_TYPE_OBJECT:
      if (!validate_object_blob (data, len, offset, error))
	return FALSE;
      break;
    case BLOB_TYPE_INTERFACE:
      if (!validate_interface_blob (data, len, offset, error))
	return FALSE;
      break;
    case BLOB_TYPE_CONSTANT:
      if (!validate_constant_blob (data, len, offset, error))
	return FALSE;
      break;
    case BLOB_TYPE_ERROR_DOMAIN:
      if (!validate_errordomain_blob (data, len, offset, error))
	return FALSE;
      break;
    case BLOB_TYPE_UNION:
      if (!validate_union_blob (data, len, offset, error))
	return FALSE;
      break;
    default:
      g_set_error (error, 
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID_ENTRY,
		   "Invalid blob type");
      return FALSE;
    }

  return TRUE;
}

static gboolean 
validate_directory (const guchar  *data,
		    guint          len,
		    GError       **error)
{
  Header *header = (Header *)data;
  DirEntry *entry;
  gint i;
  
  if (len < header->directory + header->n_entries * sizeof (DirEntry))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;
    }

  for (i = 0; i < header->n_entries; i++)
    {
      entry = g_metadata_get_dir_entry (data, i + 1);

      if (!is_name (data, entry->name))
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_DIRECTORY,
		       "Invalid entry name");
	  return FALSE; 
	}
      
      if ((entry->local && entry->blob_type == BLOB_TYPE_INVALID) ||
	  entry->blob_type > BLOB_TYPE_UNION)
	{
	  g_set_error (error,
		       G_METADATA_ERROR,
		       G_METADATA_ERROR_INVALID_DIRECTORY,
		       "Invalid entry type");
	  return FALSE; 
	}

      if (i < header->n_local_entries)
	{
	  if (!entry->local)
	    {
	      g_set_error (error,
			   G_METADATA_ERROR,
			   G_METADATA_ERROR_INVALID_DIRECTORY,
			   "Too few local directory entries");
	      return FALSE;
	    }

	  if (!is_aligned (entry->offset))
	    {
	      g_set_error (error,
			   G_METADATA_ERROR,
			   G_METADATA_ERROR_INVALID_DIRECTORY,
			   "Misaligned entry");
	      return FALSE;
	    }

	  if (!validate_blob (data, len, entry->offset, error))
	    return FALSE;
	}
      else
	{
	  if (entry->local)
	    {
	      g_set_error (error,
			   G_METADATA_ERROR,
			   G_METADATA_ERROR_INVALID_DIRECTORY,
			   "Too many local directory entries");
	      return FALSE;
	    }

	  if (!is_name (data, entry->offset))
	    {
	      g_set_error (error,
			   G_METADATA_ERROR,
			   G_METADATA_ERROR_INVALID_DIRECTORY,
			   "Invalid namespace name");
	      return FALSE; 
	    }
	}
    }

  return TRUE;
}

static gboolean
validate_annotations (const guchar  *data, 
		      guint          len, 
		      GError       **error)
{
  Header *header = (Header *)data;

  if (header->size < header->annotations + header->n_annotations * sizeof (AnnotationBlob))
    {
      g_set_error (error,
		   G_METADATA_ERROR,
		   G_METADATA_ERROR_INVALID,
		   "The buffer is too short");
      return FALSE;      
    }
  
  return TRUE;
}

gboolean 
g_metadata_validate (const guchar  *data,
		     guint          len,
		     GError       **error)
{
  if (!validate_header (data, len, error))
    return FALSE;

  if (!validate_directory (data, len, error))
    return FALSE;

  if (!validate_annotations (data, len, error))
    return FALSE;

  return TRUE;
}

GQuark
g_metadata_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("g-metadata-error-quark");
  return quark;
}

