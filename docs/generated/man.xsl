<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:date="http://exslt.org/dates-and-times" version="1.0">
<xsl:output method="text" indent="no" charset="UTF-8"/>
<xsl:strip-space elements="*"/>
<xsl:preserve-space elements="code display smalldisplay format smallformat example smallexample lisp smalllisp"/>

<xsl:param name="man_section"/>

<!-- {{{ start parsing -->
<xsl:template match="texinfo">
<xsl:text>.\" t</xsl:text>
<xsl:call-template name="document-font"/>
<xsl:text>
.\" Execute commands only for PostScript/DVI/etc. kind of outputs
.de psdvi
.if '\*(.T'ps'  \\$*
.if '\*(.T'dvi' \\$*
..
.\" Don't execute commands for PostScript/DVI/etc. kind of outputs
.de npsdvi
.if !'\*(.T'ps' .if !'\*(.T'dvi' \\$*
..
.\" Use stderr only if we are outputting PostScript documents
.de tmw
.psdvi .tm \\$*
..
.
.\" Break pages for PS/DVI outputs
.de bpw
.psdvi .bp
..
.
.\" Index macro
.de IX
.ie '\\n(.z'' .tmw IX: {\\$*}{\\n%}
.el \\!.IX \\$*{\\n%}
..
.
.\" Xref macro
.de XR
.ds xrsect \\$1 \\$2
.shift
.shift
.ie '\\n(.z'' \{\
.  if !'\\*[xrsect]'- -' .tmw XR: .ds \\$*-sect \\*[xrsect]
.  tmw XR: .ds \\$* \\n%
.\}
.el \\!.XR .nr \\$* \\n%
.rm xrtype
..
.
.\" Title macros
.\" Lower case headings only set font sizes, etc.
.de h1
.ne 8
.SH \\$*
.\" FIXME These titles, etc. should look a lot better with PostScript
..
.de h2
.SS \\$*
..
.de h3
.SS \s-1\\$*\s+1
..
.de h4
.SS \s-2\\$*\s+2
..
.\" Upper case headings call lower case ones first, and then the 
.\" relevant table of contents macros
.de H1
.h1 \\$*
.T1 \\$*
..
.de H2
.h2 \\$*
.T2 \\$*
..
.de H3
.h3 \\$*
.T3 \\$*
..
.de H4
.h4 \\$*
.T4 \\$*
..
.
.\" Table of contents macros
.de T1
.ie '\\n(.z'' \{\
.  tmw TC: .sp
.  tmw TC: .ps +4
.  tmw TC: .psdvi .ft B
.  tmw TC: .nr pnowidth \\\\\\\\w'u \\n%'
.  tmw TC: \\$* \\\\\\\\l'\\\\\\\\n[.l]u-\\\\\\\\n[.k]u-\\\\\\\\n[.i]u-\\\\\\\\n[pnowidth]u.' \\n%
.  tmw TC. .rr pnowidth
.  tmw TC: .psdvi .ft P
.  tmw TC: .ps -4
.\}
.el \\!.T1 \\$*{\\n%}
..
.de T2
.ie '\\n(.z'' \{\
.  tmw TC: .in +2m
.  tmw TC: .nr pnowidth \\\\\\\\w'u \\n%'
.  tmw TC: \\$* \\\\\\\\l'\\\\\\\\n[.l]u-\\\\\\\\n[.k]u-\\\\\\\\n[.i]u-\\\\\\\\n[pnowidth]u.' \\n%
.  tmw TC. .rr pnowidth
.  tmw TC: .in -2m
.\}
.el \\!.T2 \\$*{\\n%}
..
.de T3
.ie '\\n(.z'' \{\
.  tmw TC: .in +4m
.  tmw TC: .nr pnowidth \\\\\\\\w'u \\n%'
.  tmw TC: \\$* \\\\\\\\l'\\\\\\\\n[.l]u-\\\\\\\\n[.k]u-\\\\\\\\n[.i]u-\\\\\\\\n[pnowidth]u.' \\n%
.  tmw TC. .rr pnowidth
.  tmw TC: .in -4m
.\}
.el \\!.T3 \\$*{\\n%}
..
.de T4
.ie '\\n(.z'' \{\
.  tmw TC: .in +6m
.  tmw TC: .nr pnowidth \\\\\\\\w'u \\n%'
.  tmw TC: \\$* \\\\\\\\l'\\\\\\\\n[.l]u-\\\\\\\\n[.k]u-\\\\\\\\n[.i]u-\\\\\\\\n[pnowidth]u.' \\n%
.  tmw TC. .rr pnowidth
.  tmw TC: .in -6m
.\}
.el \\!.T4 \\$*{\\n%}
..
.
.\" Understrike macro
.de us
\\$1\l'|0\(ul'
..
.
.
.\" Turn hyphenation off
.nh
.\" Load monospace fonts (Courier)
.\" 5 -> Regular text
.fp 5 C
.\" 6 -> Bold text
.fp 6 CB
.\" 7 -> Italic text
.fp 7 CI
.
.\" xref definitions
@XR@
.
.\" Start the document
</xsl:text>
<xsl:text>.TH "</xsl:text>
<xsl:value-of select="settitle"/>
<xsl:text>" "</xsl:text>
<xsl:value-of select="$man_section"/>
<xsl:text>" "</xsl:text>
<xsl:call-template name="date"/>
<xsl:text>" "</xsl:text>
<xsl:value-of select="para/document-package"/>
<xsl:text>" "</xsl:text>
<xsl:value-of select="para/document-package"/>
<xsl:text>"</xsl:text>
<xsl:call-template name="title_page"/>
<xsl:text>
</xsl:text>
<xsl:apply-templates/>
</xsl:template>
<!-- }}} -->

