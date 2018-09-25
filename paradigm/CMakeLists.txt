#file(GLOB_RECURSE SRC_FILES *.cpp *.c *.cc)
#file(GLOB_RECURSE HEADER_FILES *.h *.hpp)

add_executable(stl main.cpp)
target_link_libraries(stl ${LIBS})
