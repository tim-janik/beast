<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" indent="yes" encoding="ISO-8859-9" />
<!-- <xsl:strip-space elements="*" /> -->

<xsl:param name="targets_prefix" select="''"/>
<xsl:param name="this_file" select="'-'"/>

<xsl:template name="navigation">
  <xsl:if test="string-length(/texinfo/para/document-navigation) > 0">
    <div id="navigation">
      <xsl:if test="string(/texinfo/para/document-hasbanner) != 'large'">
        <xsl:attribute name="class">with_nobanner</xsl:attribute>
      </xsl:if>
      <xsl:apply-templates select="document(string(/texinfo/para/document-navigation))"/>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template match="node[@target != $this_file]">
  <xsl:call-template name="dash"/>
  <a>
    <xsl:call-template name="navigation-href"/>
    <xsl:choose>
      <xsl:when test="count(.//node[@target=$this_file]) > 0"><xsl:attribute name="class">open_tree</xsl:attribute></xsl:when>
      <xsl:when test="count(./node) > 0"><xsl:attribute name="class">closed_tree</xsl:attribute></xsl:when>
    </xsl:choose>
    <xsl:call-template name="navigation-image"/>
  </a>
  <xsl:if test="count(.//node[@target=$this_file]) > 0">
    <span class="hidden">: </span>
    <span class="subtree">
      <xsl:apply-templates/>
    </span>
    <br class="hidden"/>
  </xsl:if>
</xsl:template>

<xsl:template match="node[@target = $this_file]">
  <xsl:call-template name="dash"/>
  <strong><span>
    <xsl:attribute name="class">
      <xsl:choose>
	<xsl:when test="count(./node) > 0">current-tree</xsl:when>
	<xsl:otherwise>current</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:call-template name="navigation-image"/>
  </span></strong>
  <xsl:if test="count(./node) > 0">
    <span class="hidden">: </span>
    <span class="subtree">
      <xsl:apply-templates/>
    </span>
    <br class="hidden"/>
  </xsl:if>
</xsl:template>

<xsl:template match="node[@target = '']">
  <xsl:call-template name="dash"/>
  <span>
    <xsl:attribute name="class">
      <xsl:choose>
	<xsl:when test="count(./node) > 0">nolink-tree</xsl:when>
	<xsl:otherwise>nolink</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:call-template name="navigation-image"/>
  </span>
  <xsl:if test="count(./node) > 0">
    <span class="hidden">: </span>
    <span class="subtree">
      <xsl:apply-templates/>
    </span>
    <br class="hidden"/>
  </xsl:if>
</xsl:template>

<xsl:template name="dash">
  <xsl:variable name="prev" select="(position() - 2) div 2"/>
  <xsl:choose>
    <xsl:when test="count(./node) > 0 and (@target = $this_file or @target = '') and ../node[$prev]/@target != ''"><br class="hidden"/></xsl:when>
    <xsl:when test="count(.//node[@target = $this_file]) > 0"><br class="hidden"/></xsl:when>
    <xsl:when test="count(../node[$prev]//*) > 0 and ../node[$prev]//@target = ''"/>
    <xsl:when test="count(../node[$prev]//*) > 0 and ../node[$prev]//@target = $this_file"/>
    <xsl:when test="count(../node[$prev]/*)  > 0 and ../node[$prev]/@target = ''"/>
    <xsl:when test="position() > 2"><span class="hidden"> - </span></xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="navigation-image">
  <xsl:choose>
    <xsl:when test="string-length(@image) > 0">
      <img border="0">
	<xsl:attribute name="src">
	  <xsl:value-of select="concat($images_prefix, @image)"/>
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

<xsl:template name="navigation-href">
  <xsl:attribute name="href">
    <xsl:if test="string-length($targets_prefix) > 0">
      <xsl:value-of select="$targets_prefix" />
    </xsl:if>
    <xsl:value-of select="@target"/>
  </xsl:attribute>
</xsl:template>

<xsl:template name="home-link">
  <xsl:if test="string-length(/texinfo/para/document-navigation) = 0">
    <a class="homelink" href="index.html">Home</a>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
