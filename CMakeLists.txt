#Search for all the library files
file(GLOB sources source/*.cpp)
file(GLOB includes include/*.h)

#Search for Externals
file(GLOB_RECURSE md5source source/externals/md5.cpp)
file(GLOB_RECURSE md5include include/externals/md5.h)

#Add this library
add_library(geUtilities SHARED ${sources} ${includes} ${md5include} ${md5source})

source_group("Externals\\MD5" FILES ${md5include} ${md5source})

target_include_directories(geUtilities PUBLIC include)

#Set the target name for this library
if(NDEBUG)
	set_target_properties(geUtilities PROPERTIES SUFFIX "d")
endif()