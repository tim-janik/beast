<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:date="http://exslt.org/dates-and-times" version="1.0">
<xsl:output method="text" indent="no" charset="UTF-8"/>
<xsl:strip-space elements="*"/>
<xsl:preserve-space elements="code display smalldisplay format smallformat example smallexample lisp smalllisp"/>

<xsl:param name="man_section"/>

<!-- {{{ start parsing -->
<xsl:template match="texinfo">.\" t<xsl:call-template name="document-font"/>
.\" Understrike macro
.de us
\\$1\l'|0\(ul'
..
.\" Paragraph macro
.de pg
.ft R
.ps 10
.vs 12p
.in 0
.sp 0.4
.ne 1+\\n(.Vu
.ti 0.2i
..
.\" Turn hyphenation off
.nh
.\" Load monospace fonts (Courier)
.\" 5 -> Regular text
.fp 5 C
.\" 6 -> Italic text
.fp 6 CI
.\" Start the document
.TH "<xsl:value-of select="settitle"/>" "<xsl:value-of select="$man_section"/>" "<xsl:call-template name="date"/>" "<xsl:value-of select="para/document-package"/>" "<xsl:value-of select="para/document-package"/>"
<xsl:call-template name="title_page"/><xsl:apply-templates/>
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
    <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'"><xsl:text>
.fam H</xsl:text></xsl:when>
    <xsl:when test="$font='story' or $font='storystyle' or $font='serif'"><xsl:text>
.fam T</xsl:text></xsl:when>
    <xsl:when test="$font='mono' or $font='monospace' or $font='mono-space' or $font='fixed'"><xsl:text>
.fam C</xsl:text></xsl:when>
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: omitting unknown font style '<xsl:value-of select="$font"/>'</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ creating a title page for documents -->
<xsl:template name="title_page">
  <xsl:if test="string-length(/texinfo/para/document-title) > 0 or count(/texinfo/para/document-author) > 0">
    <xsl:if test="string-length(/texinfo/para/document-title) > 0"><xsl:text>.ce
\s+4\fB</xsl:text><xsl:value-of select="/texinfo/para/document-title"/><xsl:text>\fP\s0
.sp 2m
</xsl:text></xsl:if>
    <xsl:if test="count(/texinfo/para/document-author) > 0">
    <xsl:text>.ce </xsl:text><xsl:value-of select="count(/texinfo/para/document-author)"/><xsl:text>
</xsl:text>
      <xsl:for-each select="/texinfo/para/document-author">
<xsl:apply-templates/><xsl:text>
</xsl:text></xsl:for-each></xsl:if>
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
  <xsl:for-each select="/texinfo/chapter|/texinfo/unnumbered|/texinfo/appendix">
    <xsl:choose>
      <xsl:when test="local-name() = 'chapter'">
	<xsl:call-template name="toc_chapter"/>
      </xsl:when>
      <xsl:when test="local-name() = 'unnumbered'">
	<xsl:call-template name="toc_unnumbered"/>
      </xsl:when>
      <xsl:when test="local-name() = 'appendix'">
	<xsl:call-template name="toc_appendix"/>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="toc_chapter"><xsl:number format="1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./section) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./section">
      <xsl:call-template name="toc_section"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_section"><xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./subsection) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./subsection">
      <xsl:call-template name="toc_subsection"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_subsection"><xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./subsubsection) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./subsubsection">
      <xsl:call-template name="toc_subsubsection"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_subsubsection"><xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template name="toc_appendix"><xsl:text>Appendix </xsl:text><xsl:number format="A - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsec">
      <xsl:call-template name="toc_appendixsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsec"><xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsubsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsubsec">
      <xsl:call-template name="toc_appendixsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsubsec"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./appendixsubsubsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./appendixsubsubsec">
      <xsl:call-template name="toc_appendixsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_appendixsubsubsec"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>

<xsl:template name="toc_unnumbered"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsec">
      <xsl:call-template name="toc_unnumberedsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsec"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsubsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsec">
      <xsl:call-template name="toc_unnumberedsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsubsec"><xsl:value-of select="title"/>
  <xsl:choose>
    <xsl:when test="count(./unnumberedsubsubsec) > 0">
<xsl:text>
.in +2
</xsl:text>
    <xsl:for-each select="./unnumberedsubsubsec">
      <xsl:call-template name="toc_unnumberedsubsubsec"/>
    </xsl:for-each>
<xsl:text>
.in -2
</xsl:text>
    </xsl:when>
    <xsl:otherwise><xsl:text>
.br
</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="toc_unnumberedsubsubsec"><xsl:value-of select="title"/><xsl:text>
.br
</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ section titles stuff -->
<xsl:template match="chapter/title">.SH <xsl:number count="chapter" format="1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="section/title">.SS <xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsection/title"> <xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsubsection/title"> <xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendix/title">.SH <xsl:number count="appendix" format="A - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsec/title">.SS <xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsubsec/title"> <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="appendixsubsubsec/title"> <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumbered/title">.SH <xsl:apply-templates/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="unnumberedsec/title">.SS <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumberedsubsec/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="unnumberedsubsubsec/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="chapheading/title|majorheading/title">.SH <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="heading/title">.SS <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subheading/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>

<xsl:template match="subsubheading/title"> <xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ reference generation -->
<xsl:template match="reference-function">\fB<xsl:apply-templates/>\fP</xsl:template>
<xsl:template match="reference-parameter">\fI<xsl:apply-templates/>\fP</xsl:template>
<xsl:template match="reference-type">\fI<xsl:apply-templates/>\fP</xsl:template>
<xsl:template match="reference-returns">\h'-2m'\fI<xsl:apply-templates/>\fP</xsl:template>
<!-- <xsl:template match="reference-blurb">\fI<xsl:apply-templates/>\fP</xsl:template> -->
<xsl:template match="reference-struct-name"> \fI<xsl:apply-templates/>\fP</xsl:template>
<xsl:template match="reference-struct-open"> \fB{\fP</xsl:template>
<xsl:template match="reference-struct-close"><xsl:text>.sp -1em
.TP
.PD 0
\fB};\fP

</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ paragraphs -->
<xsl:template match="para"><xsl:text>.PP
</xsl:text><xsl:apply-templates/><xsl:text>
</xsl:text></xsl:template>
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

<xsl:template match="acronym|cite|dfn|kbd|samp|var|url|email|key|env|file|command|option|code">\f5<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="menupath">\f6<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="pagepath">\f6<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="object">\f5<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="channel">\f6<xsl:apply-templates/>\f1</xsl:template>

<xsl:template match="property">"<xsl:apply-templates/>"</xsl:template>

<xsl:template match="emph">\fI<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="strong|important">\fB<xsl:apply-templates/>\fP</xsl:template>

<xsl:template match="quotation">
<!-- TODO fill this space -->
</xsl:template>

<xsl:template match="example|smallexample|display|smalldisplay|format|smallformat|lisp|smalllisp">
.nf
.na
<xsl:apply-templates/>
.fi
.ad
</xsl:template>
<!-- }}} -->

<!-- {{{ enumeration and itemization handlng -->
<xsl:template match="itemize|enumerate"><xsl:apply-templates/></xsl:template>

<xsl:template match="itemize/item"><xsl:text>.IP \(bu 4
</xsl:text><xsl:apply-templates/></xsl:template>

<xsl:template match="enumerate/item"><xsl:text>.IP </xsl:text><xsl:number format="1."/><xsl:text> 4
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
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="concat($protocol, '://', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f6<xsl:value-of select="concat($protocol, '://', $url)"/>\fP</xsl:otherwise>
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
	<xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="concat($protocol, ':', $url)"/>\fP)</xsl:when>
	<xsl:otherwise>\fI\f6<xsl:value-of select="concat($protocol, ':', $url)"/>\fP</xsl:otherwise>
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
	    <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (\fI\f6<xsl:value-of select="urefurl"/>\fP)</xsl:when>
	    <xsl:otherwise>\fI\f6<xsl:value-of select="urefurl"/>\fP</xsl:otherwise>
	  </xsl:choose>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
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
<xsl:template match="indexterm"/>

<xsl:template match="printindex">
  <xsl:variable name="type" select="."/>
  <xsl:text>.PP
.TS
l l.
Name	Section
</xsl:text>
  <xsl:for-each select="//indexterm[@index=$type]">
    <xsl:sort/>
      <xsl:apply-templates/><xsl:text>	</xsl:text><xsl:value-of select="../../title"/>
  </xsl:for-each>
<xsl:text>
.TE
</xsl:text>
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
