/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#ifndef __BST_SAMPLE_REPO_H__
#define __BST_SAMPLE_REPO_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures & typedefs --- */
typedef	struct	_BstSampleLoc	BstSampleLoc;
typedef	struct	_BstSampleRepo	BstSampleRepo;
struct _BstSampleLoc
{
  BstSampleRepo	*repo;
  gchar		*name;
};
struct _BstSampleRepo
{
  gchar		*name;
  GSList	*sample_locs;
};


/* --- prototypes --- */
void		bst_sample_repo_init			(void);
BstSampleRepo*	bst_sample_repo_new			(const gchar	*name);
void		bst_sample_repo_add_sample		(BstSampleRepo	*repo,
							 const gchar	*sample_name);
GList*		bst_sample_repo_list_sample_locs	(void);
BstSampleLoc*	bst_sample_repo_find_sample_loc		(const gchar	*sample_name);
BseSample*	bst_sample_repo_load_sample		(BstSampleLoc	*loc,
							 BseProject	*project);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_SAMPLE_REPO_H__ */
