/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2001 Tim Janik
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
#include        <bsw/bsw.h>

int
main (int   argc,
      char *argv[])
{
  BswProxy project;
  gint error;
  
  g_thread_init (NULL);
  bsw_init (&argc, &argv, NULL);	// FIXME

  g_print ("server id: %lu\n", BSW_SERVER);

  bsw_hello_world ();
  project = bsw_server_use_new_project (BSW_SERVER, "test-project");
  g_print ("project id: %lu\n", project);
  error = bsw_project_restore_from_file (project, "/usr/src/beast/test/geekworld.bse");
  g_print ("load project result: %s\n", bsw_error_blurb (error));

  bsw_item_unuse (project);
  //  g_print ("project removal result: %s\n", bsw_error_blurb (error));

  // proxy = bsw_server_default_pcm_device (BSW_SERVER);
  // g_print ("server default pcm device proxy id: %u\n", proxy);
  
  return 0;
}
