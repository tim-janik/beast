/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <sfi/glib-extra.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "sfidl-parser.h"
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include <iostream>
#include <set>
#include <stack>

namespace {
using namespace Sfidl;
using namespace std;

/* --- variables --- */
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
  0			/* cpair_comment_single */,
  
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
};

/* --- defines --- */

#define DEBUG(x)         // dprintf(2,"%s",x)

enum ExtraToken {
  TOKEN_NAMESPACE = G_TOKEN_LAST + 1,
  TOKEN_CLASS,
  TOKEN_CHOICE,
  TOKEN_RECORD,
  TOKEN_SEQUENCE,
  TOKEN_PROPERTY,
  TOKEN_GROUP,
  TOKEN_USING,
  TOKEN_CONST,
  TOKEN_CONST_IDENT,
  TOKEN_INFO,
  TOKEN_ISTREAM,
  TOKEN_JSTREAM,
  TOKEN_OSTREAM,
  TOKEN_ERROR
};

const char *token_symbols[] = {
  "namespace", "class", "choice", "record", "sequence",
  "property", "group", "using",
  "Const", "ConstIdent", "Info", "IStream", "JStream", "OStream",
  0
};

bool operator== (GTokenType t, ExtraToken e) { return (int) t == (int) e; }

#define parse_or_return(token)  G_STMT_START{ \
  GTokenType _t = GTokenType(token); \
  if (g_scanner_get_next_token (scanner) != _t) \
    return _t; \
}G_STMT_END
#define peek_or_return(token)   G_STMT_START{ \
  GScanner *__s = (scanner); GTokenType _t = GTokenType(token); \
  if (g_scanner_peek_next_token (__s) != _t) { \
    g_scanner_get_next_token (__s); /* advance position for error-handler */ \
    return _t; \
  } \
}G_STMT_END
#define parse_string_or_return(str) G_STMT_START{ \
  string& __s = str; /* type safety - assume argument is a std::string */ \
  GTokenType __t, __expected; \
  bool __bracket = (g_scanner_peek_next_token (scanner) == GTokenType('(')); \
  if (__bracket) \
    parse_or_return ('('); \
  __expected = parseStringOrConst (__s); \
  if (__expected != G_TOKEN_NONE) return __expected; \
  __t = g_scanner_peek_next_token (scanner); \
  while (__t == G_TOKEN_STRING || __t == G_TOKEN_IDENTIFIER) { \
    string __snew; \
    __expected = parseStringOrConst (__snew); \
    if (__expected != G_TOKEN_NONE) return __expected; \
    __s += __snew; \
    __t = g_scanner_peek_next_token (scanner); \
  } \
  if (__bracket) \
    parse_or_return (')'); \
}G_STMT_END
#define parse_istring_or_return(str) G_STMT_START{ \
  IString& __is = str; /* type safety - assume argument is an IString */ \
  __is.i18n = (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER && \
               strcmp (scanner->next_value.v_string, "_") == 0); \
  if (__is.i18n) { \
    parse_or_return (G_TOKEN_IDENTIFIER); \
    peek_or_return ('('); \
  } \
  parse_string_or_return (__is); \
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

/* --- methods --- */

bool Parser::isChoice(const string& type) const
{
  map<string,int>::const_iterator i = typeMap.find (type);
  return (i != typeMap.end()) && ((i->second & (~tdProto)) == tdChoice);
}

bool Parser::isSequence(const string& type) const
{
  map<string,int>::const_iterator i = typeMap.find (type);
  return (i != typeMap.end()) && ((i->second & (~tdProto)) == tdSequence);
}

bool Parser::isRecord(const string& type) const
{
  map<string,int>::const_iterator i = typeMap.find (type);
  return (i != typeMap.end()) && ((i->second & (~tdProto)) == tdRecord);
}

bool Parser::isClass(const string& type) const
{
  map<string,int>::const_iterator i = typeMap.find (type);
  return (i != typeMap.end()) && ((i->second & (~tdProto)) == tdClass);
}

Type Parser::typeOf (const string& type) const
{
  if (type == "void")	      return VOID;
  if (type == "Sfi::Bool")    return BOOL;
  if (type == "Sfi::Int")     return INT;
  if (type == "Sfi::Num")     return NUM;
  if (type == "Sfi::Real")    return REAL;
  if (type == "Sfi::String")  return STRING;
  if (isChoice (type))	      return CHOICE;
  if (type == "Sfi::BBlock")  return BBLOCK;
  if (type == "Sfi::FBlock")  return FBLOCK;
  if (type == "Sfi::Rec")     return SFIREC;
  if (isSequence (type))      return SEQUENCE;
  if (isRecord (type))	      return RECORD;
  if (isClass (type))	      return OBJECT;
  g_error (("invalid type: " + type).c_str());
  return VOID;
}

Sequence Parser::findSequence(const string& name) const
{
  vector<Sequence>::const_iterator i;
  
  for (i=sequences.begin(); i != sequences.end(); i++)
    if (i->name == name)
      return *i;
  
  return Sequence();
}

Record Parser::findRecord(const string& name) const
{
  vector<Record>::const_iterator i;
  
  for (i=records.begin(); i != records.end(); i++)
    if (i->name == name)
      return *i;
  
  return Record();
}

const Class*
Parser::findClass (const string& name) const
{
  for (vector<Class>::const_iterator ci = classes.begin(); ci != classes.end(); ci++)
    if (ci->name == name)
      return &*ci;

  return 0;
}

Parser::Parser () : options (*Options::the())
{
  scanner = g_scanner_new64 (&scanner_config_template);
  
  for (int n = 0; token_symbols[n]; n++)
    g_scanner_add_symbol (scanner, token_symbols[n], GUINT_TO_POINTER (G_TOKEN_LAST + 1 + n));
  
  scanner->max_parse_errors = 10;
  scanner->parse_errors = 0;
  scanner->msg_handler = scannerMsgHandler;
  scanner->user_data = this;
}

void Parser::printError (const gchar *format, ...)
{
  va_list args;
  gchar *string;
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  if (scanner->parse_errors < scanner->max_parse_errors)
    g_scanner_error (scanner, "%s", string);
  
  g_free (string);
}

void Parser::printWarning (const gchar *format, ...)
{
  va_list args;
  gchar *string;
  
  va_start (args, format);
  string = g_strdup_vprintf (format, args);
  va_end (args);
  
  g_scanner_warn (scanner, "%s", string);
  g_free (string);
}

void Parser::scannerMsgHandler (GScanner *scanner, gchar *message, gboolean is_error)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (scanner->user_data != NULL);

  Parser *parser = static_cast<Parser *>(scanner->user_data);
  if (scanner->line > 0 && parser->scannerLineInfo.size() >= scanner->line)
    {
      const LineInfo& info = parser->scannerLineInfo[scanner->line-1];
      fprintf (stderr, "%s: ", info.location().c_str());
    }
  else
    {
      fprintf (stderr, "%s:%d: ", scanner->input_name, scanner->line);
    }
  if (is_error)
    fprintf (stderr, "error: ");
  fprintf (stderr, "%s\n", message);
}

