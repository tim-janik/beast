/* XmlAntiSpace - Eliminate whitespaces from XML markup
 * Copyright (C) 2002 Tim Janik
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
#include "xmlantispace.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


/* --- rule setup --- */
static XasRule rule_fallback = {
  INHERIT,	/* tab2space */
  INHERIT,	/* newline2space */
  INHERIT,	/* compress_spaces */
  INHERIT,	/* del2newline */
  INHERIT,	/* kill_leading_space */
  INHERIT,	/* kill_trailing_space */
  INHERIT,	/* no_leading_spaces */
  INHERIT,	/* no_trailing_spaces */
  INHERIT,	/* back2newline */
};
static XasRule rule_strip_outer = {
  INHERIT,	/* tab2space */
  INHERIT,	/* newline2space */
  INHERIT,	/* compress_spaces */
  INHERIT,	/* del2newline */
  INHERIT,	/* kill_leading_space */
  INHERIT,	/* kill_trailing_space */
  TRUE,		/* no_leading_spaces */
  TRUE,		/* no_trailing_spaces */
  INHERIT,	/* back2newline */
};
static XasRule rule_2space_compress = {
  TRUE,		/* tab2space */
  TRUE,		/* newline2space */
  TRUE,		/* compress_spaces */
  INHERIT,	/* del2newline */
  INHERIT,	/* kill_leading_space */
  INHERIT,	/* kill_trailing_space */
  INHERIT,	/* no_leading_spaces */
  INHERIT,	/* no_trailing_spaces */
  INHERIT,	/* back2newline */
};
static XasRule rule_keepspace = {
  FALSE,	/* tab2space */
  FALSE,	/* newline2space */
  FALSE,	/* compress_spaces */
  TRUE,		/* del2newline */
  FALSE,	/* kill_leading_space */
  FALSE,	/* kill_trailing_space */
  FALSE,	/* no_leading_spaces */
  FALSE,	/* no_trailing_spaces */
  TRUE,		/* back2newline */
};
static const struct { const gchar *tname; const XasRule *rule; } tag_rules[] = {
  { "texinfo",		&rule_2space_compress, },
  { "para",		&rule_strip_outer, },
  { "display",		&rule_keepspace, },
  { "smalldisplay",	&rule_keepspace, },
  { "example",		&rule_keepspace, },
  { "smallexample",	&rule_keepspace, },
  { "format",		&rule_keepspace, },
  { "smallformat",	&rule_keepspace, },
  { "lisp",		&rule_keepspace, },
  { "smalllisp",	&rule_keepspace, },
};


/* --- macros --- */
#define ASSERT(code)	do { if (code) /**/; else { g_printerr ("failed to assert: %s\n", G_STRINGIFY (code)); exit (2); } } while (0)
#define ABORT(msg)	do { g_printerr ("%s\n", msg); exit (2); } while (0)
#define ABORT2(msg,a)	do { g_printerr ("%s\n", g_strdup_printf (msg, a)); exit (2); } while (0)
#define ABORT3(msg,a,b)	do { g_printerr ("%s\n", g_strdup_printf (msg, a, b)); exit (2); } while (0)
#define ABORT_EOF(msg)	do { g_printerr ("premature end of file while parsing %s\n", msg); exit (2); } while (0)
#define	g_string_clear(gs)	g_string_truncate ((gs), 0)


/* --- output processor --- */
static void
flush_output (XasOutput *out)
{
  gint count, n = out->p - out->buffer;
  errno = 0;
  if (n)
    {
      do
	count = write (out->fd, out->buffer, n);
      while (count == -1 && (errno == EINTR || errno == EAGAIN));
      if (count != n)
	ABORT3 ("failed to write %u bytes: %s", n, g_strerror (errno));
    }
  out->p = out->buffer;
}

