<?xml version="1.0"?>
<!DOCTYPE html-stylesheet [
  <!ENTITY sp "&amp;nbsp;">
]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" indent="yes" encoding="ISO-8859-1" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" doctype-system="http://www.w3.org/TR/html4/loose.dtd" />
<!-- <xsl:strip-space elements="*"/> -->
<xsl:preserve-space elements="code display smalldisplay format smallformat example smallexample lisp smalllisp"/>

<xsl:param name="images_prefix" select="''"/>

<!-- {{{ start parsing -->
<xsl:template match="texinfo">
<html>
 <head>
  <meta http-equiv="Default-Style" content="Default"/>
  <style type="text/css" media="all">
    body { background-color: White; color: Black; <xsl:call-template name="document-font"/> }
    <!-- This is here because 'inherit' is not a valid CSS1 value -->
    span.default { <xsl:call-template name="document-font"/> }
  </style>
  <style type="text/css" media="all" title="Default">
    @import 'css/default.css';
  </style>
  <title><xsl:value-of select="settitle"/></title>
 </head>
 <!-- Stupid bgcolor attribute to override default Netscape 4 background color :\ -->
 <!-- The id is for user-agent side site specific CSS overriding goodness,
      ie. #beast-gtk-org { font-size: 10px !important; } -->
 <body id="beast-gtk-org" bgcolor="White">
  <xsl:call-template name="banner"/>
  <xsl:call-template name="navigation"/>
  <div id="content">
   <xsl:call-template name="document-size"/>

   <xsl:call-template name="home-link"/>
   <xsl:call-template name="title_page"/>

   <xsl:apply-templates/>

   <xsl:call-template name="home-link"/>
  </div>
 </body>
</html>
</xsl:template>

<xsl:template name="document-size">
  <xsl:choose>
    <!-- the banner is large and there is no navigation stuff on the left -->
    <xsl:when test="string(/texinfo/para/document-hasbanner) = 'large' and string(/texinfo/para/document-navigation) = ''">
      <xsl:attribute name="class">with-banner-nonav</xsl:attribute>
    </xsl:when>
    <!-- the banner is large and there is also navigation stuff -->
    <xsl:when test="string(/texinfo/para/document-hasbanner) = 'large' and string(/texinfo/para/document-navigation) != ''">
      <xsl:attribute name="class">with-banner-nav</xsl:attribute>
    </xsl:when>
    <!-- the banner is small and there is navigation on the left -->
    <xsl:when test="string(/texinfo/para/document-hasbanner) != 'large' and string(/texinfo/para/document-navigation) != ''">
      <xsl:attribute name="class">with-nobanner-nav</xsl:attribute>
    </xsl:when>
    <!-- Otherwise: banner is small and no navigation. largest content size -->
    <xsl:otherwise>
      <xsl:attribute name="class">with-nobanner-nonav</xsl:attribute>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ useless tags -->
<xsl:template match="setfilename|settitle|document-title|document-author|document-package|document-font|document-navigation|document-hasbanner|itemfunction|columnfraction"/>
<!-- }}} -->

