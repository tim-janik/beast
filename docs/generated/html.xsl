<?xml version="1.0"?>
<!DOCTYPE html-stylesheet [
  <!ENTITY sp "&amp;nbsp;">
]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" indent="no" charset="UTF-8" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"/>
<!-- <xsl:strip-space elements="*"/> -->
<xsl:preserve-space elements="keepspace code display format example lisp"/>

<xsl:param name="banner"/>
<xsl:param name="navigation"/>
<xsl:param name="uplinks"/>
<xsl:param name="this_file" select="''"/>
<xsl:param name="image_prefix" select="''"/>

<!-- {{{ start parsing -->
<xsl:template match="texinfo">
<html>
 <head>
  <meta http-equiv="Default-Style" content="Default"/>
  <link href="default.css" title="Default" rel="stylesheet" type="text/css"/>
  <title><xsl:value-of select="settitle"/></title>
  <style type="text/css">
body {
  background-color: White;
  color: Black;
  <xsl:call-template name="document-font"/>
}

a {
  font-style: normal;
  text-decoration: none;
}

a:hover {
  text-decoration: underline;
}

h1 {
  font-size: 150%;
  font-weight: normal;
}

h1.banner {
  margin: 0px;
  color: #d0e4d0;
}

h2 {
  font-size: 130%;
}

h3 {
  font-size: 120%;
}

h4 {
  font-size: 110%;
}

dt {
  <!-- text-decoration: underline; -->
  font-weight: bold;
}

dd {
  margin-left: 2em;
}

div.banner {
  background-color: #005d5d;
  padding: 3px 5px;
  margin-bottom: 1em;
  margin-right: 0.5em;
  <!-- width: 100%; -->
}

div.title_page {
  text-align: center;
  margin-bottom: 3em;
}

div.title_document_title {
}

div.title_author {
  font-size: 140%;
  font-weight: bold;
  margin: 2em;
}

thead td {
  background-color: #005d5d;
  color: #d0e4d0;
  font-weight: bold;
  padding-left: 4px;
}

div.index {
}

span.toc_chapter {
  font-size: 120%;
  color: #222266;
}

span.toc_section {
  font-size: 110%;
  color: #222266;
}

span.toc_subsection {
  font-size: 100%;
  color: #222266;
}

span.toc_subsubsection {
  font-size: 90%;
  color: #222266;
}

span.url {
}

span.email {
}

span.key {
}

span.env {
}

span.file {
}

span.command {
}

span.option {
}

span.menupath {
  font-weight: bold;
  font-style: italic;
  font-family: monospace;
  background-color: #e0e0e0;
  padding: 1px 5px;
  border: 1px outset #808080;
}

span.pagepath {
  font-weight: bold;
  font-style: italic;
  font-family: monospace;
  background-color: #f0f0f0;
  padding: 1px 5px;
  border: 1px solid #c0c0c0;
}

span.property {
  font-style: italic;
}

span.channel {
  font-style: italic;
}

span.object {
  font-family: monospace;
  font-style: italic;
}

span.emph {
  font-style: italic;
}

span.strong {
  font-weight: bold;
}

span.important {
  font-style: italic;
  text-decoration: underline;
}

span.reference-function {
  font-weight: bold;
  color: #5555cc;
}

span.reference-parameter {
  color: #198e86;
  font-weight: bold;
}

span.reference-returns {
  color: #228822;
  font-weight: bold;
  margin-left: -1em;
}

span.reference-type {
  color: #555555;
  font-weight: normal;
}

span.reference-blurb {
  color: #555555;
}

span.reference-struct-name {
  color: #668c1a;
  font-weight: bold;
}

span.reference-struct {
  font-weight: bold;
}

span.reference-struct-close {
  font-weight: bold;
  margin-left: -2em;
}

span.keepspace {
  white-space: pre;
}

pre.lisp {
}

div.toc {
}

div.chapter {
  margin-bottom: 1em;
}

div.unnumbered {
  margin-bottom: 1em;
}

div.appendix {
  margin-bottom: 1em;
}

div.chapheading {
  margin-bottom: 1em;
}

div.center {
  text-align: center;
}

table.multitable {
}

table.indented {
  margin-left: 2em;
}

   </style>
 </head>

 <body text="#000000" bgcolor="#FFFFFF"> 
  <a name="top"/>

  <xsl:call-template name="banner"/>

  <!-- outer table starts -->
  <table cellspacing="0" cellpadding="5" border="0" summary="Page" width="100%">

   <tr>

    <xsl:call-template name="navigation"/>

    <td valign="top">
    <!-- content starts -->

      <xsl:call-template name="title_page"/>

      <xsl:apply-templates/>

    <!-- content ends -->
    </td>
   </tr>
  </table>

 </body>
</html>
</xsl:template>
<!-- }}} -->

