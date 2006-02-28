#!/usr/bin/env python2.4
#
# Dictionary of various types of link targets
# Copyright (C) 2005-2006 Tim Janik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

##
# 15.08.2005	modularised linkdict, read from linkdict_gnome.py
# 14.08.2005	initial python version
VERSION=0.3

# --- variables ---
linkdict_dictionary = None
linkdict_custom_dictionary = {}
linkdict_fallback_lookups = []

# --- functions ---
def lookup (keyword):
  global linkdict_dictionary, linkdict_custom_dictionary
  if linkdict_dictionary == None:	# initialize
    linkdict_dictionary = {}
    execdict = {}
    # we want to defer reading of linkdict_gnome.py as long as possible to not
    # take unneccessary speed penalties (i.e. when just importing linkdict)
    # this relies on having ./linkdict_gnome.py in the same dir as ./linkdict.py
    import linkdict_gnome # defines gnome_dictionary
    linkdict_dictionary.update (linkdict_gnome.gnome_dictionary);
    global stdc_dictionary
    linkdict_dictionary.update (stdc_dictionary);
    add_fallback_lookup (linkdict_gnome.gnome_lookup_fallback)
  result = None
  if not result:
    result = linkdict_custom_dictionary.get (keyword, None)
  if not result:
    result = linkdict_dictionary.get (keyword, None)
  if not result:
    global linkdict_fallback_lookups
    for handler in linkdict_fallback_lookups:
      result = handler (keyword)
      if result:
        break
  if result:
    return result
  return None

def elevated_lookup (keyword, custom_lookup = None, fallback_lookup = None):
  result = None
  if not result and custom_lookup:
    result = custom_lookup (keyword)
  if not result:
    result = lookup (keyword)
  if not result and fallback_lookup:
    result = fallback_lookup (keyword)
  if result:
    return result
  return None

def add_fallback_lookup (handler):
  global linkdict_fallback_lookups
  linkdict_fallback_lookups += [ handler ]

def add_custom_dict (dictionary):
  global linkdict_custom_dictionary
  linkdict_custom_dictionary.update (dictionary)

def markup_tuple_word (keyword, custom_lookup = None, fallback_lookup = None):
  link = elevated_lookup (keyword, custom_lookup, fallback_lookup)
  if link:
    return (link, keyword)
  return keyword

def markup_tuples (text, custom_lookup = None, fallback_lookup = None):
  import re				# defer imports as long as possible
  word_list = re.split ('(\W+)', text)	# split into list of word/non-word elements
  result = []
  for string in word_list:
    if string:
      result += [ markup_tuple_word (string, custom_lookup, fallback_lookup) ]
  return result

def markup_html_word (keyword, custom_lookup = None, fallback_lookup = None):
  link = elevated_lookup (keyword, custom_lookup, fallback_lookup)
  if link:
    return '<a href="' + link + '">' + keyword + '</a>'
  return keyword

def markup_html (text, custom_lookup = None, fallback_lookup = None):
  import re				# defer imports as long as possible
  word_list = re.split ('(\W+)', text)	# split into list of word/non-word elements
  result = ""
  for string in word_list:
    result += markup_html_word (string, custom_lookup, fallback_lookup)
  # if (result != text): raise RuntimeError ("word list split+join malfunction")
  return result

def lookup_ctype (keyword):
  global ctype_dictionary;
  return ctype_dictionary.get (keyword, None)

# --- dictionaries ---

###
### C typename dictionary (contains normal language words)
###
ctype_dictionary = {
    "void"				: "http://en.wikipedia.org/wiki/Void_return_type",
    "char"				: "http://www.unix.org/whitepapers/64bit.html",		# FIXME: related link
    "short"				: "http://www.unix.org/whitepapers/64bit.html",
    "int"				: "http://www.unix.org/whitepapers/64bit.html",
    "long"				: "http://www.unix.org/whitepapers/64bit.html",
#    "int32"				: "http://www.unix.org/whitepapers/64bit.html",
#    "int64"				: "http://www.unix.org/whitepapers/64bit.html",
    "float"				: "http://en.wikipedia.org/wiki/IEEE_Floating_Point_Standard",
    "double"				: "http://en.wikipedia.org/wiki/IEEE_Floating_Point_Standard",
    "signed"				: "http://en.wikipedia.org/wiki/Signedness",
    "unsigned"				: "http://en.wikipedia.org/wiki/Signedness",
    "const"				: "http://en.wikipedia.org/wiki/Const",
    "auto"				: "http://en.wikipedia.org/wiki/C99#Data_storage",	# FIXME: sketchy blurb
#    "static"				: "http://en.wikipedia.org/wiki/C99#Data_storage",
    "static"				: "http://en.wikipedia.org/wiki/Static",
    "..."				: "http://en.wikipedia.org/wiki/Variadic_function",
    "enum"				: "http://en.wikipedia.org/wiki/C_programming_language", # FIXME: unspecific link
    "typedef"				: "http://en.wikipedia.org/wiki/C_programming_language", # FIXME: unspecific link
    "struct"				: "http://en.wikipedia.org/wiki/C_programming_language", # FIXME: unspecific link
    "union"				: "http://en.wikipedia.org/wiki/C_programming_language", # FIXME: unspecific link
    "class"				: "http://en.wikipedia.org/wiki/Class_(computer_science)",
};

