cmake_minimum_required(VERSION 3.13)
##############################################################################################################

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

set(EXE_NAME "bios-csv")

# Build exe
etn_target(exe ${EXE_NAME}
    SOURCES
        bios-csv.cc
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    USES
        ${PROJECT_NAME}-lib
)

# Install usr/libexec/fty-rest/bios-csv
install(FILES ${PROJECT_BINARY_DIR}/bios-csv/${EXE_NAME} DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}/${PROJECT_NAME_HYPHEN})
