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

#include <vector>
#include <map>
#include <string>

namespace Sfidl {

struct LineInfo {
  bool isInclude;
  int line;
  string filename;
};

struct ConstantDef {
  std::string name;
  enum { tString = 1, tFloat = 2, tInt = 3 } type;

  std::string str;
  float f;
  int i;
};

struct ParamDef {
  std::string type;
  std::string name;
  
  std::string pspec;
  int         line;
  std::string args;
};

struct EnumComponent {
  std::string name;
  std::string text;
  
  int         value;
  bool        neutral;
};

struct EnumDef {
  /*
   * name if the enum, "_anonymous_" for anonymous enum - of course, when
   * using namespaces, this can also lead to things like "Arts::_anonymous_",
   * which would mean an anonymous enum in the Arts namespace
   */
  std::string name;
  
  std::vector<EnumComponent> contents;
  std::map<std::string, std::string> infos;
};

struct RecordDef {
  std::string name;
  
  std::vector<ParamDef> contents;
  std::map<std::string, std::string> infos;
};

struct SequenceDef {
  std::string name;
  ParamDef content;
  std::map<std::string, std::string> infos;
};

struct MethodDef {
  std::string name;

  std::vector<ParamDef> params;
  ParamDef result;
  std::map<std::string, std::string> infos;
};

struct ClassDef {
  std::string name;
  std::string inherits;
  
  std::vector<MethodDef> methods;
  std::vector<MethodDef> signals;
  std::vector<ParamDef> properties;
  std::map<std::string, std::string> infos;
};

class Parser {
protected:
  const class Options&      options;

  GScanner                 *scanner;
  std::vector<char>         scannerInputData;
  std::vector<LineInfo>     scannerLineInfo;

  std::vector<std::string>  includedNames;
  std::vector<std::string>  types;

  std::vector<std::string>  includes;          // files to include
  std::vector<ConstantDef>  constants;
  std::vector<EnumDef>	    enums;
  std::vector<SequenceDef>  sequences;
  std::vector<RecordDef>    records;
  std::vector<ClassDef>	    classes;
  std::vector<MethodDef>    procedures;
  
  static void scannerMsgHandler (GScanner *scanner, gchar *message, gboolean is_error);
  void printError (const gchar *format, ...);

  void preprocess (const string& filename);
  bool haveIncluded (const string& filename) const;
  bool insideInclude () const;
  
  void addConstantTodo(const ConstantDef& cdef);
  void addEnumTodo(const EnumDef& edef);
  void addRecordTodo(const RecordDef& rdef);
  void addSequenceTodo(const SequenceDef& sdef);
  void addClassTodo(const ClassDef& cdef);
  void addProcedureTodo(const MethodDef& pdef);

  GTokenType parseStringOrConst (std::string &s);
  GTokenType parseConstantDef ();
  GTokenType parseNamespace ();
  GTokenType parseEnumDef ();
  GTokenType parseEnumComponent (EnumComponent& comp, int& value);
  GTokenType parseRecordDef ();
  GTokenType parseRecordField (ParamDef& comp);
  GTokenType parseSequenceDef ();
  GTokenType parseParamDefHints (ParamDef &def);
  GTokenType parseClass ();
  GTokenType parseMethodDef (MethodDef& def);
  GTokenType parseInfoOptional (std::map<std::string,std::string>& infos);
public:
  Parser ();
  
  bool parse (const string& fileName);
 
  std::string fileName() const				  { return scanner->input_name; }
  const std::vector<std::string>& getIncludes () const	  { return includes; }
  const std::vector<ConstantDef>& getConstants () const	  { return constants; }
  const std::vector<EnumDef>& getEnums () const		  { return enums; }
  const std::vector<SequenceDef>& getSequences () const   { return sequences; }
  const std::vector<RecordDef>& getRecords () const	  { return records; }
  const std::vector<ClassDef>& getClasses () const	  { return classes; }
  const std::vector<MethodDef>& getProcedures () const    { return procedures; }
  const std::vector<std::string>& getTypes () const       { return types; }
  
  SequenceDef findSequence (const std::string& name) const;
  RecordDef findRecord (const std::string& name) const;
  
  bool isEnum(const std::string& type) const;
  bool isSequence(const std::string& type) const;
  bool isRecord(const std::string& type) const;
  bool isClass(const std::string& type) const;
  bool fromInclude(const std::string& type) const;
};

}
#endif /* _SFIDL_PARSER_H_ */

/* vim:set ts=8 sts=2 sw=2: */
