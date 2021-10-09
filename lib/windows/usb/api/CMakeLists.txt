project(AdbWinApi)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SRCS "*.h" "*.cpp" "*rc")
add_library(${PROJECT_NAME} STATIC ${SRCS})
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE _USRDLL _WINDLL)
target_link_libraries(${PROJECT_NAME} ole32 Advapi32 Kernel32 User32 oleaut32 wbemuuid uuid setupapi)

