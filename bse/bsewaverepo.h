/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2003 Tim Janik
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
 */
#ifndef	__BSE_WAVE_REPO_H__
#define	__BSE_WAVE_REPO_H__

#include	<bse/bsesuper.h>


G_BEGIN_DECLS


/* --- object type macros --- */
#define BSE_TYPE_WAVE_REPO	        (BSE_TYPE_ID (BseWaveRepo))
#define BSE_WAVE_REPO(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_REPO, BseWaveRepo))
#define BSE_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))
#define BSE_IS_WAVE_REPO(object)	(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_REPO))
#define BSE_IS_WAVE_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_REPO))
#define BSE_WAVE_REPO_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_REPO, BseWaveRepoClass))


/* --- BseWaveRepo object --- */
struct _BseWaveRepo
{
  BseSuper	 parent_object;

  GList		*waves;
};
struct _BseWaveRepoClass
{
  BseSuperClass parent_class;
};


/* --- prototypes --- */
  

G_END_DECLS

#endif /* __BSE_WAVE_REPO_H__ */
