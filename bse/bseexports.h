/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bseexports.h: export declarations for external plugins
 */
#ifndef __BSE_EXPORTS_H__
#define __BSE_EXPORTS_H__

#include	<bse/bseprocedure.h>

G_BEGIN_DECLS

typedef enum {
  BSE_EXPORT_NODE_NONE,
  BSE_EXPORT_NODE_LINK,
  BSE_EXPORT_NODE_ENUM,
  BSE_EXPORT_NODE_RECORD,
  BSE_EXPORT_NODE_SEQUENCE,
  BSE_EXPORT_NODE_CLASS,
  BSE_EXPORT_NODE_PROC
} BseExportNodeType;
struct _BseExportNode {
  BseExportNode    *next;
  BseExportNodeType ntype;
  const char       *name;
  const char       *options;
  const char       *category;
  const char       *blurb;
  const char       *authors;
  const char       *license;
  const guint8     *pixstream;
  GType             type;
};
typedef struct {
  BseExportNode node;
  GEnumValue   *values;
} BseExportNodeEnum;
struct _BseExportNodeBoxed {
  BseExportNode   node;
  GBoxedCopyFunc  copy;
  GBoxedFreeFunc  free;
  GValueTransform boxed2recseq;
  GValueTransform seqrec2boxed;
  SfiBoxedFields  fields;
};
typedef struct {
  BseExportNode      node;
  const char        *parent;
  /* GTypeInfo fields */
  guint16            class_size;
  GClassInitFunc     class_init;
  GClassFinalizeFunc class_finalize;
  guint16            instance_size;
  GInstanceInitFunc  instance_init;
} BseExportNodeClass;
typedef struct {
  BseExportNode     node;
  guint             private_id;
  BseProcedureInit  init;
  BseProcedureExec  exec;
} BseExportNodeProc;

/* plugin export identity (name, bse-version and actual types) */
#define BSE_EXPORT_IDENTITY_SYMBOL      bse_export__identity
#define BSE_EXPORT_IDENTITY_STRING     "bse_export__identity"
typedef struct {
  const gchar   *name;
  guint          major, minor, micro;
  guint          binary_age, interface_age;
  BseExportNode *export_chain;
} BseExportIdentity;
#define BSE_EXPORT_IDENTITY(Name, HEAD)                                 \
  { Name, BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION,      \
    BSE_BINARY_AGE, BSE_INTERFACE_AGE, &HEAD }

/* implementation prototype */
void	bse_procedure_complete_info	(const BseExportNodeProc *pnode,
					 GTypeInfo               *info);

G_END_DECLS

#endif /* __BSE_EXPORTS_H__ */
