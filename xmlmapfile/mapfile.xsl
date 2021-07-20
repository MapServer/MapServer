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
  <xsl:if test="*[concat('ms:',name()) = $nodeName] | *[name() = $nodeName]">
    <xsl:choose>
      <xsl:when test="$quote = '1'">
        <xsl:value-of select="translate(substring-after($nodeName,'ms:'), $smallcase, $uppercase)"/><xsl:text> "</xsl:text><xsl:value-of select="dyn:evaluate($nodeName)"/><xsl:text>"</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="translate(substring-after($nodeName,'ms:'), $smallcase, $uppercase)"/><xsl:text> </xsl:text><xsl:value-of select="dyn:evaluate($nodeName)"/>
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

<xsl:template name="printExpression">
  <xsl:param name="indent"/>
  <xsl:param name="node" select="/"/>

  <xsl:if test="$node">
    <xsl:variable name="quote">
      <xsl:choose>
        <xsl:when test="(dyn:evaluate($node)/@type = 'CONSTANT') or (not(dyn:evaluate($node)/@type))">
          <xsl:value-of select='1'/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select='0'/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="textValue">
      <xsl:call-template name="getString">
        <xsl:with-param name="nodeName" select="$node"/>
        <xsl:with-param name="quote" select="$quote"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="quote" select="$quote"/>
      <xsl:with-param name="text" select="$textValue"/>
    </xsl:call-template>

  </xsl:if>
</xsl:template>

<xsl:template name="printSymbol">
  <xsl:param name="indent"/>
  <xsl:param name="node" select="/"/>

  <xsl:if test="$node">
    <xsl:variable name="quote">
      <xsl:choose>
        <xsl:when test="dyn:evaluate($node)/@type = 'NAME'">
          <xsl:value-of select='1'/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select='0'/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="textValue">
      <xsl:call-template name="getString">
        <xsl:with-param name="nodeName" select="$node"/>
        <xsl:with-param name="quote" select="$quote"/>
      </xsl:call-template>
    </xsl:variable>
    
    <xsl:call-template name="print">
      <xsl:with-param name="quote" select="$quote"/>
      <xsl:with-param name="text" select="$textValue"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>

  </xsl:if>
</xsl:template>

  <xsl:template match="/">
    <xsl:apply-templates select="ms:SymbolSet"/>
    <xsl:apply-templates select="ms:LayerSet"/>
    <xsl:apply-templates select="ms:Map"/>
  </xsl:template>

<xsl:template match="ms:color | ms:backgroundColor | ms:outlineColor | ms:shadowColor | ms:imageColor | ms:offsite">
  <xsl:param name="indent"/>
   <xsl:choose>
     <xsl:when test="starts-with(name(), 'ms:')">
       <xsl:call-template name="print">
         <xsl:with-param name="text" select="concat(translate(substring-after(name(), 'ms:'), $smallcase, $uppercase),' ', @red, ' ', @green, ' ', @blue)"/>
         <xsl:with-param name="indent" select="$indent"/>
       </xsl:call-template>
     </xsl:when>
     <xsl:otherwise>
       <xsl:call-template name="print">
         <xsl:with-param name="text" select="concat(translate(name(), $smallcase, $uppercase),' ', @red, ' ', @green, ' ', @blue)"/>
         <xsl:with-param name="indent" select="$indent"/>
       </xsl:call-template>
     </xsl:otherwise>
   </xsl:choose>
