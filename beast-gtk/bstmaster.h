/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_MASTER_H__
#define __BST_MASTER_H__


#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define	BST_MASTER_PRIORITY	G_PRIORITY_LOW // ((G_PRIORITY_HIGH - G_PRIORITY_DEFAULT) / 2)


/* --- typedefs --- */
typedef BseMaster BstMaster;


/* --- prototypes --- */
gchar*		bst_master_init		(void);
BstMaster*	bst_master_ref		(void);
void		bst_master_unref	(BstMaster	*master);
void		bst_master_shutdown	(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_MASTER_H__ */
