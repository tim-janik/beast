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

#ifndef _SFIDL_PARSER_H_
#define _SFIDL_PARSER_H_

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <string>

namespace Sfidl {

struct LineInfo {
  bool isInclude;
  int line;
  std::string filename;
};

struct Constant {
  std::string name;
  enum { tString = 1, tFloat = 2, tInt = 3 } type;

  std::string str;
  float f;
  int i;
};

struct Param {
  std::string type;
  std::string name;
 
  std::string group;
  std::string pspec;
  int         line;
  std::string args;
};

struct Stream {
  enum Type { IStream, JStream, OStream } type;
  std::string ident;
  std::string name;
  std::string blurb;
  int         line;
};
 
struct ChoiceValue {
  std::string name;
  std::string text;
  
  int         value;
  bool        neutral;
};

struct Choice {
  /*
   * name if the enum, "_anonymous_" for anonymous enum - of course, when
   * using namespaces, this can also lead to things like "Arts::_anonymous_",
   * which would mean an anonymous enum in the Arts namespace
   */
  std::string name;
  
  std::vector<ChoiceValue> contents;
  std::map<std::string, std::string> infos;
};

struct Record {
  std::string name;
  
  std::vector<Param> contents;
  std::map<std::string, std::string> infos;
};

struct Sequence {
  std::string name;
  Param content;
  std::map<std::string, std::string> infos;
};

struct Method {
  std::string name;

  std::vector<Param> params;
  Param result;
  std::map<std::string, std::string> infos;
};

struct Class {
  std::string name;
  std::string inherits;
  
  std::vector<Method> methods;
  std::vector<Method> signals;
  std::vector<Param> properties;
  std::vector<Stream> istreams, jstreams, ostreams;
  std::map<std::string, std::string> infos;
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

class Parser {
protected:
  const class Options&      options;

  GScanner                 *scanner;
  std::vector<char>         scannerInputData;
  std::vector<LineInfo>     scannerLineInfo;

  std::vector<std::string>  includedNames;
  std::vector<std::string>  types;
  std::map<std::string,int> typeMap;

  std::vector<std::string>  includes;          // files to include
  std::vector<Constant>	    constants;
  std::vector<Choice>	    choices;
  std::vector<Sequence>	    sequences;
  std::vector<Record>	    records;
  std::vector<Class>	    classes;
  std::vector<Method>	    procedures;
  
  static void scannerMsgHandler (GScanner *scanner, gchar *message, gboolean is_error);
  void printError (const gchar *format, ...);

  void preprocess (const std::string& filename);
  bool haveIncluded (const std::string& filename) const;
  bool insideInclude () const;
  
  void addConstantTodo(const Constant& cdef);
  void addChoiceTodo(const Choice& cdef);
  void addRecordTodo(const Record& rdef);
  void addSequenceTodo(const Sequence& sdef);
  void addClassTodo(const Class& cdef);
  void addProcedureTodo(const Method& pdef);

  void addPrototype (const std::string& type, TypeDeclaration typeDecl);
  void addType (const std::string& type, TypeDeclaration typeDecl);

  GTokenType parseTypeName (std::string& s);
  GTokenType parseStringOrConst (std::string &s);
  GTokenType parseConstant ();
  GTokenType parseNamespace ();
  GTokenType parseChoice ();
  GTokenType parseChoiceValue (ChoiceValue& comp, int& value);
  GTokenType parseRecord ();
  GTokenType parseRecordField (Param& comp, const std::string& group);
  GTokenType parseStream (Stream& stream, Stream::Type);
  GTokenType parseSequence ();
  GTokenType parseParamHints (Param &def);
  GTokenType parseClass ();
  GTokenType parseMethod (Method& def);
  GTokenType parseInfoOptional (std::map<std::string,std::string>& infos);
public:
  Parser ();
  
  bool parse (const std::string& fileName);
 
  std::string fileName() const				  { return scanner->input_name; }
  const std::vector<std::string>& getIncludes () const	  { return includes; }
  const std::vector<Constant>& getConstants () const	  { return constants; }
  const std::vector<Choice>& getChoices () const	  { return choices; }
  const std::vector<Sequence>& getSequences () const	  { return sequences; }
  const std::vector<Record>& getRecords () const	  { return records; }
  const std::vector<Class>& getClasses () const		  { return classes; }
  const std::vector<Method>& getProcedures () const	  { return procedures; }
  const std::vector<std::string>& getTypes () const       { return types; }
  
  Sequence findSequence (const std::string& name) const;
  Record findRecord (const std::string& name) const;
  
  bool isChoice(const std::string& type) const;
  bool isSequence(const std::string& type) const;
  bool isRecord(const std::string& type) const;
  bool isClass(const std::string& type) const;
  bool fromInclude(const std::string& type) const;
};

}
#endif /* _SFIDL_PARSER_H_ */

/* vim:set ts=8 sts=2 sw=2: */
