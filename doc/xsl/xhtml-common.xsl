<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 version="1.0">

<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="section.autolabel" select="1"/>
<xsl:param name="chapter.autolabel" select="1"/>
<xsl:param name="ulink.target" select="''"/>
 
<!-- Custom template for programlisting, screen and synopsis to generate a gray
     background to the item. -->
<xsl:template match="programlisting|screen|synopsis">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="vendor" select="system-property('xsl:vendor')"/>
  <xsl:variable name="id"><xsl:call-template name="object.id"/></xsl:variable>
 
  <xsl:if test="@id">
    <a href="{$id}"/>
  </xsl:if>
 
  <xsl:choose>
    <xsl:when test="$suppress-numbers = '0'
                    and @linenumbering = 'numbered'
                    and $use.extensions != '0'
                    and $linenumbering.extension != '0'">
      <xsl:variable name="rtf">
        <xsl:apply-templates/>
      </xsl:variable>
      <!-- Change the color background color in the line below. -->
      <table border="0" style="background: #E0E0E0;" width="90%">
      <tr><td>
      <pre class="{name(.)}">
        <xsl:call-template name="number.rtf.lines">
          <xsl:with-param name="rtf" select="$rtf"/>
        </xsl:call-template>
      </pre>
      </td></tr></table>
    </xsl:when>
    <xsl:otherwise>
      <!-- Change the color background color in the line below. -->
      <table border="0" style="background: #E0E0E0;" width="90%">
      <tr><td>
      <pre class="{name(.)}">
        <xsl:apply-templates/>
      </pre>
      </td></tr></table>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template> 
 
</xsl:stylesheet>
