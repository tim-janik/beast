/* BSE Feature Comparision Tool
 * Copyright (C) 2004-2006 Stefan Westerfeld
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

#include <bse/bseengine.h>
#include <bse/bsemathsignal.h>

#include <bse/gsldatautils.h>
#include <bse/gslfft.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "topconfig.h"

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

using namespace std;

struct Options {
  string	      program_name;
  double              threshold;
  bool                compact;
  bool                strict;

  Options ();
  void parse (int *argc_p, char **argv_p[]);
  static void print_usage ();
} options;

Options::Options ()
{
  program_name = "bsefcompare";
  threshold = 100;
  compact = false;
  strict = false;
}

static bool
check_arg (uint         argc,
           char        *argv[],
           uint        *nth,
           const char  *opt,              /* for example: --foo */
           const char **opt_arg = NULL)   /* if foo needs an argument, pass a pointer to get the argument */
{
  g_return_val_if_fail (opt != NULL, false);
  g_return_val_if_fail (*nth < argc, false);

  const char *arg = argv[*nth];
  if (!arg)
    return false;

  uint opt_len = strlen (opt);
  if (strcmp (arg, opt) == 0)
    {
      if (opt_arg && *nth + 1 < argc)     /* match foo option with argument: --foo bar */
        {
          argv[(*nth)++] = NULL;
          *opt_arg = argv[*nth];
          argv[*nth] = NULL;
          return true;
        }
      else if (!opt_arg)                  /* match foo option without argument: --foo */
        {
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else if (strncmp (arg, opt, opt_len) == 0 && arg[opt_len] == '=')
    {
      if (opt_arg)                        /* match foo option with argument: --foo=bar */
        {
          *opt_arg = arg + opt_len + 1;
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else
    return false;

  Options::print_usage();
  exit (1);
}

void
Options::parse (int   *argc_p,
                char **argv_p[])
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  unsigned int i;

  g_return_if_fail (argc >= 0);

  /*  I am tired of seeing .libs/lt-bsefcompare all the time,
   *  but basically this should be done (to allow renaming the binary):
   *
  if (argc && argv[0])
    program_name = argv[0];
  */

  for (i = 1; i < argc; i++)
    {
      const char *opt_arg;
      if (strcmp (argv[i], "--help") == 0 ||
          strcmp (argv[i], "-h") == 0)
        {
          print_usage();
          exit (0);
        }
      else if (strcmp (argv[i], "--version") == 0 ||
               strcmp (argv[i], "-v") == 0)
        {
          printf ("%s %s\n", program_name.c_str(), BST_VERSION);
          exit (0);
        }
      else if (check_arg (argc, argv, &i, "--compact"))
        compact = true;
      else if (check_arg (argc, argv, &i, "--strict"))
        strict = true;
      else if (check_arg (argc, argv, &i, "--permissive"))
        strict = false;
      else if (check_arg (argc, argv, &i, "--threshold", &opt_arg))
        threshold = g_ascii_strtod (opt_arg, NULL);
    }

  /* resort argc/argv */
  guint e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

void
Options::print_usage ()
{
  std::string program_name = "bsefcompare";
  fprintf (stderr, "usage: %s [ <options> ] <featurefile1> <featurefile2>\n", program_name.c_str());
  fprintf (stderr, "\n");
  fprintf (stderr, "options:\n");
  fprintf (stderr, " --threshold=<percent>  set threshold for returning that two files match\n");
  fprintf (stderr, " --strict               only compare features with exactly the same dimension\n");
  fprintf (stderr, " --permissive           enable algorithm to compare features with different dimension [default]\n");
  fprintf (stderr, " --compact              suppress printing individual similarities\n");
  fprintf (stderr, " --help                 help for %s\n", program_name.c_str());
  fprintf (stderr, " --version              print version\n");
}

static double
vector_len (const vector<double>& v)
{
  double sqrlen = 0.0;
  for (size_t i = 0; i < v.size(); i++)
    sqrlen += v[i] * v[i];

  return sqrt (sqrlen);
}

static double
number_similarity (double a, double b)
{
  /*
   * we do relative comparision
   *
   * however, its hard to say what this means for very small values
   * so we assume that if both values is < 1.0, we rate their similarity
   * relative to the base 1.0
   */
  double base = 1;
  base = max (base, fabs (a));
  base = max (base, fabs (b));

  return (1.0 - fabs (a - b) / base);
}

static double
vector_similarity (const vector<double>& f1, const vector<double>& f2)
{
  /* different size vectors: only compare if "permissive" comparisions enabled */
  if (f1.size() != f2.size())
    {
      if (options.strict)
	return -1;

      uint min_size = min (f1.size(), f2.size());
      vector<double> common_f1 (f1.begin(), f1.begin() + min_size);
      vector<double> common_f2 (f2.begin(), f2.begin() + min_size);
      return vector_similarity (common_f1, common_f2);
    }

  double f1len = vector_len (f1);
  double f2len = vector_len (f2);

  bool f1null = f1len < BSE_DOUBLE_MIN_NORMAL;
  bool f2null = f2len < BSE_DOUBLE_MIN_NORMAL;

  if (f1null && f2null)
    return 1.0;

  if (f1null || f2null)
    return 0.0;           /* FIXME: is this a good result in that case? */

  /* 
   * this computes the angle between the two vectors in n-dimensional space
   */
  double diff = 0.0;
  for (size_t i = 0; i < f1.size(); i++)
    diff += f1[i] * f2[i];

  return diff / f1len / f2len;
}

static string string_printf (const char *format, ...) G_GNUC_PRINTF (1, 2);

static string
string_printf (const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  char *c_str = g_strdup_vprintf (format, ap);
  va_end(ap);
  string str = c_str;
  g_free (c_str);
  return str;
}


static  GScannerConfig  scanner_config_template = {
  const_cast<gchar *>   /* FIXME: glib should use const gchar* here */
  (
   " \t\r\n"
   )                    /* cset_skip_characters */,
  const_cast<gchar *>
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  const_cast<gchar *>
  (
   G_CSET_a_2_z
   "_0123456789"
   G_CSET_A_2_Z
   )                    /* cset_identifier_nth */,
  const_cast<gchar *>
  ( "#\n" )             /* cpair_comment_single */,
  
  TRUE                  /* case_sensitive */,
  
  TRUE                  /* skip_comment_multi */,
  TRUE                  /* skip_comment_single */,
  TRUE                  /* scan_comment_multi */,
  TRUE                  /* scan_identifier */,
  TRUE                  /* scan_identifier_1char */,
  FALSE                 /* scan_identifier_NULL */,
  TRUE                  /* scan_symbols */,
  FALSE                 /* scan_binary */,
  TRUE                  /* scan_octal */,
  TRUE                  /* scan_float */,
  TRUE                  /* scan_hex */,
  FALSE                 /* scan_hex_dollar */,
  FALSE                 /* scan_string_sq */,
  TRUE                  /* scan_string_dq */,
  TRUE                  /* numbers_2_int */,
  FALSE                 /* int_2_float */,
  FALSE                 /* identifier_2_string */,
  TRUE                  /* char_2_token */,
  TRUE                  /* symbol_2_token */,
  FALSE                 /* scope_0_fallback */,
  TRUE                  /* store_int64 */,
};

#define parse_or_return(token)  G_STMT_START{ \
  GTokenType _t = GTokenType(token); \
  if (g_scanner_get_next_token (scanner) != _t) \
    return _t; \
}G_STMT_END
#define parse_float_or_return(f) G_STMT_START{ \
  bool negate = false; \
  GTokenType t = g_scanner_get_next_token (scanner); \
  if (t == GTokenType('-')) \
  { \
    negate = true; \
    t = g_scanner_get_next_token (scanner); \
  } \
  if (t == G_TOKEN_INT) \
    f = scanner->value.v_int64; \
  else if (t == G_TOKEN_FLOAT) \
    f = scanner->value.v_float; \
  else \
    return G_TOKEN_FLOAT; \
  if (negate) f = -f; \
}G_STMT_END
#define parse_int_or_return(i) G_STMT_START{ \
  bool negate = (g_scanner_peek_next_token (scanner) == GTokenType('-')); \
  if (negate) \
    g_scanner_get_next_token(scanner); \
  if (g_scanner_get_next_token (scanner) != G_TOKEN_INT) \
    return G_TOKEN_INT; \
  i = scanner->value.v_int64; \
  if (negate) i = -i; \
}G_STMT_END

struct FeatureValue {
  string name;
  enum Type {
    TYPE_NUMBER,
    TYPE_VECTOR,
    TYPE_MATRIX
  } type;

  FeatureValue (string name, Type type) : name (name), type (type)
  {
  }
  virtual
  ~FeatureValue()
  {
  }
  virtual GTokenType parse (GScanner *scanner) = 0;
  virtual string     printable_type() const = 0;
  /**
   * returns a value between 0 and 1 if the features could be
   * compared, where
   *   0 means they are not similar at all
   *   1 means they are maximally similar
   * 
   * or, on error
   *  -1 which means that the features could not be compared due to type mismatch
   */
  virtual double     similarity (const FeatureValue *value) const = 0; 
};

struct FeatureValueNumber : FeatureValue {
  double number;

  FeatureValueNumber (const string& name) : FeatureValue (name, TYPE_NUMBER)
  {
    number = 0.0; /* hopefully never used, but initialized by parse */
  }

  GTokenType parse (GScanner *scanner);
  string     printable_type() const;
  double     similarity (const FeatureValue *value) const;
};

struct FeatureValueVector : FeatureValue {
  int n;
  vector<double> data; /* well, can't call it vector */

  FeatureValueVector (const string& name, int n) : FeatureValue (name, TYPE_VECTOR), n (n)
  {
  }

  GTokenType parse (GScanner *scanner);
  string     printable_type() const;
  double     similarity (const FeatureValue *value) const;
};

struct FeatureValueMatrix : FeatureValue {
  int m, n;
  vector< vector<double> > matrix;

  FeatureValueMatrix (const string& name, int m, int n) : FeatureValue (name, TYPE_MATRIX), m (m), n (n)
  {
  }

  GTokenType parse (GScanner *scanner);
  string     printable_type() const;
  double     similarity (const FeatureValue *value) const;
};

struct FeatureValueFile {
  string filename;
  vector<FeatureValue*> feature_values;

  GTokenType parseFeatureValue (GScanner *scanner);
  void       parse (const string& filename);

  ~FeatureValueFile();
};

class ComparisionStrategy
{
  FeatureValue::Type                       m_type;
  static map<string, ComparisionStrategy*> m_strategies;

public:
  ComparisionStrategy (FeatureValue::Type type) :
    m_type (type)
  {
  }
  virtual
  ~ComparisionStrategy()
  {
  }
  FeatureValue::Type
  type()
  {
    return m_type;
  }
  void
  register_strategy (const string& feature_name)
  {
    g_return_if_fail (!m_strategies[feature_name]);

    m_strategies[feature_name] = this;
  }
  static ComparisionStrategy*
  find_strategy (const string&      feature_name,
                 FeatureValue::Type type)
  {
    ComparisionStrategy* s = m_strategies[feature_name];

    // produce a warning for a strategy with the right feature name but a wrong type
    if (s)
      {
	g_return_val_if_fail (type == s->type(), 0);
      }
    return s;
  }
  virtual double
  similarity (const FeatureValue *value1,
              const FeatureValue *value2) const = 0;
};

map<string, ComparisionStrategy*> ComparisionStrategy::m_strategies;

//------- FeatureValueNumber implementation --------

GTokenType
FeatureValueNumber::parse (GScanner *scanner)
{
  parse_float_or_return (number);

  return G_TOKEN_NONE;
}

string
FeatureValueNumber::printable_type() const
{
  return "number";
}

double
FeatureValueNumber::similarity (const FeatureValue *value) const
{
  if (value->type != type || value->name != name)
    return -1;

  // custom strategy?
  const ComparisionStrategy *strategy = ComparisionStrategy::find_strategy (name, type);
  if (strategy)
    return strategy->similarity (this, value);

  // no? -> default strategy
  const FeatureValueNumber *v = static_cast<const FeatureValueNumber *> (value);
  return number_similarity (number, v->number);
}

//------- FeatureValueVector implementation --------

GTokenType
FeatureValueVector::parse (GScanner *scanner)
{
  parse_or_return ('{');
  for (int j = 0; j < n; j++)
    {
      double d;
      parse_float_or_return (d);
      data.push_back (d);
    }
  parse_or_return ('}');

  return G_TOKEN_NONE;
}

string
FeatureValueVector::printable_type() const
{
  return string_printf ("%d element vector", n);
}

double
FeatureValueVector::similarity (const FeatureValue *value) const
{
  if (value->type != type || value->name != name)
    return -1;

  // custom strategy?
  const ComparisionStrategy *strategy = ComparisionStrategy::find_strategy (name, type);
  if (strategy)
    return strategy->similarity (this, value);

  // no? -> default strategy
  const FeatureValueVector *v = static_cast<const FeatureValueVector *> (value);
  return vector_similarity (data, v->data); // can return -1 on size mismatch when --strict is set
}

//------- FeatureValueMatrix implementation --------

GTokenType
FeatureValueMatrix::parse (GScanner *scanner)
{
  /* for a m x n matrix, we write
   * 
   * data[m,n] = {
   *   { x_11 x_12 ... x_1n }
   *   { x_21 x_22 ... x_2n }
   *   {  .    .   ...  .   }
   *   {  .    .   ...  .   }
   *   { x_m1 x_m2 ... x_mn }
   * };
   */
  parse_or_return ('{');
  for (int i = 0; i < m; i++)
    {
      parse_or_return ('{');

      vector<double> line;
      for (int j = 0; j < n; j++)
	{
	  double d;
	  parse_float_or_return (d);
	  line.push_back (d);
	}
      matrix.push_back (line);
      parse_or_return ('}');
    }
  parse_or_return ('}');

  return G_TOKEN_NONE;
}

string
FeatureValueMatrix::printable_type() const
{
  return string_printf ("%d x %d matrix", m, n);
}

double
FeatureValueMatrix::similarity (const FeatureValue *value) const
{
  if (value->type != type || value->name != name)
    return -1;

  // custom strategy?
  const ComparisionStrategy *strategy = ComparisionStrategy::find_strategy (name, type);
  if (strategy)
    return strategy->similarity (this, value);

  // no? -> default strategy
  const FeatureValueMatrix *v = static_cast<const FeatureValueMatrix *> (value);

  /* matrix dimensions must match for strict comparision */
  if (options.strict && (m != v->m || n != v->n))
    return -1;

  double s = 0;
  uint min_m = min (m, v->m);
  if (min_m)
    {
      for (uint i = 0; i < min_m; i++)
	s += vector_similarity (matrix[i], v->matrix[i]);
      return s / min_m;
    }
  else
    {
      return 0; /* FIXME: or return 1, or return 1 IFF permissive? */
    }
}

//------- FeatureValueFile implementation --------

GTokenType
FeatureValueFile::parseFeatureValue (GScanner *scanner)
{
  FeatureValue *value = NULL;

  string name = scanner->value.v_identifier;

  if (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_BRACE)
    {
      int a;

      parse_or_return (G_TOKEN_LEFT_BRACE);
      parse_int_or_return (a);

      if (g_scanner_peek_next_token (scanner) == ',')
	{
	  int b;

	  parse_or_return (',');
	  parse_int_or_return (b);
	  parse_or_return (G_TOKEN_RIGHT_BRACE);
	  parse_or_return ('=');

	  /* a x b matrix */
	  value = new FeatureValueMatrix (name, a, b);
	}
      else
	{
	  parse_or_return (G_TOKEN_RIGHT_BRACE);
	  parse_or_return ('=');

	  /* a elements vector */
	  value = new FeatureValueVector (name, a);
	}
    }
  else
    {
      parse_or_return ('=');
      value = new FeatureValueNumber (name);
    }

  /* parse the actual value */
  GTokenType expected_token = value->parse (scanner);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  parse_or_return (';');
  feature_values.push_back (value);
  return G_TOKEN_NONE;
}

void
FeatureValueFile::parse (const string& filename)
{
  g_return_if_fail (this->filename == "");
  this->filename = filename;

  GScanner *scanner = g_scanner_new64 (&scanner_config_template);
  int fd = open (filename.c_str(), O_RDONLY);
  if (fd < 0)
    {
      fprintf (stderr, "%s: failed to open input file \"%s\": %s\n",
	       options.program_name.c_str(), filename.c_str(), strerror (errno));
      exit (1);
    }
  g_scanner_input_file (scanner, fd);
  scanner->input_name = this->filename.c_str();

  GTokenType expected_token = G_TOKEN_NONE;
  while (!g_scanner_eof (scanner) && expected_token == G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);
      
      if (scanner->token == G_TOKEN_EOF)
        break;
      else if (scanner->token == G_TOKEN_IDENTIFIER)
        expected_token = parseFeatureValue (scanner);
      else
        expected_token = G_TOKEN_EOF; /* '('; */
    }

  if (expected_token != G_TOKEN_NONE)
    {
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL, NULL, TRUE);
      exit (1);
    }
}

