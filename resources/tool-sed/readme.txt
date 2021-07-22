[zproject] sed'ification of original 99_end.xml (Makemodule-local.am)


./post-process-xml.sh ./99_end.xml.original ./99_end.xml
mv ./99_end.xml ..



<<<

Makemodule-local.am extract:

### This file contains the TNTNET settings distributed as an example for users
### Note: in daemon mode, "stderr > /dev/null" unless errorLog is defined
tntnet.xml.example: src/web/tntnet.xml Makefile
	$(call sed-tntnet-example,$<,$@)

99_end.xml.example: src/web/99_end.xml Makefile
	$(call sed-tntnet-example,$<,$@)

### Generate $2 from $1 in a way expected by OS image bundling
define sed-tntnet-example
	${SED} -e 's|\(.*\)\(<!--.*<dir>/</dir>.*-->.*\)|\1<dir>$(datarootdir)/@PACKAGE@/web</dir>\n\1<compPath>\n\1\ \ \ <entry>$(bios_libdir)</entry>\n\1\ \ \ <entry>$(pkglibdir)</entry>\n\1\ \ \ <entry>$(libdir)</entry>\n\1\ \ \ <entry>$(bios_libexecdir)</entry>\n\1</compPath>|' \
	       -e 's|<!-- <errorLog>/var/log/tntnet/error.log</errorLog> -->|<errorLog>/var/log/tntnet/error.log</errorLog>|' \
	       -e 's|<port>8000</port>|<port>80</port>|' \
	       -e 's|<port>8443</port>|<port>443</port>|' \
	       -e 's|<!-- ssl start.*|<!-- ssl start -->|' \
	       -e 's|^\([[:blank:]]*\)ssl end -->|\1<!-- ssl end -->|' \
	       -e 's|<!--\ <daemon>0</daemon>\ -->|<daemon>1</daemon>|' $(1) > $(2) \
	|| rm -f $(2)
endef

>>>
