/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#include "bseundostack.h"
#include "bseproject.h"
#include "bsecontainer.h"
#include <string.h>

static BIRNET_MSG_TYPE_DEFINE (debug_undo, "undo", BIRNET_MSG_DEBUG, NULL);
#define DEBUG(...)      sfi_debug (debug_undo, __VA_ARGS__)

/* --- variables --- */
static BseUndoStack *dummy_ustack = NULL;


/* --- functions --- */
BseUndoStack*
bse_undo_stack_dummy (void)
{
  if (!dummy_ustack)
    {
      dummy_ustack = g_new0 (BseUndoStack, 1);
      dummy_ustack->project = NULL;
      dummy_ustack->notify = NULL;
      dummy_ustack->max_steps = 0;
      dummy_ustack->ignore_steps = 0x77777777; /* dummy specific value */
      dummy_ustack->n_open_groups = 0;
      dummy_ustack->group = NULL;
      dummy_ustack->debug_names = NULL;
      dummy_ustack->n_undo_groups = 0;
      dummy_ustack->undo_groups = NULL;
      dummy_ustack->dirt_counter = 0;
      dummy_ustack->n_merge_requests = 0;
      dummy_ustack->merge_name = NULL;
      dummy_ustack->merge_next = FALSE;
    }
  return dummy_ustack;
}

BseUndoStack*
bse_undo_stack_new (BseProject   *project,
                    BseUndoNotify notify)
{
  BseUndoStack *self;

  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  self = g_memdup (bse_undo_stack_dummy (), sizeof (*self));
  self->ignore_steps = 0; /* reset dummy specific value */
  self->project = project;
  self->notify = notify;
  self->max_steps = 999;
  return self;
}

void
bse_undo_stack_limit (BseUndoStack *self,
                      guint         max_steps)
{
  self->max_steps = max_steps;
  while (self->n_undo_groups > self->max_steps)
    {
      BseUndoGroup *group = sfi_ring_pop_tail (&self->undo_groups);
      self->n_undo_groups--;
      while (group->undo_steps)
        bse_undo_step_free (sfi_ring_pop_head (&group->undo_steps));
      g_free (group->name);
      g_free (group);
    }
}

void
bse_undo_stack_clear (BseUndoStack *self)
{
  guint max_steps = self->max_steps;
  bse_undo_stack_limit (self, 0);
  self->max_steps = max_steps;
}

gboolean
bse_undo_stack_dirty (BseUndoStack *self)
{
  return self->dirt_counter || (self->group && self->group->undo_steps);
}

void
bse_undo_stack_clean_dirty (BseUndoStack *self)
{
  self->dirt_counter = 0;
}

void
bse_undo_stack_force_dirty (BseUndoStack *self)
{
  if (self->dirt_counter <= 0)
    {
      /* make unrecoverably dirty */
      self->dirt_counter = self->n_undo_groups + 1;
    }
}

void
bse_undo_stack_destroy (BseUndoStack *self)
{
  while (self->n_open_groups)
    bse_undo_group_close (self);
  bse_undo_stack_clear (self);
  while (self->debug_names)
    g_free (g_slist_pop_head (&self->debug_names));
  g_free (self);
}

void
bse_undo_group_open (BseUndoStack   *self,
                     const gchar    *name)
{
  g_return_if_fail (name != NULL);

  if (!self->n_open_groups)
    {
      self->group = g_new0 (BseUndoGroup, 1);
      self->group->stamp = 0;
      self->group->name = g_strdup (name);
      self->group->undo_steps = NULL;
      DEBUG ("undo open: { // %s", name);
    }
  self->n_open_groups++;
  self->debug_names = g_slist_prepend (self->debug_names, g_strdup (name));
}

void
bse_undo_stack_ignore_steps (BseUndoStack *self)
{
  self->ignore_steps++;
}

