XSLTPROC=xsltproc
XSLTPROC_FLAGS=@XSLTPROC_FLAGS@
XHTML_STYLESHEET=$(srcdir)/xsl/xhtml.xsl
CHUNK_XHTML_STYLESHEET=$(srcdir)/xsl/xhtml-chunk.xsl
XML_CATALOG_FILES=$(srcdir)/xsl/catalog.xml

htmldir = $(prefix)/share/doc/sysbench
dist_html_DATA = manual.html

EXTRA_DIST=manual.xml

SUBDIRS = xsl

if have_xsltproc

manual.html: ${top_srcdir}/doc/manual.xml
	XML_CATALOG_FILES=$(XML_CATALOG_FILES) $(XSLTPROC) $(XSLTPROC_FLAGS) -o $@ $(XHTML_STYLESHEET) $<

chunk: ${top_srcdir}/doc/manual.xml
	$(XSLTPROC) $(XSLTPROC_FLAGS) $(CHUNK_XHTML_STYLESHEET) $<

else

chunk:

manual.html:
	touch $@

endif

clean-local:
	$(RM) -f *.html

distclean-local:
	$(RM) -f xsl/catalog.xml
