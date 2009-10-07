<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:ms="http://www.mapserver.org/mapserver"
                extension-element-prefixes="dyn">
<xsl:output  method="text"/>

<xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz'"/>
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

<xsl:template name="getString">
  <xsl:param name="nodeName"/>
  <xsl:param name="quote" select="'0'"/>
  <xsl:if test="*[name() = $nodeName]">
    <xsl:choose>
      <xsl:when test="$quote = '1'">
        <xsl:value-of select="translate($nodeName, $smallcase, $uppercase)"/><xsl:text> "</xsl:text><xsl:value-of select="dyn:evaluate($nodeName)"/><xsl:text>"</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="translate($nodeName, $smallcase, $uppercase)"/><xsl:text> </xsl:text><xsl:value-of select="dyn:evaluate($nodeName)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>

<xsl:template name="indent">
  <xsl:param name="count"/>
  <xsl:if test="$count &gt; 0">
    <xsl:call-template name="indent">
      <xsl:with-param name="count" select="$count - 1"/>
    </xsl:call-template>
    <xsl:value-of select="'  '"/>
  </xsl:if>
</xsl:template>

<xsl:template name="print">
  <xsl:param name="text" select="'null'"/>
  <xsl:param name="indent"/>
  <xsl:param name="quote" select="'0'"/>
  <xsl:param name="node" select="/"/>

  <xsl:variable name="textValue">
    <xsl:choose>
      <xsl:when test="$text = 'null'">
            <xsl:call-template name="getString">
              <xsl:with-param name="nodeName" select="$node"/>
              <xsl:with-param name="quote" select="$quote"/>
            </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
  <xsl:when test="$text = 'null'">
    <xsl:if test="$textValue != ''">
      <xsl:call-template name="indent">
           <xsl:with-param name="count" select="$indent"/>
       </xsl:call-template>
         <xsl:value-of select="$textValue"/>
       <xsl:text>
</xsl:text>
    </xsl:if>
  </xsl:when>
  <xsl:otherwise>
    <xsl:if test="$node">
       <xsl:call-template name="indent">
           <xsl:with-param name="count" select="$indent"/>
       </xsl:call-template>
         <xsl:value-of select="$textValue"/>
       <xsl:text>
</xsl:text>
    </xsl:if>
  </xsl:otherwise>
  </xsl:choose>
</xsl:template>

  <xsl:template match="/">
    <xsl:apply-templates select="SymbolSet"/>
    <xsl:apply-templates select="LayerSet"/>
    <xsl:apply-templates select="Map"/>
  </xsl:template>

<xsl:template match="color | backgroundColor | outlineColor | backgroundShadowColor | shadowColor | imageColor">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="concat(translate(name(), $smallcase, $uppercase),' ', @red, ' ', @green, ' ', @blue)"/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
</xsl:template>

 <xsl:template match="colorAttribute | outlineColorAttribute">
  <xsl:param name="indent"/>
  <xsl:param name="tagName" select="translate(name(), $smallcase, $uppercase)"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="concat($tagName, ' ',.)"/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="size | offset | keySize | keySpacing | backgroundShadowSize | shadowSize">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="concat(translate(name(), $smallcase, $uppercase), ' ', @x, ' ', @y)"/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="Points">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'POINTS'"/>
    <xsl:with-param name="indent" select="$indent -1"/>
  </xsl:call-template>
  <xsl:for-each select="point">
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat(@x,' ',@y)"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:for-each>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'END'"/>
    <xsl:with-param name="indent" select="$indent -1"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="Metadata | Validation">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="translate(name(),$smallcase, $uppercase)"/>
    <xsl:with-param name="indent" select="$indent -1"/>
  </xsl:call-template>
  <xsl:for-each select="item">
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('&#34;',@name,'&#34; ','&#34;',.,'&#34;')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:for-each>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'END'"/>
    <xsl:with-param name="indent" select="$indent - 1"/>
  </xsl:call-template>
 </xsl:template>

<xsl:template match="Config">
  <xsl:param name="indent"/>
  <xsl:for-each select="item">
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('CONFIG ','&#34;',@name,'&#34; ','&#34;',.,'&#34;')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:for-each>
 </xsl:template>


