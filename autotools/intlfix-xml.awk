# GNU AWK Script to extract translatable strings from XML files
# Copyright (C) 2010 Tim Janik
#
# The original public domain XML parser was authored by at least:
# - getXMLEVENT.awk:	Copyright (C) 2007 Juergen Kahrs
# - getXML.awk:     	Copyright (C) 2005 Jan Weber
# - xmlparse.awk:   	Copyright (C) 2001 Steve Coile

BEGIN {
  MARKER = ""; if (ENVIRON["INTLFIX"] ~ /:marker:/) MARKER = "__INTLFIX__:";
  OLINE = 1
  TAG = ""
  while (getXMLEVENT(ARGV[1])) {
    while (OLINE < XMLROW) { print ""; OLINE++ }
    next_tag = ""
    if (XMLEVENT == "STARTELEM") {
      next_tag = XMLSTARTELEM
      printf ("%*s<%s>", 2 * XMLDEPTH - 2, "", XMLSTARTELEM)
      for (i = 1; i <= NF; i++) {
	if ($i ~ /^_[^_]/) {
	  aname = $i; avalue = XMLATTR[$i]
	  sub ("^_", "", aname); gsub ("[\\\"]", "\\" "\\&", avalue)
          printf ("%s /*%s=*/_(\"%s\"); ", MARKER, aname, avalue)
	}
      }
      #print ""
    } else if (XMLEVENT == "COMMENT") {
      gsub ("\\*/", "* /", XMLNAME)     # escape enclosed C-comment ends
      ostring = sprintf ("/* %s */\n", XMLNAME)
      printf ("__XML_LINE__%-3d: %s", OLINE, ostring); gsub (/[^\n]/, "", ostring); OLINE += length (ostring)
    } else if (XMLEVENT == "CHARDATA" && match (TAG, "^_[^_]")) {
      gsub ("[\\\"]", "\\" "\\&", XMLNAME) # escape string quotes
      gsub (/\n/, "\\n\"\n\"", XMLNAME)    # escape newlines
      sub ("^_", "", TAG)
      ostring = sprintf ("%s /*<%s/>*/_(\"%s\");\n", MARKER, TAG, XMLNAME);
      printf ("__XML_LINE__%-3d: %s", OLINE, ostring); gsub (/[^\n]/, "", ostring); OLINE += length (ostring)
    } else if (XMLEVENT == "ENDELEM") {
      printf ("</>")
    }
    TAG = next_tag
  }
}


##
# getXMLEVENT( file ): # read next xml-data into XMLEVENT,XMLNAME,XMLATTR
#                      # referenced entities are not resolved
# Parameters:
#   file       -- path to xml file
# External variables:
#   XMLEVENT   -- type of item read, e.g. "STARTELEM"(tag), "ENDELEM"(end tag),
#                 "COMMENT"(comment), "CHARDATA"(data)
#   XMLNAME    -- value of item, e.g. tagname if type is "STARTELEM" or "ENDELEM"
#   XMLATTR    -- Map of attributes, only set if XMLEVENT=="STARTELEM"
#   XMLPATH    -- Path to current tag, e.g. /TopLevelTag/SubTag1/SubTag2
#   XMLROW     -- current line number in input file
#   XMLERROR   -- error text, set on parse error
# Returns:
#    1         on successful read: XMLEVENT, XMLNAME, XMLATTR are set accordingly
#    ""        at end of file or parse error, XMLERROR is set on error
# Private Data:
#   _XMLIO     -- buffer, XMLROW, XMLPATH for open files
##