FeatureValueFile::~FeatureValueFile()
{
  for (vector<FeatureValue *>::iterator fvi = feature_values.begin(); fvi != feature_values.end(); fvi++)
    delete *fvi;
}

int
main (int argc, char **argv)
{
  /* parse options */
  options.parse (&argc, &argv);

  if (argc != 3)
    {
      options.print_usage ();
      return 1;
    }

  FeatureValueFile file1, file2;
  file1.parse (argv[1]);
  file2.parse (argv[2]);

  if (file1.feature_values.size() != file2.feature_values.size())
    {
      g_printerr ("%s: can't compare files\n", options.program_name.c_str());
      g_printerr ("  * file \"%s\" contains %zd feature values\n", file1.filename.c_str(), file1.feature_values.size());
      g_printerr ("  * file \"%s\" contains %zd feature values\n", file2.filename.c_str(), file2.feature_values.size());
      exit (1);
    }

  vector<double> similarity;
  for (uint i = 0; i < file1.feature_values.size(); i++)
    {
      const FeatureValue *f1 = file1.feature_values[i];
      const FeatureValue *f2 = file2.feature_values[i];
      double s = f1->similarity (f2);
      if (s < 0)
	{
	  g_printerr ("%s: can't compare features:\n", options.program_name.c_str());
	  g_printerr ("  * %s which is a %s from file \"%s\"\n",
	              f1->name.c_str(), f1->printable_type().c_str(), file1.filename.c_str());
	  g_printerr ("  * %s which is a %s from file \"%s\"\n",
	              f2->name.c_str(), f2->printable_type().c_str(), file2.filename.c_str());
	  return 1;
	}
      similarity.push_back (s);
    }

  double s = 0.0;
  double min_s = similarity.empty() ? 0.0 : similarity[0];
  double max_s = min_s;

  printf ("similarities: ");
  for (size_t i = 0; i < similarity.size(); i++)
    {
      if (!options.compact)
        {
          if (i != 0)
            printf (", ");
          printf ("%s=%.2f%%", file1.feature_values[i]->name.c_str(), similarity[i] * 100.0); /* percent */
        }
      s += similarity[i];
      min_s = min (similarity[i], min_s);
      min_s = min (similarity[i], max_s);
    }
  if (options.compact)
    printf ("minimum=%.2f%% maximum=%.2f%%", min_s * 100.0, max_s * 100.0);
  printf ("\n");

  double average_similarity = s / similarity.size() * 100.0; /* percent */
  /* We check this first, because we explicitely allow setting the threshold
   * to a value > 100%, which will make bsefcompare always fail.
   */
  std::string rating;
  int result = 0;
  if (average_similarity >= options.threshold)
    {
      if (average_similarity == 100.0)
	rating = "perfect match";
      else
	rating = "good match";
    }
  else
    {
      rating = "similarity below threshold";
      result = 1;
    }
  printf ("average similarity rating (%s): %.2f%%\n", rating.c_str(), average_similarity);
  return result;
}

// extra comparision strategies

class TimingComparisionStrategy : public ComparisionStrategy
{
public:
  TimingComparisionStrategy() :
    ComparisionStrategy (FeatureValue::TYPE_VECTOR)
  {
    register_strategy ("attack_times");
    register_strategy ("release_times");
  }
  double similarity (const FeatureValue *value1,
                     const FeatureValue *value2) const
  {
    const FeatureValueVector *vec1 = static_cast<const FeatureValueVector *> (value1);
    const FeatureValueVector *vec2 = static_cast<const FeatureValueVector *> (value2);

    g_printerr ("using custom TimingComparisionStrategy\n");

    // the sizes of the two vectors can be different
    uint   size = min (vec1->data.size(), vec2->data.size());
    double match = 0.0;

    for (uint i = 0; i < size; i++)
      {
	// this is a really stupid epsilon diff
	if (fabs (vec1->data[i] - vec2->data[i]) < 0.01)
	  match++;
      }
    // return a value in the interval [0..1] here, where 0 is no match and 1 is a 100% match
    return size ? match / size : 1;
  }
} timingComparisionStrategy;

/* vim:set ts=8 sts=2 sw=2: */
