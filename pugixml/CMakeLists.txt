#file(GLOB_RECURSE SRC_FILES *.cpp *.c *.cc)
#file(GLOB_RECURSE HEADER_FILES *.h *.hpp)

add_executable(xml main.cpp ${PROJECT_SOURCE_DIR}/module/pugixml/pugixml.cpp)
target_link_libraries(xml ${LIBS})