/* --- preprocessing related functions --- */

static bool match(vector<char>::iterator start, const char *string)
{
  /* FIXME: can we exceed the bounds of the input vector? */
  while(*string && *start)
    if(*string++ != *start++) return false;

  return (*string == 0);
}

static bool fileExists(const string& filename)
{
  FILE *test = fopen(filename.c_str(),"r");
  if(test)
    {
      fclose(test);
      return true;
    }
  return false;
}

static void loadFile (FILE *file, vector<char>& v)
{
  char buffer[1024];
  long l;
  while (!feof (file) && (l = fread(buffer,1,1024,file)) > 0)
    v.insert(v.end(),buffer, buffer+l);
}

static void loadFile(const char *filename, vector<char>& v)
{
  if (strcmp (filename, "-") == 0) /* stdin */
    {
      loadFile (stdin, v);
    }
  else
    {
      FILE *file = fopen (filename,"r");

      if (!file)
	{
	  fprintf(stderr,"file '%s' not found\n",filename);
	  exit(1);
	}

      loadFile (file, v);
      fclose (file);
    }
}

bool Parser::haveIncluded (const string& filename) const
{
  vector<string>::const_iterator i;

  for(i = includes.begin();i != includes.end();i++)
    if(*i == filename) return true;

  return false;
}

static void collectImplIncludes (set<string>& result,
				 const string& root,
				 map<string, set<string> >& implIncludeMap)
{
  if (result.count (root) == 0)
    {
      result.insert (root);

      set<string>::const_iterator si;
      for (si = implIncludeMap[root].begin(); si != implIncludeMap[root].end(); si++)
	collectImplIncludes (result, *si, implIncludeMap);
    }
}

void Parser::preprocess (const string& filename, bool includeImpl)
{
  static stack<string> includeStack;
  static map<string, set<string> > implIncludeMap;

  // make a note whenever "file 1" implincludes "file 2"
  if (!includeStack.empty() && includeImpl)
    implIncludeMap[includeStack.top()].insert(filename);

  // do the actual preprocessing on the file
  if (!haveIncluded (filename))
    {
      includes.push_back (filename);

      includeStack.push (filename);
      preprocessContents (filename);
      includeStack.pop ();
    }

  // on the outer level compute which code to build
  if (includeStack.empty())
    {
      set<string> implIncludes;
      collectImplIncludes (implIncludes, filename, implIncludeMap);
      implIncludeMap.clear();

      vector<LineInfo>::iterator li;
      for (li = scannerLineInfo.begin(); li != scannerLineInfo.end(); li++)
	li->isInclude = (implIncludes.count (li->filename) == 0);
    }
}

void Parser::preprocessContents (const string& input_filename)
{
  string filename;
  bool includeImpl = false; // always initialized again, this just silences the compiler
  enum
    {
      lineStart, idlCode, commentC, commentCxx,
      filenameFind, filenameIn1, filenameIn2,
      inString, inStringEscape
    } state = lineStart;

  LineInfo linfo;
  linfo.line = 1;
  linfo.filename = input_filename;

  vector<char> input;
  loadFile (input_filename.c_str(), input);
  input.push_back('\n'); // line number counting assumes files end with a newline

  vector<char>::iterator i = input.begin();
  while(i != input.end())
    {
      if(state == commentCxx) // eat C++ style comments
	{
	  if(*i == '\n')
	    {
	      scannerInputData.push_back(*i); // keep line numbering
	      scannerLineInfo.push_back(linfo);
	      linfo.line++;
	      state = lineStart;
	    }
	  i++;
	}
      else if(state == commentC) // eat C style comments
	{
	  if(match(i,"*/")) // leave comment state?
	    {
	      state = idlCode;
	      i += 2;
	    }
	  else // skip comments
	    {
	      if(*i == '\n')
		{
		  scannerInputData.push_back(*i); // keep line numbering
		  scannerLineInfo.push_back(linfo);
		  linfo.line++;
		}
	      i++;
	    }
	}
      else if(state == inString)
	{
	  if (*i == '\"') state = idlCode;
	  if (*i == '\\') state = inStringEscape;
	  scannerInputData.push_back(*i++);
	}
      else if(state == inStringEscape)
	{
	  state = inString;
	  scannerInputData.push_back(*i++);
	}
      else if(match (i, "//"))	// C++ style comment start?
	{
	  state = commentCxx;
	  i += 2;
	}
      else if(match(i,"/*"))	// C style comment start?
	{
	  state = commentC;
	  i += 2;
	}
      else if(state == filenameFind)
	{
	  switch(*i++)
	    {
	    case ' ':	// skip whitespaces
	    case '\t':
	      break;

	    case '"':	state = filenameIn1;
			break;
	    case '<':	state = filenameIn2;
			break;
	    default:	cerr << "bad char after #include statement" << endl;
			g_assert_not_reached (); // error handling!
	    }
	}
      else if((state == filenameIn1 && *i == '"')
	   || (state == filenameIn2 && *i == '>'))
	{
	  string location;
	  // #include "/usr/include/foo.idl" (absolute path includes)
	  if (g_path_is_absolute (filename.c_str()))
	    {
	      if (fileExists (filename))
		location = filename;
	    }
	  else
	    {
	      // #include "foo.idl" => search in local directory (relative to input_file)
	      if (state == filenameIn1)
		{
		  gchar *dir = g_path_get_dirname (input_filename.c_str());
		  if (fileExists (dir + string(G_DIR_SEPARATOR_S) + filename))
		    location = filename;
		  g_free (dir);
		}
	      // all #include directives => search includepath with standard include dirs
	      if (location == "")
		{
		  vector<string>::const_iterator oi;
		  for(oi = options.includePath.begin(); oi != options.includePath.end(); oi++)
		    {
		      string testFilename = *oi + "/" + filename;
		      if (fileExists (testFilename))
			{
			  location = testFilename;
			  break;
			}
		    }
		}
	    }
	  if (location == "")
	    {
	      fprintf (stderr, "%s: ", linfo.location().c_str());
	      fprintf (stderr, "include file '%s' not found\n", filename.c_str());
	      exit(1);
	    }
	  preprocess (location, includeImpl);

	  state = idlCode;
	  i++;
	}
      else if(state == filenameIn1 || state == filenameIn2)
	{
	  filename += *i++;
	}
      else if(*i == '"') // string start?
	{
	  state = inString;
	  scannerInputData.push_back(*i++);
	}
      else if(state == lineStart) // check if we're on lineStart
	{
	  if(match(i,"#include-impl"))
	    {
	      i += 13;
	      state = filenameFind;
	      filename = "";
	      includeImpl = true;
	    }
	  else if(match(i,"#include"))
	    {
	      i += 8;
	      state = filenameFind;
	      filename = "";
	      includeImpl = false;
	    }
	  else
	    {
	      if(*i != ' ' && *i != '\t' && *i != '\n') state = idlCode;
	      if(*i == '\n')
		{
		  state = lineStart;	// newline handling
		  scannerLineInfo.push_back(linfo);
		  linfo.line++;
		}
	      scannerInputData.push_back(*i++);
	    }
	}
      else
	{
	  if(*i == '\n')
	    {
	      state = lineStart;	// newline handling
	      scannerLineInfo.push_back(linfo);
	      linfo.line++;
	    }
	  scannerInputData.push_back(*i++);
	}
    }
}

