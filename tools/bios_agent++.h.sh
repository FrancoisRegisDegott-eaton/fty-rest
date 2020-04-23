#!/bin/bash

#
# Copyright (C) 2015 - 2020 Eaton
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#


#! \file   bios_agent++.h.sh
#  \author Tomas Halman <TomasHalman@Eaton.com>
#  \author Arnaud Quette <ArnaudQuette@Eaton.com>
#  \brief  This script compiles bios_agent++.h.in into bios_agent++.h

DIR=$(dirname $0)
DIR=$(realpath $DIR)
[ -z "${abs_top_srcdir-}" ] && abs_top_srcdir="$DIR/.."
[ -z "${abs_top_builddir-}" ] && abs_top_builddir="$abs_top_srcdir"

INCLUDE="$abs_top_srcdir/include/bios_agent.h"
TEMPLATE="$abs_top_srcdir/tools/bios_agent++.h.in"
OUTPUT="$abs_top_builddir/include/bios_agent++.h"
mkdir -p "`dirname "$OUTPUT"`" || exit $?

for _AWK in gawk nawk oawk awk; do
    AWK="`which "${_AWK}" 2>/dev/null`" || AWK=""
    [ -n "$AWK" ] && [ -x "$AWK" ] && break
done
if [ -z "$AWK" ] || [ ! -x "$AWK" ] ; then
    echo "No usable AWK was found!" >&2
    if [ -s "$OUTPUT" ]; then
        echo "Keeping the existing '$OUTPUT' file" >&2
        exit 0
    fi
    exit 1
fi

extract_bios_h(){
$AWK '
BEGIN{
    FS="[ \t,()]+";
    IGNORE["bios_agent_new"] = 1;
    IGNORE["bios_agent_destroy"] = 1;
}
/^BIOS_EXPORT/{
    type = "";
    for( a = 2; a <= NF; a++ ) {
        type = sprintf("%s%s ", type, $a);
    }
    gsub(/[[:space:]]{2,}/," ",type);
    gsub(/^[[:space:]]+/,"",type);
    gsub(/[[:space:]]+$/,"",type);
}
/^[\t\ ]+bios_agent_.+\) *;/{
    # printf("%s %s\n", type, $0 );
    funcName=$2;
    if( funcName in IGNORE ) next;
    funcNameObj=substr($2,12)
    i = 3;
    idx = 0;
    while( i < NF ) {
        ptype = $i
        i++;
        if( ptype == "const" ) {
            ptype = sprintf("%s %s", ptype, $i);
            i++;
        }
        ptypes[idx] = ptype;
        name = $i
        pnames[idx] = $i;
        i++;
        idx++
    }
    printf("    %s %s( ",type, funcNameObj);
    for( a=1; a<idx; a++ ) {
        printf(" %s %s", ptypes[a], pnames[a]);
        if( a != idx - 1) printf(", "); ;
    }
    printf(" ) { ");
    if( type != "void" ) printf("return ");
    if( substr( pnames[0], 0, 2) == "**" ) {
        printf("%s( &_bios_agent",funcName);
    } else {
        printf("%s( _bios_agent",funcName);
    }
    for( a=1; a<idx; a++ ) {
        pname = pnames[a]
        gsub(/^\*+/,"",pname);
        printf(", %s", pname);
    }
    printf(" ); };\n");
    type = "";
}
'< "$INCLUDE" | sed -e 's,[ \t][ \t]*, ,g' -e 's,^[ \t]*\([^ \t].*\)$,    \1,' -e 's,[ \t]*$,,g'
# The SED filter should ensure that all nonempty lines do not have two or more
# conseqent spaces except a leading 4-space indentation to minimize "git diff"
}

(
#set -x
echo "/* THIS FILE IS GENERATED FROM tools/bios_agent++.h.in, DO NOT EDIT THIS FILE! */"
echo ""
while IFS='' read line ; do
    if [ "$line" = "@extract_bios_agent_h@" ] ; then
        extract_bios_h
    else
        printf '%s\n' "$line"
    fi
done <"$TEMPLATE"
) >"$OUTPUT.tmp"

if [ $? = 0 ]; then
    mv -f "$OUTPUT.tmp" "$OUTPUT"
    exit $?
fi

echo "Error generating '$OUTPUT' file" >&2
[ x"$DEBUG" = xyes ] || rm -f "$OUTPUT.tmp"
if [ -s "$OUTPUT" ]; then
    echo "Keeping the existing '$OUTPUT' file" >&2
    exit 0
fi
exit 1