</xsl:template>

 <xsl:template match="ms:colorAttribute | ms:outlineColorAttribute">
  <xsl:param name="indent"/>
  <xsl:param name="tagName" select="translate(name(), $smallcase, $uppercase)"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="concat($tagName, ' ',.)"/>
    <xsl:with-param name="indent" select="$indent"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="ms:size | ms:offset | ms:keySize | ms:keySpacing | ms:shadowSize | ms:anchorPoint">
  <xsl:param name="indent"/>
  <xsl:choose>
    <xsl:when test="starts-with(name(), 'ms:')">
      <xsl:call-template name="print">
        <xsl:with-param name="text" select="concat(translate(substring-after(name(), 'ms:'), $smallcase, $uppercase), ' ', @x, ' ', @y)"/>
        <xsl:with-param name="indent" select="$indent"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="print">
        <xsl:with-param name="text" select="concat(translate(name(), $smallcase, $uppercase), ' ', @x, ' ', @y)"/>
        <xsl:with-param name="indent" select="$indent"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="ms:Points">
  <xsl:param name="indent"/>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'POINTS'"/>
    <xsl:with-param name="indent" select="$indent - 1"/>
  </xsl:call-template>
  <xsl:for-each select="ms:point">
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat(@x,' ',@y)"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:for-each>
  <xsl:call-template name="print">
    <xsl:with-param name="text" select="'END'"/>
    <xsl:with-param name="indent" select="$indent - 1"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="ms:Metadata | ms:Validation">
  <xsl:param name="indent"/>
  <xsl:choose>
    <xsl:when test="starts-with(name(), 'ms:')">
      <xsl:call-template name="print">
        <xsl:with-param name="text" select="translate(substring-after(name(), 'ms:'),$smallcase, $uppercase)"/>
        <xsl:with-param name="indent" select="$indent -1"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="print">
        <xsl:with-param name="text" select="translate(name(),$smallcase, $uppercase)"/>
        <xsl:with-param name="indent" select="$indent -1"/>
      </xsl:call-template>
    </xsl:otherwise>
    </xsl:choose>
  <xsl:for-each select="ms:item">
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

<xsl:template match="ms:Config">
  <xsl:param name="indent"/>
  <xsl:for-each select="ms:item">
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('CONFIG ','&#34;',@name,'&#34; ','&#34;',.,'&#34;')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:for-each>
 </xsl:template>