bool Parser::insideInclude () const
{
  int scanner_line = scanner->line - 1;
  g_return_val_if_fail (scanner_line >= 0 && scanner_line < (gint) scannerLineInfo.size(), false);

  return scannerLineInfo[scanner_line].isInclude;
}

/*
 * comparation class, used to sort classes in such an order that bases
 * classes appear before derived classes
 */
class ClassCompare {
public:
  const Parser& parser;
  map<string, int> inheritanceLevel;

  ClassCompare (const Parser& parser) : parser (parser)
  {
    vector<Class>::const_iterator ci;

    /*
     * precompute the inheritance levels, as the classes data structure within the
     * parser will be modified by sort() during sorting with ClassCompare ; besides,
     * it should be more efficient that way
     */
    for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      computeInheritanceLevel (*ci);
  }

  int computeInheritanceLevel (const Class& cdef)
  {
    int& level = inheritanceLevel[cdef.name];
    if (!level)
      {
	const Class *parent = (cdef.inherits == "") ? 0 : parser.findClass (cdef.inherits);
	if (parent)
	  level = computeInheritanceLevel (*parent) + 1;
	else
	  level = 1;
      }
    return level;
  }

  /* return true if c1 is to be placed before c2 */
  bool operator()(const Class& c1, const Class& c2)
  {
    int l1 = inheritanceLevel[c1.name];
    int l2 = inheritanceLevel[c2.name];
    if (l1 < l2) return true;
    if (l1 > l2) return false;
    return c1.name < c2.name;
  }
};

/* --- parsing functions --- */

bool Parser::parse (const string& filename)
{
  /* preprocess (throws includes into contents, removes C-style comments) */
  preprocess (filename);
  g_scanner_input_text (scanner, &scannerInputData[0], scannerInputData.size());
  /* provide abosulte input file name for fileName() */
  if (g_path_is_absolute (filename.c_str()))
    scanner->input_name = g_strdup (filename.c_str());
  else if (filename == "-") /* stdin */
    {
      scanner->input_name = "stdin";
    }
  else
    {
      gchar *dir = g_get_current_dir();
      scanner->input_name = g_strconcat (dir, G_DIR_SEPARATOR_S, filename.c_str(), NULL);
      g_free (dir);
    }
  /* FIXME: we leak scanner->input_name later on */

  currentNamespace = &rootNamespace;

  /* define primitive types into the basic namespace */
  enterNamespace ("Sfi");
  defineSymbol ("Bool");
  defineSymbol ("Int");
  defineSymbol ("Num");
  defineSymbol ("Real");
  defineSymbol ("String");
  defineSymbol ("BBlock");
  defineSymbol ("FBlock");
  defineSymbol ("Rec");
  leaveNamespace ();

  GTokenType expected_token = G_TOKEN_NONE;
  
  while (!g_scanner_eof (scanner) && expected_token == G_TOKEN_NONE)
    {
      g_scanner_get_next_token (scanner);
      
      if (scanner->token == G_TOKEN_EOF)
        break;
      else if (scanner->token == TOKEN_NAMESPACE)
        expected_token = parseNamespace ();
      else
        expected_token = G_TOKEN_EOF; /* '('; */
    }
  
  if (expected_token != G_TOKEN_NONE && expected_token != (GTokenType)TOKEN_ERROR)
    {
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL, NULL, TRUE);
      return false;
    }

  if (scanner->parse_errors > 0)
    return false;

  /* postprocessing of the data structures: */
  sort (classes.begin(), classes.end(), ClassCompare (*this));
  return true;
}

