/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2007 Stefan Westerfeld
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

#ifndef _SFIDL_PARSER_H_
#define _SFIDL_PARSER_H_

#include <map>
#include "sfidl-utils.hh"

namespace Sfidl {

/* we implement a get() function since operator[] is not const */
template<typename Key, typename Value>
class Map : public std::map<Key,Value> {
private:
  Value default_value;

public:
  const Value& get(const Key& k) const {
    typename std::map<Key,Value>::const_iterator i = this->find(k);
    if (i != this->end())
      return i->second;
    else
      return default_value;
  }
};

/*
 * internationalized string: can store a conventional string,
 * however it can also store whether the string was given in
 * conventional form    "foo"     (i18n = false)
 * on in the i18n form  _("foo")  (i18n = true)
 */
class IString : public String {
public:
  bool i18n;

  IString() : i18n (false) {
  }
  
  IString(const char *str) : String (str), i18n (false) {
  }

  /* produces an escaped version "foo" or _("foo") */
  String escaped (const String &i18n_prefix = "_") const
  {
    String result;
    char *x = g_strescape (c_str(), 0);
    if (i18n)
      result = i18n_prefix + "(\"" + String (x) + "\")";
    else
      result = "\"" + String(x) + "\"";
    g_free (x);
    return result;
  }
};

struct LineInfo {
  bool	  isInclude;
  int	  line;
  String  filename;

  // Produce a human readable location (file:line, using "stdin" where appropriate) 
  String location() const
  {
    String result;
    char *x = g_strdup_printf ("%s:%d", (filename == "-") ? "stdin" : filename.c_str(), line);
    result = x;
    g_free (x);
    return result;
  }
};

struct Pragma {
  String  filename;
  String  text;
  int	  line;
  bool	  fromInclude; /* true for normal includes; false for implIncludes and the base file */

  bool getString (const String& key, String& value);
};

struct Constant {
  String  name;
  String  file;
  enum { tString = 1, tFloat = 2, tInt = 3, tIdent = 4 } type;

  String  str;
  float	  f;
  int64	  i;
};

struct Param {
  String  type;
  String  name;
  String  file;
  
  IString group;
  String  pspec;
  int     line;
  String  args;

  String  label; /* first argument of the param spec contructor */
  String  blurb; /* second argument of the param spec contructor */
  String  options; /* last argument of the param spec contructor */
  String  literal_options; /* the real option string; note that conversion might not work,
				  if building the literal option string requires things like C function calls */
};

struct Stream {
  enum Type { IStream, JStream, OStream } type;
  String  ident;
  IString label;
  IString blurb;
  String  file;
  int     line;
};
 
struct ChoiceValue {
  String  name;
  String  file;
  IString label;
  IString blurb;
  
  int     value;
  int     sequentialValue;
  bool    neutral;
};

struct Choice {
  /*
   * name if the enum, "_anonymous_" for anonymous enum - of course, when
   * using namespaces, this can also lead to things like "Arts::_anonymous_",
   * which would mean an anonymous enum in the Arts namespace
   */
  String name;
  String file;
  
  std::vector<ChoiceValue> contents;
  Map<String, IString> infos;
};

struct Record {
  String name;
  String file;
  
  std::vector<Param> contents;
  Map<String, IString> infos;
};

struct Sequence {
  String  name;
  String  file;
  Param	  content;
  Map<String, IString> infos;
};

struct Method {
  String  name;
  String  file;
  
  std::vector<Param> params;
  Param	  result;
  Map<String, IString> infos;
};

struct Class {
  String name;
  String file;
  String inherits;
  
  std::vector<Method>	methods;
  std::vector<Method>	signals;
  std::vector<Param>	properties;
  std::vector<Stream>	istreams, jstreams, ostreams;
  Map<String, IString>	infos;
};

enum TypeDeclaration {
  tdChoice        = 1,
  tdRecord        = 2,
  tdSequence      = 3,
  tdClass         = 4,
  tdProto         = 8,
  tdChoiceProto   = tdChoice | tdProto,
  tdRecordProto   = tdRecord | tdProto,
  tdSequenceProto = tdSequence | tdProto,
  tdClassProto    = tdClass | tdProto,
};

enum Type {
  // primitive types
  VOID,
  BOOL,
  INT,
  NUM,
  REAL,
  STRING,
  // enums
  CHOICE,
  // blocks of byte/float
  BBLOCK,
  FBLOCK,
  // generic record type:
  SFIREC,
  // user defined types
  SEQUENCE,
  RECORD,
  OBJECT,     /* PROXY */
};

class Symbol {
public:
  String name;

