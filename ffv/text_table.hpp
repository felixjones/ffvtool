#ifndef FFV_TEXT_TABLE_HPP
#define FFV_TEXT_TABLE_HPP

#include <istream>
#include <string>

#include "tree.hpp"

namespace ffv {
namespace text_table {
	using type = tree<std::byte, std::string>;
	using const_type = const type;
	using const_iterator = type::const_iterator;

	const_type	read( std::istream& streamSource );

} // text_table
} // ffv

#endif // define FFV_TEXT_TABLE_HPP