void
bse_undo_stack_push (BseUndoStack *self,
                     BseUndoStep  *ustep)
{
  const gchar *debug_name = self->debug_names ? self->debug_names->data : "-";

  g_return_if_fail (self->n_open_groups > 0);
  g_return_if_fail (ustep != NULL);

  if (self->ignore_steps)
    {
      DEBUG ("undo step:  -    ignored: ((BseUndoFunc) %p) (%s)", ustep->undo_func, debug_name);
      bse_undo_step_free (ustep);
    }
  else
    {
      DEBUG ("undo step:  *    ((BseUndoFunc) %p) (%s)", ustep->undo_func, debug_name);
      ustep->debug_name = g_strdup (debug_name);
      self->group->undo_steps = sfi_ring_push_head (self->group->undo_steps, ustep);
    }
}

void
bse_undo_stack_push_add_on (BseUndoStack *self,
                            BseUndoStep  *ustep)
{
  g_return_if_fail (ustep != NULL);

  /* add-ons are generally used as state-guards. that is, if a an already added
   * undo-steps requires the object to be in a certain state, an add-on step
   * can be queued after the fact, to ensure the required object state.
   */

  /* add this step to the last undo step if we have one */
  if (self->group && self->group->undo_steps)
    {
      DEBUG ("undo step:  *    ((BseUndoFunc) %p) [AddOn to current group]", ustep->undo_func);
      ustep->debug_name = g_strdup ("AddOn");
      self->group->undo_steps = sfi_ring_push_head (self->group->undo_steps, ustep);
    }
  else if (self->undo_groups)
    {
      BseUndoGroup *group = self->undo_groups->data;    /* fetch last group */
      g_return_if_fail (group->undo_steps != NULL);     /* empty groups are not allowed */
      DEBUG ("undo step:  *    ((BseUndoFunc) %p) [AddOn to last group]", ustep->undo_func);
      ustep->debug_name = g_strdup ("AddOn");
      group->undo_steps = sfi_ring_push_head (group->undo_steps, ustep);
    }
  else
    {
      DEBUG ("undo step:  -    ignored: ((BseUndoFunc) %p) [AddOn]", ustep->undo_func);
      bse_undo_step_free (ustep);
    }
}

void
bse_undo_stack_unignore_steps (BseUndoStack *self)
{
  g_return_if_fail (self->ignore_steps > 0);
  self->ignore_steps--;
}

void
bse_undo_group_close (BseUndoStack *self)
{
  g_return_if_fail (self->n_open_groups > 0);

  g_free (g_slist_pop_head (&self->debug_names));
  self->n_open_groups--;
  if (!self->n_open_groups)
    {
      gboolean step_added = TRUE;
      if (!self->group->undo_steps)
        {
          step_added = FALSE;
          g_free (self->group->name);
          g_free (self->group);
          DEBUG ("undo skip  }");
        }
      else
        {
          self->group->stamp = 0; // sfi_time_system ();
          if (self->merge_next && self->undo_groups)
            {
              BseUndoGroup *mgroup = self->undo_groups->data;
              g_free (mgroup->name);
              mgroup->name = g_strdup (self->merge_name);
              mgroup->undo_steps = sfi_ring_concat (self->group->undo_steps,
                                                    mgroup->undo_steps);
              g_free (self->group->name);
              g_free (self->group);
              if (!self->dirt_counter)  /* ensure dirty */
                bse_undo_stack_force_dirty (self);
            }
          else
            {
              self->n_undo_groups++;
              self->undo_groups = sfi_ring_push_head (self->undo_groups, self->group);
              self->merge_next = self->n_merge_requests > 0;
              self->dirt_counter++;
            }
          bse_undo_stack_limit (self, self->max_steps);
          DEBUG ("undo close }");
        }
      self->group = NULL;
      if (self->notify && step_added)
        self->notify (self->project, self, TRUE);
    }
}

const BseUndoStep*
bse_undo_group_peek_last_atom (BseUndoStack *self,
                               SfiTime      *stamp_p)
{
  if (self->n_open_groups == 1 &&
      !self->group->undo_steps &&
      self->max_steps > 1 &&
      self->undo_groups)
    {
      BseUndoGroup *group = self->undo_groups->data;
      if (sfi_ring_cmp_length (group->undo_steps, 1) == 0)
        {
          /* last undo commit was atomic step */
          if (stamp_p)
            *stamp_p = group->stamp;
          return group->undo_steps->data;
        }
    }
  return NULL;
}

