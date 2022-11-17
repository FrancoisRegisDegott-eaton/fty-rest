cmake_minimum_required(VERSION 3.13)
##############################################################################################################

project(fty_rest
  VERSION "1.0.0"
  DESCRIPTION "Common core REST API for 42ity project"
  LANGUAGES CXX)

# Workaround '-' vs '_'
set(PROJECT_NAME_HYPHEN fty-rest)

##############################################################################################################
find_package(fty-cmake      PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
find_package(fty-cmake-rest PATHS ${CMAKE_BINARY_DIR}/fty-cmake-rest)
resolve_lib(tntnet)
##############################################################################################################

etn_target(static ${PROJECT_NAME}-lib
    SOURCES
        src/*.cc
        src/*.h
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    FLAGS
        -Wno-format-nonliteral
    USES
        fty-cmake-rest
        fty-utils
        cidr
        magic
        cxxtools
        tntnet
        tntdb
        czmq
        z
        mlm
        fty_proto
        fty_shm
        fty_asset_activator
        fty_common
        fty_common_logging
        fty_common_rest
        fty_common_db
        fty_common_mlm
        fty_common_messagebus
        fty_common_dto
        fty-pack
    PRIVATE
)

# Define the servlet
etn_target(shared ${PROJECT_NAME}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    USES
        ${PROJECT_NAME}-lib
)

# Get the ecpp
file(GLOB ECPP_FILES
  ecpp/*.ecpp
)

# Transform ecpp files
fty_ecppbuilder(
  TARGET ${PROJECT_NAME}
  WORKDIR src
  ECPP
    ${ECPP_FILES}
)

# bios-csv
add_subdirectory(bios-csv)

# scripts
add_subdirectory(scripts)

# Install tntnet mapping files in etc/tntnet/bios.d/
# Files, *in order*
set(MAPPING_FILES
  00_start.xml
  10_common_statics.xml
  20_common_basics.xml
  50_main_api_start.xml
  51_asset_api_fty-rest.xml
  60_main_api_until_eula.xml
  89_main_api_end.xml
  99_end.xml
)

foreach(file ${MAPPING_FILES})
  set(f_in ${PROJECT_SOURCE_DIR}/resources/${file})
  set(f_out ${PROJECT_BINARY_DIR}/resources/${file})
  configure_file(${f_in} ${f_out} @ONLY)
  install(FILES ${f_out} DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tntnet/bios.d/)
endforeach()

# Install tntnet mapping files examples in usr/share/fty-rest/examples
# tntnet.xml.example: concat of all mapping files *in order*
set(TNTNET_XML_EXAMPLE ${PROJECT_BINARY_DIR}/tntnet.xml.example)
foreach(file ${MAPPING_FILES})
  set(f ${PROJECT_BINARY_DIR}/resources/${file})
  install(FILES ${f} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME_HYPHEN}/examples/ RENAME ${file}.example)
  # cat f in TNTNET_XML_EXAMPLE
  file(READ ${f} aux)
  file(APPEND ${TNTNET_XML_EXAMPLE} "${aux}")
endforeach()
install(FILES ${TNTNET_XML_EXAMPLE} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME_HYPHEN}/examples/)

#install swagger
install(FILES "resources/automation.yaml" DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/public_swagger/)
