cmake_minimum_required(VERSION 3.13)
##############################################################################################################

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

# pre create
execute_process(
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/src/git_details.sh
  OUTPUT_FILE ${PROJECT_BINARY_DIR}/scripts/.git_details-${PROJECT_NAME_HYPHEN}
)

configure_file(src/bios-passwd.in ${PROJECT_BINARY_DIR}/scripts/bios-passwd @ONLY)
configure_file(src/testpass.sh ${PROJECT_BINARY_DIR}/scripts/testpass.sh @ONLY)

## libexec
install(FILES ${PROJECT_BINARY_DIR}/scripts/bios-passwd DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}/${PROJECT_NAME_HYPHEN})
install(FILES ${PROJECT_BINARY_DIR}/scripts/testpass.sh DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}/${PROJECT_NAME_HYPHEN})
## lib
install(FILES ${PROJECT_BINARY_DIR}/scripts/bios-passwd DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME_HYPHEN})
install(FILES ${PROJECT_BINARY_DIR}/scripts/testpass.sh DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_NAME_HYPHEN})
## share
install(FILES ${PROJECT_BINARY_DIR}/scripts/.git_details-${PROJECT_NAME_HYPHEN} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/bios)
