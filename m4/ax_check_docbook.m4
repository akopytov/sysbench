dnl ---------------------------------------------------------------------------
dnl Macro: AX_CHECK_DOCBOOK
dnl Check for availability of various DocBook utilities and perform necessary
dnl substitutions
dnl ---------------------------------------------------------------------------

AC_DEFUN([AX_CHECK_DOCBOOK], [
# It's just rude to go over the net to build
XSLTPROC_FLAGS=--nonet
DOCBOOK_ROOT=

for i in /etc/xml/catalog /usr/local/etc/xml/catalog /opt/local/etc/xml/catalog ;
do
	if test -f $i; then
  		XML_CATALOG="$i"
	fi
done

if test -z "$XML_CATALOG" ; then
	for i in /usr/share/sgml/docbook/stylesheet/xsl/nwalsh /usr/share/sgml/docbook/xsl-stylesheets/ /opt/local/share/xsl/docbook-xsl/xhtml/ ;
	do
		if test -d "$i"; then
			DOCBOOK_ROOT=$i
		fi
	done

	# Last resort - try net
	if test -z "$DOCBOOK_ROOT"; then
		XSLTPROC_FLAGS=
	fi
else
	CAT_ENTRY_START='<!--'
	CAT_ENTRY_END='-->'
fi

AC_CHECK_PROG(XSLTPROC,xsltproc,xsltproc,)
XSLTPROC_WORKS=no
if test -n "$XSLTPROC"; then
	AC_MSG_CHECKING([whether xsltproc works])

	if test -n "$XML_CATALOG"; then
		DB_FILE="http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl"
	else
		DB_FILE="$DOCBOOK_ROOT/docbook.xsl"
	fi
	
	$XSLTPROC $XSLTPROC_FLAGS $DB_FILE >/dev/null 2>&1 << END
<?xml version="1.0" encoding='ISO-8859-1'?>
<!DOCTYPE book PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN" "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">
<book id="test">
</book>
END
	if test "$?" = 0; then
		XSLTPROC_WORKS=yes
	fi
	AC_MSG_RESULT($XSLTPROC_WORKS)
fi
AM_CONDITIONAL(have_xsltproc, test "$XSLTPROC_WORKS" = "yes")

AC_SUBST(XML_CATALOG)
AC_SUBST(XSLTPROC_FLAGS)
AC_SUBST(DOCBOOK_ROOT)
AC_SUBST(CAT_ENTRY_START)
AC_SUBST(CAT_ENTRY_END)
])
