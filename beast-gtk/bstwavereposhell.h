/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2001 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_WAVE_REPO_SHELL_H__
#define __BST_WAVE_REPO_SHELL_H__

#include	"bstdefs.h"
#include	"bstsupershell.h"
#include	"bstparamview.h"
#include	"bstwaveview.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_WAVE_REPO_SHELL	    (bst_wave_repo_shell_get_type ())
#define	BST_WAVE_REPO_SHELL(object)	    (GTK_CHECK_CAST ((object), BST_TYPE_WAVE_REPO_SHELL, BstWaveRepoShell))
#define	BST_WAVE_REPO_SHELL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_WAVE_REPO_SHELL, BstWaveRepoShellClass))
#define	BST_IS_WAVE_REPO_SHELL(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_WAVE_REPO_SHELL))
#define	BST_IS_WAVE_REPO_SHELL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_WAVE_REPO_SHELL))
#define BST_WAVE_REPO_SHELL_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_WAVE_REPO_SHELL, BstWaveRepoShellClass))


/* --- structures & typedefs --- */
typedef	struct	_BstWaveRepoShell	BstWaveRepoShell;
typedef	struct	_BstWaveRepoShellClass	BstWaveRepoShellClass;
struct _BstWaveRepoShell
{
  BstSuperShell	parent_object;

  BstParamView   *param_view;
  BstItemView    *wave_view;
};
struct _BstWaveRepoShellClass
{
  BstSuperShellClass	parent_class;
};


/* --- prototypes --- */
GtkType		bst_wave_repo_shell_get_type	(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_WAVE_REPO_SHELL_H__ */