<xsl:template match="ms:projection">
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

  <xsl:template match="ms:pattern">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="concat('PATTERN ',.,' END')"/>
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:QueryMap">
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
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:style'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>

  </xsl:template>


  <xsl:template match="ms:Web">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'WEB'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:browseFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:empty'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:error'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:imagePath'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:tempPath'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:imageUrl'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:legendFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:log'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxTemplate'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Metadata">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minTemplate'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:queryFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Validation">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:OutputFormat">
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
      <xsl:with-param name="node" select="'ms:driver'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:extension'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:for-each select="ms:formatOption">
       <xsl:call-template name="print">
         <xsl:with-param name="indent" select="$indent"/>
         <xsl:with-param name="text" select="concat('FORMATOPTION ','&#34;',.,'&#34;')"/>
       </xsl:call-template>
    </xsl:for-each>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:imageMode'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:mimeType'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:transparent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Reference">
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
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:extent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:image'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:marker'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:markerSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxBoxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minBoxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:Legend">
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
    <xsl:apply-templates select="ms:imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:keySize">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:keySpacing">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:Label">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:postLabelCache'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Label">
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
      <xsl:with-param name="node" select="'ms:align'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:angle'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:antialias'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:buffer'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxOverlapAngle'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:colorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'COLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:encoding'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="printExpression">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:expression'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:font'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:force'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxLength'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minDistance'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minFeatureSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:offset">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:outlineWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:partials'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:priority'"/>
    </xsl:call-template>
   <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:repeatDistance'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:shadowColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:shadowSize">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:size'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Style">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:text'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>    
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:wrap'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:ScaleBar">
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
      <xsl:with-param name="node" select="'ms:align'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:backgroundColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:intervals'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Label">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:position'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:postLabelCache'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:style'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:transparent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:units'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:LabelLeader">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'LEADER'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:gridstep'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxdistance'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Style">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:Style">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'STYLE'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:angle'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:antialias'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:colorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'COLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:geomTransform'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:initialGap'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:gap'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:lineCap'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:lineJoin'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:lineJoinMaxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:pattern">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:polarOffset'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:offset">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:opacity'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:size'"/>
    </xsl:call-template>
    <xsl:call-template name="printSymbol">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:symbol'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:width'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Class">
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
    <xsl:apply-templates select="ms:backgroundColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:color">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:debug'"/>
    </xsl:call-template>
    <xsl:call-template name="printExpression">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:expression'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:group'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:keyImage'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:for-each select="ms:Label">
      <xsl:apply-templates select=".">
        <xsl:with-param name="indent" select="$indent + 1"/>
      </xsl:apply-templates>
    </xsl:for-each>
    <xsl:apply-templates select="ms:LabelLeader">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxSize'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:outlineColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:outlineColorAttribute">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="tagName" select="'OUTLINECOLOR'"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:size'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Style">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="printSymbol">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:symbol'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:text'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Symbol">
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
    <xsl:apply-templates select="ms:anchorPoint">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:antialias'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:character'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
     <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:filled'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:font'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:image'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Points">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:transparent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Feature">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'FEATURE'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Points">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:items'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:text'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:wkt'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Grid">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'GRID'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelFormat'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxArcs'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxInterval'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxSubdivide'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minArcs'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minInterval'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minSubdivide'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:Join">
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
      <xsl:with-param name="node" select="'ms:connection'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:connectionType'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:from'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:table'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:to'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
   <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:Layer">
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
    <xsl:apply-templates select="ms:Class">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:classGroup'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:classItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:connection'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:connectionType'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:data'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:debug'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:extent'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Feature">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="printExpression">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:filter'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:filterItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:footer'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Grid">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:group'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:header'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Join">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelCache'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelMaxScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelMinScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:labelRequires'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:mask'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxFeatures'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxGeoWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxScaleDenom'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Metadata">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minGeoWidth'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:minScaleDenom'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:offsite">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:opacity'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:plugin'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:postLabelCache'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Cluster">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:for-each select="ms:processing">
      <xsl:call-template name="print">
        <xsl:with-param name="indent" select="$indent"/>
        <xsl:with-param name="text">
          <xsl:value-of select="concat('PROCESSING ','&#34;',.,'&#34;')"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:for-each>
    <xsl:apply-templates select="ms:projection">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:requires'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:sizeUnits'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:styleItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:symbolScaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:template'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:tileIndex'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:tileItem'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:tolerance'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:toleranceUnits'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:transform'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:units'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Validation">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:Cluster">
    <xsl:param name="indent"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'CLUSTER'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxdistance'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:region'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="printExpression">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:group'"/>
    </xsl:call-template>
    <xsl:call-template name="printExpression">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:filter'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
      <xsl:with-param name="indent" select="$indent - 1"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template match="ms:SymbolSet">
    <xsl:param name="indent" select="0"/>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'SYMBOLSET&#xa;'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Symbol">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
   <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="ms:LayerSet">
    <xsl:param name="indent" select="0"/>
    <xsl:apply-templates select="ms:Layer">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="ms:Map">
    <xsl:param name="indent" select="0"/>
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
      <xsl:with-param name="node" select="'ms:angle'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Config">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:dataPattern'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:debug'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:defResolution'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:extent'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:fontSet'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:imageColor">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:imageType'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:for-each select="ms:include">
      <xsl:call-template name="print">
        <xsl:with-param name="indent" select="$indent"/>
        <xsl:with-param name="text">
          <xsl:value-of select="concat('INCLUDE ','&#34;',.,'&#34;')"/>
        </xsl:with-param>
      </xsl:call-template>
      </xsl:for-each>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:maxSize'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:projection">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:resolution'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:scaleDenom'"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:shapePath'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:size">
      <xsl:with-param name="indent" select="$indent"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:symbolSet'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:templatePattern'"/>
      <xsl:with-param name="quote" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="print">
      <xsl:with-param name="indent" select="$indent"/>
      <xsl:with-param name="node" select="'ms:units'"/>
    </xsl:call-template>
    <xsl:apply-templates select="ms:Web">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:OutputFormat">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:QueryMap">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:Reference">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:Legend">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:ScaleBar">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:Layer">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="ms:Symbol">
      <xsl:with-param name="indent" select="$indent + 1"/>
    </xsl:apply-templates>
    <xsl:call-template name="print">
      <xsl:with-param name="text" select="'END&#xa;'"/>
    </xsl:call-template>
  </xsl:template>

</xsl:stylesheet>


