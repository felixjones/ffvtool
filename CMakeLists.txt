# CMakeList.txt : CMake project for ffvtool, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ffvtool")

# Add source to this project's executable.
add_executable (ffvtool "main.cpp" "ffv/rom.hpp" "ffv/crc.hpp" "ffv/rom.cpp" "ffv/ips.hpp" "ffv/ips_ext.hpp"    "ffv/text_table.hpp" "ffv/text_table.cpp"  "ffv/tree.hpp"   )

set_property(TARGET ffvtool PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.