GTokenType Parser::parseNamespace()
{
  DEBUG("parse namespace\n");
  parse_or_return (G_TOKEN_IDENTIFIER);

  if (!enterNamespace (scanner->value.v_identifier))
    return GTokenType (TOKEN_ERROR);
  
  parse_or_return (G_TOKEN_LEFT_CURLY);

  bool ready = false;
  do
    {
      switch (g_scanner_peek_next_token (scanner))
	{
	  case TOKEN_CHOICE:
	    {
	      GTokenType expected_token = parseChoice ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case TOKEN_RECORD:
	    {
	      GTokenType expected_token = parseRecord ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case TOKEN_SEQUENCE:
	    {
	      GTokenType expected_token = parseSequence ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case TOKEN_CLASS:
	    {
	      GTokenType expected_token = parseClass ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case G_TOKEN_IDENTIFIER:
	    {
	      Method procedure;
	      GTokenType expected_token = parseMethod (procedure);
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;

	      procedure.name = defineSymbol (procedure.name.c_str());
	      procedure.file = fileName();
	      addProcedureTodo (procedure);
	    }
	    break;
	  case TOKEN_CONST:
	    {
	      GTokenType expected_token = parseConstant ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case TOKEN_CONST_IDENT:
	    {
	      GTokenType expected_token = parseConstant (true);
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  case TOKEN_USING:
	    {
	      parse_or_return (TOKEN_USING);
	      parse_or_return (TOKEN_NAMESPACE);
	      parse_or_return (G_TOKEN_IDENTIFIER);

	      if (!usingNamespace (scanner->value.v_identifier))
		return GTokenType (TOKEN_ERROR);

	      parse_or_return (';');
	    }
	    break;
	  case TOKEN_NAMESPACE:
	    {
	      parse_or_return (TOKEN_NAMESPACE);
	      GTokenType expected_token = parseNamespace ();
	      if (expected_token != G_TOKEN_NONE)
		return expected_token;
	    }
	    break;
	  default:
	    ready = true;
	}
    }
  while (!ready);
  
  parse_or_return (G_TOKEN_RIGHT_CURLY);

  /* semicolon after namespaces is optional (like in C++) */
  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    parse_or_return (';');
  
  leaveNamespace();
  
  return G_TOKEN_NONE;
}

GTokenType Parser::parseTypeName (string& type)
{
  parse_or_return (G_TOKEN_IDENTIFIER);
  type = scanner->value.v_identifier;
  while (g_scanner_peek_next_token (scanner) == GTokenType(':'))
    {
      parse_or_return (':');
      parse_or_return (':');
      type += "::";

      parse_or_return (G_TOKEN_IDENTIFIER);
      type += scanner->value.v_identifier;
    }

  string qtype = qualifySymbol (type.c_str());

  if (qtype == "")
    printError ("can't find prior definition for type '%s'", type.c_str());
  else
    type = qtype;

  return G_TOKEN_NONE;
}

GTokenType Parser::parseStringOrConst (string &s)
{
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      GTokenType expected_token = parseTypeName (s);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;

      for(vector<Constant>::iterator ci = constants.begin(); ci != constants.end(); ci++)
	{
	  if (ci->name == s &&
              ci->type != Constant::tIdent)     /* identifier constants can't be expanded to strings */
	    {
	      char *x = 0;
	      switch (ci->type)
		{
		  case Constant::tInt:
		    s = x = g_strdup_printf ("%lldLL", ci->i);
		    g_free (x);
		    break;
		  case Constant::tFloat:
		    s = x = g_strdup_printf ("%.17g", ci->f);
		    g_free (x);
		    break;
		  case Constant::tString:
		    s = ci->str;
		    break;
		  default:
		    g_assert_not_reached ();
		    break;
		}
	      return G_TOKEN_NONE;
	    }
	}
      printError("undeclared constant %s used", s.c_str());
    }

  parse_or_return (G_TOKEN_STRING);
  s = scanner->value.v_string;
  return G_TOKEN_NONE;
}

GTokenType Parser::parseConstant (bool isident)
{
  /*
   * constant BAR = 3;
   */
  Constant cdef;

  if (isident)
    parse_or_return (TOKEN_CONST_IDENT);
  else
    parse_or_return (TOKEN_CONST);
  parse_or_return (G_TOKEN_IDENTIFIER);
  cdef.name = defineSymbol (scanner->value.v_identifier);
  cdef.file = fileName();
  
  parse_or_return ('=');

  /* handle ConstIdent */
  if (isident)
    {
      parse_or_return (G_TOKEN_IDENTIFIER);
      cdef.str = scanner->value.v_identifier;
      cdef.type = Constant::tIdent;
      parse_or_return (';');
      addConstantTodo (cdef);
      return G_TOKEN_NONE;
    }

  GTokenType t = g_scanner_peek_next_token (scanner);

  /* allow positive/negative prefixing */
  bool negate = FALSE;
  while (t == '+' || t == '-')
    {
      t = g_scanner_get_next_token (scanner);
      negate = negate ^ (t == '-');
      t = g_scanner_peek_next_token (scanner);
    }

  if (t == G_TOKEN_INT)
  {
    parse_or_return (G_TOKEN_INT);

    cdef.type = Constant::tInt;
    cdef.i = scanner->value.v_int64;
    if (negate)
      cdef.i = -cdef.i;
  }
  else if (t == G_TOKEN_FLOAT)
  {
    parse_or_return (G_TOKEN_FLOAT);

    cdef.type = Constant::tFloat;
    cdef.f = scanner->value.v_float;
    if (negate)
      cdef.f = -cdef.f;
  }
  else if (!negate)
  {
    parse_string_or_return (cdef.str);

    cdef.type = Constant::tString;
  }
  else
  {
    return G_TOKEN_FLOAT;
  }
  parse_or_return (';');

  addConstantTodo (cdef);
  return G_TOKEN_NONE;
}

GTokenType
Parser::parseChoice ()
{
  Choice choice;
  int value = 0, sequentialValue = 1;
  DEBUG("parse choice\n");
  
  parse_or_return (TOKEN_CHOICE);
  parse_or_return (G_TOKEN_IDENTIFIER);
  choice.name = defineSymbol (scanner->value.v_identifier);
  choice.file = fileName();
  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    {
      parse_or_return (';');
      addPrototype (choice.name, tdChoice);
      return G_TOKEN_NONE;
    }
  parse_or_return (G_TOKEN_LEFT_CURLY);
  while (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      ChoiceValue comp;
      
      GTokenType expected_token = parseChoiceValue (comp, value, sequentialValue);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      
      choice.contents.push_back(comp);
    }
  parse_or_return (G_TOKEN_RIGHT_CURLY);
  parse_or_return (';');
  
  addChoiceTodo (choice);
  return G_TOKEN_NONE;
}

void
skip_ascii_at (GScanner *scanner)
{
  if (g_scanner_peek_next_token (scanner) == '@')
    g_scanner_get_next_token (scanner);
}

#define to_lower(c)                             ( \
        (guchar) (                                                      \
          ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |  \
          ( (((guchar)(c))>=192 && ((guchar)(c))<=214) * (224-192) ) |  \
          ( (((guchar)(c))>=216 && ((guchar)(c))<=222) * (248-216) ) |  \
          ((guchar)(c))                                                 \
        )                                                               \
)


GTokenType
Parser::parseChoiceValue (ChoiceValue& comp, int& value, int& sequentialValue)
{
  parse_or_return (G_TOKEN_IDENTIFIER);
  comp.name = defineSymbol(scanner->value.v_identifier);
  comp.file = fileName();
  comp.neutral = false;
  string str;
  comp.name.c_str(); // don't remove this
  for (guint i = 0; i < comp.name.size(); i++)
    if (comp.name[i] != ':' || comp.name[i + 1] != ':')
      str += comp.name[i] == ':' ? '_' : to_lower(comp.name[i]);
  comp.label = g_type_name_to_sname (str.c_str());
  
  /*
    YES,
    YES = 1,
    YES = "Yes",
    YES = Neutral,
    YES = (1),
    YES = (1, "Yes"),
    YES = (Neutral, "Yes"),
    YES = (1, "Yes", "this is the Yes value"),
    YES = ("Yes", "this is the Yes value"),
  */
  if (g_scanner_peek_next_token (scanner) == GTokenType('='))
    {
      parse_or_return ('=');
      if (g_scanner_peek_next_token (scanner) == GTokenType('('))
        {
          bool need_arg = true;
          parse_or_return ('(');
          if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
            {
              parse_or_return (G_TOKEN_INT);
              value = scanner->value.v_int64;
              if (g_scanner_peek_next_token (scanner) == ',')
                parse_or_return (',');
              else
                need_arg = false;
            }
          else if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
                   strcmp (scanner->next_value.v_string, "Neutral") == 0)
            {
              parse_or_return (G_TOKEN_IDENTIFIER);
              comp.neutral = true;
              if (g_scanner_peek_next_token (scanner) == ',')
                parse_or_return (',');
              else
                need_arg = false;
            }
          if (need_arg)
            {
              parse_istring_or_return (comp.label);
              if (g_scanner_peek_next_token (scanner) == ',')
                parse_or_return (',');
              else
                need_arg = false;
            }
          if (need_arg)
            {
              parse_istring_or_return (comp.blurb);
              if (g_scanner_peek_next_token (scanner) == ',')
                parse_or_return (',');
              else
                need_arg = false;
            }
          parse_or_return (')');
	}
      else if (g_scanner_peek_next_token (scanner) == G_TOKEN_INT)
        {
          parse_or_return (G_TOKEN_INT);
          value = scanner->value.v_int64;
        }
      else if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
               strcmp (scanner->next_value.v_string, "Neutral") == 0)
        {
          parse_or_return (G_TOKEN_IDENTIFIER);
          comp.neutral = true;
        }
      else if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER &&
               strcmp (scanner->next_value.v_string, "_") == 0)
        {
          parse_istring_or_return (comp.label);
        }
      else if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
        {
          parse_istring_or_return (comp.label);
        }
      else
        return G_TOKEN_INT;
    }

  comp.value = value++;
  if (comp.neutral)
    comp.sequentialValue = 0;
  else
    comp.sequentialValue = sequentialValue++;
  
  if (g_scanner_peek_next_token (scanner) == GTokenType(','))
    parse_or_return (',');
  else
    peek_or_return ('}');
  
  return G_TOKEN_NONE;
}

GTokenType Parser::parseRecord ()
{
  Record record;
  DEBUG("parse record\n");
  
  parse_or_return (TOKEN_RECORD);
  parse_or_return (G_TOKEN_IDENTIFIER);
  record.name = defineSymbol (scanner->value.v_identifier);
  record.file = fileName();
  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    {
      parse_or_return (';');
      addPrototype (record.name, tdRecord);
      return G_TOKEN_NONE;
    }
  parse_or_return (G_TOKEN_LEFT_CURLY);
  
  bool ready = false;
  while (!ready)
    {
      GTokenType expected_token;
      switch (g_scanner_peek_next_token (scanner))
        {
        case G_TOKEN_IDENTIFIER:
          {
            Param def;
            
            expected_token = parseRecordField (def, "");
            if (expected_token != G_TOKEN_NONE)
              return expected_token;
            
            if (def.type != "")
              record.contents.push_back(def);
          }
          break;
        case TOKEN_GROUP:
          {
            IString group;
            parse_or_return (TOKEN_GROUP);
	    parse_istring_or_return (group);
            parse_or_return ('{');
            while (g_scanner_peek_next_token (scanner) != '}' && !g_scanner_eof (scanner))
              {
                Param property;
                expected_token = parseRecordField (property, group);
                if (expected_token != G_TOKEN_NONE)
                  return expected_token;
                record.contents.push_back (property);
              }
            parse_or_return ('}');
            parse_or_return (';');
          }
          break;
        case TOKEN_INFO:
          expected_token = parseInfoOptional (record.infos);
          if (expected_token != G_TOKEN_NONE)
            return expected_token;
          break;
        default:
          expected_token = g_scanner_peek_next_token (scanner);
          ready = true;
          break;
        }
    }
  parse_or_return (G_TOKEN_RIGHT_CURLY);
  parse_or_return (';');
  
  addRecordTodo (record);
  return G_TOKEN_NONE;
}

GTokenType Parser::parseRecordField (Param& def, const IString& group)
{
  /* FooVolumeType volume_type; */
  /* float         volume_perc @= ("Volume[%]", "Set how loud something is",
     50, 0.0, 100.0, 5.0,
     ":dial:readwrite"); */
  
  GTokenType expected_token = parseTypeName (def.type);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  def.pspec = NamespaceHelper::nameOf (def.type); // FIXME: correct?
  def.group = group;
  def.line = scanner->line;
  
  parse_or_return (G_TOKEN_IDENTIFIER);
  def.name = scanner->value.v_identifier;
  def.file = fileName();
  
  /* the hints are optional */
  skip_ascii_at (scanner);
  if (g_scanner_peek_next_token (scanner) == '=')
    {
      g_scanner_get_next_token (scanner);
      
      GTokenType expected_token = parseParamHints (def);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
    }
  
  parse_or_return (';');
  return G_TOKEN_NONE;
}

GTokenType
Parser::parseStream (Stream&      stream,
                     Stream::Type stype)
{
  /* OStream wave_out @= ("Audio Out", "Wave Output"); */

  stream.type = stype;
  stream.line = scanner->line;

  parse_or_return (G_TOKEN_IDENTIFIER);
  stream.ident = scanner->value.v_identifier;

  skip_ascii_at (scanner);
  parse_or_return ('=');

  parse_or_return ('(');
  parse_istring_or_return (stream.label);
  parse_or_return (',');
  parse_istring_or_return (stream.blurb);
  parse_or_return (')');

  parse_or_return (';');
  return G_TOKEN_NONE;
}

/* like g_scanner_get_token, but accepts ':' within identifiers */
GTokenType scanner_get_next_token_with_colon_identifiers (GScanner *scanner)
{
  char *cset_identifier_first_orig = scanner->config->cset_identifier_first;
  char *cset_identifier_nth_orig   = scanner->config->cset_identifier_nth;

  string identifier_first_with_colon = cset_identifier_first_orig + string (":");
  string identifier_nth_with_colon = cset_identifier_nth_orig + string (":");

  scanner->config->cset_identifier_first = const_cast<char *>(identifier_first_with_colon.c_str());
  scanner->config->cset_identifier_nth   = const_cast<char *>(identifier_nth_with_colon.c_str());

  GTokenType token = g_scanner_get_next_token (scanner);

  scanner->config->cset_identifier_first = cset_identifier_first_orig;
  scanner->config->cset_identifier_nth   = cset_identifier_nth_orig;

  return token;
}

/* returns true for C++ style identifiers (Foo::BAR) - only the colons are checked, not individual chars */
bool isCxxTypeName (const string& str)
{
  enum { valid, colon1, colon2, invalid } state = valid;

  for (string::const_iterator i = str.begin(); i != str.end(); i++)
    {
      if (state == valid)
	{
	  if (*i == ':')
	    state = colon1;
	}
      else if (state == colon1)
	{
	  if (*i == ':')
	    state = colon2;
	  else
	    state = invalid;
	}
      else if (state == colon2)
	{
	  if (*i == ':')
	    state = invalid;
	  else
	    state = valid;
	}
    }
  return (state == valid) && (str.size() != 0);
}

static bool
makeLiteralOptions (const string& options, string& literal_options)
{
  bool failed = false;
  bool in_string = false;

  /*
   * this does handle
   *
   *    \":r\"	      => :r
   *	\":r\" \":w\" => :r:w
   *	(\":r\"
   *	 \":w\")      => :r:w
   *
   * this does not handle
   *
   *    \":r\" _(\":i18n-hint\") \":w\"
   *	\":r\" C_DEFINED_IDENTIFIER \":w\"
   *	g_strdup (\":r:w\")
   *	\":r\" /+ (imagine this would be a C-style-comment with real asterisks) +/ \":w\"
   *
   * however, the last case should not occur, as options have already been GScanner scanned
   */
  for (string::const_iterator oi = options.begin(); oi != options.end(); oi++)
    {
      bool last_was_backslash = false;
      if (*oi == '\\')
	{
	  last_was_backslash = true;

	  if ((oi + 1) != options.end())      // unescape stuff
	    oi++;
	  else
	    failed = true;		      // option string ends with a single backslash? huh?
	}
      switch (*oi)
	{
	case '"':   if (!last_was_backslash)  // keep track of whether we are inside a string or not
		      in_string = !in_string;
		    else if (in_string)	      // if we got an \" inside a string, replace it with the real thing
		      literal_options += *oi;
		    else		      // we got an \" outside a string? why?
		      failed = true;
		    break;
	case '(':			      // somewhat lazy way of handling string concatenation:
	case ')':			      // will match (C compiler will check this anyways)
	case ' ':			      // so we just ignore whitespace and paranthesis outside strings
	case '\t':
	case '\n':  if (in_string)
		      literal_options += *oi; // inside strings, treat them as real
		    break;
	default:    if (in_string)	      // inside strings, just collect all characters that we see
		      literal_options += *oi;
		    else
		      failed = true;	      // outside strings, we shouldn't see things
		    break;
	}
    }

  if (failed)
    literal_options = ""; // what we've got so far might be unusable

  return !failed;
}

GTokenType Parser::parseParamHints (Param &def)
{
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      parse_or_return (G_TOKEN_IDENTIFIER);
      def.pspec = scanner->value.v_identifier;
    }

  parse_or_return ('(');

  int bracelevel = 1;
  string args;
  string current_arg;
  int arg_count = 0;
  while (!g_scanner_eof (scanner) && bracelevel > 0)
    {
      GTokenType t = scanner_get_next_token_with_colon_identifiers (scanner);
      gchar *token_as_string = 0, *x = 0;
      bool current_arg_complete = false;

      if(int(t) > 0 && int(t) <= 255)
	{
	  token_as_string = g_new0 (char, 2); /* FIXME: leak */
	  token_as_string[0] = char(t);
	}
      switch (t)
	{
	case '(':		  bracelevel++;
	  break;
	case ')':		  bracelevel--;
				  if (bracelevel == 0)
				    current_arg_complete = true;
	  break;
	case ',':		  current_arg_complete = true;
	  break;
	case G_TOKEN_STRING:	  x = g_strescape (scanner->value.v_string, 0);
				  token_as_string = g_strdup_printf ("\"%s\"", x);
				  g_free (x);
	  break;
	case G_TOKEN_INT:	  token_as_string = g_strdup_printf ("%llu", scanner->value.v_int64);
	  break;
	case G_TOKEN_FLOAT:	  token_as_string = g_strdup_printf ("%.17g", scanner->value.v_float);
	  break;
	case G_TOKEN_IDENTIFIER:
          {
	    vector<Constant>::iterator ci = constants.end();
	    if (isCxxTypeName (scanner->value.v_identifier))
	      {
		string coname = qualifySymbol (scanner->value.v_identifier);

		/* FIXME: there should be a generic const_to_string() function */
		for (ci = constants.begin(); ci != constants.end(); ci++)
		  if (ci->name == coname)
		    break;
	      }

            if (ci == constants.end())
              token_as_string = g_strdup_printf ("%s", scanner->value.v_identifier);
            else switch (ci->type)
              {
              case Constant::tInt:
                token_as_string = g_strdup_printf ("%lldLL", ci->i);
                break;
              case Constant::tFloat:
                token_as_string = g_strdup_printf ("%.17g", ci->f);
                break;
              case Constant::tIdent:
                token_as_string = g_strdup (ci->str.c_str());
                break;
              case Constant::tString:
                x = g_strescape (ci->str.c_str(), 0);
                token_as_string = g_strdup_printf ("\"%s\"", x);
                g_free (x);
                break;
              default:
                g_assert_not_reached ();
                break;
              }
          }
	  break;
	default:
	  if (!token_as_string)
	    return GTokenType (')');
	}
      if (current_arg_complete)
	{
	  switch (arg_count++)
	    {
	    case 0:   def.label = current_arg;	 /* first pspec constructor argument */
		      break;
	    case 1:   def.blurb = current_arg;	 /* second pspec constructor argument */
		      break;
	    default:  def.options = current_arg; /* last pspec constructor argument */
	    }
	  current_arg = "";
	}
      else if (token_as_string)
	{
	  current_arg += token_as_string;
	}
      if (token_as_string && bracelevel)
	{
	  if (args != "")
	    args += ' ';
	  args += token_as_string;
	  g_free (token_as_string);
	}
    }
  def.args = args;
  if (!makeLiteralOptions (def.options, def.literal_options))
    printWarning ("can't parse option string: %s\n", def.options.c_str());
  return G_TOKEN_NONE;
}

