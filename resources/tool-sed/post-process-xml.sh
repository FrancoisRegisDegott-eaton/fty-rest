#!/bin/sh
set +x

SED=/bin/sed

### Generate $2 from $1 in a way expected by OS image bundling
#sed-tntnet-example

${SED} -e 's|\(.*\)\(<!--.*<dir>/</dir>.*-->.*\)|\1<dir>@CMAKE_INSTALL_FULL_DATAROOTDIR@/@PROJECT_NAME@/web</dir>\n\1<compPath>\n\1\ \ \ <entry>@CMAKE_INSTALL_PREFIX@/lib/bios</entry>\n\1\ \ \ <entry>@CMAKE_INSTALL_FULL_LIBDIR@</entry>\n\1\ \ \ <entry>@CMAKE_INSTALL_FULL_LIBDIR@/@PROJECT_NAME@</entry>\n\1\ \ \ <entry>@CMAKE_INSTALL_FULL_LIBEXECDIR@/bios</entry>\n\1</compPath>|' \
       -e 's|<!-- <errorLog>/var/log/tntnet/error.log</errorLog> -->|<errorLog>/var/log/tntnet/error.log</errorLog>|' \
       -e 's|<port>8000</port>|<port>80</port>|' \
       -e 's|<port>8443</port>|<port>443</port>|' \
       -e 's|<!-- ssl start.*|<!-- ssl start -->|' \
       -e 's|^\([[:blank:]]*\)ssl end -->|\1<!-- ssl end -->|' \
       -e 's|<!--\ <daemon>0</daemon>\ -->|<daemon>1</daemon>|' $1 > $2 \
	|| rm -f $2

exit 0
