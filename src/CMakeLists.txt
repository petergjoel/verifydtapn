
add_subdirectory(Core)
add_subdirectory(DiscreteVerification)
add_executable(verifydtapn-${ARCH_TYPE} main.cpp)
add_dependencies(verifydtapn-${ARCH_TYPE} unfoldtacpn-ext)
target_link_libraries(verifydtapn-${ARCH_TYPE} Core DiscreteVerification 
    ${EXTERNAL_INSTALL_LOCATION}/lib/unfoldtacpn/libunfoldtacpn.a
    ${EXTERNAL_INSTALL_LOCATION}/lib/unfoldtacpn/libPetriEngine.a
    ${EXTERNAL_INSTALL_LOCATION}/lib/unfoldtacpn/libPetriParse.a
    ${EXTERNAL_INSTALL_LOCATION}/lib/unfoldtacpn/libPQL.a
    ${EXTERNAL_INSTALL_LOCATION}/lib/unfoldtacpn/libColored.a 
    ${EXTERNAL_INSTALL_LOCATION}/../unfoldtacpn-ext-prefix/src/unfoldtacpn-ext-build/external/lib/libglpk.a
    )
