<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" indent="no" doctype-system="markup.dtd"/>
  <!-- <xsl:strip-space elements="*"/> -->
  <xsl:preserve-space elements="code display smalldisplay format smallformat example smallexample lisp smalllisp"/>

  <!-- {{{ start parsing -->
  <xsl:template match="texinfo">
    <tag-span-markup>
      <!-- lower priority tags need to come first -->
      <tagdef name="chapter"             indent="0" />
      <tagdef name="chapter_title"       weight="bold" scale="1.5" />
      <tagdef name="section"             indent="0" />
      <tagdef name="section_title"       weight="bold" scale="1.3" />
      <tagdef name="subsection"          indent="0" />
      <tagdef name="subsection_title"    weight="bold" scale="1.15" />
      <tagdef name="subsubsection"       indent="0" />
      <tagdef name="subsubsection_title" scale="1.15" />

      <tagdef name="body"                wrap_mode="word" left_margin="5" right_margin="5">
        <xsl:attribute name="family">
	  <xsl:call-template name="document-font"/>
	</xsl:attribute>
      </tagdef>

      <!-- toc indentation -->
      <!-- starts from 2 because first level doesn't need any -->
      <tagdef name="toc_level_2"    indent="20" />
      <tagdef name="toc_level_3"    indent="40" />
      <tagdef name="toc_level_4"    indent="60" />

      <tagdef name="indent-margin"  left_margin="20" right_margin="20" />
      <tagdef name="item-margin"    left_margin="18" />
      <tagdef name="bullet-tag"     indent="-10" />
      <tagdef name="enumerate-item" weight="bold" foreground="#4046a0" />
      <tagdef name="tableterm"	    />
      <tagdef name="tableitem"	    left_margin="20" />

      <!-- Sucks but hey -->
      <tagdef name="table_entry_1"  left_margin="0"   />
      <tagdef name="table_entry_2"  left_margin="20"  />
      <tagdef name="table_entry_3"  left_margin="40"  />
      <tagdef name="table_entry_4"  left_margin="60"  />
      <tagdef name="table_entry_5"  left_margin="80"  />
      <tagdef name="table_entry_6"  left_margin="100" />
      <tagdef name="table_entry_7"  left_margin="120" />
      <tagdef name="table_entry_8"  left_margin="140" />
      <tagdef name="table_entry_9"  left_margin="160" />

      <tagdef name="multitable"     left_margin="30"/>

      <tagdef name="title_page"     justification="center" />
      <tagdef name="doc_title"      underline="single" weight="bold" scale="2.0" />
      <tagdef name="doc_author"     weight="bold" scale="1.8" />

      <tagdef name="hyperlink"      underline="single" foreground="#0000ff" />

      <!-- contextual tags -->
      <tagdef name="strong"         weight="bold"      />
      <tagdef name="emph"           style="italic"     />

      <tagdef name="code"           family="monospace" foreground="#000040" />
      <tagdef name="acronym"        family="monospace" />
      <tagdef name="cite"           family="monospace" />
      <tagdef name="command"        family="monospace" />
      <tagdef name="dfn"            family="monospace" />
      <tagdef name="email"          family="monospace" />
      <tagdef name="env"            family="monospace" />
      <tagdef name="file"           family="monospace" />
      <tagdef name="kbd"            family="monospace" />
      <tagdef name="key"            family="monospace" />
      <tagdef name="option"         family="monospace" />
      <tagdef name="samp"           family="monospace" />
      <tagdef name="url"            family="monospace" underline="single" />
      <tagdef name="var"            family="monospace" />

      <tagdef name="property"       style="italic" />
      <tagdef name="channel"        style="italic" />
      <tagdef name="menupath"       weight="bold" background="#e0e0e0" style="italic" />
      <tagdef name="pagepath"       weight="bold" background="#f0f0f0" />
      <tagdef name="object"         family="monospace" style="italic" />

      <tagdef name="changelog-entry" family="monospace" />
      <tagdef name="changelog-item"  family="monospace" weight="bold" foreground="#a000a0" />

      <tagdef name="important"      underline="single" weight="bold" foreground="#df5fdf" />

      <tagdef name="quotation"      indent="10" left_margin="70" />
      <!-- FIXME Alper, until the makeinfo's @verbatim support is fixed, @example blocks
	   will not be indented. This way we emulate @verbatim blocks with @example blocks
	   and indent the real @example blocks ourselves. -->
      <tagdef name="example"        family="monospace" wrap_mode="none" />
      <tagdef name="smallexample"   family="monospace" wrap_mode="none" scale="0.9" />
      <tagdef name="lisp"           family="monospace" left_margin="70" wrap_mode="none" foreground="#000040" />
      <tagdef name="smalllisp"      family="monospace" left_margin="70" wrap_mode="none" foreground="#000040" scale="0.9" />
      <tagdef name="display"        left_margin="70" wrap_mode="none" />
      <tagdef name="smalldisplay"   left_margin="70" wrap_mode="none" scale="0.9" />
      <tagdef name="format"         wrap_mode="none" />
      <tagdef name="smallformat"    wrap_mode="none" scale="0.9" />

      <!-- reference elements -->
      <tagdef name="reference-function"	    family="monospace" foreground="#5555cc" />
      <tagdef name="reference-parameter"    family="monospace" foreground="#555555" />
      <tagdef name="reference-constant"     family="monospace" foreground="#00b0b0" />
      <tagdef name="reference-returns"	    family="monospace" foreground="#228822" />
      <tagdef name="reference-type"	    family="monospace" foreground="#228822" />
      <tagdef name="reference-blurb"	    family="monospace" />
      <tagdef name="reference-struct"	    family="monospace" left_margin="5" />
      <tagdef name="reference-struct-type"  family="monospace" foreground="#662407" left_margin="5" />
      <tagdef name="reference-struct-name"  family="monospace" foreground="#228822" />

      <!-- high priority markup primitives -->
      <tagdef name="center"         justification="center" />

      <!-- default text (which can be used to get, say sans-serif fonts
           in code blocks). -->
      <tagdef name="default">
        <xsl:attribute name="family">
	  <xsl:call-template name="document-font"/>
	</xsl:attribute>
      </tagdef>

      <!-- generate body -->
      <span tag="body">
	<xsl:call-template name="title_page"/>
	<xsl:apply-templates/>
      </span>
    </tag-span-markup>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ useless tags -->
  <xsl:template match="setfilename|settitle|document-title|document-author|document-package|document-font|document-navigation|document-hasbanner|itemfunction|columnfraction"/>
  <!-- }}} -->

  <!-- {{{ setting a default font for documents -->
  <xsl:template name="document-font">
    <xsl:variable name="font" select="string(/texinfo/para/document-font)"/>
    <xsl:choose>
      <xsl:when test="$font='tech' or $font='techstyle' or $font='sans' or $font='sans-serif'"><xsl:text>sans</xsl:text></xsl:when>
      <xsl:when test="$font='story' or $font='storystyle' or $font='serif' or $font=''"><xsl:text>serif</xsl:text></xsl:when>
      <xsl:otherwise>
	<xsl:message>XSL-WARNING: omitting unknown font style '<xsl:value-of select="$font"/>'</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ creating a title page for documents -->
  <xsl:template name="title_page">
    <xsl:if test="string-length(/texinfo/para/document-title) > 0 or count(/texinfo/para/document-author) > 0">
      <breakline/>
      <span tag="title_page">
	<xsl:if test="string-length(/texinfo/para/document-title) > 0">
	  <span tag="doc_title">
	    <xsl:value-of select="/texinfo/para/document-title"/>
	  </span>
	  <newline/><newline/>
	</xsl:if>
	<xsl:if test="count(/texinfo/para/document-author) > 0">
	  <xsl:for-each select="/texinfo/para/document-author">
	    <span tag="doc_author">
	      <xsl:apply-templates/>
	    </span>
	    <breakline/>
	  </xsl:for-each>
	</xsl:if>
      </span>
      <newline/>
    </xsl:if>
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

  <xsl:template name="node_name">
    <anchor>
      <xsl:attribute name="name">
	<xsl:call-template name="node_number"/>
      </xsl:attribute>
    </anchor>
  </xsl:template>

  <xsl:template match="para/table-of-contents">
    <breakline/>
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
  </xsl:template>

  <xsl:template name="toc_chapter">
    <span tag="hyperlink">
      <xlink>
	<xsl:attribute name="ref">
	  <xsl:text>file:#</xsl:text><xsl:call-template name="node_number"/>
	</xsl:attribute>
	<xsl:apply-templates select="title">
	  <xsl:with-param name="toc" select="1"/>
	</xsl:apply-templates>
      </xlink>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template name="toc_section">
    <span tag="toc_level_2">
      <xsl:call-template name="toc_rest"/>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template name="toc_subsection">
    <span tag="toc_level_3">
      <xsl:call-template name="toc_rest"/>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template name="toc_subsubsection">
    <span tag="toc_level_4">
      <xsl:call-template name="toc_rest"/>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template name="toc_rest">
    <span tag="hyperlink">
      <xlink>
	<xsl:attribute name="ref">
	  <xsl:text>file:#</xsl:text><xsl:call-template name="node_number"/>
	</xsl:attribute>
	<xsl:apply-templates select="title">
	  <xsl:with-param name="toc" select="1"/>
	</xsl:apply-templates>
      </xlink>
    </span>
  </xsl:template>

  <!-- }}} -->

  <!-- {{{ document sections -->
  <xsl:template match="chapter|appendix|unnumbered">
    <span tag="chapter">
      <xsl:apply-templates/>
    </span>

    <!-- I (alper) disabled the following, because it causes inconsistency. -->
    <!-- If before the chapter, there's a paragraph that's not a part of    -->
    <!-- any sections, there will be only one blank line. However, the      -->
    <!-- following causes two blank lines between two consequtive chapters. -->

    <!-- <xsl:if test="not(position()=last())"> -->
      <!-- <newline/> -->
    <!-- </xsl:if> -->
  </xsl:template>

  <xsl:template match="section|appendixsec|unnumberedsec">
    <span tag="section">
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="subsection|appendixsubsec|unnumberedsubsec">
    <span tag="subsection">
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="subsubsection|appendixsubsubsec|unnumberedsubsubsec">
    <span tag="subsubsection">
      <xsl:apply-templates/>
    </span>
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

  <!-- {{{ section titles stuff -->

  <!-- {{{ chapters -->
  <xsl:template match="chapter/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_chapter_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="chapter_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_chapter_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="section/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_section_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="section_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_section_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="subsection/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_subsection_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="subsection_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_subsection_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="subsubsection/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_subsubsection_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="subsubsection_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_subsubsection_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ appendices -->
  <xsl:template match="appendix/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_appendix_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="chapter_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_appendix_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="appendixsec/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_appendixsec_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="section_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_appendixsec_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="appendixsubsec/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_appendixsubsec_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="subsection_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_appendixsubsec_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="appendixsubsubsec/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:call-template name="title_appendixsubsubsec_number"/> - <xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="subsubsection_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:call-template name="title_appendixsubsubsec_number"/> - <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ unnumbered/etc. -->
  <xsl:template match="unnumbered/title|chapheading/title|majorheading/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span tag="chapter_title">
	  <xsl:call-template name="node_name"/>
	  <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="unnumberedsec/title|unnumberedsubsec/title|unnumberedsubsubsec/title">
    <xsl:param name="toc" select="0"/>
    <xsl:choose>
      <xsl:when test="$toc">
	<xsl:apply-templates/>
      </xsl:when>
      <xsl:otherwise>
	<span>
	  <xsl:attribute name="tag">
	    <xsl:choose>
	      <xsl:when test="local-name(..) = 'unnumberedsec'">section_title</xsl:when>
	      <xsl:when test="local-name(..) = 'unnumberedsubsec'">subsection_title</xsl:when>
	      <xsl:when test="local-name(..) = 'unnumberedsubsubsec'">subsection_title</xsl:when>
	    </xsl:choose>
	  </xsl:attribute>
	  <xsl:call-template name="node_name"/>
	  <xsl:apply-templates/>
	</span>
	<breakline/><newline/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="heading/title|subheading/title|subsubheading/title">
    <span>
      <xsl:attribute name="tag">
	<xsl:choose>
	  <xsl:when test="local-name(..) = 'heading'">section_title</xsl:when>
	  <xsl:when test="local-name(..) = 'subheading'">subsection_title</xsl:when>
	  <xsl:when test="local-name(..) = 'subsubheading'">subsection_title</xsl:when>
	</xsl:choose>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
    <breakline/><newline/>
  </xsl:template>
  <!-- }}} -->

  <!-- }}} -->

  <!-- {{{ reference generation -->
  <xsl:template match="reference-function|reference-parameter|reference-returns|reference-type|reference-blurb|reference-struct-name|reference-struct-type">
    <span>
      <xsl:attribute name="tag">
	<xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="reference-struct-open">
    <span tag="reference-struct"> {</span>
  </xsl:template>

  <xsl:template match="reference-struct-close">
    <span tag="reference-struct">};</span>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ paragrapghs -->
  <xsl:template match="para">
    <xsl:apply-templates/>
    <xsl:choose>
      <xsl:when test="count(document-font|document-title|document-author)"/>
      <!-- <xsl:when test="count(reference-function|reference-struct-name)"><breakline/></xsl:when> -->
      <xsl:otherwise><breakline/><newline/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ line breaks, forced spaces -->
  <xsl:template match="linebreak">
    <breakline/>
  </xsl:template>

  <xsl:template match="space">
    <keep-space><xsl:text> </xsl:text></keep-space>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ contextual tags -->
  <xsl:template match="acronym|cite|dfn|kbd|samp|var|strong|emph|url|email|key|env|file|command|option|menupath|pagepath|object|channel|important|code|logentry">
    <span>
      <xsl:attribute name="tag">
	<xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="property">
    <span tag="property">
      "<xsl:apply-templates/>"
    </span>
  </xsl:template>

  <xsl:template match="center">
    <span tag="center">
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="quotation|example|smallexample|display|smalldisplay|format|smallformat|lisp|smalllisp">
    <breakline/>
    <span>
      <xsl:attribute name="tag">
	<xsl:value-of select="local-name()"/>
      </xsl:attribute>
      <keep-space><xsl:apply-templates/></keep-space>
    </span>
    <breakline/>
    <newline/>
  </xsl:template>
  <!-- }}} -->

<!-- {{{ font specification commands -->

<!-- note that these commands are here for the sake of completeness
     their use is not recommended in the texinfo manual -->

<xsl:template match="b">
  <!-- we map this to 'strong'. -->
  <span tag="strong"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="i">
  <!-- we map this to 'emph'. -->
  <span tag="emph"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="r">
  <!-- this should use the document font.  since 'body' has margin
       settings as well, 'default' is a stripped down version of it. -->
  <span tag="default"><xsl:apply-templates/></span>
</xsl:template>

<xsl:template match="tt">
  <!-- we map this to 'code'.  note that 'code' blocks have other settings
       than font-family. -->
  <span tag="code"><xsl:apply-templates/></span>
</xsl:template>
<!-- }}} -->

  <!-- {{{ enumeration and itemization handling -->
  <xsl:template match="itemize|enumerate">
    <breakline/>
    <span tag="item-margin">
      <xsl:apply-templates/>
    </span>
    <!-- <xsl:if test="not(position()=last())"> -->
      <newline/>
    <!-- </xsl:if> -->
  </xsl:template>

  <xsl:template match="itemize/item">
    <span tag="bullet-tag"><image stock="gtk-yes" size="10x10"/></span>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="enumerate/item">
    <span tag="enumerate-item"><xsl:number format="1. "/></span>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="itemize/item/para|enumerate/item/para">
    <xsl:apply-templates/>
    <breakline/>
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
	<span tag="hyperlink">
	  <xlink>
	    <xsl:attribute name="ref">
	      <xsl:value-of select="concat($protocol, '://', $url)"/>
	    </xsl:attribute>
	    <xsl:choose>
	      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($protocol, '://', $url)"/>)</xsl:when>
	      <xsl:otherwise><xsl:value-of select="concat($protocol, '://', $url)"/></xsl:otherwise>
	    </xsl:choose>
	  </xlink>
	</span>
      </xsl:when>
      <!-- PROTOCOL: MAILTO NEWS -->
      <xsl:when test="$protocol='mailto' or $protocol='news'">
	<span tag="hyperlink">
	  <xlink>
	    <xsl:attribute name="ref">
	      <xsl:value-of select="concat($protocol, ':', $url)"/>
	    </xsl:attribute>
	    <xsl:choose>
	      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
	      <xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
	    </xsl:choose>
	  </xlink>
	</span>
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
	    <span tag="hyperlink">
	      <xlink>
		<xsl:attribute name="ref">
		  <xsl:value-of select="concat($page, '.', $section, '.markup')"/>
		</xsl:attribute>
		<xsl:choose>
		  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
		  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($page, '.', $section, '.markup')"/>)</xsl:when>
		  <xsl:otherwise><xsl:value-of select="concat($page, '.', $section, '.markup')"/></xsl:otherwise>
		</xsl:choose>
	      </xlink>
	    </span>
	  </xsl:when>
	</xsl:choose>
      </xsl:when>
      <!-- PROTOCOL: Beast Document -->
      <xsl:when test="$protocol='beast-doc'">
	<!-- Get the file name and append the target specific extension (markup) -->
	<xsl:variable name="filename">
	  <xsl:choose>
	    <xsl:when test="substring($url, string-length($url), 1) = '/'">
	      <xsl:value-of select="$url"/>
	    </xsl:when>
	    <xsl:when test="substring-before($url, '#') = ''">
	      <xsl:value-of select="concat($url, '.markup')"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="concat(substring-before($url, '#'), '.markup')"/>
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
	<span tag="hyperlink">
	  <xlink>
	    <xsl:attribute name="ref">
	      <xsl:value-of select="concat($filename, $anchor)"/>
	    </xsl:attribute>
	    <xsl:choose>
	      <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	      <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="concat($filename, $anchor)"/>)</xsl:when>
	      <xsl:otherwise><xsl:value-of select="concat($filename, $anchor)"/></xsl:otherwise>
	    </xsl:choose>
	  </xlink>
	</span>
      </xsl:when>
      <!-- Unknown Protocol -->
      <xsl:otherwise>
	<xsl:choose>
	  <!-- or maybe it is mailto: ? -->
	  <xsl:when test="substring-before(urefurl, ':') = 'mailto'">
	    <xsl:variable name="url" select="substring-after(urefurl, ':')"/>
	    <span tag="hyperlink">
	      <xlink>
		<xsl:attribute name="ref">
		  <xsl:value-of select="concat('mailto:', $url)"/>
		</xsl:attribute>
		<xsl:choose>
		  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
		  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="$url"/>)</xsl:when>
		  <xsl:otherwise><xsl:value-of select="$url"/></xsl:otherwise>
		</xsl:choose>
	      </xlink>
	    </span>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:message>XSL-WARNING: unknown protocol '<xsl:value-of select="$protocol"/>' in <xsl:value-of select="urefurl"/>, using as-is</xsl:message>
	    <span tag="hyperlink">
	      <xlink>
		<xsl:attribute name="ref">
		  <xsl:value-of select="urefurl"/>
		</xsl:attribute>
		<xsl:choose>
		  <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
		  <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:value-of select="urefurl"/>)</xsl:when>
		  <xsl:otherwise><xsl:value-of select="urefurl"/></xsl:otherwise>
		</xsl:choose>
	      </xlink>
	    </span>
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
    <anchor>
      <xsl:attribute name="name">
	<xsl:value-of select="@name"/>
      </xsl:attribute>
    </anchor>
  </xsl:template>

  <!-- we don't make use of up, next and previous nodes -->
  <xsl:template match="nodeup|nodenext|nodeprev|nodename"/>

  <xsl:template match="node">
    <anchor>
      <xsl:attribute name="name">
	<xsl:value-of select="nodename"/>
      </xsl:attribute>
    </anchor>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- makeinfo creates an xref node for also ref and pxref -->
  <xsl:template match="xref">
    <xsl:text>See </xsl:text>
    <span tag="hyperlink">
      <xlink>
	<xsl:attribute name="ref">
	  <xsl:if test="string-length(./xrefinfofile)">
	    <xsl:choose>
	      <xsl:when test="string-length(substring-before(xrefinfofile, '.info'))">
		<xsl:value-of select="concat(substring-before(xrefinfofile, '.info'), '.markup')"/>
	      </xsl:when>
	      <xsl:otherwise>
		<xsl:value-of select="concat(xrefinfofile, '.markup')"/>
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
      </xlink>
    </span>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ inline images -->
  <xsl:template match="image">
    <!-- <xlink ref="error:DEADEND"> -->
    <image>
      <xsl:attribute name="file">
	<xsl:value-of select="."/>.<xsl:value-of select="@extension"/>
      </xsl:attribute>
      [<xsl:value-of select="@alttext"/>]
    </image>
    <!-- </xlink> -->
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ table handling -->

  <!-- {{{ simple definition tables -->
  <xsl:template match="tableterm">
    <span tag="tableterm">
      <xsl:apply-templates/>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template match="tableitem/item">
    <span tag="tableitem">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ multicolumn tables -->
  <xsl:template match="multitable">
    <breakline/>
    <span tag="multitable">
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <xsl:template match="multitable/row">
    <xsl:apply-templates/>
    <breakline/>
  </xsl:template>

  <xsl:template match="multitable/row/entry">
    <!-- Spanning each entry in a new line is disabled -->
    <!-- <span> -->
      <!-- <xsl:attribute name="tag"> -->
	<!-- <xsl:text>table_entry_</xsl:text><xsl:number/> -->
      <!-- </xsl:attribute> -->
      <xsl:apply-templates/>
      <xsl:if test="not(position()=last())">
        <xsl:text> </xsl:text>
      </xsl:if>
    <!-- </span> -->
    <!-- <newline/> -->
  </xsl:template>
  <!-- }}} -->

  <!-- }}} -->

  <!-- {{{ indice generation -->
  <xsl:template match="indexterm">
    <anchor>
      <xsl:attribute name="name">
	<xsl:value-of select="@index"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
      </xsl:attribute>
    </anchor>
  </xsl:template>

  <xsl:template match="printindex">
    <xsl:variable name="type" select="."/>
    <xsl:for-each select="//indexterm[@index=$type]">
      <xsl:sort/>
      <span tag="hyperlink">
	<xlink>
	  <xsl:attribute name="ref">
	    <xsl:text>file:#</xsl:text><xsl:value-of select="$type"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
	  </xsl:attribute>
	  <xsl:apply-templates/>
	</xlink>
      </span>
      <keep-space>    (<xsl:value-of select="../../title"/>)</keep-space>
      <breakline/>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="para/printplainindex">
    <xsl:variable name="type" select="."/>
    <xsl:for-each select="//indexterm[@index=$type]">
      <xsl:sort/>
      <span tag="hyperlink">
	<xlink>
	  <xsl:attribute name="ref">
	    <xsl:text>file:#</xsl:text><xsl:value-of select="$type"/><xsl:text>index-</xsl:text><xsl:number level="any"/>
	  </xsl:attribute>
	  <xsl:apply-templates/>
	</xlink>
      </span>
      <xsl:if test="not(position()=last())">
	<breakline/>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>
  <!-- }}} -->

</xsl:stylesheet>
<!-- vim: set fdm=marker: -->
