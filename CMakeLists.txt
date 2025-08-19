cmake_minimum_required( VERSION 3.15 )
project( wedselsware )

add_definitions( -DUNICODE -D_UNICODE )
add_compile_options( /utf-8 )

set( CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE )
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

set( CMAKE_BUILD_TYPE Release )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DEBUG:NONE" )
add_definitions( -D_ITERATOR_DEBUG_LEVEL=0 )

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ffmpeg/include
    ${CMAKE_SOURCE_DIR}/fftw
)

link_directories(
    "${CMAKE_SOURCE_DIR}/ffmpeg/lib"
    "${CMAKE_SOURCE_DIR}/fftw"
)

file( GLOB_RECURSE SOURCES CONFIGURE_DEPENDS code/*.cpp )
add_executable( wedselsware WIN32 resource.rc ${SOURCES} )
target_link_libraries( wedselsware PRIVATE libfftw3-3 avformat avcodec avutil swresample DbgHelp )

target_compile_options( wedselsware PRIVATE /GL /GS- /Gy /Zc:inline )
set_target_properties( wedselsware PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE )
target_link_options( wedselsware PRIVATE /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO )