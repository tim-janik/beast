// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfitypes.hh"
#include "sfivalues.hh"
#include "sfiparams.hh"
#include "sfiprimitives.hh"
#include "sfitime.hh"
#include "sfiglue.hh"
#include "sfifilecrawler.hh"
#include <string.h>



/* --- variables --- */


/* --- functions --- */

/* --- FIXME: hacks! */
void
sfi_set_error (GError       **errorp,
	       GQuark         domain,
	       gint           code,
	       const gchar   *format,
	       ...)
{
  if (errorp && !*errorp)
    {
      gchar *message;
      va_list args;
      va_start (args, format);
      message = g_strdup_vprintf (format, args);
      *errorp = g_error_new_literal (domain, code, message);
      g_free (message);
      va_end (args);
    }
}

static inline gchar
char_canon (gchar c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

gchar*
sfi_strdup_canon (const gchar *identifier)
{
  gchar *str = g_strdup (identifier);

  if (str)
    {
      gchar *p;
      for (p = str; *p; p++)
	*p = char_canon (*p);
    }
  return str;
}

static inline gboolean
eval_match (const gchar *str1,
	    const gchar *str2)
{
  while (*str1 && *str2)
    {
      guchar s1 = char_canon (*str1++);
      guchar s2 = char_canon (*str2++);
      if (s1 != s2)
	return FALSE;
    }
  return *str1 == 0 && *str2 == 0;
}

#define isalnum(c)      ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))

gboolean
sfi_choice_match_detailed (const gchar *choice_val1,
			   const gchar *choice_val2,
			   gboolean     l1_ge_l2)
{
  g_return_val_if_fail (choice_val1 != NULL, FALSE);
  g_return_val_if_fail (choice_val2 != NULL, FALSE);

  guint l1 = strlen (choice_val1);
  guint l2 = strlen (choice_val2);
  if (l1_ge_l2 && l1 < l2)
    return FALSE;
  if (l2 > l1)
    {
      const gchar *ts = choice_val2;
      choice_val2 = choice_val1;
      choice_val1 = ts;
      guint tl = l2;
      l2 = l1;
      l1 = tl;
    }
  const gchar *cv1 = choice_val1 + l1 - MIN (l1, l2);
  const gchar *cv2 = choice_val2 + l2 - MIN (l1, l2);
  if (cv1 > choice_val1)  /* only allow partial matches on word boundary */
    {
      if (isalnum (cv1[-1]) && isalnum (cv1[0])) /* no word boundary */
        return FALSE;
    }
  return cv2[0] && eval_match (cv1, cv2);
}

gboolean
sfi_choice_match (const gchar *choice_val1,
		  const gchar *choice_val2)
{
  return sfi_choice_match_detailed (choice_val1, choice_val2, FALSE);
}

static inline gint
consts_rmatch (guint        l1,
	       const gchar *str1,
	       guint        l2,
	       const gchar *str2)
{
  gint i, length = MIN (l1, l2);
  for (i = 1; i <= length; i++)
    {
      gint c1 = str1[l1 - i], c2 = str2[l2 - i];
      if (c1 != c2)
	return c1 > c2 ? +1 : -1;
    }
  return 0; /* missing out the length check here which normal strcmp() does */
}

guint
sfi_constants_get_index (guint               n_consts,
			 const SfiConstants *rsorted_consts,
			 const gchar        *constant)
{
  guint l, offs, order, n_nodes = n_consts;
  gchar *key;
  gint i, cmp;
  
  g_return_val_if_fail (constant != NULL, 0);

  /* canonicalize key */
  l = strlen (constant);
  key = g_new (gchar, l);
  for (offs = 0; offs < l; offs++)
    key[offs] = char_canon (constant[offs]);

  /* perform binary search with chopped tail match */
  offs = 0;
  while (offs < n_nodes)
    {
      i = (offs + n_nodes) >> 1;
      cmp = consts_rmatch (l, key, rsorted_consts[i].name_length, rsorted_consts[i].name);
      if (cmp == 0)
	goto have_match;
      else if (cmp < 0)
	n_nodes = i;
      else /* (cmp > 0) */
	offs = i + 1;
    }
  /* no match */
  g_free (key);
  return 0;

  /* explore neighboured matches and favour early indices */
 have_match:
  offs = i;
  order = rsorted_consts[offs].index;
  /* walk lesser matches */
  for (i = 1; i <= int (offs); i++)
    if (consts_rmatch (l, key, rsorted_consts[offs - i].name_length, rsorted_consts[offs - i].name) == 0)
      order = MIN (order, rsorted_consts[offs - i].index);
    else
      break;
  /* walk greater matches */
  for (i = 1; offs + i < n_consts; i++)
    if (consts_rmatch (l, key, rsorted_consts[offs + i].name_length, rsorted_consts[offs + i].name) == 0)
      order = MIN (order, rsorted_consts[offs + i].index);
    else
      break;
  g_free (key);
  return order;
}

const gchar*
sfi_constants_get_name (guint               n_consts,
			const SfiConstants *consts,
			guint               index)
{
  guint i;

  for (i = 0; i < n_consts; i++)
    if (consts[i].index == index)
      return consts[i].name;
  return NULL;
}

gint
sfi_constants_rcmp (const gchar *canon_identifier1,
		    const gchar *canon_identifier2)
{
  gint cmp, l1, l2;

  g_return_val_if_fail (canon_identifier1 != NULL, 0);
  g_return_val_if_fail (canon_identifier2 != NULL, 0);

  l1 = strlen (canon_identifier1);
  l2 = strlen (canon_identifier2);
  cmp = consts_rmatch (l1, canon_identifier1, l2, canon_identifier2);
  if (!cmp)	/* fixup missing length check */
    return l1 - l2;
  return cmp;
}

const char*
sfi_category_concat (const char         *prefix,
                     const char         *trunk)
{
  if (prefix && !prefix[0])
    prefix = NULL;
  if (!trunk || !trunk[0])
    return NULL;
  gboolean prefix_needs_slash1 = prefix && prefix[0] != '/';
  gboolean prefix_last = prefix ? prefix[strlen (prefix) - 1] : 0;
  gboolean prefix_has_slash2 = prefix_last == '/';
  gboolean prefix_needs_slash2 = prefix && !prefix_has_slash2;
  if (prefix_has_slash2 && trunk)
    while (trunk[0] == '/')
      trunk++;
  gboolean trunk_needs_slash1 = !prefix && trunk[0] != '/';
  return g_intern_strconcat (prefix_needs_slash1 ? "/" : "",
                             prefix ? prefix : "",
                             prefix_needs_slash2 || trunk_needs_slash1 ? "/" : "",
                             trunk,
                             NULL);
}