static void
write_char (XasOutput *out,
	    gint       ch)
{
  ASSERT (strlen (out->space_pipe) == 0);
  *(out->p++) = ch;
  if (out->p - out->buffer >= BUFFER_SIZE)
    flush_output (out);
}

static void
space_pipe_add (XasOutput   *out,
		const gchar *sp)
{
  guint n = strlen (out->space_pipe);
  guint l = strlen (sp);
  out->space_pipe = g_renew (char, out->space_pipe, n + l + 1);
  memcpy (out->space_pipe + n, sp, l);
  out->space_pipe[n + l] = 0;
}

static void
flush_space_pipe (XasOutput *out)
{
  if (out->space_pipe[0])
    {
      guint i;
      for (i = 0; out->space_pipe[i]; i++)
	{
	  char c = out->space_pipe[i];
	  out->space_pipe[i] = 0;
	  write_char (out, c);
	}
    }
}

static void
putc (XasOutput *out,
      gint       ch)
{
  if (!out->literal)
    {
      if (out->del2newline && (ch == ' ' ||
			       ch == '\t' ||
			       ch == '\n'))
	{
	  out->del2newline = ch != '\n';
	  return;
	}
      else
	out->del2newline = FALSE;
      if (ch == '\t' && out->tab2space)
	ch = ' ';
      else if (ch == '\n')
	{
	  if (out->newline2space)
	    ch = ' ';
	  else
	    {
	      out->last = '\n';
	      space_pipe_add (out, "\n");
	      return;
	    }
	}
      if (ch == ' ')
	{
	  if (out->compress_spaces && out->last == ' ')
	    return;
	  out->last = ' ';
	  if (out->skip_spaces)
	    out->skip_spaces--;
	  else
	    space_pipe_add (out, " ");
	  return;
	}
    }
  /* literal or non-space */
  out->skip_spaces = 0;
  flush_space_pipe (out);
  out->last = ch;
  write_char (out, ch);
}
static void
puts (XasOutput   *out,
      const gchar *str)
{
  while (*str)
    putc (out, *str++);
}
static void
puts_literal (XasOutput   *out,
	      const gchar *str)
{
  gboolean was_literal = out->literal;
  out->literal = TRUE;
  while (*str)
    putc (out, *str++);
  out->literal = was_literal;
}

static void
configure_output (XasOutput     *out,
		  const XasRule *rule)
{
  out->tab2space = rule->tab2space;
  out->newline2space = rule->newline2space;
  out->compress_spaces = rule->compress_spaces;
}


/* --- input parser --- */
static int
peekc (XasInput *inp)
{
  if (inp->text < inp->bound)
    return *inp->text;
  else if (inp->fd >= 0)
    {
      gint count;
      gchar *buffer = inp->buffer;
      do
	count = read (inp->fd, buffer, BUFFER_SIZE);
      while (count == -1 && (errno == EINTR || errno == EAGAIN));
      if (count < 1)
	{
	  inp->fd = -1;
	  return 0;
	}
      else
	{
	  inp->text = buffer;
	  inp->bound = buffer + count;
	  return *inp->text;
	}
    }
  else
    return 0;
}
static int
getc (XasInput *inp)
{
  gint c = inp->last = peekc (inp);
  if (c)
    inp->text++;
  return c;
}
static int
lastc (XasInput *inp)
{
  return inp->last;
}


/* --- tag stack --- */
static XasTag *tcurrent = NULL;

#define	DO_INHERIT(tag, parent, member) do { if ((tag).member == INHERIT) (tag).member = (parent).member; } while (0)
static void
push_rule_tag (gchar         *name,
	       const XasRule *rule)
{
  XasTag *parent = tcurrent;
  XasTag *tag = g_new (XasTag, 1);
  tag->name = name;
  tag->next = parent;
  tcurrent = tag;
  tag->rule = *rule;
  if (parent)
    {
      DO_INHERIT (tag->rule, parent->rule, tab2space);
      DO_INHERIT (tag->rule, parent->rule, newline2space);
      DO_INHERIT (tag->rule, parent->rule, compress_spaces);
      DO_INHERIT (tag->rule, parent->rule, del2newline);
      DO_INHERIT (tag->rule, parent->rule, kill_leading_space);
      DO_INHERIT (tag->rule, parent->rule, kill_trailing_space);
      DO_INHERIT (tag->rule, parent->rule, no_leading_spaces);
      DO_INHERIT (tag->rule, parent->rule, no_trailing_spaces);
      DO_INHERIT (tag->rule, parent->rule, back2newline);
    }
}

