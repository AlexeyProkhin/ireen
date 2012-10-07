CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Ireen)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Library version
set(CMAKE_IREEN_VERSION_MAJOR 0 CACHE INT "Major Ireen version number" FORCE)
set(CMAKE_IREEN_VERSION_MINOR 1 CACHE INT "Minor Ireen version number" FORCE)
set(CMAKE_IREEN_VERSION_PATCH 0 CACHE INT "Release Ireen version number" FORCE)
set(CMAKE_IREEN_VERSION_STRING "${CMAKE_IREEN_VERSION_MAJOR}.${CMAKE_IREEN_VERSION_MINOR}.${CMAKE_IREEN_VERSION_PATCH}" CACHE STRING "Ireen version string" FORCE)

# Options
option(IREEN_USE_EXTERNAL_K8JSON "Use external k8json library" OFF)
option(IREEN_USE_INTERNAL_HMAC "Use internal hmac-sha256 implemntation instead of QCA2" OFF)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hmac")
    set(IREEN_USE_INTERNAL_HMAC OFF)
endif()

# Library sources
file(GLOB SRC *.cpp core/*.cpp metainfo/*.cpp)

# Library headers
file(GLOB HDR *.h   core/*.h   metainfo/*.h)

# Build k8json
if (NOT IREEN_USE_EXTERNAL_K8JSON)
    set(K8JSON_BUILD_STATIC_LIBRARY ON)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(K8JSON_CXX_FLAGS -fPIC)
    endif()
    add_subdirectory(3rdparty/k8json)
endif()
LIST(APPEND EXTRA_LIBS k8json)

# Require QT 4.6
SET(QT_MIN_VERSION "4.6.0")

# Search for QT4
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtNetwork QtGui REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE(ireenMacros)


if(IREEN_USE_INTERNAL_HMAC)
    FILE(GLOB_RECURSE HMAC_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hmac_sha2.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sha2.c"
    )
    LIST(APPEND SRC ${HMAC_SRC})
else()
    FIND_PACKAGE(QCA2)
    if( NOT QCA2_FOUND )
        message(FATAL_ERROR "QCA2 not found. Can't build OSCAR protocol" )
    endif( NOT QCA2_FOUND )
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
    ADD_DEFINITIONS(-DOSCAR_USE_QCA2)
    LIST(APPEND EXTRA_LIBS ${QCA2_LIBRARIES})
endif()

# Check if qt was built with openssl support
if( QT_QCONFIG MATCHES "openssl" )
    ADD_DEFINITIONS(-DIREEN_SSL_SUPPORT=1)
else()
    LIST(REMOVE_ITEM SRC oscarauth.cpp)
    LIST(REMOVE_ITEM HDR oscarauth.h)
endif()

# Add include directories
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    core
    3rdparty
)

# Generate moc files
IREEN_WRAP_CPP(MOC_SRC ${HDR})

# This project will generate library
ADD_LIBRARY(ireen SHARED ${SRC} ${MOC_SRC} ${HDR} )


#compiler flags
#fixme add macro to clang
if(CMAKE_COMPILER_IS_GNUCXX)
    IF(SYMBIAN)
        #disable stupid warnings
        ADD_DEFINITIONS("-w")
    ELSE()
        ADD_DEFINITIONS("-Wall -Wextra" ) #add -Werror
    ENDIF()
    if(NOT WIN32)
        ADD_DEFINITIONS("-fvisibility=hidden" )
    endif(NOT WIN32)
elseif(MSVC)
    ADD_DEFINITIONS("/W3" )
endif()

set_target_properties(ireen PROPERTIES
    VERSION ${CMAKE_IREEN_VERSION_STRING}
    SOVERSION ${CMAKE_IREEN_VERSION_MAJOR}
    DEFINE_SYMBOL IREEN_BUILD_LIBRARY
)

set(IREEN_LIBRARIES ireen CACHE INTERNAL "" )
set(IREEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")

IF(WIN32)
    LIST(APPEND EXTRA_LIBS ws2_32)
ENDIF()


TARGET_LINK_LIBRARIES(ireen
    ${QT_LIBRARIES}
    ${QCA2_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${EXTRA_LIBS}
)

install( TARGETS ireen
    RUNTIME DESTINATION bin
        COMPONENT ireen
    LIBRARY DESTINATION lib${LIB_SUFFIX}
        COMPONENT ireen
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
        COMPONENT ireenDevel
    BUNDLE DESTINATION library
)

INSTALL(FILES ${HDR}
    DESTINATION include/ireen
        COMPONENT ireenDevel
)
