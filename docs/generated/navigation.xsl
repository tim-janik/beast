<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" indent="yes" encoding="ISO-8859-1" />
<!-- <xsl:strip-space elements="*" /> -->

<xsl:param name="targets_prefix" select="''"/>
<xsl:param name="this_file" select="'-'"/>

<xsl:template name="navigation">
  <xsl:if test="string-length(/texinfo/para/document-navigation) > 0">
    <ul id="navigation">
      <xsl:apply-templates select="document(string(/texinfo/para/document-navigation))"/>
    </ul>
  </xsl:if>
</xsl:template>

<xsl:template match="navigation-node[@target != $this_file]">
  <li>
    <a>
      <xsl:call-template name="navigation-href"/>
      <xsl:attribute name="class">
	<xsl:choose>
	  <xsl:when test="count(.//navigation-node[@target=$this_file]) > 0">open-tree</xsl:when>
	  <xsl:when test="count(./navigation-node) > 0">closed-tree</xsl:when>
	</xsl:choose>
      </xsl:attribute>
      <xsl:call-template name="navigation-image"/>
    </a>
    <xsl:if test="count(.//navigation-node[@target=$this_file]) > 0">
      <ul class="subtree">
	<xsl:apply-templates/>
      </ul>
    </xsl:if>
  </li>
</xsl:template>

<xsl:template match="navigation-node[@target = $this_file]">
  <li>
  <strong><span>
    <xsl:attribute name="class">
      <xsl:choose>
	<xsl:when test="count(./navigation-node) > 0">current-tree</xsl:when>
	<xsl:otherwise>current</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:call-template name="navigation-image"/>
  </span></strong>
  <xsl:if test="count(./navigation-node) > 0">
    <ul class="subtree">
      <xsl:apply-templates/>
    </ul>
  </xsl:if>
  </li>
</xsl:template>

<xsl:template match="navigation-node[@target = '']">
  <li><span>
    <xsl:attribute name="class">
      <xsl:choose>
	<xsl:when test="count(./navigation-node) > 0">nolink-tree</xsl:when>
	<xsl:otherwise>nolink</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:call-template name="navigation-image"/>
  </span>
  <xsl:if test="count(./navigation-node) > 0">
    <ul class="subtree">
      <xsl:apply-templates/>
    </ul>
  </xsl:if>
  </li>
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