static void
push_tag (const gchar *name,
	  XasOutput   *out)
{
  const XasRule *rule = &rule_fallback;
  guint i;
  for (i = 0; i < G_N_ELEMENTS (tag_rules); i++)
    if (strcmp (name, tag_rules[i].tname) == 0)
      {
	rule = tag_rules[i].rule;
	break;
      }
  push_rule_tag (g_strdup (name), rule);
  // g_printerr ("PUSH: %s\n", name);
  configure_output (out, &tcurrent->rule);
}

static void
pop_tag (const gchar *name,
	 XasOutput   *out)
{
  XasTag *t = tcurrent;
  ASSERT (tcurrent != NULL);
  if (!t->next)
    ABORT2 ("tag '%s' was never opened", name);
  if (strcmp (t->name, name))
    ABORT3 ("tag '%s' closed by '%s'", tcurrent->name, name);
  tcurrent = t->next;
  g_free (t->name);
  g_free (t);
  configure_output (out, &tcurrent->rule);
}


/* --- processing --- */
static gchar*
xtract_name (const gchar *string)
{
  const gchar *s = string;
  const gchar *cset = (G_CSET_A_2_Z
		       G_CSET_a_2_z
		       G_CSET_DIGITS
		       ":-_"
		       );
  /* advance across valid xml tag name characters only */
  while (strchr (cset, *s))
    s++;
  return g_strndup (string, s - string);
}

static void
read_string_rest (XasInput *inp,
		  GString  *gstring,
		  gchar     term)
{
  gboolean escape = FALSE;
  while (1)
    {
      gint c = getc (inp);
      switch (c)
	{
	case 0:
	  ABORT_EOF ("string");
	case '\\':
	  if (escape)
	    {
	      g_string_append_c (gstring, '\\');
	      escape = FALSE;
	    }
	  else
	    escape = TRUE;
	  break;
	default:
	  if (c == term && !escape)
	    {
	      g_string_append_c (gstring, term);
	      return;
	    }
	  g_string_append_c (gstring, c);
	  escape = FALSE;
	  break;
	}
    }
}

static void
read_comment_rest (XasInput *inp,
		   GString  *gstring)
{
  gboolean seenm = 0;
  while (1)
    {
      gint c = getc (inp);
      switch (c)
	{
	case 0:
	  ABORT_EOF ("comment");
	case '-':
	  seenm++;
	  if (seenm == 2)
	    {
	      g_string_append_c (gstring, '-');
	      c = getc (inp);
	      if (c != '>')
		ABORT ("invalid comment syntax");
	      g_string_append_c (gstring, '>');
	      // g_printerr ("COMMENT: %s\n", gstring->str);
	      return;
	    }
	  g_string_append_c (gstring, '-');
	  break;
	default:
	  g_string_append_c (gstring, c);
	  seenm = 0;
	  break;
	}
    }
}

static void
read_tag (XasInput *inp,
	  GString  *gstring)
{
  gint c = getc (inp);
  ASSERT (c == '<');
  g_string_append_c (gstring, '<');
  while (1)
    {
      c = getc (inp);
      switch (c)
	{
	case 0:
	  ABORT_EOF ("tag");
	case '<':
	  ABORT ("'<' in '<'");
	case '>':
	  g_string_append_c (gstring, '>');
	  return;
	case '-':
	  g_string_append_c (gstring, c);
	  if (gstring->len == 4 && strcmp (gstring->str, "<!--") == 0)
	    {
	      read_comment_rest (inp, gstring);
	      return;
	    }
	  break;
	case '"':
	case '\'':
	  g_string_append_c (gstring, c);
	  read_string_rest (inp, gstring, c);
	  break;
	default:
	  g_string_append_c (gstring, c);
	  break;
	}
    }
}