<!-- {{{ useless tags -->
<xsl:template match="setfilename|settitle|document-title|document-author|document-package|document-font|itemfunction|columnfraction"/>
<!-- }}} -->

<!-- {{{ setting a default font for documents -->
<xsl:template name="document-font">
  <xsl:variable name="font" select="string(/texinfo/para/document-font)"/>
  <xsl:choose>
    <xsl:when test="$font=''"/>
    <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'">
      <xsl:text>font-family: sans-serif;</xsl:text>
    </xsl:when>
    <xsl:when test="$font='story' or $font='storystyle' or $font='serif'">
      <xsl:text>font-family: serif;</xsl:text>
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
    <div class="title_page">
      <xsl:if test="string-length(/texinfo/para/document-title) > 0">
	<div class="title_document_title">
	  <xsl:call-template name="big_title">
	    <xsl:with-param name="title" select="/texinfo/para/document-title"/>
	  </xsl:call-template>
	</div>
      </xsl:if>
      <xsl:if test="count(/texinfo/para/document-author) > 0">
	<div class="title_author">
	  <xsl:for-each select="/texinfo/para/document-author">
	    <xsl:apply-templates/>
	    <br/>
	  </xsl:for-each>
	</div>
      </xsl:if>
    </div>
  </xsl:if>
</xsl:template>
<!-- }}} -->

<!-- {{{ creating a banner at top -->
<xsl:template name="banner">
  <xsl:if test="string-length($banner) > 0 and not(substring-before($this_file, '.html')='')">
    <div align="center">
     <img border="0">
       <xsl:attribute name="src"><xsl:value-of select="concat($image_prefix, 'images/banner/', substring-before($this_file, '.html'), '.png')"/></xsl:attribute>
       <xsl:attribute name="alt"><xsl:value-of select="settitle"/></xsl:attribute>
     </img>
    </div>
  </xsl:if>
</xsl:template>
<!-- }}} -->

<!-- {{{ table of contents related stuff -->
<xsl:template name="node_number">
  <xsl:text>node-</xsl:text><xsl:number level="multiple" count="chapter|section|subsection|subsubsection|appendix|appendixsec|appendixsubsec|appendixsubsubsec|unnumbered|unnumberedsec|unnumberedsubsec|unnumberedsubsubsec" format="1-1-1-1"/>
</xsl:template>

<!-- Alper: fix this template by removing para tags when makeinfo is fixed -->
<xsl:template match="para/table-of-contents">
  <div class="toc">
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
  </div>
</xsl:template>

<xsl:template name="node_name">
  <a>
    <xsl:attribute name="name">
      <xsl:call-template name="node_number"/>
    </xsl:attribute>
  </a>
</xsl:template>

<xsl:template name="toc_chapter">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./section) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./section">
	<xsl:call-template name="toc_section"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_section">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./subsection) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./subsection">
	<xsl:call-template name="toc_subsection"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_subsection">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./subsubsection) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./subsubsection">
	<xsl:call-template name="toc_subsubsection"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_subsubsection">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
</xsl:template>

<xsl:template name="toc_appendix">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./appendixsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./appendixsec">
	<xsl:call-template name="toc_appendixsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_appendixsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./appendixsubsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./appendixsubsec">
	<xsl:call-template name="toc_appendixsubsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_appendixsubsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./appendixsubsubsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./appendixsubsubsec">
	<xsl:call-template name="toc_appendixsubsubsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_appendixsubsubsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
</xsl:template>

<xsl:template name="toc_unnumbered">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./unnumberedsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./unnumberedsec">
	<xsl:call-template name="toc_unnumberedsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_unnumberedsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./unnumberedsubsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./unnumberedsubsec">
	<xsl:call-template name="toc_unnumberedsubsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_unnumberedsubsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
  <xsl:if test="count(./unnumberedsubsubsec) > 0">
    <div style="padding-left: 1em;">
      <xsl:for-each select="./unnumberedsubsubsec">
	<xsl:call-template name="toc_unnumberedsubsubsec"/>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="toc_unnumberedsubsubsec">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <xsl:apply-templates select="title">
      <xsl:with-param name="toc" select="1"/>
    </xsl:apply-templates>
  </a><br/>
</xsl:template>
<!-- }}} -->

