#!/bin/sh
#
# Utility script to merge an xml snippet from one file into a document.
#
# To insert the file foo.inc into bar.xml,
# after the first line containing the marker <!--foo--> enter
#
# xml_insert.sh bar.xml foo foo.inc
#
# 2005 © Øyvind Kolås
#
# FIXME: add argument checking / error handling

which tempfile > /dev/null && TMP_FILE=`tempfile` || TMP_FILE="/tmp/temp_file"

cp $1 $TMP_FILE

SPLIT=`grep -n "<\!--$2-->" $TMP_FILE|head -n 1|sed -e "s/:.*//"`;
head -n $SPLIT $TMP_FILE > $1
cat $3 >> $1
tail -n $((`wc -l $TMP_FILE | sed -e "s/ .*//"` - $SPLIT )) $TMP_FILE >> $1

rm $TMP_FILE
