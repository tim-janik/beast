/* BSE Feature Comparision Tool
 * Copyright (C) 2004 Stefan Westerfeld
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

#include <bse/bseengine.h>
#include <bse/gslsignal.h>

#include <bse/gsldatautils.h>
#include <bse/gslloader.h>
#include <bse/gslfft.h>
#include <bse/gslsignal.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "topconfig.h"

#include <map>
#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Options {
  string	      programName;
  double              threshold;

  Options ();
  void parse (int *argc_p, char **argv_p[]);
  void printUsage ();
} options;

Options::Options ()
{
  programName = "bsefcompare";
  threshold = 100;
}

void Options::parse (int *argc_p, char **argv_p[])
{
  unsigned int argc;
  char **argv;
  unsigned int i, e;

  g_return_if_fail (argc_p != NULL);
  g_return_if_fail (argv_p != NULL);
  g_return_if_fail (*argc_p >= 0);

  argc = *argc_p;
  argv = *argv_p;

  /*  I am tired of seeing .libs/lt-bsefcompare all the time,
   *  but basically this should be done (to allow renaming the binary):
   *
  if (argc && argv[0])
    programName = argv[0];
  */

  for (i = 1; i < argc; i++)
    {
      const char *opt = strtok (argv[i], "=");
      const char *arg = opt ? strtok (NULL, "\n") : NULL;

      if (strcmp ("--help", argv[i]) == 0)
	{
	  printUsage();
	  exit (0);
	}
      else if (strcmp ("--version", argv[i]) == 0)
	{
	  printf ("%s %s\n", programName.c_str(), BST_VERSION);
	  exit (0);
	}
      else if (strcmp ("--threshold", argv[i]) == 0)
	{
	  threshold = atof (arg);
	  argv[i] = NULL;
	}
    }

  /* resort argc/argv */
  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;
}

void Options::printUsage ()
{
  fprintf (stderr, "usage: %s [ <options> ] <featurefile1> <featurefile2>\n", programName.c_str());
  fprintf (stderr, "\n");
  fprintf (stderr, "options:\n");
  fprintf (stderr, " --threshold=<percent>       set threshold for returning that two files match\n");
  fprintf (stderr, " --help                      help for %s\n", programName.c_str());
  fprintf (stderr, " --version                   print version\n");
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
  double f1len = vector_len (f1);
  double f2len = vector_len (f2);

  bool f1null = f1len < GSL_DOUBLE_MIN_NORMAL;
  bool f2null = f2len < GSL_DOUBLE_MIN_NORMAL;

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

vector<double>
readFeature (FILE *f)
{
  vector<double> result;
  for(;;)
    {
      char buffer[4096];
      if (fgets (buffer, 4096, f))
	{
	  if (buffer[0] == '#')
	    continue;

	  char *p = strtok (buffer, " ");
	  if (p)
	    result.push_back (atof (p));

	  while ((p = strtok (NULL, " \n")))
	    result.push_back (atof (p));

	  if (result.size())
	    return result;
	}
      else
	{
	  return result; /* likely: eof */
	}
    }
}

int main (int argc, char **argv)
{
  /* parse options */
  options.parse (&argc, &argv);

  if (argc != 3)
    {
      options.printUsage ();
      return 1;
    }

  FILE *file1 = fopen (argv[1], "r");
  if (!file1)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], strerror (errno));
      exit (1);
    }

  FILE *file2 = fopen (argv[2], "r");
  if (!file2)
    {
      fprintf (stderr, "%s: can't open the input file %s: %s\n", options.programName.c_str(), argv[1], strerror (errno));
      exit (1);
    }

  vector<double> f1, f2, similarity;

  for (;;)
    {
      f1 = readFeature (file1);
      f2 = readFeature (file2);

      if (f1.size() != f2.size())
	{
	  fprintf (stderr, "feature dimensionalities don't match\n");
	  return 1;
	}
      else if (f1.size() == 0 && f2.size() == 0)
	{
	  double s = 0.0;
	  printf ("similarities: ");
	  for (size_t i = 0; i < similarity.size(); i++)
	    {
	      printf (i == 0 ? "%f" : ",%f", similarity[i] * 100.0); /* percent */
	      s += similarity[i];
	    }
	  printf ("\n");

	  double overall = s / similarity.size() * 100.0; /* percent */

	  printf ("overall similarity rating: %f%% => ", overall);
	  if (overall == 100.0)
	    {
	      printf ("perfect match.\n");
	      return 0;
	    }
	  else if (overall >= options.threshold)
	    {
	      printf ("good match.\n");
	      return 0;
	    }
	  else
	    {
	      printf ("the two files differ.\n");
	      return 1;
	    }
	}
      else if (f1.size() == 1)
	{
	  similarity.push_back (number_similarity (f1[0], f2[0]));
	}
      else
	{
	  similarity.push_back (vector_similarity (f1, f2));
	}
    }
}

/* vim:set ts=8 sts=2 sw=2: */