function getXMLEVENT( file            ,end,p,q,tag,att,accu,mline,mode,S0,ex,dtd) {
    XMLEVENT=XMLNAME=XMLERROR=XMLSTARTELEM=XMLENDELEM = ""
    split("", XMLATTR)
    S0    = _XMLIO[file,"S0"]
    XMLROW  = _XMLIO[file,"line"];
    XMLPATH = _XMLIO[file,"path"];
    XMLDEPTH=_XMLIO[file,"depth"]+0;
    dtd   = _XMLIO[file,"dtd"];
    while (!XMLEVENT) {
        if (S0 == "") {
            if (1 != (getline S0 < file))
                break;
             XMLROW ++;
             S0 = S0 RS;
        }
        if (mode == "") {
            mline = XMLROW
            accu=""
            p = substr(S0,1,1)
            if (p != "<" && !(dtd && p=="]"))
                mode="CHARDATA"
            else if (p == "]") {
                S0 = substr(S0,2)
                mode="ENDDOCT"
                end=">"
                dtd=0
            } else if ( substr(S0,1,4) == "<!--" ) {
                S0=substr(S0,5)
                mode="COMMENT"
                end="-->"
            } else if ( substr(S0,1,9) == "<!DOCTYPE" ) {
                S0 = substr(S0,10)
                mode = "STARTDOCT"
                end  = ">"
            } else if (substr(S0,1,9) == "<![CDATA[" ) {
                S0 = substr(S0,10)
                mode = "CDA"
                end = "]]>"
            } else if ( substr(S0,1,2) == "<!" ) {
                S0 = substr(S0,3)
                mode = "DEC"
                end = ">"
            } else if (substr(S0,1,2) == "<?") {
                S0 = substr(S0,3)
                mode = "PROCINST"
                end = "?>"
            } else if ( substr(S0,1,2)=="</" ) {
                S0 = substr(S0,3)
                mode = "ENDELEM"
                end = ">";
                tag = S0
                sub(/[ \n\r\t>].*$/,"",tag)
                S0 = substr(S0,length(tag)+1)
                ex = XMLPATH
                sub(/\/[^\/]*$/,"",XMLPATH)
                ex = substr(ex, length(XMLPATH)+2)
                if (tag != ex) {
                    XMLERROR = "unexpected close tag <" ex ">..</" tag ">"
                    break
                }
            } else {
                S0 = substr(S0,2)
                mode = "STARTELEM"
                tag = S0
                sub(/[ \n\r\t\/>].*$/,"",tag)
                S0 = substr(S0, length(tag)+1)
                if (tag !~ /^[A-Za-z:_][0-9A-Za-z:_.-]*$/ ) { # /^[[:alpha:]:_][[:alnum:]:_.-]*$/
                    XMLERROR = "invalid tag name '" tag "'"
                    break
                }
                XMLPATH = XMLPATH "/" tag;
            }
        } else if (mode == "CHARDATA") {                            # terminated by "<" or EOF
            p = index(S0, "<")
            if (dtd && (q=index(S0,"]")) && (!p || q<p) )
                p = q
            if (p) {
                XMLEVENT = "CHARDATA"
                XMLNAME = accu unescapeXML(substr(S0, 1, p-1))
                S0 = substr(S0, p)
                mode = ""
            } else {
                accu = accu unescapeXML(S0)
                S0 = ""
            }
        } else if ( mode == "STARTELEM" ) {
            sub(/^[ \n\r\t]*/,"",S0)
            if (S0 == "")
                continue
            if (substr(S0, 1, 2) == "/>" ) {
                S0 = substr(S0, 3)
                mode = ""
                XMLEVENT = "STARTELEM"
                XMLNAME = XMLSTARTELEM = tag
                XMLDEPTH ++
                S0 = "</" tag ">" S0
            } else if (substr(S0, 1, 1) == ">" ) {
                S0 = substr(S0, 2)
                mode = ""
                XMLEVENT = "STARTELEM"
                XMLNAME = XMLSTARTELEM = tag
                XMLDEPTH ++
            } else {
                att = S0
                sub(/[= \n\r\t\/>].*$/,"",att)
                S0 = substr(S0, length(att) + 1)
                mode = "ATTR"
                if (att !~ /^[A-Za-z:_][0-9A-Za-z:_.-]*$/ ) { # /^[[:alpha:]:_][[:alnum:]:_.-]*$/
                    XMLERROR = "invalid attribute name '" att "'"
                    break
                }
            }
        } else if (mode == "ATTR") {
            sub(/^[ \n\r\t]*/, "", S0)
            if (S0 == "")
                continue
            if (substr(S0,1,1) == "=" ) {
                S0 = substr(S0,2)
                mode = "EQ"
            } else {
                XMLATTR[att] = att
                mode = "STARTELEM"
            }
        } else if (mode == "EQ") {
            sub(/^[ \n\r\t]*/,"",S0)
            if (S0 == "")
              continue
            end = substr(S0,1,1)
            if (end == "\"" || end == "'") {
                S0 = substr(S0,2)
                accu = ""
                mode = "VALUE"
            } else {
                accu = S0
                sub(/[ \n\r\t\/>].*$/,"", accu)
                S0 = substr(S0, length(accu)+1)
                XMLATTR[att] = unescapeXML(accu)
                mode = "STARTELEM"
            }
        } else if (mode == "VALUE") {                          # terminated by end
            if (p = index(S0, end)) {
                XMLATTR[att] = accu unescapeXML(substr(S0,1,p-1))
                S0 = substr(S0, p+length(end))
                mode = "STARTELEM"
            } else {
                accu = accu unescapeXML(S0)
                S0=""
            }
        } else if (mode == "STARTDOCT") {                      # terminated by "[" or ">"
            if ((q = index(S0, "[")) && (!(p = index(S0,end)) || q<p )) {
                XMLEVENT = mode
                XMLNAME = accu substr(S0, 1, q-1)
                S0 = substr(S0, q+1)
                mode = ""
                dtd = 1
            } else if (p = index(S0,end)) {
                XMLEVENT = mode
                XMLNAME = accu substr(S0, 1, p-1)
                S0 = "]" substr(S0, p)
                mode = ""
                dtd = 1
            } else {
                accu = accu S0
                S0 = ""
            }
        } else if (p = index(S0,end)) {  # terminated by end
            XMLEVENT = mode
            XMLNAME = XMLENDELEM = ( mode=="ENDELEM" ? tag : accu substr(S0,1,p-1))
            if (mode=="ENDELEM") XMLDEPTH --
            S0 = substr(S0, p+length(end))
            mode = ""
        } else {
            accu = accu S0
            S0 = ""
        }
    }
    _XMLIO[file, "S0"]   = S0;
    _XMLIO[file, "line"] = XMLROW;
    _XMLIO[file, "path"] = XMLPATH;
    _XMLIO[file, "depth"] = XMLDEPTH;
    _XMLIO[file, "dtd"]  = dtd;
    if (mode == "CHARDATA") {
        mode = ""
        if (accu != "")
            XMLEVENT = "CHARDATA"
        XMLNAME = ""
        $0 = accu
    }
    if (XMLEVENT) {
        if (XMLEVENT == "STARTELEM") {
            # Copy attributes into $0.
            NF=0
            for (ex in XMLATTR) {
                NF ++
                $NF = ex
            }
        }
        return 1
    }
    close(file);
    delete _XMLIO[file, "S0"];
    delete _XMLIO[file, "line"];
    delete _XMLIO[file, "path"];
    delete _XMLIO[file, "depth"];
    delete _XMLIO[file, "dtd"];
    if (XMLERROR)
        XMLERROR = file ":" XMLROW": " XMLERROR
    else if (mode) XMLERROR=file ":" mline ": " "unterminated " mode
    else if (XMLPATH) XMLERROR=file ":" XMLROW": "  "unclosed tag(s) " XMLPATH
} # function getXMLEVENT

# unescape data and attribute values, used by getXMLEVENT
function unescapeXML(text) {
    gsub( "&apos;", "'",  text )
    gsub( "&quot;", "\"", text )
    gsub( "&gt;",   ">",  text )
    gsub( "&lt;",   "<",  text )
    gsub( "&amp;",  "\\&",  text)
    return text
}

# close xml file
function closeXMLEVENT(file) {
    close(file);
    delete _XMLIO[file,"S0"]
    delete _XMLIO[file,"line"]
    delete _XMLIO[file,"path"];
    delete _XMLIO[file,"depth"];
    delete _XMLIO[file,"dtd"]
    delete _XMLIO[file,"open"]
    delete _XMLIO[file,"IND"]
}