  Symbol *parent;
  std::vector<Symbol *> children;

  Symbol();
  virtual ~Symbol();

  String   fullName ();
  Symbol  *find (const String& name);
  bool     insert (Symbol *symbol);
};

class Namespace : public Symbol {
public:
  std::vector<Namespace *> used; /* from "using namespace Foo;" statements */
};

class Parser {
protected:
  const class Options&      options;

  GScanner                 *scanner;
  std::vector<char>         scannerInputData;
  std::vector<LineInfo>     scannerLineInfo;

  Namespace                 rootNamespace;
  Namespace                *currentNamespace;

  std::vector<String>	    includedNames;
  std::vector<String>	    types;
  std::map<String,int>	    typeMap;

  std::vector<String>	    includes;          // files to include
  std::vector<Pragma>	    pragmas;
  std::vector<Constant>	    constants;
  std::vector<Choice>	    choices;
  std::vector<Sequence>	    sequences;
  std::vector<Record>	    records;
  std::vector<Class>	    classes;
  std::vector<Method>	    procedures;

  // namespace related functions

  String defineSymbol (const String& name);
  Symbol *qualifyHelper (const String& name);
  String qualifySymbol (const String& name);
  bool enterNamespace (const String& name);
  void leaveNamespace ();
  bool usingNamespace (const String& name);

  // scanner related functions

  static void scannerMsgHandler (GScanner *scanner, gchar *message, gboolean is_error);
  void printError (const gchar *format, ...) G_GNUC_PRINTF (2, 3);
  void printWarning (const gchar *format, ...) G_GNUC_PRINTF (2, 3);

  // preprocessor

  void preprocess (const String& filename, bool includeImpl = false);
  void preprocessContents (const String& filename);
  bool haveIncluded (const String& filename) const;
  bool insideInclude () const;

  // parser

  void addConstantTodo (const Constant& cdef);
  void addChoiceTodo (const Choice& cdef);
  void addRecordTodo (const Record& rdef);
  void addSequenceTodo (const Sequence& sdef);
  void addClassTodo (const Class& cdef);
  void addProcedureTodo (const Method& pdef);

  void addPrototype (const String& type, TypeDeclaration typeDecl);
  void addType (const String& type, TypeDeclaration typeDecl);

  GTokenType parseTypeName (String& s);
  GTokenType parseStringOrConst (String &s);
  GTokenType parseConstant (bool isident = false);
  GTokenType parseNamespace ();
  GTokenType parseChoice ();
  GTokenType parseChoiceValue (ChoiceValue& comp, int& value, int& sequentialValue);
  GTokenType parseRecord ();
  GTokenType parseRecordField (Param& comp, const IString& group);
  GTokenType parseStream (Stream& stream, Stream::Type);
  GTokenType parseSequence ();
  GTokenType parseParamHints (Param &def);
  GTokenType parseClass ();
  GTokenType parseMethod (Method& def);
  GTokenType parseInfoOptional (Map<String,IString>& infos);
public:
  Parser ();
  
  bool parse (const String& fileName);
 
  String fileName() const				  { return scanner->input_name; }
  const std::vector<String>& getIncludes () const	  { return includes; }
  const std::vector<Constant>& getConstants () const	  { return constants; }
  const std::vector<Choice>& getChoices () const	  { return choices; }
  const std::vector<Sequence>& getSequences () const	  { return sequences; }
  const std::vector<Record>& getRecords () const	  { return records; }
  const std::vector<Class>& getClasses () const 	  { return classes; }
  const std::vector<Method>& getProcedures () const	  { return procedures; }
  const std::vector<String>& getTypes () const		  { return types; }
 
  std::vector<Pragma> getPragmas (const String& binding) const;

  Sequence findSequence (const String& name) const;
  Record findRecord (const String& name) const;
  const Class* findClass (const String &name) const;
  
  bool isChoice (const String& type) const;
  bool isSequence (const String& type) const;
  bool isRecord (const String& type) const;
  bool isClass (const String& type) const;
  Type typeOf (const String& type) const;
  bool fromInclude (const String& type) const;
};

}
#endif /* _SFIDL_PARSER_H_ */

/* vim:set ts=8 sts=2 sw=2: */