GTokenType Parser::parseInfoOptional (Map<string,IString>& infos)
{
  /*
   * info HELP = "don't panic";
   * info FOO = "bar";
   */
  while (g_scanner_peek_next_token (scanner) == TOKEN_INFO)
  {
    string  key;
    IString value;

    parse_or_return (TOKEN_INFO);
    parse_or_return (G_TOKEN_IDENTIFIER);
    key = scanner->value.v_identifier;
    parse_or_return ('=');
    parse_istring_or_return (value);
    parse_or_return (';');

    for (guint i = 0; i < key.size(); i++)
      key[i] = to_lower (key[i]);
    infos[key] = value;
  }
  return G_TOKEN_NONE;
}

GTokenType Parser::parseSequence ()
{
  GTokenType expected_token;
  Sequence sequence;

  /*
   * sequence IntSeq {
   *   Int ints @= (...);
   * };
   */
  
  parse_or_return (TOKEN_SEQUENCE);
  parse_or_return (G_TOKEN_IDENTIFIER);
  sequence.name = defineSymbol (scanner->value.v_identifier);
  sequence.file = fileName();
  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    {
      parse_or_return (';');
      addPrototype (sequence.name, tdSequence);
      return G_TOKEN_NONE;
    }
  parse_or_return ('{');

  expected_token = parseInfoOptional (sequence.infos);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  expected_token = parseRecordField (sequence.content, "");
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  expected_token = parseInfoOptional (sequence.infos);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;

  parse_or_return ('}');
  parse_or_return (';');
  
  addSequenceTodo (sequence);
  return G_TOKEN_NONE;
}