<!-- {{{ navigation -->
<xsl:template name="navigation">
  <xsl:if test="string-length($navigation) > 0">
    <td width="150" valign="top">
      <xsl:apply-templates select="document($navigation)"/>
    </td>
  </xsl:if>
</xsl:template>

<xsl:template match="/navigation//node">
  <xsl:param name="depth" select="''"/>
  <xsl:value-of disable-output-escaping="yes" select="$depth"/>
  <xsl:choose>
    <xsl:when test="string-length(@target) > 0 and @target != $this_file">
      <a>
        <xsl:attribute name="href">
	  <xsl:value-of select="@target"/>
	</xsl:attribute>
	<xsl:call-template name="navigation-image"/>
      </a>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="navigation-image"/>
    </xsl:otherwise>
  </xsl:choose>
  <br/>
  <xsl:if test="count(./*) > 0">
    <xsl:apply-templates>
      <xsl:with-param name="depth" select="concat($depth, '&sp;&sp;&sp;')"/>
    </xsl:apply-templates>
  </xsl:if>
</xsl:template>

<xsl:template name="navigation-image">
  <xsl:choose>
    <xsl:when test="string-length(@image) > 0">
      <img border="0">
	<xsl:attribute name="src">
	  <xsl:value-of select="concat($image_prefix, @image)"/>
	</xsl:attribute>
	<xsl:attribute name="alt">
	  <xsl:value-of select="@title"/>
	</xsl:attribute>
      </img>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="@title"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="up-link">
  <xsl:if test="string-length($uplinks) > 0">
    <a href="#top">
      <img border="0" alt="Up">
        <xsl:attribute name="src"><xsl:value-of select="concat($image_prefix, 'images/nav/up.png')"/></xsl:attribute>
      </img>
    </a>
  </xsl:if>
</xsl:template>
<!-- }}} -->

<!-- {{{ document sections -->
<xsl:template match="chapter|unnumbered|appendix">
  <div>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
    <xsl:call-template name="up-link"/>
  </div>
</xsl:template>

<xsl:template match="chapheading|majorheading">
  <div class="chapheading">
    <xsl:apply-templates/>
    <xsl:call-template name="up-link"/>
  </div>
</xsl:template>
<!-- }}} -->

<!-- {{{ highest level titles, ie. chapter, unnumbered, etc. -->
<xsl:template name="big_title">
  <xsl:param name="node"></xsl:param>
  <div class="banner">
    <xsl:if test="string-length($node) > 0">
      <a>
	<xsl:attribute name="name">
	  <xsl:value-of select="$node"/>
	</xsl:attribute>
      </a>
    </xsl:if>
    <h1 class="banner">
      <xsl:value-of select="$title"/>
    </h1>
  </div>
</xsl:template>
<!-- }}} -->