<!-- {{{ useless tags -->
<xsl:template match="setfilename|settitle|document-title|document-author|document-package|document-font|document-navigation|document-hasbanner|itemfunction|columnfraction"/>
<!-- }}} -->

<!-- {{{ setting a default font for documents -->
<xsl:template name="document-font">
  <xsl:variable name="font" select="string(/texinfo/para/document-font)"/>
  <xsl:choose>
    <xsl:when test="$font=''"/>
    <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'">
      <xsl:text>
.fam H</xsl:text>
    </xsl:when>
    <xsl:when test="$font='story' or $font='storystyle' or $font='serif'">
      <xsl:text>
.fam T</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: omitting unknown font style '<xsl:value-of select="$font"/>'</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ creating a title page for documents -->
<xsl:template name="title_page">
  <xsl:if test="string-length(/texinfo/para/document-title) > 0 or count(/texinfo/para/document-author) > 0">
    <xsl:if test="string-length(/texinfo/para/document-title) > 0">
      <xsl:text>
.ce
\s+4\fB</xsl:text>
      <xsl:value-of select="/texinfo/para/document-title"/>
      <xsl:text>\fP\s0
.sp 2m
</xsl:text>
    </xsl:if>
    <xsl:if test="count(/texinfo/para/document-author) > 0">
      <xsl:text>.ce </xsl:text>
      <xsl:value-of select="count(/texinfo/para/document-author)"/>
      <xsl:text>
</xsl:text>
      <xsl:for-each select="/texinfo/para/document-author">
	<xsl:apply-templates/>
	<xsl:if test="position()!=last()">
	  <xsl:text>
</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:if>
  </xsl:if>
</xsl:template>
<!-- }}} -->

<!-- {{{ date and time creation -->
<xsl:template name="date">
  <xsl:value-of select="concat(date:day-in-month(), ' ', date:month-abbreviation(), ' ', date:year())"/>
</xsl:template>

<xsl:template name="time">
  <xsl:value-of select="concat(date:hour(), ':', date:minute-in-hour())"/>
</xsl:template>

<xsl:template name="date-time">
  <xsl:call-template name="date"/> <xsl:call-template name="time"/>
</xsl:template>
<!-- }}} -->

