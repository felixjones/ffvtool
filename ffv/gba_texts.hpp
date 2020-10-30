#ifndef FFV_GBA_TEXTS_HPP
#define FFV_GBA_TEXTS_HPP

#include <vector>

#include "gba.hpp"
#include "text_table.hpp"

namespace ffv {
namespace gba {

std::size_t max_text_length( const text_data& textData, std::vector<std::pair<std::size_t, std::size_t>> ranges, const text_table::type& textTable, const font_table& fontTable );

} // gba
} // ffv

#endif // define FFV_GBA_TEXTS_HPP
