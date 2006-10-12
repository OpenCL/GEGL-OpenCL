#!/bin/sh
#
# Utility script to merge an xml snippet from one file into a document.
#
# To insert the file foo.inc into bar.xml,
# after the first line containing the marker <!--foo--> enter
#
# xml_insert.sh bar.xml foo foo.inc
#
# adding yet another argument will encode <,> and & as html entites.
#
# 2005 © Øyvind Kolås
#
# FIXME: add argument checking / error handling

which tempfile > /dev/null && TMP_FILE=`tempfile` || TMP_FILE="/tmp/temp_file"

cp $1 $TMP_FILE

SPLIT=`grep -n "<\!--$2-->" $TMP_FILE|head -n 1|sed -e "s/:.*//"`;
head -n $SPLIT $TMP_FILE > $1
if [ -z "$4" ]; then
  cat $3 >> $1
else
  cat $3 | sed -e "s/\&/\&amp;/g" -e "s/</\&lt;/g" -e "s/>/\&gt;/g" >> $1
fi
tail -n $((`wc -l $TMP_FILE | sed -e "s/ .*//"` - $SPLIT )) $TMP_FILE >> $1

rm $TMP_FILE
