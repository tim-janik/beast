<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" indent="yes" charset="UTF-8" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"/>

<xsl:param name="revision"/>
<xsl:param name="banner"/>
<xsl:param name="navigation"/>
<xsl:param name="base_href"/>

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

div.banner {
  background-color: #005d5d;
  padding: 3px 5px;
  margin-bottom: 1em;
  width: 100%;
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

span.keepspace {
  white-space: pre;
}

span.revision {
  font-style: italic;
}

span.tableterm {
}

pre.programlisting {
}

div.toc {
  margin-bottom: 2em;
}

div.chapter {
  margin-bottom: 2em;
}

div.unnumbered {
  margin-bottom: 2em;
}

div.appendix {
  margin-bottom: 2em;
}

div.chapheading {
  margin-bottom: 2em;
}

div.preformat {
  white-space: nowrap;
}

div.center {
  text-align: center;
}

div.table {
}

table.multitable {
}

p.tableitem {
  margin: 0px;
  margin-bottom: 1em;
  padding: 0px;
  padding-left: 4em;
}
   </style>
   <xsl:call-template name="base_href"/>
 </head>

 <body text="#000000" bgcolor="#FFFFFF"> 

  <xsl:call-template name="banner"/>

  <!-- outer table starts -->
  <table cellspacing="0" cellpadding="5" border="0" summary="Page" width="100%">

   <tr>

    <xsl:call-template name="navigation"/>

    <td valign="top">
    <!-- content starts -->

      <xsl:apply-templates/>

    <!-- content ends -->
    </td>
   </tr>
  </table>

 </body>
</html>
</xsl:template>

<xsl:template match="setfilename|settitle|itemfunction|columnfraction"/>

<xsl:template name="base_href">
  <xsl:if test="string-length($base_href) > 0">
    <base>
      <xsl:attribute name="href">
	<xsl:value-of select="$base_href"/>
      </xsl:attribute>
    </base>
  </xsl:if>
</xsl:template>

<xsl:template name="banner">
  <xsl:if test="string-length($banner) > 0">
    <div align="center">
     <img border="0">
       <xsl:attribute name="src"><xsl:value-of select="$banner"/></xsl:attribute>
       <xsl:attribute name="alt"><xsl:value-of select="settitle"/></xsl:attribute>
     </img>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="navigation">
  <xsl:if test="string-length($navigation) > 0">
    <td width="150" valign="top">
     <table cellspacing="0" cellpadding="5" border="0" summary="">
      <tr>
       <td>
	<a>
	  <xsl:attribute name="href">
	    <xsl:value-of select="$base_href"/>
	  </xsl:attribute>
	  <img src="images/home.png" alt="Home" border="0" width="67" height="46" />
	</a>
       </td>
      </tr>
     </table>
    </td>
  </xsl:if>
</xsl:template>

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

<xsl:template match="para/revision">
  <xsl:choose>
    <xsl:when test="string-length($revision) > 0">
      <em><span class="revision"><xsl:text>Document revised: </xsl:text><xsl:value-of select="$revision"/></span></em>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>XSL-WARNING: Skipping Document Revision line, revision date not provided.</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="node_number">
  <xsl:text>node-</xsl:text><xsl:number level="multiple" count="chapter|section|subsection|subsubsection|appendix|appendixsec|appendixsubsec|appendixsubsubsec|unnumbered|unnumberedsec|unnumberedsubsec|unnumberedsubsubsec" format="1-1-1-1"/>
</xsl:template>

<!-- Alper: fix this template by removing para tags when makeinfo is fixed -->
<xsl:template match="para/table-of-contents">
  <xsl:call-template name="big_title">
    <xsl:with-param name="title">Table of Contents</xsl:with-param>
    <xsl:with-param name="node"><xsl:text>toc-</xsl:text><xsl:number count="para"/></xsl:with-param>
  </xsl:call-template>

  <div class="toc">
    <xsl:for-each select="/texinfo/chapter|/texinfo/unnumbered|/texinfo/appendix">
      <xsl:if test="local-name() = 'chapter'">
	<xsl:call-template name="toc_chapter"/>
      </xsl:if>
      <xsl:if test="local-name() = 'unnumbered'">
	<xsl:call-template name="toc_unnumbered"/>
      </xsl:if>
      <xsl:if test="local-name() = 'appendix'">
	<xsl:call-template name="toc_appendix"/>
      </xsl:if>
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
    <strong><span class="toc_chapter"><xsl:number format="1 - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_section"><xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsection"><xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsubsection"><xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:value-of select="title"/></span></strong>
  </a><br/>
</xsl:template>

<xsl:template name="toc_appendix">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <strong><span class="toc_chapter"><xsl:text>Appendix </xsl:text><xsl:number format="A - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_section"><xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsection"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsubsection"><xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:value-of select="title"/></span></strong>
  </a><br/>
</xsl:template>

<xsl:template name="toc_unnumbered">
  <a>
    <xsl:attribute name="href">
      <xsl:text>#</xsl:text><xsl:call-template name="node_number"/>
    </xsl:attribute>
    <strong><span class="toc_chapter"><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_section"><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsection"><xsl:value-of select="title"/></span></strong>
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
    <strong><span class="toc_subsubsection"><xsl:value-of select="title"/></span></strong>
  </a><br/>
</xsl:template>

<xsl:template match="chapter/title">
  <xsl:call-template name="big_title">
    <xsl:with-param name="title">
      <xsl:number count="chapter" format="1 - "/><xsl:apply-templates/>
    </xsl:with-param>
    <xsl:with-param name="node">
      <xsl:call-template name="node_number"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="section/title">
  <xsl:call-template name="node_name"/>
  <h2>
    <xsl:number level="multiple" count="chapter|section" format="1.1 - "/><xsl:apply-templates/>
  </h2>
</xsl:template>

<xsl:template match="subsection/title">
  <xsl:call-template name="node_name"/>
  <h3>
    <xsl:number level="multiple" count="chapter|section|subsection" format="1.1.1 - "/><xsl:apply-templates/>
  </h3>
</xsl:template>

<xsl:template match="subsubsection/title">
  <xsl:call-template name="node_name"/>
  <h4>
    <xsl:number level="multiple" count="chapter|section|subsection|subsubsection" format="1.1.1.1 - "/><xsl:apply-templates/>
  </h4>
</xsl:template>

<xsl:template match="appendix/title">
  <xsl:call-template name="big_title">
    <xsl:with-param name="title">
      <xsl:number count="appendix" format="A - "/><xsl:apply-templates/>
    </xsl:with-param>
    <xsl:with-param name="node">
      <xsl:call-template name="node_number"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="appendixsec/title">
  <xsl:call-template name="node_name"/>
  <h2>
    <xsl:number level="multiple" count="appendix|appendixsec" format="A.1 - "/><xsl:apply-templates/>
  </h2>
</xsl:template>

<xsl:template match="appendixsubsec/title">
  <xsl:call-template name="node_name"/>
  <h3>
    <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec" format="A.1.1 - "/><xsl:apply-templates/>
  </h3>
</xsl:template>

<xsl:template match="appendixsubsubsec/title">
  <xsl:call-template name="node_name"/>
  <h4>
    <xsl:number level="multiple" count="appendix|appendixsec|appendixsubsec|appendixsubsubsec" format="A.1.1.1 - "/><xsl:apply-templates/>
  </h4>
</xsl:template>

<xsl:template match="unnumbered/title">
  <xsl:call-template name="big_title">
    <xsl:with-param name="title">
      <xsl:apply-templates/>
    </xsl:with-param>
    <xsl:with-param name="node">
      <xsl:call-template name="node_number"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template match="unnumberedsec/title">
  <xsl:call-template name="node_name"/>
  <h2>
    <xsl:apply-templates/>
  </h2>
</xsl:template>

<xsl:template match="unnumberedsubsec/title">
  <xsl:call-template name="node_name"/>
  <h3>
    <xsl:apply-templates/>
  </h3>
</xsl:template>

<xsl:template match="unnumberedsubsubsec/title">
  <xsl:call-template name="node_name"/>
  <h4>
    <xsl:apply-templates/>
  </h4>
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

<xsl:template match="chapter|unnumbered|appendix">
  <div>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="chapheading|majorheading">
  <div class="chapheading">
    <xsl:apply-templates/>
  </div>
</xsl:template>

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

<xsl:template match="multitable">
  <table class="multitable" summary="">
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

<xsl:template match="table">
  <div class="table">
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="tableterm">
  <span class="tableterm"><xsl:apply-templates/></span><br/>
</xsl:template>

<xsl:template match="tableitem/item/para">
  <p class="tableitem"><xsl:apply-templates/></p>
</xsl:template>

<xsl:template match="para">
  <xsl:choose>
    <!-- If this para is the parent of a revision or toc tag, then we -->
    <!-- omit the <p> tag in output -->
    <xsl:when test="count(./revision) > 0 or count(./table-of-contents) > 0">
      <xsl:apply-templates/>
    </xsl:when>
    <!-- Paragrapgh is bogus (ie. white-space only), skip it -->
    <xsl:when test="normalize-space(.) = ''"/>
    <xsl:otherwise>
      <p><xsl:apply-templates/></p>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="uref">
  <a>
    <xsl:attribute name="href">
      <xsl:if test="contains(urefurl, '@') and not(contains(substring-before(urefurl, '@'),':'))">
	<xsl:text>mailto:</xsl:text>
      </xsl:if><xsl:value-of select="urefurl"/>
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:apply-templates select="urefurl"/>)</xsl:when>
      <xsl:otherwise><xsl:apply-templates select="urefurl"/></xsl:otherwise>
    </xsl:choose>
  </a>
