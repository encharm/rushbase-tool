project(rushbase)
cmake_minimum_required(VERSION 3.0)

if(STATIC_CURL)
set(CURL_LIBRARIES ${STATIC_CURL})
else()
find_package(CURL)
endif()

if(STATIC_OPENSSL)
set(OPENSSL_LIBRARIES ${STATIC_OPENSSL})
else()
find_package(OpenSSL)
endif()

include_directories(${CURL_INCLUDE_DIRS})

add_executable(rushbase main.c)

if(WIN32)
	set(WINDOWS_LIBRARIES ws2_32)
endif()

target_link_libraries(rushbase ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} ${WINDOWS_LIBRARIES})