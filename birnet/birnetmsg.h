/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_LOG_H__
#define __SFI_LOG_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- logging --- */
#define	SFI_LOG_ERROR	('E')
#define	SFI_LOG_WARN	('W')
#define	SFI_LOG_INFO	('I')
#define	SFI_LOG_DEBUG	('D')
static void	sfi_error	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_warn	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_info	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_debug	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
static void	sfi_nodebug	(const gchar *format,
				 ...) G_GNUC_PRINTF (1,2) G_GNUC_UNUSED;
void		sfi_log_valist	(const gchar *log_domain,
				 guint        level,
				 const gchar *format,
				 va_list      args);
void		sfi_log_message	(const gchar *log_domain,
				 guint        level,
				 const gchar *message);


/* --- implementation --- */
static void
sfi_error (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_ERROR, format, args);
  va_end (args);
}
static void
sfi_warn (const gchar *format,
	  ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_WARN, format, args);
  va_end (args);
}
static void
sfi_info (const gchar *format,
	  ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_INFO, format, args);
  va_end (args);
}
static void
sfi_debug (const gchar *format,
	   ...)
{
  va_list args;
  va_start (args, format);
  sfi_log_valist (G_LOG_DOMAIN, SFI_LOG_DEBUG, format, args);
  va_end (args);
}
static void
sfi_nodebug (const gchar *format,
	     ...)
{
}
#define	sfi_nodebug	while (0) sfi_nodebug
#if SFI_DISABLE_DEBUG
#define	sfi_debug	sfi_nodebug
#endif


G_END_DECLS

#endif /* __SFI_LOG_H__ */

/* vim:set ts=8 sts=2 sw=2: */