</xsl:template>

<!-- Untested ftuff -->

<xsl:template match="code">
  <code><xsl:apply-templates/></code>
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

<xsl:template match="dfn">
  <dfn><xsl:apply-templates/></dfn>
</xsl:template>

<xsl:template match="cite">
  <cite><xsl:apply-templates/></cite>
</xsl:template>

<xsl:template match="acronym">
  <acronym><xsl:apply-templates/></acronym>
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

<xsl:template match="programlisting">
  <pre class="programlisting"><xsl:apply-templates/></pre>
</xsl:template>

<xsl:template match="menupath">
  <strong><span class="menupath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="pagepath">
  <strong><span class="pagepath"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="property">
  "<em><span class="property"><xsl:apply-templates/></span></em>"
</xsl:template>

<xsl:template match="channel">
  <em><span class="channel"><xsl:apply-templates/></span></em>
</xsl:template>

<xsl:template match="object">
  <em><code><span class="object"><xsl:apply-templates/></span></code></em>
</xsl:template>

<xsl:template match="emph|emphasize">
  <em><span class="emph"><xsl:apply-templates/></span></em>
</xsl:template>

<xsl:template match="strong">
  <strong><span class="strong"><xsl:apply-templates/></span></strong>
</xsl:template>

<xsl:template match="important">
  <em><u><span class="important"><xsl:apply-templates/></span></u></em>
</xsl:template>

<xsl:template match="preformat">
  <br/>
    <pre><xsl:apply-templates/></pre>
  <br/>
</xsl:template>

<xsl:template match="keepspace">
  <span class="keepspace"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="center">
  <div class="center" align="center"><xsl:apply-templates/></div>
</xsl:template>

<xsl:template match="linebreak">
  <br/>
</xsl:template>

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
      <xsl:value-of select="."/><xsl:text>.</xsl:text><xsl:value-of select="@extension"/>
    </xsl:attribute>
  </img>
</xsl:template>

<xsl:template match="indexterm">
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="@index"/><xsl:text>index-</xsl:text><xsl:number/>
    </xsl:attribute>
  </a>
</xsl:template>

<xsl:template match="printindex">
</xsl:template>

</xsl:stylesheet>
