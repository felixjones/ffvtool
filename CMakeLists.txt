# CMakeList.txt : CMake project for ffvtool, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ffvtool")

# Add source to this project's executable.
add_executable (ffvtool "main.cpp" "ffv/rom.hpp" "ffv/crc.hpp" "ffv/rom.cpp" "ffv/ips.hpp" "ffv/ips_ext.hpp"    "ffv/text_table.hpp" "ffv/text_table.cpp"  "ffv/tree.hpp"    "ffv/gba.hpp" "ffv/gba.cpp" "ffv/agb_huff.hpp"  "ffv/istream_find.hpp" "ffv/text_mutator.hpp" "ffv/text_mutator.cpp" "ffv/ips_writer.hpp" "ffv/gba_texts.hpp" "ffv/gba_texts.cpp")

set_property(TARGET ffvtool PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.