<!-- {{{ setting a default font for documents -->
<xsl:template name="document-font">
  <xsl:variable name="font" select="string(/texinfo/para/document-font)"/>
  <xsl:choose>
    <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'">
      <xsl:text>font-family: sans-serif;</xsl:text>
    </xsl:when>
    <xsl:when test="$font='' or $font='story' or $font='storystyle' or $font='serif'">
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
  <!-- We put a document title only if it does not have large banner -->
  <xsl:if test="string(/texinfo/para/document-hasbanner) != 'large' and count(/texinfo/para/document-title) > 0">
    <h1 class="document-title">
      <xsl:call-template name="document-title"/>
    </h1>
  </xsl:if>
  <xsl:if test="count(/texinfo/para/document-author) > 0">
    <div class="document-author">
      <xsl:for-each select="/texinfo/para/document-author">
	<xsl:apply-templates/>
	<xsl:if test="position()!=last()"><br/></xsl:if>
      </xsl:for-each>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template name="document-title">
  <!-- prefer document-title over settitle if one is available -->
  <xsl:choose>
    <xsl:when test="count(/texinfo/para/document-title) > 0">
      <xsl:for-each select="/texinfo/para/document-title">
	<xsl:apply-templates/>
      </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="/texinfo/settitle"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ creating a banner at top -->
<xsl:template name="banner">
  <xsl:choose>
    <xsl:when test="string(/texinfo/para/document-hasbanner) = 'large'">
      <div id="bigbanner">
	<a name="_top"/>
	<h1 id="bannertitle">
	  <xsl:call-template name="document-title"/>
	</h1>
      </div>
      <div id="bannerleft"/>
    </xsl:when>
    <xsl:otherwise>
      <div id="bannerright"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ table of contents related stuff -->
<xsl:template name="node_number">
  <xsl:text>node-</xsl:text>
  <xsl:number level="any" count="chapter|appendix|unnumbered" />
  <xsl:text>-</xsl:text>
  <xsl:number level="any" count="section|appendixsec|unnumberedsec" from="chapter|appendix|unnumbered" />
  <xsl:text>-</xsl:text>
  <xsl:number level="any" count="subsection|appendixsubsec|unnumberedsubsec" from="section|appendixsec|unnumberedsec" />
  <xsl:text>-</xsl:text>
  <xsl:number level="any" count="subsubsection|appendixsubsubsec|unnumberedsubsubsec" from="subsection|appendixsubsec|unnumberedsubsec" />
</xsl:template>

<!-- Alper: fix this template by removing para tags when makeinfo is fixed -->
<xsl:template match="para/table-of-contents">
  <div class="toc">
    <xsl:for-each select="//chapter|//section|//subsection|//subsubsection|//appendix|//appendixsec|//appendixsubsec|//appendixsubsubsec|//unnumbered|//unnumberedsec|//unnumberedsubsec|//unnumberedsubsubsec">
      <!-- easy access to local-name() -->
      <xsl:variable name="n" select="local-name()" />
      <xsl:choose>
        <!-- chapter/appendix/unnumbered -->
	<xsl:when test="$n='chapter' or $n='appendix' or $n='unnumbered'">
	  <xsl:call-template name="toc_chapter"/>
	</xsl:when>
        <!-- section/appendixsec/unnumberedsec -->
	<xsl:when test="$n='section' or $n='appendixsec' or $n='unnumberedsec'">
	  <xsl:call-template name="toc_section"/>
	</xsl:when>
        <!-- subsection/appendixsubsec/unnumberedsubsec -->
	<xsl:when test="$n='subsection' or $n='appendixsubsec' or $n='unnumberedsubsec'">
	  <xsl:call-template name="toc_subsection"/>
	</xsl:when>
        <!-- subsubsection/appendixsubsubsec/unnumberedsubsubsec -->
	<xsl:when test="$n='subsubsection' or $n='appendixsubsubsec' or $n='unnumberedsubsubsec'">
	  <xsl:call-template name="toc_subsubsection"/>
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

<!-- }}} -->

<!-- {{{ navigation -->

<!-- Now it's handled outside this stylesheet, so that HTML that's generated
     without xsltproc can make use of it too -->

<xsl:include href="navigation.xsl" />
<!-- }}} -->

<!-- {{{ document sections -->
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

  <!-- }}} -->

