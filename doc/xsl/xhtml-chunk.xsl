<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 version="1.0">
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/xhtml/chunk.xsl"/>
<xsl:import href="xhtml-common.xsl"/>
 
<xsl:template name="process-chunk">
  <xsl:param name="prev" select="."/>
  <xsl:param name="next" select="."/>
 
  <xsl:variable name="ischunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>
 
  <xsl:variable name="chunkfn">
    <xsl:if test="$ischunk='1'">
      <xsl:apply-templates mode="chunk-filename" select="."/>
    </xsl:if>
  </xsl:variable>
 
  <xsl:if test="$ischunk='0'">
    <xsl:message>
      <xsl:text>Error </xsl:text>
      <xsl:value-of select="name(.)"/>
      <xsl:text> is not a chunk!</xsl:text>
    </xsl:message>
  </xsl:if>
 
  <xsl:variable name="filename">
    <xsl:call-template name="make-relative-filename">
      <xsl:with-param name="base.dir" select="$base.dir"/>
      <xsl:with-param name="base.name" select="$chunkfn"/>
    </xsl:call-template>
  </xsl:variable>
 
<!-- FIXME: use Strict when the problems with width on td/th are
  sorted out. Not yet. -->
  <xsl:call-template name="write.chunk.with.doctype">
    <xsl:with-param name="filename" select="$filename"/>
    <xsl:with-param name="indent" select="'yes'"/>
   <xsl:with-param name="doctype-public">-//W3C//DTD XHTML 1.0 Transitional//EN</xsl:with-param>
   <xsl:with-param name="doctype-system">http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd</xsl:with-param>
    <xsl:with-param name="content">
      <xsl:call-template name="chunk-element-content">
        <xsl:with-param name="prev" select="$prev"/>
        <xsl:with-param name="next" select="$next"/>
      </xsl:call-template>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>
 
</xsl:stylesheet>
