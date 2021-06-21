set(DOXYGEN_EXTRACT_ON YES)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

file(GLOB_RECURSE headerList ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h)

doxygen_add_docs(docs ${headerList}
	mainpage.md WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include)