<!-- {{{ highest level titles, ie. chapter, unnumbered, etc. -->
<xsl:template name="big_title">
  <xsl:param name="node"/>
  <xsl:param name="title"/>
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

<!-- {{{ chapters -->
<xsl:template match="chapter/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-chapter">
	<xsl:call-template name="title_chapter_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="big_title">
	<xsl:with-param name="title">
	  <xsl:call-template name="title_chapter_number"/> - <xsl:apply-templates/>
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
      <span class="toc-section">
	<xsl:call-template name="title_section_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h2>
	<xsl:call-template name="title_section_number"/> - <xsl:apply-templates/>
      </h2>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="subsection/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-subsection">
	<xsl:call-template name="title_subsection_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h3>
	<xsl:call-template name="title_subsection_number"/> - <xsl:apply-templates/>
      </h3>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="subsubsection/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-subsubsection">
	<xsl:call-template name="title_subsubsection_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:call-template name="title_subsubsection_number"/> - <xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ appendices -->
<xsl:template match="appendix/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-chapter">
	<xsl:call-template name="title_appendix_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="big_title">
	<xsl:with-param name="title">
	  <xsl:call-template name="title_appendix_number"/> - <xsl:apply-templates/>
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
      <span class="toc-section">
	<xsl:call-template name="title_appendixsec_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h2>
	<xsl:call-template name="title_appendixsec_number"/> - <xsl:apply-templates/>
      </h2>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendixsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-subsection">
	<xsl:call-template name="title_appendixsubsec_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h3>
	<xsl:call-template name="title_appendixsubsec_number"/> - <xsl:apply-templates/>
      </h3>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendixsubsubsec/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-subsubsection">
	<xsl:call-template name="title_appendixsubsubsec_number"/> - <xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:call-template name="title_appendixsubsubsec_number"/> - <xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ unnumbered -->
<xsl:template match="unnumbered/title">
  <xsl:param name="toc" select="0"/>
  <xsl:choose>
    <xsl:when test="$toc">
      <span class="toc-chapter">
	<xsl:apply-templates/>
      </span>
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
      <span class="toc-section">
	<xsl:apply-templates/>
      </span>
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
      <span class="toc-subsection">
	<xsl:apply-templates/>
      </span>
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
      <span class="toc-subsubsection">
	<xsl:apply-templates/>
      </span>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="node_name"/>
      <h4>
	<xsl:apply-templates/>
      </h4>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

<!-- {{{ other -->
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

<!-- }}} -->

<!-- {{{ reference generation -->
<xsl:template match="reference-function|reference-parameter|reference-constant|reference-returns|reference-type|reference-blurb|reference-struct-name|reference-struct-type">
  <code>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </code>
</xsl:template>

<xsl:template match="reference-struct-open">
  <code class="reference-struct"> {</code>
</xsl:template>

<xsl:template match="reference-struct-close">
  <code class="reference-struct-close">};</code>
</xsl:template>
<!-- }}} -->

<!-- {{{ paragraphs -->
<xsl:template match="para">
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="count(document-font|document-title|document-author|document-navigation|document-hasbanner)"/>
    <xsl:when test="count(news-title|news-date)"/>
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
<xsl:template match="acronym|cite|dfn|kbd|samp|var|strong">
  <xsl:variable name="tag" select="local-name()"/>
  <xsl:element name="{$tag}">
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="emph">
  <em><xsl:apply-templates/></em>
</xsl:template>

<xsl:template match="code|url|email|key|env|file|command|option|menupath|pagepath|object|channel|changelog-entry|changelog-item">
  <code>
    <xsl:attribute name="class">
      <xsl:value-of select="local-name()"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </code>
</xsl:template>

<xsl:template match="property">
  "<code class="property"><xsl:apply-templates/></code>"
</xsl:template>

<xsl:template match="important">
  <strong class="important"><xsl:apply-templates/></strong>
</xsl:template>

<xsl:template match="center">
  <div class="center"><xsl:apply-templates/></div>
</xsl:template>

<xsl:template match="quotation">
  <blockquote>
    <xsl:apply-templates/>
  </blockquote>
</xsl:template>

<xsl:template match="example|smallexample|display|smalldisplay|format|smallformat|lisp|smalllisp">
  <div>
    <!-- FIXME Alper, until the makeinfo's @verbatim support is fixed, @example blocks
         will not be indented. This way we emulate @verbatim blocks with @example blocks
	 and indent the real @example blocks ourselves. -->
    <xsl:if test="not(local-name()='format' or local-name()='smallformat' or local-name()='example' or local-name()='smallexample')">
      <xsl:attribute name="class">indented-block</xsl:attribute>
    </xsl:if>
    <pre>
      <xsl:attribute name="class">
	<xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </pre>
  </div>
</xsl:template>
<!-- }}} -->

<!-- {{{ font specification commands -->

<!-- note that these commands are here for the sake of completeness
     their use is not recommended in the texinfo manual -->

<xsl:template match="b">
  <!-- we map this to 'strong'. -->
  <span class="strong"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="i">
  <!-- we map this to 'emph'. -->
  <span class="emph"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="r">
  <!-- this should use the document font.  unfortunately 'inherit'
       is not a supported value in CSS1, so we map this to 'default'.
       'span.default' is created in the way body font is created. -->
  <span class="default"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="tt">
  <!-- we map this to 'code'.
       (note that code tag has no CSS properties set, so we get
       typewriter font only.) -->
  <code><xsl:apply-templates/></code>
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

<xsl:template match="itemize/item/para|enumerate/item/para">
  <xsl:apply-templates/>
</xsl:template>
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
	  <xsl:when test="substring($url, string-length($url), 1) = '/'">
	    <xsl:value-of select="$url"/>
	  </xsl:when>
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
      <xsl:choose>
        <!-- or maybe it is mailto: ? -->
	<xsl:when test="substring-before(urefurl, ':') = 'mailto'">
	  <xsl:variable name="url" select="substring-after(urefurl, ':')"/>
	  <a>
	    <xsl:attribute name="href">
	      <xsl:value-of select="concat('mailto:', $url)"/>
	    </xsl:attribute>
	    <xsl:choose>
	      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	      <xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
	    </xsl:choose>
	  </a>
	</xsl:when>
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
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
<!-- }}} -->

  <!-- {{{ menus -->

  <!-- TODO should we handle menus? -->
  <xsl:template match="menu"/>

  <!-- }}} -->

<!-- {{{ anchors, nodes and references -->
<xsl:template match="anchor">
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="@name"/>
    </xsl:attribute>
  </a>
</xsl:template>

<!-- we don't make use of up, next and previous nodes -->
<xsl:template match="nodeup|nodenext|nodeprev|nodename"/>

<xsl:template match="node">
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="nodename"/>
    </xsl:attribute>
  </a>
  <xsl:apply-templates/>
</xsl:template>

<!-- makeinfo creates an xref node for also ref and pxref -->
<xsl:template match="xref">
  <a>
    <xsl:attribute name="href">
      <xsl:if test="string-length(./xrefinfofile)">
	<xsl:choose>
	  <xsl:when test="string-length(substring-before(xrefinfofile, '.info'))">
	    <xsl:value-of select="concat(substring-before(xrefinfofile, '.info'), '.html')"/>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:value-of select="concat(xrefinfofile, '.html')"/>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:if>
      <xsl:text>#</xsl:text><xsl:value-of select="./xrefnodename"/>
    </xsl:attribute>
    <!-- it's quite useless to make the file name visible -->
    <!--
    <xsl:if test="string-length(./xrefinfofile)">
      <xsl:text>(</xsl:text><xsl:apply-templates select="./xrefinfofile"/><xsl:text>.html) </xsl:text>
    </xsl:if>
    -->
    <xsl:if test="string-length(./xrefinfoname)">
      <xsl:apply-templates select="./xrefinfoname"/><xsl:text>: </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="./xrefnodename"/>
  </a>
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
      <xsl:value-of select="concat($images_prefix, .)"/><xsl:text>.</xsl:text><xsl:value-of select="@extension"/>
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
  <xsl:if test="not(string(.) = '')">
    <span class="news-date">
      <xsl:apply-templates/>
    </span>
    <br/>
    <xsl:if test="string(../news-title) = ''">
      <br/>
    </xsl:if>
  </xsl:if>
</xsl:template>

<xsl:template match="para/news-title">
  <xsl:if test="not(string(.) = '')">
    <strong>
      <span class="news-heading">
	<xsl:apply-templates/>
      </span>
    </strong>
    <br/>
    <br/>
  </xsl:if>
</xsl:template>
<!-- }}} -->

</xsl:stylesheet>
<!-- vim: set fdm=marker: -->