<!-- {{{ section titles stuff -->
<xsl:template match="chapter/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_chapter">
	  <xsl:number count="chapter" format="1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="big_title">
	<xsl:with-param name="title">
	  <xsl:number count="chapter" format="1 - "/><xsl:apply-templates/>
	</xsl:with-param>
	<xsl:with-param name="node">
	  <xsl:call-template name="node_number"/>
	</xsl:with-param>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="section/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_section">
	  <xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h2>
	<xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:apply-templates/>
      </h2>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="subsection/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsection">
	  <xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h3>
	<xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:apply-templates/>
      </h3>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="subsubsection/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsubsection">
	  <xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendix/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_chapter">
	  <xsl:text>Appendix </xsl:text>
	  <xsl:number count="appendix" format="A - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="big_title">
	<xsl:with-param name="title">
	  <xsl:number count="appendix" format="A - "/><xsl:apply-templates/>
	</xsl:with-param>
	<xsl:with-param name="node">
	  <xsl:call-template name="node_number"/>
	</xsl:with-param>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendixsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_section">
	  <xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h2>
	<xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:apply-templates/>
      </h2>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendixsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsection">
	  <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h3>
	<xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:apply-templates/>
      </h3>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendixsubsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsubsection">
	  <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="unnumbered/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_chapter">
	  <xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="big_title">
	<xsl:with-param name="title">
	  <xsl:apply-templates/>
	</xsl:with-param>
	<xsl:with-param name="node">
	  <xsl:call-template name="node_number"/>
	</xsl:with-param>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="unnumberedsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_section">
	  <xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h2>
	<xsl:apply-templates/>
      </h2>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="unnumberedsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsection">
	  <xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h3>
	<xsl:apply-templates/>
      </h3>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="unnumberedsubsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <strong>
        <span class="toc_subsubsection">
	  <xsl:apply-templates/>
	</span>
      </strong>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="chapheading/title|majorheading/title">
  <xsl:call-template name="big_title">
    <xsl:with-param name="title">
      <xsl:apply-templates/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="heading/title">
  <h2>
    <xsl:apply-templates/>
  </h2>
</xsl:template>

<xsl:template match="subheading/title">
  <h3>
    <xsl:apply-templates/>
  </h3>
</xsl:template>

<xsl:template match="subsubheading/title">
  <h4>
    <xsl:apply-templates/>
  </h4>
</xsl:template>
<!-- }}} -->

<!-- {{{ reference generation -->
<xsl:template match="reference-function|reference-parameter|reference-returns|reference-type|reference-blurb|reference-struct-name">
  <span>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </span>
</xsl:template>

<xsl:template match="reference-struct-open">
  <span class="reference-struct"> {</span>
</xsl:template>

<xsl:template match="reference-struct-close">
  <span class="reference-struct-close">};</span>
</xsl:template>
<!-- }}} -->

<!-- {{{ paragraphs -->
<xsl:template match="para">
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="count(document-font|document-title|document-author)"/>
    <!-- <xsl:when test="count(reference-function|reference-struct-name)"><breakline/></xsl:when> -->
    <xsl:otherwise><br/><br/></xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ line breaks, forced spaces -->
<xsl:template match="linebreak">
  <br/>
</xsl:template>

<xsl:template match="space"><xsl:text disable-output-escaping="yes">&sp;</xsl:text></xsl:template>
<!-- }}} -->

<!-- {{{ contextual tags -->
<xsl:template match="acronym">
  <acronym><xsl:apply-templates/></acronym>
</xsl:template>

<xsl:template match="cite">
  <cite><xsl:apply-templates/></cite>
</xsl:template>

<xsl:template match="dfn">
  <dfn><xsl:apply-templates/></dfn>
</xsl:template>

<xsl:template match="kbd">
  <kbd><xsl:apply-templates/></kbd>
</xsl:template>

<xsl:template match="samp">
  <samp><xsl:apply-templates/></samp>
</xsl:template>

<xsl:template match="var">
  <var><xsl:apply-templates/></var>
</xsl:template>

<xsl:template match="emph">
  <em><xsl:apply-templates/></em>
</xsl:template>

<xsl:template match="strong">
  <strong><xsl:apply-templates/></strong>
</xsl:template>

<xsl:template match="url|email|key|env|file|command|option">
  <code>
    <span>
      <xsl:attribute name="class">
        <xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
  </code>
</xsl:template>

<xsl:template match="menupath">
  <strong><span class="menupath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="pagepath">
  <strong><span class="pagepath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="object">
  <em><code><span class="object"><xsl:apply-templates/></span></code></em>
</xsl:template>

<xsl:template match="channel">
  <em><span class="channel"><xsl:apply-templates/></span></em>
</xsl:template>

<xsl:template match="important">
  <em><strong><span class="important"><xsl:apply-templates/></span></strong></em>
</xsl:template>

<xsl:template match="code">
  <code><xsl:apply-templates/></code>
</xsl:template>

<xsl:template match="property">
  "<em><span class="property"><xsl:apply-templates/></span></em>"
</xsl:template>

<xsl:template match="center">
  <div class="center" align="center"><xsl:apply-templates/></div>
</xsl:template>

<xsl:template match="example|display|format|lisp">
  <pre>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </pre>
</xsl:template>

<xsl:template match="keepspace">
  <span class="keepspace"><xsl:apply-templates/></span>
</xsl:template>
<!-- }}} -->

