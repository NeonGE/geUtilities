#Search for all the library files
file(GLOB sources source/*.cpp)
file(GLOB includes include/*.h)

#Search for Externals
file(GLOB md5code source/externals/md5.* )
file(GLOB md5inc include/externals/md5.*)

file(GLOB tetgencode source/externals/tetgen.cxx source/externals/predicates.cxx)
file(GLOB tetgeninc include/externals/tetgen.h)

file(GLOB lz4code source/externals/lz4.c)
file(GLOB lz4inc include/externals/lz4.h)

file(GLOB jsoncode include/externals/json.hpp)

if(WIN32)
	file(GLOB_RECURSE platformSrc source/win32/*.cpp)
	file(GLOB_RECURSE platformInc include/win32/*.h)
	set(platformDir include/win32)
else()
	set(platformSrc "")
	set(platformInc "")
	set(platformDir "")
endif()

#Add this library
if(GE_SHARED_LIBS)
	add_library(geUtilities SHARED ${sources} ${includes} ${md5code} ${md5inc} ${tetgencode} ${tetgeninc} ${lz4code} ${lz4inc} ${jsoncode} ${platformSrc} ${platformInc})
	add_compile_definitions(GE_UTILITY_EXPORTS)
else()
	add_library(geUtilities STATIC ${sources} ${includes} ${md5code} ${md5inc} ${tetgencode} ${tetgeninc} ${lz4code} ${lz4inc} ${jsoncode} ${platformSrc} ${platformInc})
	add_compile_definitions(GE_STATIC_LIB)
endif()

source_group("Externals\\MD5" FILES ${md5code} ${md5inc})
source_group("Externals\\TetGen" FILES ${tetgencode} ${tetgeninc})
source_group("Externals\\LZ4" FILES ${lz4code} ${lz4inc})
source_group("Externals\\JSON" FILES ${jsoncode})
source_group("Win32" FILES ${platformSrc} ${platformInc})

target_include_directories(geUtilities PUBLIC include include/externals include/externals/simdpp ${platformDir})

#Set the target name for this library
#set_target_properties(geUtilities PROPERTIES SUFFIX "d")

target_link_libraries(geUtilities PUBLIC DbgHelp IPHLPAPI Rpcrt4)

if(MSVC)
	if(NOT GE_ENABLE_RTTI)
		target_compile_options(geUtilities PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/GR->)
	endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	if(NOT BSF_ENABLE_EXCEPTIONS)
		target_compile_options(geUtilities PUBLIC -fno-exceptions)
	endif()
	if(NOT GE_ENABLE_RTTI)
		target_compile_options(geUtilities PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
	endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	if(NOT BSF_ENABLE_EXCEPTIONS)
		target_compile_options(geUtilities PUBLIC -fno-exceptions)
	endif()
	if(NOT GE_ENABLE_RTTI)
		target_compile_options(geUtilities PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
	endif()
else()
#TODO_OTHER_COMPILERS_GO_HERE
endif()