<xsl:template match="projection">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'PROJECTION'"/>
    <xsl:with-param name="indent" select="$indent - 1"/>
  </xsl:call-template>
  <xsl:call-template name="split-projection">
    <xsl:with-param name="list" select="."/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'END'"/>
    <xsl:with-param name="indent" select="$indent - 1"/>
  </xsl:call-template>
 </xsl:template>

 <xsl:template name="split-projection">
   <xsl:param name="list"/>
   <xsl:param name="indent"/>
   <xsl:variable name="newlist" select="concat(normalize-space($list), ' ')" />
   <xsl:variable name="first" select="substring-before($newlist, ' ')" />
   <xsl:variable name="remaining" select="substring-after($newlist, ' ')" />
   <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('&#34;',$first,'&#34;')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
   <xsl:if test="$remaining">
     <xsl:call-template name="split-projection">
       <xsl:with-param name="list" select="$remaining" />
       <xsl:with-param name="indent" select="$indent" />
     </xsl:call-template>
   </xsl:if>
  </xsl:template>

  <xsl:template match="pattern">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('PATTERN ',.,' END')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="QueryMap">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'QUERYMAP'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'style'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>


  <xsl:template match="Web">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'WEB'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'browseFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'empty'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'error'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'imagePath'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'imageUrl'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'legendFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'log'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxTemplate'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Metadata">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minTemplate'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'queryFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Validation">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="OutputFormat">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'OUTPUTFORMAT'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'driver'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'extension'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'formatOption'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'imageMode'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'mimeType'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'transparent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Reference">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'REFERENCE'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'extent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'image'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'marker'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'markerSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxBoxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minBoxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="Legend">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'LEGEND'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="keySize">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="keySpacing">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="Label">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'postLabelCache'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Label">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'LABEL'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('TYPE ',@type)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@type"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'align'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'angle'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'antialias'"/>
    </xsl:call-template>
     <xsl:apply-templates select="backgroundColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="backgroundShadowColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="backgroundShadowSize">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'buffer'"/>
    </xsl:call-template>
    <xsl:apply-templates select="color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="colorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'COLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'encoding'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'font'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'force'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxLength'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minDistance'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minFeatureSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="offset">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'outlineWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'partials'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'priority'"/>
    </xsl:call-template>
   <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'repeatDistance'"/>
    </xsl:call-template>
    <xsl:apply-templates select="shadowColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="shadowSize">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'size'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'wrap'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ScaleBar">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'SCALEBAR'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'align'"/>
    </xsl:call-template>
    <xsl:apply-templates select="backgroundColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'intervals'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Label">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'postLabelCache'"/>
    </xsl:call-template>
    <xsl:apply-templates select="size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'style'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'transparent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'units'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Style">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'STYLE'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'angle'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'antialias'"/>
    </xsl:call-template>
    <xsl:apply-templates select="backgroundColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="colorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'COLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minWidth'"/>
    </xsl:call-template>
    <xsl:apply-templates select="offset">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'size'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'symbol'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'width'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Class">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'CLASS'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'debug'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'expression'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'group'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'keyImage'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Label">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'size'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Style">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'symbol'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'text'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Symbol">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'SYMBOL'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('TYPE ',@type)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@type"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'antialias'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'character'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
     <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'filled'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'font'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'gap'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'image'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'lineCap'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'lineJoin'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'lineJoinMaxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="pattern">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Feature">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'FEATURE'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Points">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'items'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'text'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'wkt'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Grid">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'GRID'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxArcs'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxInterval'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxSubdivide'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minArcs'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minInterval'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minSubdivide'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="Join">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'JOIN'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('TYPE ',@type)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@type"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'connection'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'connectionType'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'from'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'table'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'to'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
   <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="Layer">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'LAYER'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('TYPE ',@type)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@type"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Class">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'classGroup'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'classItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'connection'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'connectionType'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'data'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'debug'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'dump'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'extent'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Feature">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'filter'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'filterItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Grid">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'group'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="Join">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelCache'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelMaxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelMinScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'labelRequires'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxFeatures'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxGeoWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Metadata">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minGeoWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'minScaleDenom'"/>
    </xsl:call-template>
    <xsl:apply-templates select="offsite">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'opacity'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'plugin'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'postLabelCache'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'processing'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="projection">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'requires'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'sizeUnits'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'styleItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'symbolScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'tileIndex'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'tileItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'tolerance'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'toleranceUnits'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'transform'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'units'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Validation">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="/SymbolSet">
    <xsl:param name="indent" select="1"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'SYMBOLSET&#xa;'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Symbol">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
   <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="/LayerSet">
    <xsl:param name="indent" select="1"/>
    <xsl:apply-templates select="Layer">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="/Map">
    <xsl:param name="indent" select="1"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'MAP&#xa;'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('NAME ','&quot;',@name,'&quot;')"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@name"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('STATUS ',@status)"/>
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="@status"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'angle'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Config">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'dataPattern'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'debug'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'defResolution'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'extent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'fontSet'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'imageType'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'include'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'maxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="projection">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'resolution'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'scaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'shapePath'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'symbolSet'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'templatePattern'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'units'"/>
    </xsl:call-template>
    <xsl:apply-templates select="Web">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="OutputFormat">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="QueryMap">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="Reference">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="Legend">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ScaleBar">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="Layer">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="Symbol">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
    </xsl:call-template>
  </xsl:template>

</xsl:stylesheet>


