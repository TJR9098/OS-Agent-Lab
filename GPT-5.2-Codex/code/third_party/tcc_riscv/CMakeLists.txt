cmake_minimum_required(VERSION 3.0)
set(TCC_VERSION_MAJOR 0)
set(TCC_VERSION_MINOR 1)
project(TCC VERSION ${TCC_VERSION_MAJOR}.${TCC_VERSION_MINOR} LANGUAGES CXX)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 14)

set(FRONT_END_DIR ${CMAKE_SOURCE_DIR}/front-end)

add_subdirectory(front-end)
add_subdirectory(test)
#add_subdirectory(mid-end)
#add_subdirectory(backend-end)
#add_subdirectory(ir)
#add_subdirectory(tools)

set(TCC_COMPONENTS tcc-front-end tcc-test)
set(tcc_targets)

foreach (COMPONENT ${TCC_COMPONENTS})
message(STATUS "Adding Component ${COMPONENT}")
set (mainfest ${CMKAE_CURRENT_BINARY_DIR}/target_mainfest_${COMPONENT}.txt)
  add_custom_command(
      OUTPUT ${mainfest}
      COMMAND ${CMAKE_COMMAND} "-DCMAKE_INSTALL_COMPONENT=${COMPONENT}"
                               -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
      COMMENT "Compiling ${COMPONENT}")
  list(APPEND tcc_targets ${mainfest})
endforeach(COMPONENT)

add_custom_target(tcc DEPENDS ${tcc_targets})