GTokenType Parser::parseClass ()
{
  Class cdef;
  DEBUG("parse class\n");
  
  parse_or_return (TOKEN_CLASS);
  parse_or_return (G_TOKEN_IDENTIFIER);
  cdef.name = defineSymbol (scanner->value.v_identifier);
  cdef.file = fileName();
  
  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    {
      parse_or_return (';');
      addPrototype (cdef.name, tdClass);
      return G_TOKEN_NONE;
    }
  if (g_scanner_peek_next_token (scanner) == GTokenType(':'))
    {
      parse_or_return (':');
      
      GTokenType expected_token = parseTypeName (cdef.inherits);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
    }
  
  parse_or_return ('{');
  while (g_scanner_peek_next_token (scanner) != G_TOKEN_RIGHT_CURLY)
    {
      GTokenType expected_token;
      switch (g_scanner_peek_next_token (scanner))
        {
	case G_TOKEN_IDENTIFIER:
	  {
	    Method method;
	    GTokenType expected_token = parseMethod (method);
	    if (expected_token != G_TOKEN_NONE)
	      return expected_token;
            
	    if (method.result.type == "signal")
	      cdef.signals.push_back(method);
	    else
	      cdef.methods.push_back(method);
	  }
	  break;
	case TOKEN_INFO:
	  {
	    GTokenType expected_token = parseInfoOptional (cdef.infos);
	    if (expected_token != G_TOKEN_NONE)
	      return expected_token;
	  }
	  break;
        case TOKEN_PROPERTY: /* FIXME: remove me due to deprecated syntax */
	  {
	    parse_or_return (TOKEN_PROPERTY);

	    Param property;

	    expected_token = parseRecordField (property, "");  // no i18n support, deprecated
	    if (expected_token != G_TOKEN_NONE)
	      return expected_token;
            
	    cdef.properties.push_back (property);
	  }
	  break;
        case TOKEN_GROUP:
          {
            IString group;
	    parse_or_return (TOKEN_GROUP);
	    parse_istring_or_return (group);
	    parse_or_return ('{');
            while (g_scanner_peek_next_token (scanner) != '}' && !g_scanner_eof (scanner))
              {
                Param property;
                expected_token = parseRecordField (property, group);
                if (expected_token != G_TOKEN_NONE)
                  return expected_token;
                cdef.properties.push_back (property);
              }
            parse_or_return ('}');
            parse_or_return (';');
	  }
	  break;
        case TOKEN_ISTREAM:
        case TOKEN_JSTREAM:
        case TOKEN_OSTREAM:
          {
            Stream::Type stype = Stream::IStream;
            switch ((int) scanner->next_token) {
            case TOKEN_JSTREAM:   stype = Stream::JStream; break;
            case TOKEN_OSTREAM:   stype = Stream::OStream; break;
            }
            g_scanner_get_next_token (scanner); /* eat *Stream */
            Stream stream;
            GTokenType expected_token = parseStream (stream, stype);
            if (expected_token != G_TOKEN_NONE)
              return expected_token;
            
            switch (stream.type) {
            case Stream::IStream: cdef.istreams.push_back (stream); break;
            case Stream::JStream: cdef.jstreams.push_back (stream); break;
            case Stream::OStream: cdef.ostreams.push_back (stream); break;
            }
          }
          break;
	default:
	  parse_or_return (G_TOKEN_IDENTIFIER); /* will fail; */
        }
    }
  parse_or_return ('}');
  parse_or_return (';');
  
  addClassTodo (cdef);
  return G_TOKEN_NONE;
}