<!-- {{{ table of contents related stuff -->
<!-- Alper: fix this template by removing para tags when makeinfo is fixed -->
<xsl:template match="para/table-of-contents">
<xsl:text>.\" no refilling
.ds adj \*(.j
.ad l
@TF@
.\" adjustment (justification) to it's old state
.ad \*[adj]
.\" start a new page
.bpw
</xsl:text>
</xsl:template>
<!-- }}} -->

<!-- {{{ document sections -->
<!-- we only care for appendices; they should start a new page immediately -->
<xsl:template match="appendix">
  <xsl:text>\ 
.bpw
</xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<!-- table of contents unnumbered section should start a page -->
<xsl:template match="unnumbered">
  <xsl:if test="count(para/table-of-contents)">
    <xsl:text>\ 
.bpw
</xsl:text>
  </xsl:if>
  <xsl:apply-templates/>
</xsl:template>
<!-- }}} -->

<!-- {{{ section numbering -->
<xsl:template name="title_chapter_number">
  <xsl:number level="any" count="chapter"/>
</xsl:template>

<xsl:template name="title_section_number">
  <xsl:call-template name="title_chapter_number"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="section" from="chapter"/>
</xsl:template>

<xsl:template name="title_subsection_number">
  <xsl:call-template name="title_section_number"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="subsection" from="section"/>
</xsl:template>

<xsl:template name="title_subsubsection_number">
  <xsl:call-template name="title_subsection_number"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="subsubsection" from="subsection"/>
</xsl:template>

<xsl:template name="title_appendix_number">
  <xsl:text>Appendix </xsl:text><xsl:number level="any" count="appendix" format="A"/>
</xsl:template>

<xsl:template name="title_appendixsec_number">
  <!-- don't call title_appendix_number, because it also prepends Appendix
       to title -->
  <xsl:number level="any" count="appendix" format="A"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="appendixsec" from="appendix"/>
</xsl:template>

<xsl:template name="title_appendixsubsec_number">
  <xsl:call-template name="title_appendixsec_number"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="appendixsubsec" from="appendixsec"/>
</xsl:template>

<xsl:template name="title_appendixsubsubsec_number">
  <xsl:call-template name="title_appendixsubsec_number"/>
  <xsl:text>.</xsl:text>
  <xsl:number level="any" count="appendixsubsubsec" from="appendixsubsec"/>
</xsl:template>

<!-- since nodename tags appear before the chapter/appendix/etc. sections, 
     we need to manually increment the counts by one -->
<xsl:template name="node_chapter_number">
  <xsl:variable name="count">
    <xsl:number level="any" count="chapter"/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count(chapter)">
      <xsl:value-of select="$count+1"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="node_section_number">
  <xsl:call-template name="node_chapter_number"/>
  <xsl:text>.</xsl:text>
  <xsl:variable name="count">
    <xsl:number level="any" count="section" from="chapter"/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count(section)">
      <xsl:value-of select="$count+1"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="node_appendix_number">
  <xsl:variable name="count">
    <xsl:number level="any" count="appendix"/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count(appendix)">
      <!-- FIXME limits the number of appendices to 9 :\ -->
      <!-- then it will look like AA, AB, etc. -->
      <xsl:value-of select="translate($count+1, '123456789', 'ABCDEFGHI')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
      <xsl:value-of select="translate($count, '123456789', 'ABCDEFGHI')"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="node_appendixsec_number">
  <xsl:call-template name="node_appendix_number"/>
  <xsl:text>.</xsl:text>
  <xsl:variable name="count">
    <xsl:number level="any" count="appendixsec" from="appendix"/>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="count(appendixsec)">
      <xsl:value-of select="$count+1"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- }}} -->

<!-- {{{ section titles stuff -->

<!-- {{{ chapters -->
<xsl:template match="chapter/title">.H1 <xsl:call-template name="title_chapter_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="section/title">.H2 <xsl:call-template name="title_section_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="subsection/title">.H3 <xsl:call-template name="title_subsection_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="subsubsection/title">.H4 <xsl:call-template name="title_subsubsection_number"/>\ \ \ <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ appendices -->
<xsl:template match="appendix/title">.H1 <xsl:call-template name="title_appendix_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="appendixsec/title">.H2 <xsl:call-template name="title_appendixsec_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="appendixsubsec/title">.H3 <xsl:call-template name="title_appendixsubsec_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="appendixsubsubsec/title">.H4 <xsl:call-template name="title_appendixsubsubsec_number"/>\ \ \ <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>
<!-- }}} -->

<!-- {{{ unnumbered -->
<xsl:template match="unnumbered/title">.H1 <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="unnumberedsec/title">.H2 <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="unnumberedsubsec/title">.H3 <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="unnumberedsubsubsec/title">.H4 <xsl:apply-templates/>
<xsl:text>
</xsl:text>
</xsl:template>
<!-- }}} -->

<!-- {{{ title-only sections -->
<xsl:template match="chapheading/title|majorheading/title">.H1 <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="heading/title">.H2 <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subheading/title">.H3 <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsubheading/title">.H4 <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
<!-- }}} -->

<!-- }}} -->

<!-- {{{ reference generation -->
<xsl:template match="reference-function">\fB\f6<xsl:apply-templates/>\fP\f1</xsl:template>
<xsl:template match="reference-parameter">\fI\f7<xsl:apply-templates/>\fP\f1</xsl:template>
<xsl:template match="reference-constant">\f5<xsl:apply-templates/>\f1</xsl:template>
<xsl:template match="reference-type">\fI\f7<xsl:apply-templates/>\fP\f1</xsl:template>
<xsl:template match="reference-returns">\h'-2m'\fI\f7<xsl:apply-templates/>\fP\f1</xsl:template>
<!-- <xsl:template match="reference-blurb">\fI\f7<xsl:apply-templates/>\fP\f1</xsl:template> -->
<xsl:template match="reference-struct-name"> \fB\f6<xsl:apply-templates/>\fP\f6</xsl:template>
<xsl:template match="reference-struct-type"> \fI\f7<xsl:apply-templates/>\fP\f1</xsl:template>
<xsl:template match="reference-struct-open"> \fB\f6{\fP\f1</xsl:template>
<xsl:template match="reference-struct-close"><xsl:text>.sp -1em
.TP
.PD 0
 \fB\f6};\fP\f1

</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ paragraphs -->
<xsl:template match="para">
  <xsl:choose>
    <xsl:when test="count(document-font|document-title|document-author)"/>
    <xsl:otherwise>
      <xsl:text>.PP
</xsl:text>
      <xsl:apply-templates/>
    <xsl:text>
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ line breaks, forced spaces -->
<xsl:template match="linebreak"><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template match="space"><xsl:text> </xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ contextual tags -->
<xsl:template match="center">.ce
<xsl:apply-templates/></xsl:template>

<xsl:template match="acronym|cite|dfn|kbd|samp|var|url|email|key|env|file|command|option|code|changelog-entry|changelog-item">
  <xsl:text>\f5</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>\f1</xsl:text>
</xsl:template>

<xsl:template match="menupath">\f7<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="pagepath">\f7<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="object">\f5<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="channel">\f7<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="property">"<xsl:apply-templates/>"</xsl:template>

<xsl:template match="emph">\fI<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="strong|important">\fB<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="quotation"><xsl:text>
.in +4
.ll -8
</xsl:text>
<xsl:apply-templates/><xsl:text>
.in -4
.ll +8
</xsl:text></xsl:template>

<xsl:template match="example|smallexample|display|smalldisplay|format|smallformat|lisp|smalllisp">
<xsl:text>
.nf
.na
</xsl:text>
<xsl:if test="local-name()='lisp' or local-name()='smalllisp' or local-name()='display' or local-name()='smalldisplay'"><xsl:text>.in +4
</xsl:text></xsl:if>
<xsl:if test="local-name()='example' or local-name()='smallexample' or local-name()='lisp' or local-name()='smalllisp'">\fC</xsl:if>
<xsl:if test="local-name()='smallexample' or local-name()='smalllisp' or local-name()='smalldisplay' or local-name()='smallformat'">\s-2</xsl:if>
<xsl:apply-templates/>
<xsl:if test="local-name()='example' or local-name()='smallexample' or local-name()='lisp' or local-name()='smalllisp'">\f1</xsl:if>
<xsl:if test="local-name()='smallexample' or local-name()='smalllisp' or local-name()='smalldisplay' or local-name()='smallformat'">\s+2</xsl:if>
<xsl:if test="local-name()='lisp' or local-name()='smalllisp' or local-name()='display' or local-name()='smalldisplay'"><xsl:text>
.in -4</xsl:text></xsl:if>
<xsl:text>
.fi
.ad
</xsl:text>
</xsl:template>
<!-- }}} -->

<!-- {{{ font specification commands -->

<!-- note that these commands are here for the sake of completeness
     their use is not recommended in the texinfo manual -->


<xsl:template match="b">\fB<xsl:apply-templates/>\fP</xsl:template>
<xsl:template match="i">\fI<xsl:apply-templates/>\fP</xsl:template>
<!-- save current font in a register, and then retrieve it back -->
<xsl:template match="r">\R'pf (\n(.f'\fP<xsl:apply-templates/>\f\n(pf</xsl:template>
<xsl:template match="tt">\fC<xsl:apply-templates/>\fP</xsl:template>
<!-- }}} -->

<!-- {{{ enumeration and itemization handlng -->
<xsl:template match="itemize">.psdvi .sp 0.5
.npsdvi .sp 1
.PD 0
<xsl:apply-templates/>.PD
</xsl:template>

<xsl:template match="itemize/item"><xsl:text>.IP \(bu 2
</xsl:text><xsl:apply-templates/></xsl:template>

<xsl:template match="enumerate/item"><xsl:text>.IP </xsl:text><xsl:number format="1."/><xsl:text> 3
</xsl:text><xsl:apply-templates/></xsl:template>

<xsl:template match="itemize/item/para|enumerate/item/para"><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ parsing and printing urefs according to their protocols -->
<xsl:template match="uref">
  <!-- protocol for this link type -->
  <xsl:variable name="protocol" select="substring-before(urefurl, '://')"/>
  <xsl:if test="$protocol=''">
    <!-- another test before we bail out. Not all protocols need a ://, ie. mailto: -->
    <xsl:if test="substring-before(urefurl, ':') = ''">
      <xsl:message terminate="yes">XSL-ERROR: unset protocol for <xsl:value-of select="urefurl"/></xsl:message>
    </xsl:if>
  </xsl:if>

  <!-- actual link -->
  <xsl:variable name="url" select="substring-after(urefurl, '://')"/>

  <!-- feedback -->
  <!-- <xsl:message>DEBUG: protocol is <xsl:value-of select="$protocol"/> for <xsl:value-of select="urefurl"/></xsl:message> -->

  <xsl:choose>
    <!-- PROTOCOL: HTTP FTP -->
    <xsl:when test="$protocol='http' or $protocol='ftp'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f7<xsl:value-of select="concat($protocol, '://', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f7<xsl:value-of select="concat($protocol, '://', $url)"/>\fP</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: FILE MAILTO -->
    <xsl:when test="$protocol='file' or $protocol='mailto'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: NEWS -->
    <xsl:when test="$protocol='news'">
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f7<xsl:value-of select="concat($protocol, ':', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f7<xsl:value-of select="concat($protocol, ':', $url)"/>\fP</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: System and BEAST Man Pages -->
    <xsl:when test="$protocol='man' or $protocol='beast-man'">
      <!-- Get the section the man page belongs to -->
      <xsl:variable name="section">
	<xsl:choose>
	  <xsl:when test="substring-before($url, '/') = ''">
	    <xsl:message>XSL-WARNING: unset man section in <xsl:value-of select="urefurl"/>, using default (1)</xsl:message>
	    <xsl:value-of select="1"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="substring-before($url, '/')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Name of the page -->
      <xsl:variable name="page">
	<xsl:choose>
	  <xsl:when test="substring-after($url, '/') = ''">
	    <xsl:value-of select="$url"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="substring-after($url, '/')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Print it -->
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($page, '(', $section, ')')"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="concat($page, '(', $section, ')')"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: Beast Document -->
    <xsl:when test="$protocol='beast-doc'">
      <!-- Get the file name and append the target specific extension (html) -->
      <xsl:variable name="filename">
	<xsl:choose>
	  <xsl:when test="substring-before($url, '#') = ''">
	    <xsl:value-of select="concat($url, '(1)')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(substring-before($url, '#'), '(1)')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Get the anchor -->
      <xsl:variable name="anchor">
	<xsl:choose>
	  <xsl:when test="substring-after($url, '#') = ''"/>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(', ', substring-after($url, '#'))"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Print the link -->
      <xsl:choose>
	<xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($filename, $anchor)"/>)</xsl:when>
	<xsl:otherwise><xsl:value-of select="concat($filename, $anchor)"/></xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- Unknown Protocol -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- or maybe it is mailto: ? -->
	<xsl:when test="substring-before(urefurl, ':') = 'mailto'">
	  <xsl:variable name="url" select="substring-after(urefurl, ':')"/>
	  <xsl:choose>
	    <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	    <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	    <xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
	  </xsl:choose>
	</xsl:when>
        <xsl:otherwise>
	  <xsl:message>XSL-WARNING: unknown protocol '<xsl:value-of select="$protocol"/>' in <xsl:value-of select="urefurl"/>, using as-is</xsl:message>
	  <xsl:choose>
	    <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	    <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f7<xsl:value-of select="urefurl"/>\fP)</xsl:when>
	    <xsl:otherwise>\fI\f7<xsl:value-of select="urefurl"/>\fP</xsl:otherwise>
	  </xsl:choose>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ menus -->

<!-- TODO menu -->
<xsl:template match="menu"/>

<!-- }}} -->

<!-- {{{ anchors, nodes and references -->
<xsl:template match="anchor"><xsl:text>
.XR - - </xsl:text><xsl:value-of select="translate(normalize-space(./@name), ' ', '_')"/><xsl:text>
</xsl:text></xsl:template>

<!-- we don't make use of up, next and previous nodes -->
<xsl:template match="nodeup|nodenext|nodeprev|nodename"/>

<xsl:template match="node">
  <xsl:variable name="node_section">
    <xsl:choose>
      <xsl:when test="count(chapter)">
        <xsl:text>Chapter </xsl:text>
	<xsl:call-template name="node_chapter_number"/>
      </xsl:when>
      <xsl:when test="count(section|subsection|subsubsection)">
        <xsl:text>Section </xsl:text>
	<xsl:call-template name="node_section_number"/>
      </xsl:when>
      <xsl:when test="count(appendix)">
        <xsl:text>Appendix </xsl:text>
	<xsl:call-template name="node_appendix_number"/>
      </xsl:when>
      <xsl:when test="count(appendixsec|appendixsubsec|appendixsubsubsec)">
        <xsl:text>Appendix </xsl:text>
	<xsl:call-template name="node_appendixsec_number"/>
      </xsl:when>
      <!-- eh, we need to do something for unnumbered, etc. -->
      <xsl:otherwise>-</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
<xsl:text>.XR </xsl:text><xsl:value-of select="$node_section"/><xsl:text> </xsl:text><xsl:value-of select="translate(normalize-space(string(nodename)), ' ', '_')"/><xsl:text>
</xsl:text><xsl:apply-templates/>
</xsl:template>

<xsl:template match="xref">
  <xsl:variable name="xreftext">
    <xsl:choose>
      <xsl:when test="count(xrefprintedname)">
	<xsl:text>"</xsl:text><xsl:apply-templates select="xrefprinteddesc"/><xsl:text>" in \fI</xsl:text><xsl:apply-templates select="xrefprintedname"/><xsl:text>\fP</xsl:text>
      </xsl:when>
      <xsl:when test="count(xrefinfofile)">
	<xsl:text>"</xsl:text><xsl:apply-templates select="xrefprinteddesc"/><xsl:text>" in </xsl:text><xsl:apply-templates select="xrefinfofile"/>
      </xsl:when>
      <xsl:when test="count(xrefprinteddesc)">
	<xsl:text>[</xsl:text><xsl:apply-templates select="xrefprinteddesc"/><xsl:text>]</xsl:text>
      </xsl:when>
      <xsl:otherwise test="">
	<xsl:text>[</xsl:text><xsl:apply-templates select="xrefnodename"/><xsl:text>]</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="xrefname" select="translate(normalize-space(xrefnodename), ' ', '_')"/>
  <xsl:variable name="xrefnodesect">
    <xsl:variable name="xrefnodename" select="string(xrefnodename)"/>
    <xsl:if test="count(//node[nodename = $xrefnodename]) > 0">
      <xsl:text> \*[</xsl:text><xsl:value-of select="$xrefname"/><xsl:text>-sect]</xsl:text>
    </xsl:if>
  </xsl:variable>

  <!-- print it -->
  <xsl:text>See</xsl:text>
  <xsl:value-of select="$xrefnodesect"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$xreftext"/>
  <xsl:if test="not(string-length(xrefinfofile) or string-length(xrefprintedname))">
    <xsl:text>, page \*[</xsl:text>
    <xsl:value-of select="$xrefname"/>
    <xsl:text>]</xsl:text>
  </xsl:if>
</xsl:template>
<!-- }}} -->

<!-- {{{ inline images -->
<xsl:template match="image">
  <img>
    <xsl:if test="string-length(@alttext) > 0">
      <xsl:attribute name="alt">
	<xsl:value-of select="@alttext"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="string-length(@width) > 0">
      <xsl:attribute name="width">
	<xsl:value-of select="@width"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="string-length(@height) > 0">
      <xsl:attribute name="height">
	<xsl:value-of select="@height"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:attribute name="src">
      <!-- <xsl:value-of select="concat($image_prefix, .)"/><xsl:text>.</xsl:text><xsl:value-of select="@extension"/> -->
    </xsl:attribute>
  </img>
</xsl:template>
<!-- }}} -->

<!-- {{{ table handling -->

<!-- {{{ multicolumn tables -->
<xsl:template match="multitable">.TS
nokeep;
l l l.
<xsl:apply-templates/><xsl:text>
.TE

</xsl:text>
</xsl:template>

<xsl:template match="multitable/row"><xsl:apply-templates/><xsl:if test="not(position() = last())"><xsl:text>
</xsl:text></xsl:if></xsl:template>

<xsl:template match="multitable/row/entry"><xsl:apply-templates/><xsl:if test="not(position() = last())"><xsl:text>	</xsl:text></xsl:if></xsl:template>
<!-- }}} -->

<!-- {{{ simple definition tables -->
<xsl:template match="tableitem"><xsl:text>
</xsl:text><xsl:apply-templates/></xsl:template>

<xsl:template match="tableterm">
<xsl:text>.TP
.PD 0
</xsl:text><xsl:apply-templates/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="tableitem/item/para"><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
<!-- }}} -->

<!-- }}} -->

<!-- {{{ indice generation -->
<xsl:template match="indexterm">
<xsl:variable name="indexdepth_1">
  <xsl:if test="count(ancestor::chapter|ancestor::appendix|ancestor::unnumbered)">
    <xsl:value-of select="ancestor::chapter/title|ancestor::appendix/title|ancestor::unnumbered/title"/>
    <xsl:value-of select="'!'"/>
  </xsl:if>
</xsl:variable>
<xsl:variable name="indexdepth_2">
  <xsl:if test="count(ancestor::section|ancestor::appendixsec|ancestor::unnumberedsec)">
    <xsl:value-of select="ancestor::section/title|ancestor::appendixsec/title|ancestor::unnumberedsec/title"/>
    <xsl:value-of select="'!'"/>
  </xsl:if>
</xsl:variable>
<xsl:text>
.IX </xsl:text><xsl:value-of select="concat($indexdepth_1, $indexdepth_2, ., '@')"/><xsl:apply-templates/><xsl:text>
.sp -1
</xsl:text></xsl:template>

<xsl:template match="printindex">@IF@
</xsl:template>

<xsl:template match="para/printplainindex">
  <xsl:variable name="type" select="."/>
  <xsl:for-each select="//indexterm[@index=$type]">
    <xsl:sort/>
      <xsl:apply-templates/><xsl:text>
.br
</xsl:text>
  </xsl:for-each>
</xsl:template>
<!-- }}} -->

</xsl:stylesheet>
<!-- vim: set fdm=marker: -->
