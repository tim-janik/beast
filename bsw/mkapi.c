/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2001 and Tim Janik
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
static char *bsw_log_domain_bsw = "BSW-MkApi";
#include        "../bse/bse.h"
#include        "../PKG_config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<string.h>

static FILE       *f_out = NULL;
static gchar      *prefix = "bsw";
static gboolean    gen_header = 0;
static gboolean    gen_body = 0;
static GHashTable *type_wrapper_ht = NULL;

#define to_lower(c)                             ( \
        (guchar) (                                                      \
          ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |  \
          ( (((guchar)(c))>=192 && ((guchar)(c))<=214) * (224-192) ) |  \
          ( (((guchar)(c))>=216 && ((guchar)(c))<=222) * (248-216) ) |  \
          ((guchar)(c))                                                 \
        )                                                               \
)
#define to_upper(c) (to_lower (c) + 'A' - 'a')
#define	is_lower(c) ((c) == to_lower (c))
#define	is_upper(c) ((c) == to_upper (c))

static gchar*	tmacro_from_type (GType type);
     
static const gchar*
indent (guint n_spaces)
{
  static gchar *buffer = NULL;
  static guint blength = 0;

  if (blength <= n_spaces)
    {
      blength = n_spaces + 1;
      g_free (buffer);
      buffer = g_new (gchar, blength);
    }
  memset (buffer, ' ', n_spaces);
  buffer[n_spaces] = 0;

  return buffer;
}

static void
add_type_wrapper (GType type)
{
  if (g_type_is_a (type, BSE_TYPE_OBJECT) && !g_hash_table_lookup (type_wrapper_ht, (gpointer) type))
    {
      g_hash_table_insert (type_wrapper_ht, (gpointer) type, GUINT_TO_POINTER (TRUE));
      add_type_wrapper (g_type_parent (type));
    }
}

static void
type_wrapper_foreach (gpointer key,
		      gpointer dummy1,
		      gpointer dummy2)
{
  GType type = (GType) key;
  gchar *macro = tmacro_from_type (type);
  gchar *delim = strchr (macro, '_'); /* skip namespace prefix */

  *delim = 0;
  delim = strchr (delim + 1, '_');	/* skip _TYPE portion */
  *delim = 0;
  if (macro[0] == 'B' && macro[1] == 'S' && macro[2] == 'E')
    macro[2] = 'W';
  
  fprintf (f_out, "#define %s_IS_%s(proxy)\t(bsw_object_is_a ((proxy), \"%s\"))\n",
	   macro, delim + 1,
	   g_type_name (type));
  g_free (macro);
}

static void
print_type_wrappers (void)
{
  g_hash_table_foreach (type_wrapper_ht, type_wrapper_foreach, NULL);
}

static void
print_enums (void)
{
  GType *children;
  guint n, i;

  children = g_type_children (G_TYPE_ENUM, &n);
  for (i = 0; i < n; i++)
    {
      const gchar *name = g_type_name (children[i]);

      if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  GEnumClass *eclass = g_type_class_ref (children[i]);
	  GEnumValue *val;

	  fprintf (f_out, "typedef enum\n{\n");
	  for (val = eclass->values; val->value_name; val++)
	    {
	      fprintf (f_out, "  BSW%s = %d,", val->value_name + 3, val->value);
	      if (children[i] == BSE_TYPE_ERROR_TYPE)
		fprintf (f_out, "       \t/* \"%s\" */\n", bse_error_blurb (val->value));
	      else
		fprintf (f_out, "\n");
	    }
	  fprintf (f_out, "} Bsw%s;\n", g_type_name (children[i]) + 3);
	}
    }
  g_free (children);
  children = g_type_children (G_TYPE_FLAGS, &n);
  for (i = 0; i < n; i++)
    {
      const gchar *name = g_type_name (children[i]);

      if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  GFlagsClass *fclass = g_type_class_ref (children[i]);
	  GFlagsValue *val;

	  fprintf (f_out, "typedef enum\n{\n");
	  for (val = fclass->values; val->value_name; val++)
	    fprintf (f_out, "  BSW%s = 0x%x,\n", val->value_name + 3, val->value);
	  fprintf (f_out, "} Bsw%s;\n", g_type_name (children[i]) + 3);
	}
    }
  g_free (children);
  fprintf (f_out, "\n");
}

