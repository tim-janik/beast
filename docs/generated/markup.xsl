<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="html"/>
  
  <xsl:template match="texinfo">
    <!-- basic tag definitions -->
    <tagdef name="bold"           weight="bold" stretch="expanded" />
    <tagdef name="italic"         style="italic" />
    <tagdef name="mono"           family="mono" />
    <tagdef name="center"         justification="center" />
    <tagdef name="fill"           justification="fill" />
    <tagdef name="underline"      underline="single" />
    <tagdef name="doubleline"     underline="double" />
    <tagdef name="fg-black"       foreground="#000000" />
    <tagdef name="fg-white"       foreground="#ffffff" />
    <tagdef name="fg-red"         foreground="#ff0000" />
    <tagdef name="fg-green"       foreground="#00ff00" />
    <tagdef name="fg-blue"        foreground="#0000ff" />
    <tagdef name="fg-turquoise"   foreground="#00ffff" />
    <tagdef name="fg-pink"        foreground="#ff00ff" />
    <tagdef name="fg-yellow"      foreground="#ffff00" />
    <tagdef name="bg-black"       background="#000000" />
    <tagdef name="bg-white"       background="#ffffff" />
    <tagdef name="bg-red"         background="#ff0000" />
    <tagdef name="bg-green"       background="#00ff00" />
    <tagdef name="bg-blue"        background="#0000ff" />
    <tagdef name="bg-turquoise"   background="#00ffff" />
    <tagdef name="bg-pink"        background="#ff00ff" />
    <tagdef name="bg-yellow"      background="#ffff00" />
    <!-- lower priority tags need to come first -->
    <tagdef name="section"        indent="0" />
    <tagdef name="body"           wrap_mode="word" left_margin="5" right_margin="5" />
    <tagdef name="indent-margin"  left_margin="20" right_margin="20" />
    <tagdef name="item-margin"    left_margin="18" />
    <tagdef name="bullet-tag"     indent="-10" />
    <tagdef name="dline"          underline="double" weight="bold" />
    <tagdef name="sline"          underline="single" weight="bold" />
    <tagdef name="code"           family="mono" foreground="#000040" />
    <tagdef name="programlisting" family="mono" wrap_mode="none" foreground="#000040" />
    <tagdef name="property"       style="italic" />
    <tagdef name="channel"        style="italic" />
    <tagdef name="menupath"       style="italic" weight="bold" background="#e0e0e0" />
    <tagdef name="pagepath"       style="italic" weight="bold" background="#f0f0f0" />
    <tagdef name="object"         family="mono" style="italic" />
    <tagdef name="nowrap"         wrap_mode="none" />
    <span tag="body">
      <xsl:apply-templates/>
      <breakline/>
      <newline/>
    </span>
  </xsl:template>
  
  <!-- useless tags -->
  <xsl:template match="setfilename" />
  <xsl:template match="settitle" />
  
  <xsl:template match="linebreak">
    <newline/>
  </xsl:template>
  
  <xsl:template match="section">
    <span tag="section">
      <xsl:apply-templates/>
    </span>
    <breakline/>
  </xsl:template>
  
  <xsl:template match="title">
    <breakline/>
    <newline/>
    <span tag="sline">
      <xsl:apply-templates/>
      <breakline/>
    </span>
    <newline/>
  </xsl:template>

  <xsl:template match="chapter/title">
    <breakline/>
    <newline/>
    <newline/>
    <span tag="dline">
      <span tag="center">
	<xsl:apply-templates/>
      <breakline/>
      </span>
    </span>
    <newline/>
  </xsl:template>
  
  <xsl:template match="code">
    <span tag="code">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="preformat">
    <breakline/>
    <span tag="nowrap">
      <span tag="mono">
	<keep-space><xsl:apply-templates/></keep-space>
      </span>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template match="keepspace">
    <breakline/>
    <span tag="nowrap">
      <keep-space><xsl:apply-templates/></keep-space>
    </span>
    <breakline/>
  </xsl:template>

  <xsl:template match="para">
    <breakline/>
    <xsl:apply-templates/>
    <breakline/>
  </xsl:template>

  <xsl:template match="programlisting">
    <breakline/>
    <span tag="programlisting">
      <keep-space><xsl:apply-templates/></keep-space>
    </span>
    <breakline/>
  </xsl:template>
  
  <xsl:template match="menupath">
    <span tag="menupath">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="pagepath">
    <span tag="pagepath"><xsl:apply-templates/></span>
  </xsl:template>
  
  <xsl:template match="property">
    <span tag="property">
      "<xsl:apply-templates/>"
    </span>
  </xsl:template>
  
  <xsl:template match="object">
    <span tag="object">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="module">
    <span tag="object">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="channel">
    <span tag="channel">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="emphasize">
    <span tag="italic">
      <xsl:apply-templates/>
    </span>
  </xsl:template>
  
  <xsl:template match="important">
    <span style="italic">
      <span tag="underline">
	<xsl:apply-templates/>
      </span>
    </span>
  </xsl:template>
  
  <xsl:template match="center">
    <breakline/>
    <span tag="center">
      <xsl:apply-templates/>
      <breakline/>
    </span>
  </xsl:template>
  
  <xsl:template match="indent">
    <breakline/>
    <span tag="indent-margin">
      <xsl:apply-templates/>
      <breakline/>
    </span>
  </xsl:template>
  
  <xsl:template match="fill">
    <breakline/>
    <!-- grumbl, text-widget fill is not implemented -->
    <keep-space><xsl:apply-templates/></keep-space><breakline/>
  </xsl:template>
  
  <xsl:template match="itemize">
    <breakline/>
    <xsl:apply-templates/>
    <breakline/>
  </xsl:template>
  
  <xsl:template match="enumerate">
    <breakline/>
    <xsl:apply-templates/>
    <breakline/>
  </xsl:template>
  
  <xsl:template match="item">
    <breakline/>
    <span tag="item-margin">
      <span tag="bullet-tag"><image stock="gtk-yes" size="10x10"/></span>
      <xsl:apply-templates/>
    </span>
    <breakline/>
  </xsl:template>
  
  <xsl:template match="uref">
    <span tag="fg-blue">
      <span tag="underline">
	<activatable>
	  <xsl:attribute name="data">
	    <xsl:value-of select="urefurl"/>
	  </xsl:attribute>
	  <xsl:choose>
	    <xsl:when test="count(child::urefreplacement)"><xsl:apply-templates select="urefreplacement"/></xsl:when>
	    <xsl:when test="count(child::urefdesc)"><xsl:apply-templates select="urefdesc"/> (<xsl:apply-templates select="urefurl"/>)</xsl:when>
	    <xsl:otherwise><xsl:apply-templates select="urefurl"/></xsl:otherwise>
	  </xsl:choose>
	</activatable>
      </span>
    </span>
  </xsl:template>
  
  <xsl:template match="image">
    <activatable data="ImageClick">
      <image>
	<xsl:attribute name="file">
	  <xsl:value-of select="."/>.<xsl:value-of select="@extension"/>
	</xsl:attribute>
	[<xsl:value-of select="@alttext"/>]
      </image>
    </activatable>
  </xsl:template>
  
</xsl:stylesheet>
