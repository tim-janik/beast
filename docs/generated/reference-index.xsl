<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="text" indent="no" encoding="ISO-8859-1" />
  <xsl:strip-space elements="*" />

  <!-- {{{ root of all evil -->
  <xsl:template match="/texinfo">
    <xsl:apply-templates select="//tableterm/reference-function" />
    <xsl:apply-templates select="//tableterm/reference-function-nolink" />
    <xsl:apply-templates select="//tableterm/reference-struct-name" />
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ url of the struct/function/etc. -->
  <xsl:template name="url">
    <xsl:param name="anchor"/>
    <xsl:variable name="url_tmp">
      <xsl:value-of select="substring-before(/texinfo/setfilename, '-unpatched')" />
    </xsl:variable>
    <xsl:variable name="file">
      <xsl:value-of select="substring-before($url_tmp, '.')" />
    </xsl:variable>
    <xsl:variable name="section">
      <xsl:value-of select="substring-after($url_tmp, '.')" />
    </xsl:variable>

    <xsl:text>beast-man://</xsl:text>
    <xsl:value-of select="$section" />
    <xsl:text>/</xsl:text>
    <xsl:value-of select="$file" />
    <xsl:text>#</xsl:text>
    <xsl:value-of select="$anchor" />
  </xsl:template>
  <!-- }}} -->

  <!-- {{{ match structs/functions -->
  <xsl:template match="reference-function|reference-function-nolink|reference-struct-name">
    <xsl:value-of select="normalize-space(.)" />
    <xsl:text> </xsl:text>
    <xsl:call-template name="url">
      <xsl:with-param name="anchor" select="." />
    </xsl:call-template>
    <xsl:text>
</xsl:text>
  </xsl:template>
  <!-- }}} -->

</xsl:stylesheet>
<!-- vim: set fdm=marker: -->
