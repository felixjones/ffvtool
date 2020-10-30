#include "gba_texts.hpp"

#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

using namespace ffv;

std::size_t gba::max_text_length( const text_data& textData, std::vector<std::pair<std::size_t, std::size_t>> ranges, const text_table::type& textTable, const font_table& fontTable ) {
	static constexpr auto terminate = std::string_view { "`00`" };

	std::size_t max = 0;

	for ( const auto& range : ranges ) {
		for ( auto ii = range.first; ii <= range.second; ++ii ) {
			std::stringstream buffer;

			std::size_t length = 0;

			const auto ss = textData.offsets[ii] - textData.offsets.size() * 4 - sizeof( textData.header ) - 8;
			auto first = std::cbegin( textData.data ) + ss;
			const auto last = std::cend( textData.data );
			while ( first != last ) {
				auto begin = first;
				const auto it = textTable.find( first, last );
				++first;

				if ( it->value().has_value() && std::distance( begin, first ) == 1 ) {
					if ( it->value().value() == terminate ) {
						break;
					}

					const auto& glyph = fontTable.glyphs[static_cast<int>( *begin )];
					length += glyph.advance;
				} else [[unlikely]] {
					std::cout << "Warning: Missing AGB character for code ";
					while ( begin != first ) {
						std::cout << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( *begin++ );
					}
					std::cout << '\n';
				}
			}

			max = std::max( max, length );
		}
	}

	return max;
}
