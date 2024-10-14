get_filename_component(SIMPLE_THREADPOOL_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

list(APPEND CMAKE_MODULE_PATH ${SIMPLE_THREADPOOL_CMAKE_DIR})

if(NOT TARGET wwa-simple-threadpool)
    include("${SIMPLE_THREADPOOL_CMAKE_DIR}/wwa-simple-threadpool-target.cmake")
    add_library(wwa::simple-threadpool ALIAS wwa-simple-threadpool)
endif()
