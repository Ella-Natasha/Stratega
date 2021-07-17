cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp" "*.h")

add_executable (MapBuilder ${sources})
target_include_directories(MapBuilder PUBLIC include)
target_link_libraries(MapBuilder PUBLIC Stratega)

#target_include_directories(Arena:../ PUBLIC include)
target_include_directories(MapBuilder PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Arena/include>
        $<INSTALL_INTERFACE:../Arena/include>  # <prefix>/include/mylib
        )

#Copy Assets folder
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(MapBuilder
        PRIVATE
        sfml-graphics
        sfml-system
        sfml-window
        ImGUI)