void
bse_undo_stack_add_merger (BseUndoStack   *self,
                           const gchar    *name)
{
  g_return_if_fail (name != NULL);

  self->n_merge_requests++;
  if (!self->merge_name)
    self->merge_name = g_strdup (name);
}

void
bse_undo_stack_remove_merger (BseUndoStack *self)
{
  if (self->n_merge_requests)
    {
      self->n_merge_requests--;
      if (!self->n_merge_requests)
        {
          g_free (self->merge_name);
          self->merge_name = NULL;
          self->merge_next = FALSE;
        }
    }
}

guint
bse_undo_stack_depth (BseUndoStack *self)
{
  return self->n_undo_groups;
}

const gchar*
bse_undo_stack_peek (BseUndoStack *self)
{
  BseUndoGroup *group = self->undo_groups ? self->undo_groups->data : NULL;
  return group ? group->name : NULL;
}

void
bse_undo_stack_undo (BseUndoStack *self)
{
  BseUndoGroup *group;

  if (self->group)
    g_return_if_fail (self->group->undo_steps == NULL);

  group = sfi_ring_pop_head (&self->undo_groups);
  if (group)
    {
      self->n_undo_groups--;
      self->dirt_counter--;
      DEBUG ("EXECUTE UNDO: %s", group->name);
      if (birnet_msg_check (debug_undo))
        {
          SfiRing *ring = group->undo_steps;
          for (ring = group->undo_steps; ring; ring = sfi_ring_walk (ring, group->undo_steps))
            {
              BseUndoStep *ustep = ring->data;
              DEBUG ("   STEP UNDO: %s", ustep->debug_name);
            }
        }
      while (group->undo_steps)
        {
          BseUndoStep *ustep = sfi_ring_pop_head (&group->undo_steps);
          bse_undo_step_exec (ustep, self);
          bse_undo_step_free (ustep);
        }
      g_free (group->name);
      g_free (group);
      if (self->notify)
        self->notify (self->project, self, FALSE);
    }

  if (self->group)
    g_return_if_fail (self->group->undo_steps == NULL);
}

BseUndoStep*
bse_undo_step_new (BseUndoFunc     undo_func,
                   BseUndoFree     free_func,
                   guint           n_data_fields)
{
  BseUndoStep *ustep;
  g_return_val_if_fail (undo_func != NULL, NULL);

  ustep = g_malloc0 (sizeof (BseUndoStep) +
                     sizeof (ustep->data) * (MAX (n_data_fields, 1) - 1));
  ustep->undo_func = undo_func;
  ustep->free_func = free_func;
  ustep->debug_name = NULL;
  return ustep;
}

void
bse_undo_step_exec (BseUndoStep  *ustep,
                    BseUndoStack *ustack)
{
  ustep->undo_func (ustep, ustack);
  ustep->undo_func = NULL;
}

void
bse_undo_step_free (BseUndoStep *ustep)
{
  if (ustep->free_func)
    ustep->free_func (ustep);
  g_free (ustep->debug_name);
  g_free (ustep);
}

gchar*
bse_undo_pointer_pack (gpointer      item,
                       BseUndoStack *ustack)
{
  BseProject *project;

  g_return_val_if_fail (ustack != NULL, NULL);

  if (!item)
    return NULL;
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  if (ustack == dummy_ustack)
    return NULL;

  project = bse_item_get_project (item);
  g_return_val_if_fail (project != NULL, NULL);

  /* upaths start out with chars >= 7 */
  if (item == (BseItem*) project)
    return g_strdup ("\002project\003");

  return bse_container_make_upath (BSE_CONTAINER (project), item);
}

gpointer
bse_undo_pointer_unpack (const gchar  *packed_pointer,
                         BseUndoStack *ustack)
{
  gpointer item;

  g_return_val_if_fail (ustack != NULL, NULL);

  if (!packed_pointer)
    return NULL;

  if (ustack == dummy_ustack)
    return NULL;

  if (packed_pointer[0] == 002 && strcmp (packed_pointer, "\002project\003") == 0)
    return ustack->project;

  item = bse_container_resolve_upath (BSE_CONTAINER (ustack->project), packed_pointer);

  g_return_val_if_fail (item != NULL, NULL);

  return item;
}
