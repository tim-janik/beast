/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002 Tim Janik
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
 */
#define G_LOG_DOMAIN "BswShell"

#include <string.h>
#include <errno.h>
#include "bswscminterp.h"
#include "bswscmhandle.h"
#include <bsw/bswglue.h>

/* Data types:
 * SCM
 * Constants:
 * SCM_BOOL_T, SCM_BOOL_F
 * Object:
 * SCM_UNSPECIFIED
 * Checks:
 * SCM_IMP()	- is immediate?
 * SCM_NIMP()	- is not immediate?
 *
 * catching exceptions:
 * typedef SCM (*scm_catch_body_t) (void *data);
 * typedef SCM (*scm_catch_handler_t) (void *data,
 * SCM tag = SCM_BOOL_T; means catch-all
 * SCM gh_catch(SCM tag, scm_catch_body_t body, void *body_data,
 *              scm_catch_handler_t handler, void *handler_data);
 */


/* --- BswSCMHandle SMOB --- */
/* SMOB for the BswSCMHandle, so it gets garbage collected correctly
 * (i.e. when exceptions are thwon from the various scm2... converters)
 */
static gulong bsw_scm_handle_tag = 0;

static SCM
bsw_scm_create_handle (void)
{
  BswSCMHandle *handle;

  BSW_SCM_DEFER_INTS ();
  handle = bsw_scm_handle_alloc ();
  BSW_SCM_ALLOW_INTS ();

  SCM_RETURN_NEWSMOB (bsw_scm_handle_tag, handle);
}

static BswSCMHandle*
bsw_scm2handle (SCM handle_smob)
{
  SCM_ASSERT (SCM_NIMP (handle_smob) && SCM_CAR (handle_smob) == bsw_scm_handle_tag,
	      handle_smob, SCM_ARG1, "bsw_scm_get_handle");

  return (BswSCMHandle*) SCM_CDR (handle_smob);
}

static BswSCMHandle*
bsw_scm_handle_fetch (const gchar *proc_name)
{
  BswSCMHandle *handle;
  SCM handle_smob = bsw_scm_create_handle ();

  handle = bsw_scm2handle (handle_smob);
  bsw_scm_handle_set_proc (handle, proc_name);

  return handle;
}

static SCM
bsw_scm_mark_handle (SCM handle_smob)
{
  // BswSCMHandle *handle = (BswSCMHandle*) SCM_CDR (handle_smob);

  return SCM_BOOL_F;
}

static scm_sizet
bsw_scm_free_handle (SCM image_smob)
{
  BswSCMHandle *handle = (BswSCMHandle*) SCM_CDR (image_smob);
  scm_sizet size = sizeof (1024);	/* rough guess */

  // g_print ("freeing handle!!!\n");

  BSW_SCM_DEFER_INTS ();
  bsw_scm_handle_destroy (handle);
  BSW_SCM_ALLOW_INTS ();

  return size;
}

static void
bsw_scm_init_handle_tag (void)
{
  bsw_scm_handle_tag = scm_make_smob_type ("BswSCMHandle", 1024);
  scm_set_smob_mark (bsw_scm_handle_tag, bsw_scm_mark_handle);
  scm_set_smob_free (bsw_scm_handle_tag, bsw_scm_free_handle);
}


/* --- conversion --- */
SCM
bsw_scm_from_enum (gint  eval,
		   GType type)
{
  GEnumClass *eclass;
  GEnumValue *ev;
  gchar *vname;
  SCM sym;

  BSW_SCM_DEFER_INTS ();
  eclass = g_type_class_ref (type);
  ev = g_enum_get_value (eclass, eval);
  vname = bsw_cupper_to_sname (ev->value_name);
  sym = SCM_CAR (scm_intern0 (vname));
  g_free (vname);
  g_type_class_unref (eclass);
  BSW_SCM_ALLOW_INTS ();

  return sym;
}


/* --- procedure glue --- */
/* glue code to call procedures */
#define	g_value_get_proxy	bsw_value_get_proxy
#include "bswscm-genglue.c"


/* --- initialization --- */
void
bsw_scm_interp_init (void)
{
  guint i;

  bsw_scm_init_handle_tag ();

  for (i = 0; i < G_N_ELEMENTS (bsw_scm_wrap_table); i++)
    gh_new_procedure (bsw_scm_wrap_table[i].fname, bsw_scm_wrap_table[i].func, bsw_scm_wrap_table[i].rargs, 0, 0);
}