###
### standard C dictionary
###
stdc_dictionary = {
	"blkcnt_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"blksize_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"clock_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"clockid_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"dev_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"fsblkcnt_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"fsfilcnt_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"gid_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"id_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"ino_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"key_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"mode_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"nlink_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"off_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pid_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_attr_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_cond_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_condattr_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_key_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_mutex_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_mutexattr_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_once_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_rwlock_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_rwlockattr_t"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"pthread_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"size_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"ssize_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"suseconds_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"time_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"timer_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"uid_t"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
	"useconds_t"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/systypes.h.html",
        "E2BIG"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EACCES"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EADDRINUSE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EADDRNOTAVAIL"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EAFNOSUPPORT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EAGAIN"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EALREADY"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EBADF"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EBADMSG"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EBUSY"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ECANCELED"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ECHILD"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ECONNABORTED"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ECONNREFUSED"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ECONNRESET"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EDEADLK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EDESTADDRREQ"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EDOM"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EDQUOT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EEXIST"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EFAULT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EFBIG"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EHOSTUNREACH"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EIDRM"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EILSEQ"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EINPROGRESS"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EINTR"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EINVAL"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EIO"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EISCONN"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EISDIR"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ELOOP"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EMFILE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EMLINK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EMSGSIZE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EMULTIHOP"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENAMETOOLONG"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENETDOWN"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENETUNREACH"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENFILE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOBUFS"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENODATA"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENODEV"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOENT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOEXEC"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOLCK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOLINK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOMEM"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOMSG"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOPROTOOPT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOSPC"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOSR"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOSTR"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOSYS"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTCONN"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTDIR"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTEMPTY"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTSOCK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTSUP"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENOTTY"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ENXIO"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EOPNOTSUPP"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EOVERFLOW"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EPERM"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EPIPE"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EPROTO"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EPROTONOSUPPORT"		: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EPROTOTYPE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ERANGE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EROFS"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ERRORS"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ESPIPE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ESRCH"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ESTALE"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ETIME"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ETIMEDOUT"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ETXTBSY"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EWOULDBLOCK"			: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "EXDEV"				: "http://www.opengroup.org/onlinepubs/007908799/xsh/errors.html",
        "ARFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "IFS" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAILPATH" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PS1" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "CC" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LANG" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAILRC" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PS2" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "CDPATH" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_ALL" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAKEFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PS3" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "CFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_COLLATE" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAKESHELL" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PS4" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "CHARSET" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_CTYPE" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MANPATH" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PWD" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "COLUMNS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_MESSAGES" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MBOX" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "RANDOM" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "DATEMSK" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_MONETARY" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MORE" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "SECONDS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "DEAD" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_NUMERIC" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MSGVERB" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "SHELL" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "EDITOR" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LC_TIME" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "NLSPATH" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "TERM" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "ENV" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LDFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "NPROC" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "TERMCAP" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "EXINIT" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LEX" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "OLDPWD" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "TERMINFO" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "FC" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "OPTARG" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "TMPDIR" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "FCEDIT" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LINENO" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "OPTERR" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "TZ" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "FFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LINES" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "OPTIND" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "USER" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "GET" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LISTER" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PAGER" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "VISUAL" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "GFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LOGNAME" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PATH" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "YACC" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "HISTFILE" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "LPDEST" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PPID" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "YFLAGS" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "HISTORY" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAIL" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PRINTER" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "HISTSIZE" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAILCHECK" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PROCLANG" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "HOME" 				: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "MAILER" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "PROJECTDIR" 			: "http://www.opengroup.org/onlinepubs/007908799/xbd/envvar.html",
        "SA_NOCLDSTOP" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_NOCLDWAIT" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_NODEFER" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_ONSTACK" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_RESETHAND" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_RESTART" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SA_SIGINFO" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SS_DISABLE" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SS_ONSTACK" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGABRT" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGALRM" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGBUS" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGCHLD" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGCONT" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGEV_NONE" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGEV_SIGNAL" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGEV_THREAD" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGFPE" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGHUP" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGILL" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGINT" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGKILL" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGPIPE" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGPOLL" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGPROF" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGQUIT" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGRTMIN" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGSEGV" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGSTKSZ" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGSTOP" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGSYS" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGTERM" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGTRAP" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGTSTP" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGTTIN" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGTTOU" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGURG" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGUSR1" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGUSR2" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGVTALRM" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGXCPU" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIGXFSZ" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_BLOCK" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_DFL" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_ERR" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_HOLD" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_IGN" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_SETMASK" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "SIG_UNBLOCK" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
        "MINSIGSTKSZ" 			: "http://www.opengroup.org/onlinepubs/007908799/xsh/signal.h.html",
	"sizeof"			: "http://www.unix.org/whitepapers/64bit.html",
	"int16_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"uint16_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"int32_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"uint32_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"int64_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"uint64_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"intptr_t"			: "http://www.unix.org/whitepapers/64bit.html",
	"uintptr_t"			: "http://www.unix.org/whitepapers/64bit.html",
}