GTokenType Parser::parseMethod (Method& method)
{
  peek_or_return (G_TOKEN_IDENTIFIER);
  if (strcmp (scanner->next_value.v_identifier, "signal") == 0)
  {
    parse_or_return (G_TOKEN_IDENTIFIER);
    method.result.type = "signal";
  }
  else if (strcmp (scanner->next_value.v_identifier, "void") == 0)
  {
    parse_or_return (G_TOKEN_IDENTIFIER);
    method.result.type = "void";
  }
  else
    {
      GTokenType expected_token = parseTypeName (method.result.type);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;

      method.result.name = "result";
      method.result.file = fileName();
    }

  method.result.pspec = NamespaceHelper::nameOf (method.result.type); // FIXME: correct?

  parse_or_return (G_TOKEN_IDENTIFIER);
  method.name = scanner->value.v_identifier;
  method.file = fileName();
  
  parse_or_return ('(');
  while (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      Param def;

      GTokenType expected_token = parseTypeName (def.type);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;

      def.pspec = NamespaceHelper::nameOf (def.type); // FIXME: correct?
  
      parse_or_return (G_TOKEN_IDENTIFIER);
      def.name = scanner->value.v_identifier;
      def.file = fileName();
      method.params.push_back(def);

      if (g_scanner_peek_next_token (scanner) != GTokenType(')'))
	{
	  parse_or_return (',');
	  peek_or_return (G_TOKEN_IDENTIFIER);
	}
    }
  parse_or_return (')');

  if (g_scanner_peek_next_token (scanner) == GTokenType(';'))
    {
      parse_or_return (';');
      return G_TOKEN_NONE;
    }

  parse_or_return ('{');
  while (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER ||
         g_scanner_peek_next_token (scanner) == TOKEN_INFO)
    {
      if (g_scanner_peek_next_token (scanner) == TOKEN_INFO)
	{
	  GTokenType expected_token = parseInfoOptional (method.infos);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	}
      else
	{
	  Param *pd = 0;

	  parse_or_return (G_TOKEN_IDENTIFIER);
	  string inout = scanner->value.v_identifier;

	  if (inout == "Out")
	  {
	    parse_or_return (G_TOKEN_IDENTIFIER);
	    method.result.name = scanner->value.v_identifier;
            method.result.file = fileName();
	    pd = &method.result;
	  }
	  else if(inout == "In")
	  {
	    parse_or_return (G_TOKEN_IDENTIFIER);
	    for (vector<Param>::iterator pi = method.params.begin(); pi != method.params.end(); pi++)
	    {
	      if (pi->name == scanner->value.v_identifier)
		pd = &*pi;
	    }
	  }
	  else
	  {
	    printError("In or Out expected in method/procedure details");
	    return G_TOKEN_IDENTIFIER;
	  }

	  if (!pd)
	  {
	    printError("can't associate method/procedure parameter details");
	    return G_TOKEN_IDENTIFIER;
	  }

	  pd->line = scanner->line;

          skip_ascii_at (scanner);
	  parse_or_return ('=');

	  GTokenType expected_token = parseParamHints (*pd);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;

	  parse_or_return (';');
	}
    }
  parse_or_return ('}');

  return G_TOKEN_NONE;
}

void Parser::addConstantTodo(const Constant& constant)
{
  constants.push_back(constant);
  
  if (insideInclude ())
    {
      includedNames.push_back (constant.name);
    }
  else
    {
      types.push_back (constant.name);
    }
}

void Parser::addChoiceTodo(const Choice& choice)
{
  choices.push_back(choice);
  
  if (insideInclude ())
    {
      includedNames.push_back (choice.name);
    }
  else
    {
      types.push_back (choice.name);
    }
  addType (choice.name, tdChoice);
}

void Parser::addRecordTodo(const Record& record)
{
  records.push_back(record);
  
  if (insideInclude ())
    {
      includedNames.push_back (record.name);
    }
  else
    {
      types.push_back (record.name);
    }
  addType (record.name, tdRecord);
}

void Parser::addSequenceTodo(const Sequence& sequence)
{
  sequences.push_back(sequence);
  
  if (insideInclude ())
    {
      includedNames.push_back (sequence.name);
    }
  else
    {
      types.push_back (sequence.name);
    }
  addType (sequence.name, tdSequence);
}