typedef struct {
  GType  type;
  gchar *macro; /* uppercase type macro */
  gchar *rtype;	/* C return type */
  gchar *set_func;
  gchar *get_func;
  gchar *ctype;	/* C argument type */
  gpointer next;
} MarshalType;

static MarshalType* marshal_type_list = NULL;

static void
marshal_add (GType  type,
	     gchar *macro,
	     gchar *rtype,
	     gchar *setter,
	     gchar *getter,
	     gchar *ctype)
{
  MarshalType *m = g_new (MarshalType, 1);
  m->type = type;
  m->macro = macro;
  m->rtype = rtype;
  m->set_func = setter;
  m->get_func = getter;
  m->ctype = ctype ? ctype : rtype;
  m->next = marshal_type_list;
  marshal_type_list = m;
}

static const gchar *cfile_header =
("#include \"bsw.h\"\n"
 "#include <bse/bse.h>\n"
 "#define bsw_value_initset_boolean(val,t,vbool)  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_boolean ((val), (vbool)); }\n"
 "#define bsw_value_initset_char(val,t,vchar)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_char ((val), (vchar)); }\n"
 "#define bsw_value_initset_uchar(val,t,vuchar)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uchar ((val), (vuchar)); }\n"
 "#define bsw_value_initset_int(val,t,vint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_int ((val), (vint)); }\n"
 "#define bsw_value_initset_uint(val,t,vuint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_uint ((val), (vuint)); }\n"
 "#define bsw_value_initset_ulong(val,t,vulong)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_ulong ((val), (vulong)); }\n"
 "#define bsw_value_initset_enum(val,t,vuint)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_enum ((val), (vuint)); }\n"
 "#define bsw_value_initset_float(val,t,vfloat)	  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_float ((val), (vfloat)); }\n"
 "#define bsw_value_initset_double(val,t,vdouble) { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_double ((val), (vdouble)); }\n"
 "#define bsw_value_initset_string(val,t,string)  { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_static_string ((val), (string)); }\n"
 "#define bsw_value_initset_boxed(val,t,b)        { (val)->g_type = 0; g_value_init ((val), (t)); g_value_set_static_boxed ((val), (b)); }\n"
 "#define bsw_value_initset_proxy(val,t,vproxy)	  { (val)->g_type = 0; g_value_init ((val), BSW_TYPE_PROXY); bsw_value_set_proxy ((val), (vproxy)); }\n"
 );

