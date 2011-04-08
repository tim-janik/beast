/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 *
 * bseexports.h: export declarations for external plugins
 */
#ifndef __BSE_EXPORTS_H__
#define __BSE_EXPORTS_H__

#include	<bse/bseprocedure.h>

G_BEGIN_DECLS

/* --- export node types --- */
typedef enum {
  BSE_EXPORT_NODE_NONE,
  BSE_EXPORT_NODE_LINK,
  BSE_EXPORT_NODE_HOOK,
  BSE_EXPORT_NODE_ENUM,
  BSE_EXPORT_NODE_RECORD,
  BSE_EXPORT_NODE_SEQUENCE,
  BSE_EXPORT_NODE_CLASS,
  BSE_EXPORT_NODE_PROC
} BseExportNodeType;

/* --- common export node data --- */
typedef struct {
  /* strings which need to be looked up from catalogs after
   * initialization (usually i18n strings).
   */
  const char       *blurb;
  const char       *authors;
  const char       *license;
  const char       *i18n_category;
  /* definition location */
  const char       *file;
  guint             line;
} BseExportStrings;
typedef void (*BseExportStringsFunc) (BseExportStrings *strings);

/* --- basic export node --- */
struct _BseExportNode {
  BseExportNode       *next;
  BseExportNodeType    ntype;
  const char          *name;
  const char          *options;
  const char          *category;
  const guint8        *pixstream;
  BseExportStringsFunc fill_strings;
  GType                type;
};

/* --- hook export node --- */
typedef void (*BseExportHook)	(void *data);
typedef struct {
  BseExportNode		node;
  bool			make_static;
  BseExportHook		hook;
  void		       *data;
} BseExportNodeHook;

/* --- enum export node --- */
typedef GEnumValue*     (*BseExportGetEnumValues)   (void);
typedef SfiChoiceValues (*BseExportGetChoiceValues) (void);
typedef struct {
  BseExportNode            node;
  BseExportGetEnumValues   get_enum_values;
  BseExportGetChoiceValues get_choice_values;
} BseExportNodeEnum;

/* --- boxed export node --- */
typedef SfiRecFields (*BseExportGetRecordFields)    (void);
typedef GParamSpec*  (*BseExportGetSequenceElement) (void);
struct _BseExportNodeBoxed {
  BseExportNode   node;
  GBoxedCopyFunc  copy;
  GBoxedFreeFunc  free;
  GValueTransform boxed2recseq;
  GValueTransform seqrec2boxed;
  union {
    BseExportGetRecordFields    get_fields;
    BseExportGetSequenceElement get_element;
  } func;
};

/* --- class export node --- */
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

/* --- procedure export node --- */
typedef struct {
  BseExportNode     node;
  guint             private_id;
  BseProcedureInit  init;
  BseProcedureExec  exec;
} BseExportNodeProc;

/* --- plugin identity export --- */
/* plugin export identity (name, bse-version and actual types) */
#define BSE_EXPORT_IDENTITY_SYMBOL      bse_export__identity
#define BSE_EXPORT_IDENTITY_STRING     "bse_export__identity"
typedef struct {
  guint          major, minor, micro;
  guint          binary_age, interface_age;
  guint          dummy1, dummy2, dummy3;
  guint64        export_flags;
  BseExportNode *export_chain;
} BseExportIdentity;
#define BSE_EXPORT_IDENTITY(HEAD)                               \
  { BSE_MAJOR_VERSION, BSE_MINOR_VERSION, BSE_MICRO_VERSION,    \
    BSE_BINARY_AGE, BSE_INTERFACE_AGE, 0, 0, 0,                 \
    BSE_EXPORT_CONFIG, &HEAD }

#define BSE_EXPORT_FLAG_MMX      (0x1ull << 0)
#define BSE_EXPORT_FLAG_MMXEXT   (0x1ull << 1)
#define BSE_EXPORT_FLAG_3DNOW    (0x1ull << 2)
#define BSE_EXPORT_FLAG_3DNOWEXT (0x1ull << 3)
#define BSE_EXPORT_FLAG_SSE      (0x1ull << 4)
#define BSE_EXPORT_FLAG_SSE2     (0x1ull << 5)
#define BSE_EXPORT_FLAG_SSE3     (0x1ull << 6)
#define BSE_EXPORT_FLAG_SSE4     (0x1ull << 7)

#define BSE_EXPORT_CONFIG       (BSE_EXPORT_CONFIG__MMX | BSE_EXPORT_CONFIG__3DNOW | \
                                 BSE_EXPORT_CONFIG__SSE | BSE_EXPORT_CONFIG__SSE2 |  \
                                 BSE_EXPORT_CONFIG__SSE3)



BsePlugin*      bse_exports__add_node   (const BseExportIdentity *identity,     // bseplugin.c
                                         BseExportNode           *enode);
void            bse_exports__del_node   (BsePlugin               *plugin,       // bseplugin.c
                                         BseExportNode           *enode);

/* implementation prototype */
void	bse_procedure_complete_info	(const BseExportNodeProc *pnode,
					 GTypeInfo               *info);

/* --- export config --- */
#ifdef   __MMX__
#define BSE_EXPORT_CONFIG__MMX   BSE_EXPORT_FLAG_MMX
#else
#define BSE_EXPORT_CONFIG__MMX   0
#endif
#ifdef  __3dNOW__
#define BSE_EXPORT_CONFIG__3DNOW BSE_EXPORT_FLAG_3DNOW
#else
#define BSE_EXPORT_CONFIG__3DNOW 0
#endif
#ifdef  __SSE__
#define BSE_EXPORT_CONFIG__SSE   BSE_EXPORT_FLAG_SSE
#else
#define BSE_EXPORT_CONFIG__SSE   0
#endif
#ifdef  __SSE2__
#define BSE_EXPORT_CONFIG__SSE2  BSE_EXPORT_FLAG_SSE2
#else
#define BSE_EXPORT_CONFIG__SSE2  0
#endif
#ifdef  __SSE3__
#define BSE_EXPORT_CONFIG__SSE3  BSE_EXPORT_FLAG_SSE3
#else
#define BSE_EXPORT_CONFIG__SSE3  0
#endif

G_END_DECLS

#endif /* __BSE_EXPORTS_H__ */