static void
process (XasInput  *inp,
	 XasOutput *out)
{
  GString *gstring = g_string_new ("");
  while (1)
    {
      gint c = peekc (inp);
      switch (c)
	{
	  gchar *name;
	case 0:
	  return;
	case '<':
	  g_string_clear (gstring);
	  read_tag (inp, gstring);
	  ASSERT (gstring->str[0] == '<' && gstring->str[gstring->len-1] == '>');
	  if (gstring->str[1] == '/')				/* closing tag */
	    {
	      if (out->space_pipe[0])
		{
		  guint n = strlen (out->space_pipe);
		  if (tcurrent->rule.back2newline)
		    {
		      gchar *p = strrchr (out->space_pipe, '\n');
		      if (p)
			*p = 0;
		    }
		  if (tcurrent->rule.no_trailing_spaces)
		    while (n && out->space_pipe[n - 1] == ' ')
		      out->space_pipe[--n] = 0;
		  else if (tcurrent->rule.kill_trailing_space &&
			   out->space_pipe[n - 1] == ' ')
		    out->space_pipe[--n] = 0;
		}
	      name = xtract_name (gstring->str + 2);
	      pop_tag (name, out);
	      puts_literal (out, gstring->str);
	    }
	  else if (gstring->str[1] == '?')			/* non tag */
	    {
	      name = xtract_name (gstring->str + 2);
	      puts_literal (out, gstring->str);
	    }
	  else if (strncmp (gstring->str, "<!--", 4) == 0)	/* comment */
	    {
	      name = NULL;
	      puts_literal (out, gstring->str);
	    }
	  else if (gstring->str[1] == '!')			/* non tag */
	    {
	      name = xtract_name (gstring->str + 2);
              puts_literal (out, gstring->str);
	    }
	  else if (gstring->str[gstring->len - 2] == '/')	/* auto closing tag */
	    {
              name = xtract_name (gstring->str + 1);
	      push_tag (name, out);
	      pop_tag (name, out);
	      puts_literal (out, gstring->str);
	    }
	  else							/* opening tag */
	    {
	      name = xtract_name (gstring->str + 1);
	      push_tag (name, out);
              puts_literal (out, gstring->str);
	      if (tcurrent->rule.no_leading_spaces)
		out->skip_spaces = G_MAXUINT;
	      else if (tcurrent->rule.kill_leading_space)
		out->skip_spaces = 1;
	      out->del2newline = tcurrent->rule.del2newline;
	    }
	  g_free (name);
	  break;
	default:
	  c = getc (inp);
	  putc (out, c);
	  break;
	}
    }
  g_string_free (gstring, TRUE);
}

int
main (int   argc,
      char *argv[])
{
  static XasRule rule_default = {
    FALSE,	/* tab2space */
    FALSE,	/* newline2space */
    FALSE,	/* compress_spaces */
    FALSE,	/* del2newline */
    FALSE,	/* kill_leading_space */
    FALSE,	/* kill_trailing_space */
    FALSE,	/* no_leading_spaces */
    FALSE,	/* no_trailing_spaces */
    FALSE,	/* back2newline */
  };
  XasInput  inp = { 0, 0, };
  XasOutput out = { 1, 0, };
  out.p = out.buffer;
  out.space_pipe = g_strdup ("");
  push_rule_tag ("XmlAntiSpace#TopLevel", &rule_default);
  process (&inp, &out);
  flush_space_pipe (&out);
  flush_output (&out);
  g_free (out.space_pipe);
  return 0;
}