static void
init_marshal_types (void)
{
#define add(t,r,s,g,c)	marshal_add (t, #t, r, s, g, c)
  add (G_TYPE_NONE,      "void     ",	0, 0, 0);
  add (G_TYPE_BOOLEAN,   "gboolean ",	"bsw_value_initset_boolean",	"g_value_get_boolean", 0);
  add (G_TYPE_CHAR,      "gchar    ",	"bsw_value_initset_char",	"g_value_get_char", 0);
  add (G_TYPE_UCHAR,     "guchar   ",	"bsw_value_initset_uchar",	"g_value_get_uchar", 0);
  add (G_TYPE_INT,       "gint     ",	"bsw_value_initset_int",	"g_value_get_int", 0);
  add (G_TYPE_UINT,      "guint    ",	"bsw_value_initset_uint",	"g_value_get_uint", 0);
  add (G_TYPE_ULONG,     "gulong    ",	"bsw_value_initset_ulong",	"g_value_get_ulong", 0);
  add (G_TYPE_ENUM,      "gint     ",	"bsw_value_initset_enum",	"g_value_get_enum", 0);
  add (G_TYPE_FLOAT,     "gfloat   ",	"bsw_value_initset_float",	"g_value_get_float", 0);
  add (G_TYPE_DOUBLE,    "gdouble  ",	"bsw_value_initset_double",	"g_value_get_double", 0);
  add (G_TYPE_STRING,    "gchar*",	"bsw_value_initset_string",	"bsw_collector_get_string", "const gchar*");
  add (G_TYPE_OBJECT,    "BswProxy ",	"bsw_value_initset_proxy",	"bsw_value_get_proxy", 0);
  add (BSW_TYPE_ITER_INT,         "BswIterInt*",         "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
  add (BSW_TYPE_ITER_STRING,      "BswIterString*",      "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
  add (BSW_TYPE_ITER_PROXY,       "BswIterProxy*",       "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
  add (BSW_TYPE_ITER_PART_NOTE,   "BswIterPartNote*",    "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
  add (BSW_TYPE_VALUE_BLOCK,      "BswValueBlock*",      "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
  add (BSW_TYPE_NOTE_DESCRIPTION, "BswNoteDescription*", "bsw_value_initset_boxed", "g_value_dup_boxed", 0);
#undef add
}

static MarshalType*
marshal_find (GType    type,
	      gboolean match_fundamental)
{
  MarshalType *m = marshal_type_list;

  for (m = marshal_type_list; m; m = m->next)
    if (m->type == type)
      return m;

  if (match_fundamental)
    for (m = marshal_type_list; m; m = m->next)
      if (m->type == G_TYPE_FUNDAMENTAL (type))
	return m;
  
  return NULL;
}

static const gchar*
marshal_type_name (GType    type,
		   gboolean return_value)
{
  MarshalType *m = NULL;

  if (G_TYPE_IS_DERIVED (type) && (G_TYPE_FUNDAMENTAL (type) == G_TYPE_ENUM ||
				   G_TYPE_FUNDAMENTAL (type) == G_TYPE_FLAGS))
    {
      gchar *name = g_type_name (type);

      if (name[0] == 'B' && name[1] == 's' && name[2] == 'e')
	{
	  name = g_strdup (name);
	  name[2] = 'w';
	}
      return name;
    }
  else
    m = marshal_find (type, TRUE);

  return m ? (return_value ? m->rtype : m->ctype) : NULL;
}

/* BseSomeObject        - type name (tname)
 * BSE_TYPE_SOME_OBJECT - type macro (tmacro)
 * do-funky-stuff	- proc name (pname)
 * BseSomeObject+do-it	- method name (mname)
 * some_identifier	- c identifier (cident)
 * some_function()	- c function (cfunc)
 */
static gchar*
tmacro_from_type (GType type)
{
  MarshalType *m = marshal_find (type, FALSE);
  gchar *s, *result, *p;
  guint was_upper;

  if (m)
    return m->macro;

  s = (gchar*) g_type_name (type);

  if (!is_upper (s[0]))
    g_error ("first char of type \"%s\" is not upper case", g_type_name (type));

  result = g_new (gchar, strlen (s) * 2 + 12);
  p = result;
  
  *p++ = *s++;
  while (!is_upper (*s))
    {
      *p++ = to_upper (*s);
      s++;
    }
  *p++ = '_';
  *p++ = 'T';
  *p++ = 'Y';
  *p++ = 'P';
  *p++ = 'E';
  was_upper = 0;
  while (*s)
    {
      if (is_upper (*s))
	{
	  if (!was_upper || (s[1] && is_lower (s[1]) && was_upper >= 2))
	    *p++ = '_';
	  was_upper ++;
	}
      else
	was_upper = 0;
      *p++ = to_upper (*s);
      s++;
    }
  *p++ = 0;
  
  return result;
}

static gchar*
cfunc_from_proc_name (const gchar *string)
{
  const gchar *s;
  gchar *result = g_new (gchar, strlen (string) * 2 + 12), *p = result;
  guint was_upper;
  
  /* strip procedure namespace */
  if (strchr (string, '+'))	/* methods: BseObject+get-type */
    {
      s = string + 1;
      while (*s && (*s < 'A' || *s > 'Z'))
	s++;
      string = s;
    }
  else				/* procedures: bse-error-blurb */
    {
      s = strchr (string, '-');
      if (!s)
	s = strchr (string, '_');
      if (s && s[1] != 0)
	string = s + 1;
      else
	g_warning ("procedure without namespace prefix: \"%s\"", string);
    }
  g_assert (string[0] != 0);

  was_upper = 0;
  for (s = string; *s; s++)
    switch (*s)
      {
      case '-':
      case '+':
	*p++ = '_';
	break;
      default:
	if (is_upper (*s))
	  {
	    if (!was_upper || (s[1] && is_lower (s[1]) && was_upper >= 2))
	      {
		if (s > string)	/* don't prefix '_' proc name start */
		  *p++ = '_';
	      }
	    *p++ = to_lower (*s);
	    was_upper++;
	  }
	else
	  {
	    *p++ = *s;
	    was_upper = 0;
	  }
	break;
      }
  *p++ = 0;

  return result;
}

static gchar*
cident_canonify (GType        type,
		 const gchar *string)
{
  const gchar *s;
  gchar *result, *p;

  result = g_new (gchar, strlen (string) * 2);
  p = result;
  for (s = string; *s; s++)
    switch (*s)
      {
      case '-':
      case '+':
	*p++ = '_';
	break;
      default:
	*p++ = *s;
	break;
      }
  *p++ = 0;

  return result;
}

static void
print_proc (GType              type,
	    BseProcedureClass *class)
{
  gchar *s, *cname = cfunc_from_proc_name (class->name);
  guint i, n;

  if (gen_body)
    {
      BseCategory *categories = bse_categories_from_type (type, &n);

      fprintf (f_out, "/* \"%s\" */\n", n ? categories[0].category : "?");
      g_free (categories);
    }

  s = g_strdup_printf ("%s %s_%s (",
		       class->n_out_pspecs ? marshal_type_name (class->out_pspecs[0]->value_type, TRUE) : "void",
		       prefix,
		       cname);
  n = strlen (s);
  fprintf (f_out, "%s", s);
  g_free (s);
  for (i = 0; i < class->n_in_pspecs; i++)
    {
      add_type_wrapper (G_PARAM_SPEC_VALUE_TYPE (class->in_pspecs[i]));
      fprintf (f_out, "%s%s %s%s\n",
	       i ? indent (n) : "",
	       marshal_type_name (class->in_pspecs[i]->value_type, FALSE),
	       cident_canonify (class->in_pspecs[i]->value_type, class->in_pspecs[i]->name),
	       i + 1 < class->n_in_pspecs ? "," : gen_body ? ")" : ");");
    }
  if (!class->n_in_pspecs)
    fprintf (f_out, "void%s\n", gen_body ? ")" : ");");
  if (class->n_out_pspecs)
    add_type_wrapper (G_PARAM_SPEC_VALUE_TYPE (class->out_pspecs[0]));
  if (gen_body)
    {
      fprintf (f_out, "{\n");
      if (class->n_out_pspecs)
	fprintf (f_out, "  %s result;\n", marshal_type_name (class->out_pspecs[0]->value_type, TRUE));
      fprintf (f_out, "  BswProxyProcedureCall cl;\n");
      fprintf (f_out, "  GValue *value = cl.ivalues;\n");
      for (i = 0; i < class->n_in_pspecs; i++)
	fprintf (f_out, "  %s (value, %s, %s); value++;\n",
		 marshal_find (class->in_pspecs[i]->value_type, TRUE)->set_func,
		 tmacro_from_type (class->in_pspecs[i]->value_type),
		 cident_canonify (class->in_pspecs[i]->value_type, class->in_pspecs[i]->name));
      fprintf (f_out, "  cl.n_ivalues = value - cl.ivalues;\n");
      if (class->n_out_pspecs)
	fprintf (f_out, "  %s (&cl.ovalue, %s, 0);\n",
		 marshal_find (class->out_pspecs[0]->value_type, TRUE)->set_func,
		 tmacro_from_type (class->out_pspecs[0]->value_type));
      fprintf (f_out, "  cl.proc_name = \"%s\";\n", class->name);
      fprintf (f_out, "  bsw_proxy_call_procedure (&cl);\n");
      if (class->n_out_pspecs)
	{
	  fprintf (f_out, "  result = %s (&cl.ovalue);\n",
		   marshal_find (class->out_pspecs[0]->value_type, TRUE)->get_func);
	  fprintf (f_out, "  g_value_unset (&cl.ovalue);\n");
	  fprintf (f_out, "  return result;\n");
	}
      fprintf (f_out, "}\n");
    }
  fputc ('\n', f_out);
  g_free (cname);
}

static void
print_procs (const gchar *pattern)
{
  BseCategory *categories;
  guint i, n_cats;

  categories = bse_categories_match_typed (pattern, BSE_TYPE_PROCEDURE, &n_cats);

  for (i = 0; i < n_cats; i++)
    {
      BseProcedureClass *class = g_type_class_ref (categories[i].type);
      guint j, can_wrap = TRUE;

      if (class->n_out_pspecs > 1)
	{
	  g_message ("ignoring procedure `%s' with %u output args",
		     class->name, class->n_out_pspecs);
	  can_wrap = FALSE;
	}
      for (j = 0; j < class->n_out_pspecs; j++)
	if (!marshal_type_name (G_PARAM_SPEC_VALUE_TYPE (class->out_pspecs[j]), TRUE))
	  {
	    g_message ("ignoring procedure `%s' with unwrappable output arg \"%s\" of type `%s'",
		       class->name,
		       class->out_pspecs[j]->name,
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (class->out_pspecs[j])));
	    can_wrap = FALSE;
	    break;
	  }
      for (j = 0; j < class->n_in_pspecs; j++)
	if (!marshal_type_name (G_PARAM_SPEC_VALUE_TYPE (class->in_pspecs[j]), FALSE))
	  {
	    g_message ("ignoring procedure `%s' with unwrappable input arg \"%s\" of type `%s'",
		       class->name,
		       class->in_pspecs[j]->name,
		       g_type_name (G_PARAM_SPEC_VALUE_TYPE (class->in_pspecs[j])));
	    can_wrap = FALSE;
	    break;
	  }
      if (can_wrap)
	print_proc (categories[i].type, class);
      g_type_class_unref (class);
    }
  g_free (categories);
}

int
main (gint   argc,
      gchar *argv[])
{
  static gint help (gchar *arg);
  guint i;
  
  f_out = stdout;
  
  g_thread_init (NULL);

  bse_init (&argc, &argv, NULL);

  type_wrapper_ht = g_hash_table_new (NULL, NULL);

  init_marshal_types ();

  for (i = 1; i < argc; i++)
    {
      if (strcmp ("--header", argv[i]) == 0)
	{
	  gen_header = TRUE;
	}
      else if (strcmp ("--body", argv[i]) == 0)
	{
	  gen_body = TRUE;
	}
      else if (strcmp ("-p", argv[i]) == 0)
	{
	  GList *free_list, *list;

	  /* check load BSE plugins to register types */
	  free_list = bse_plugin_dir_list_files (BSE_PATH_PLUGINS);
	  for (list = free_list; list; list = list->next)
	    {
	      gchar *error, *string = list->data;

	      error = bse_plugin_check_load (string);
	      if (error)
		g_warning ("failed to load plugin \"%s\": %s", string, error);
	      g_free (string);
	    }
	  g_list_free (free_list);
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
	  strcmp ("--help", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	return help (argv[i]);
    }

  if (gen_body)
    fprintf (f_out, "%s\n", cfile_header);
  if (gen_header)
    print_enums ();
  print_procs ("*");
  if (gen_header)
    print_type_wrappers ();
  
  return 0;
}

static gint
help (gchar *arg)
{
  fprintf (stderr, "usage: mkapi <qualifier> [-r <type>] [-{i|b} \"\"] [-s #] [-{h|p|x|y}]\n");
  fprintf (stderr, "       -p       include plugins\n");
  fprintf (stderr, "       -h       guess what ;)\n");

  return arg != NULL;
}