<!-- {{{ enumeration and itemization handling -->
<xsl:template match="enumerate">
  <ol>
    <xsl:apply-templates/>
  </ol>
</xsl:template>

<xsl:template match="itemize">
  <ul>
    <xsl:apply-templates/>
  </ul>
</xsl:template>

<xsl:template match="itemize/item|enumerate/item">
  <li>
    <xsl:apply-templates/>
  </li>
</xsl:template>
<!-- }}} -->

<!-- {{{ parsing and printing urefs according to their protocols -->
<xsl:template match="uref">
  <!-- protocol for this link type -->
  <xsl:variable name="protocol" select="substring-before(urefurl, '://')"/>
  <xsl:if test="$protocol=''">
    <xsl:message terminate="yes">XSL-ERROR: unset protocol for <xsl:value-of select="urefurl"/></xsl:message>
  </xsl:if>

  <!-- actual link -->
  <xsl:variable name="url" select="substring-after(urefurl, '://')"/>

  <!-- feedback -->
  <!-- <xsl:message>DEBUG: protocol is <xsl:value-of select="$protocol"/> for <xsl:value-of select="urefurl"/></xsl:message> -->

  <xsl:choose>
    <!-- PROTOCOL: HTTP FTP FILE -->
    <xsl:when test="$protocol='http' or $protocol='ftp' or $protocol='file'">
      <a>
	<xsl:attribute name="href">
	  <xsl:value-of select="concat($protocol, '://', $url)"/>
	</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($protocol, '://', $url)"/>)</xsl:when>
	  <xsl:otherwise><xsl:value-of select="concat($protocol, '://', $url)"/></xsl:otherwise>
	</xsl:choose>
      </a>
    </xsl:when>
    <!-- PROTOCOL: MAILTO NEWS -->
    <xsl:when test="$protocol='mailto' or $protocol='news'">
      <a>
	<xsl:attribute name="href">
	  <xsl:value-of select="concat($protocol, ':', $url)"/>
	</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	  <xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
	</xsl:choose>
      </a>
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
      <xsl:choose>
	<xsl:when test="$protocol='man'">
	  <!-- Print System Man Page (it's not a link) -->
	  <xsl:choose>
	    <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	    <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($page, '(', $section, ')')"/>)</xsl:when>
	    <xsl:otherwise><xsl:value-of select="concat($page, '(', $section, ')')"/></xsl:otherwise>
	  </xsl:choose>
	</xsl:when>
	<xsl:when test="$protocol='beast-man'">
	  <!-- Print BEAST Man Page (it's a link) -->
	  <a>
	    <xsl:attribute name="href">
	      <xsl:value-of select="concat($page, '.', $section, '.html')"/>
	    </xsl:attribute>
	    <xsl:choose>
	      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($page, '.', $section, '.html')"/>)</xsl:when>
	      <xsl:otherwise><xsl:value-of select="concat($page, '.', $section, '.html')"/></xsl:otherwise>
	    </xsl:choose>
	  </a>
	</xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- PROTOCOL: Beast Document -->
    <xsl:when test="$protocol='beast-doc'">
      <!-- Get the file name and append the target specific extension (html) -->
      <xsl:variable name="filename">
	<xsl:choose>
	  <xsl:when test="substring-before($url, '#') = ''">
	    <xsl:value-of select="concat($url, '.html')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(substring-before($url, '#'), '.html')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Get the anchor -->
      <xsl:variable name="anchor">
	<xsl:choose>
	  <xsl:when test="substring-after($url, '#') = ''"/>
	  <xsl:otherwise>
	    <xsl:value-of select="concat('#', substring-after($url, '#'))"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:variable>
      <!-- Print the link -->
      <a>
	<xsl:attribute name="href">
	  <xsl:value-of select="concat($filename, $anchor)"/>
	</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($filename, $anchor)"/>)</xsl:when>
	  <xsl:otherwise><xsl:value-of select="concat($filename, $anchor)"/></xsl:otherwise>
	</xsl:choose>
      </a>
    </xsl:when>
    <!-- Unknown Protocol -->
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: unknown protocol '<xsl:value-of select="$protocol"/>' in <xsl:value-of select="urefurl"/>, using as-is</xsl:message>
      <a>
	<xsl:attribute name="href">
	  <xsl:value-of select="urefurl"/>
	</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="urefurl"/>)</xsl:when>
	  <xsl:otherwise><xsl:value-of select="urefurl"/></xsl:otherwise>
	</xsl:choose>
      </a>
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
      <xsl:value-of select="concat($image_prefix, .)"/><xsl:text>.</xsl:text><xsl:value-of select="@extension"/>
    </xsl:attribute>
  </img>
</xsl:template>
<!-- }}} -->

<!-- {{{ table handling -->

<!-- {{{ simple definition titles -->
<xsl:template match="table">
  <dl>
    <xsl:apply-templates/>
  </dl>
</xsl:template>

<xsl:template match="tableterm">
  <dt>
    <xsl:apply-templates/>
  </dt>
</xsl:template>

<xsl:template match="tableitem/item">
  <dd>
    <xsl:apply-templates/>
  </dd>
</xsl:template>
<!-- }}} -->

<!-- {{{ multicolumn tables -->
<xsl:template match="multitable">
  <table summary="">
    <xsl:attribute name="class">
      <xsl:choose>
	<xsl:when test="local-name(..)='item'">indented</xsl:when>
	<xsl:otherwise>multitable</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:apply-templates/>
  </table>
</xsl:template>

<xsl:template match="multitable/row">
  <tr>
    <xsl:apply-templates/>
  </tr>
</xsl:template>

<xsl:template match="multitable/row/entry">
  <td style="padding-right: 2em;">
    <xsl:apply-templates/>
  </td>
</xsl:template>
<!-- }}} -->

<!-- }}} -->

<!-- {{{ indice generation -->
<xsl:template match="indexterm">
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="@index"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
    </xsl:attribute>
  </a>
</xsl:template>

<xsl:template match="printindex">
  <xsl:variable name="type" select="."/>
  <div class="index">
    <table summary="index" width="80%" border="0">
      <thead>
        <tr>
	  <td><strong>Name</strong></td>
	  <td><strong>Section</strong></td>
	</tr>
      </thead>
      <tbody>
	<xsl:for-each select="//indexterm[@index=$type]">
	  <xsl:sort/>
	  <tr>
	    <td width="40%">
	      <a>
		<xsl:attribute name="href">
		  <xsl:text>#</xsl:text><xsl:value-of select="$type"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
		</xsl:attribute>
		<xsl:apply-templates/>
	      </a>
	    </td>
	    <td>
	      <xsl:value-of select="../../title"/>
	    </td>
	  </tr>
	</xsl:for-each>
      </tbody>
    </table>
  </div>
</xsl:template>

<xsl:template match="para/printplainindex">
  <xsl:variable name="type" select="."/>
  <div class="index">
    <xsl:for-each select="//indexterm[@index=$type]">
      <xsl:sort/>
      <a>
	<xsl:attribute name="href">
	  <xsl:text>#</xsl:text><xsl:value-of select="$type"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
	</xsl:attribute>
	<xsl:apply-templates/>
      </a>
      <br/>
    </xsl:for-each>
  </div>
</xsl:template>
<!-- }}} -->

<!-- {{{ news items for the website -->
<xsl:template match="para/news-date">
  <span class="news-date">
    <xsl:apply-templates/>
  </span>
  <br/>
</xsl:template>

<xsl:template match="para/news-title">
  <strong>
    <span class="news-title">
      <xsl:apply-templates/>
    </span>
  </strong>
  <br/>
</xsl:template>
<!-- }}} -->

</xsl:stylesheet>
<!-- vim: set fdm=marker: -->