void Parser::addClassTodo(const Class& cdef)
{
  classes.push_back(cdef);
  
  if (insideInclude ())
    {
      includedNames.push_back (cdef.name);
    }
  else
    {
      types.push_back (cdef.name);
    }
  addType (cdef.name, tdClass);
}

void Parser::addProcedureTodo(const Method& pdef)
{
  procedures.push_back(pdef);
  
  if (insideInclude ())
    {
      includedNames.push_back (pdef.name);
    }
  else
    {
      types.push_back (pdef.name);
    }
}

bool Parser::fromInclude(const string& type) const
{
  vector<string>::const_iterator ii;

  for (ii = includedNames.begin(); ii != includedNames.end(); ii++)
    if (*ii == type) return true;
  return false;
}

void Parser::addType (const std::string& type, TypeDeclaration typeDecl)
{
  int& m = typeMap[type];

  if (m == 0)
    {
      m = typeDecl;
    }
  else if (m == typeDecl)
    {
      printError ("double definition of '%s' as same type\n", type.c_str());
    }
  else if (m == (typeDecl | tdProto))
    {
      m = typeDecl;
    }
  else
    {
      printError ("double definition of '%s' as different types\n", type.c_str());
    }
}

void Parser::addPrototype (const std::string& type, TypeDeclaration typeDecl)
{
  int& m = typeMap[type];

  if (m == 0)
    {
      m = typeDecl | tdProto;
    }
  else if (m == typeDecl)
    {
      // prototype after full definition is okay
    }
  else if (m == (typeDecl | tdProto))
    {
      // double prototype is okay
    }
  else
    {
      printError ("double definition of '%s' as different types", type.c_str());
    }
}

string Parser::defineSymbol (const string& name)
{
  Symbol *sym = currentNamespace->find (name);
  if (!sym)
    {
      sym = new Symbol();
      sym->name = name;
      currentNamespace->insert (sym);
    }
  return sym->fullName();
}

list<string> symbolToList (const string& symbol)
{
  list<string> result;
  string current;

  g_return_val_if_fail (isCxxTypeName (symbol), result);
  
  for (string::const_iterator si = symbol.begin(); si != symbol.end(); si++)
    {
      if (*si != ':')
	{
	  current += *si;
	}
      else
	{
	  if (current != "")
	    result.push_back(current);
	  
	  current = "";
	}
    }
  
  result.push_back(current);
  return result;
}

Symbol *matchSymbol (const list<string>& nlist, Namespace *ns)
{
  Symbol *symbol = ns;
  for (list<string>::const_iterator i = nlist.begin(); i != nlist.end(); i++)
    {
      if (symbol)
	symbol = symbol->find (*i);
    }
  return symbol;
}

void appendUnique (list<Symbol *>& symbols, Symbol *sym)
{
  for (list<Symbol *>::iterator si = symbols.begin(); si != symbols.end(); si++)
    if (*si == sym)
      return;
  symbols.push_back (sym);
}

void qualifyHelper (const string& name, Namespace *ns,
                    list<Symbol *>& alternatives, set<Namespace *>& done)
{
  /* prevent searching the same namespace twice */
  if (done.count (ns)) return;
  done.insert (ns);

  /* try to find the symbol in the current namespace */
  list<string> nlist = symbolToList (name);
  Symbol *symbol = matchSymbol (nlist, ns);
  if (symbol)
    appendUnique (alternatives, symbol);

  /* try to find the symbol in the parent namespace */
  Namespace *parent_ns = dynamic_cast<Namespace *> (ns->parent);
  if (parent_ns)
    qualifyHelper (name, parent_ns, alternatives, done);

  /* try to find the symbol in one of the namespaces used via "using namespace" */
  for (vector<Namespace *>::iterator ni = ns->used.begin(); ni != ns->used.end(); ni++)
    qualifyHelper (name, *ni, alternatives, done);
}

Symbol *Parser::qualifyHelper (const string& name)
{
  set<Namespace *> done;
  list<Symbol *> alternatives;

  ::qualifyHelper (name, currentNamespace, alternatives, done);

  /* no match? */
  if (alternatives.empty())
    return 0;

  /* good match? */
  if (alternatives.front()->parent == currentNamespace || alternatives.size() == 1)
    return alternatives.front();

  /* multiple equally valid candidates? */
  printError ("there are multiple valid interpretations of %s in this context", name.c_str());
  for (list<Symbol *>::iterator ai = alternatives.begin(); ai != alternatives.end(); ai++)
    printError (" - it could be %s", (*ai)->fullName().c_str());

  return 0;
}

string Parser::qualifySymbol (const string& name)
{
  Symbol *sym = qualifyHelper (name);
  if (sym)
    return sym->fullName();
  else
    return "";
}

bool Parser::enterNamespace (const string& name)
{
  Symbol *symbol = currentNamespace->find (name);
  if (symbol)
    {
      currentNamespace = dynamic_cast <Namespace *> (symbol);
      if (!currentNamespace)
	{
	  printError ("%s is not a namespace", name.c_str());
	  return false;
	}
    }
  else
    {
      symbol = new Namespace;
      symbol->name = name;
      currentNamespace->insert (symbol);
      currentNamespace = dynamic_cast<Namespace*>(symbol);
    }
  return true;
}

void Parser::leaveNamespace ()
{
  currentNamespace = dynamic_cast<Namespace *>(currentNamespace->parent);
  if (!currentNamespace)
    g_error ("fatal: leaveNamespace called without corresponding enterNamespace");
}

bool Parser::usingNamespace (const string& name)
{
  Symbol *sym = qualifyHelper (name);
  if (!sym)
    {
      printError ("%s is an undeclared namespace (can't be used)", name.c_str());
      return false;
    }

  Namespace *ns = dynamic_cast<Namespace *> (sym);
  if (!ns)
    {
      printError ("%s is not a namespace (can't be used)", name.c_str());
      return false;
    }
  currentNamespace->used.push_back (ns);
  return true;
}

/*
 * the beginnings of an alternate datastructure - currently only used for namespace
 * lookups
 */

string Symbol::fullName ()
{
  if (parent && parent->fullName() != "")
    return parent->fullName() + "::" + name;
  else
    return name;
}

Symbol *Symbol::find (const string& name)
{
  for (vector<Symbol *>::iterator ci = children.begin(); ci != children.end(); ci++)
    {
      if ((*ci)->name == name)
	return (*ci);
    }

  return 0;
}

bool Symbol::insert (Symbol *symbol)
{
  if (find (symbol->name))
    return false;

  children.push_back (symbol);
  symbol->parent = this;

  return true;
}

Symbol::Symbol() : parent (0)
{
}

Symbol::~Symbol()
{
  for (vector<Symbol *>::iterator ci = children.begin(); ci != children.end(); ci++)
    delete *ci;
}

}; // anon namespace

/* vim:set ts=8 sts=2 sw=2: */
