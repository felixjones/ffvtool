#include "text_mutator.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

using namespace ffv;

namespace detail {

	template <typename Type, unsigned Count>
	constexpr auto generate_sequence( const Type& start ) noexcept -> std::array<Type, Count> {
		std::array<Type, Count> sequence;
		std::generate( std::begin( sequence ), std::end( sequence ), [n = start] () mutable {
			return n++;
		} );
		return sequence;
	}

	constexpr auto lower_alphabet() noexcept {
		return generate_sequence<char, 26>( 'a' );
	}

	constexpr auto upper_alphabet() noexcept {
		return generate_sequence<char, 26>( 'A' );
	}

	constexpr auto both_alphabet() noexcept {
		constexpr auto lower = lower_alphabet();
		constexpr auto upper = upper_alphabet();

		std::array<char, 52> alphabetic;
		std::copy( std::cbegin( lower ), std::cend( lower ), std::begin( alphabetic ) );
		std::copy( std::cbegin( upper ), std::cend( upper ), std::begin( alphabetic ) + 26 );
		return alphabetic;
	}

	constexpr auto numbers() noexcept {
		return generate_sequence<char, 10>( '0' );
	}

	constexpr bool is_alphabetic( const char c ) noexcept {
		constexpr auto both = both_alphabet();

		return std::find( std::cbegin( both ), std::cend( both ), c ) != std::cend( both );
	}

	constexpr bool is_alpha_numeric( const char c ) noexcept {
		constexpr auto digits = numbers();

		return is_alphabetic( c ) || std::find( std::cbegin( digits ), std::cend( digits ), c ) != std::cend( digits );
	}

	constexpr bool is_upper( const char c ) noexcept {
		constexpr auto upper = upper_alphabet();

		return std::find( std::cbegin( upper ), std::cend( upper ), c ) != std::cend( upper );
	}

	constexpr bool is_upper( const std::string_view& sv ) noexcept {
		constexpr auto lower = lower_alphabet();
		constexpr auto upper = upper_alphabet();

		bool hasUpper = false;
		for ( const auto c : sv ) {
			if ( std::find( std::cbegin( lower ), std::cend( lower ), c ) != std::cend( lower ) ) {
				return false;
			}

			hasUpper |= ( std::find( std::cbegin( upper ), std::cend( upper ), c ) != std::cend( upper ) );
		}

		return hasUpper;
	}

	constexpr bool is_lower( const char c ) noexcept {
		constexpr auto lower = lower_alphabet();

		return std::find( std::cbegin( lower ), std::cend( lower ), c ) != std::cend( lower );
	}

	constexpr bool is_lower( const std::string_view& sv ) noexcept {
		constexpr auto lower = lower_alphabet();
		constexpr auto upper = upper_alphabet();

		bool hasLower = false;
		for ( const auto c : sv ) {
			if ( std::find( std::cbegin( upper ), std::cend( upper ), c ) != std::cend( upper ) ) {
				return false;
			}

			hasLower |= ( std::find( std::cbegin( lower ), std::cend( lower ), c ) != std::cend( lower ) );
		}

		return hasLower;
	}

	constexpr bool is_name_case( const std::string_view& sv ) noexcept {
		if ( is_upper( sv ) ) {
			return true;
		}

		constexpr auto upper = upper_alphabet();

		if ( std::find( std::cbegin( upper ), std::cend( upper ), sv[0] ) == std::cend( upper ) ) {
			return false;
		}

		for ( int ii = 1; ii < sv.size(); ++ii ) {
			if ( std::find( std::cbegin( upper ), std::cend( upper ), sv[ii] ) != std::cend( upper ) ) {
				return false;
			}
		}

		return true;
	}

	std::string to_lower( const std::string& s ) noexcept {
		auto lower = s;
		std::transform( std::cbegin( lower ), std::cend( lower ), std::begin( lower ), []( char c ) {
			return std::tolower( c );
		} );
		return lower;
	}

	std::string transform_casing( const std::string_view& in, const std::string_view& casing ) noexcept {
		std::string result;
		if ( is_upper( casing ) ) {
			std::transform( std::cbegin( in ), std::cend( in ), std::back_inserter( result ), []( char c ) {
				return std::toupper( c );
			} );
		} else if ( is_lower( casing ) ) {
			std::transform( std::cbegin( in ), std::cend( in ), std::back_inserter( result ), []( char c ) {
				return std::tolower( c );
			} );
		} else {
			static constexpr auto upper = upper_alphabet();

			result = in;
			if ( std::find( std::cbegin( upper ), std::cend( upper ), casing[0] ) != std::cend( upper ) ) {
				result[0] = std::toupper( result[0] );
			}
		}
		return result;
	}

	auto last_alphabet( const std::string_view& sv ) noexcept {
		const auto it = std::find_if( std::crbegin( sv ), std::crend( sv ), []( char c ) {
			return is_alphabetic( c );
		} );
		return std::distance( it, std::crend( sv ) );
	}

	std::string name_casing( const std::string_view& in ) noexcept {
		static constexpr auto upper = upper_alphabet();

		auto result = std::string { in };
		if ( !result.empty() ) [[likely]] {
			result[0] = std::toupper( result[0] );

			for ( int ii = 1; ii < result.size(); ++ii ) {
				result[ii] = std::tolower( result[ii] );
			}
		}
		return result;
	}

	std::string::size_type dialog_start( const std::string& str, std::string::size_type pos ) noexcept {
		static constexpr auto bartz = std::string_view { "`02`" };
		static constexpr auto colon = ':';
		static constexpr auto space = ' ';

		while ( str[pos] != colon ) {
			--pos;
		}

		const bool isBartz = ( pos >= 4 && std::string_view( &str[pos - 4], 4 ) == bartz );
		if ( !isBartz && pos >= 1 ) {
			// Step back through alphabeticals
			do {
				--pos;
			} while ( pos && ( str[pos - 1] == space || is_alphabetic( str[pos - 1] ) ) );
		} else if ( isBartz ) {
			pos -= 4;
		}

		return pos;
	}

	std::string::size_type dialog_end( const std::string& str, std::string::size_type pos ) noexcept {
		static constexpr auto terminate = std::string_view { "`00`" };
		static constexpr auto new_line = std::string_view { "`01`" };
		static constexpr auto colon = ':';

		while ( pos < str.size() ) {
			if ( pos < str.size() - 4 ) {
				const auto code = std::string_view( &str[pos], 4 );
				if ( code == terminate ) {
					return pos + 4;
				}

				if ( code == new_line ) {
					const auto nextDialog = str.find( colon, pos + 4 );
					if ( nextDialog != std::string::npos ) {
						return dialog_start( str, nextDialog ) - 1;
					}
				}
			}

			do {
				++pos;
			} while ( is_alphabetic( str[pos] ) );
		}

		return pos - 1;
	}

	bool skip_whitespace( const std::string& str, std::string::size_type& pos ) noexcept {
		constexpr auto white_spaces = std::array<std::string_view, 2> {
			std::string_view { " " }, // space
			std::string_view { "`01`" } // new line
		};

		for ( const auto& sv : white_spaces ) {
			if ( str.find( sv, pos ) == pos ) {
				pos += sv.size();
				return true;
			}
		}

		return false;
	}

	text_range_type find_white_space( const std::string& str, const std::string::size_type pos, int& numSpaces ) noexcept {
		constexpr auto white_spaces = std::array<std::string_view, 2> {
			std::string_view { " " }, // space
			std::string_view { "`01`" } // new line
		};

		std::array<std::string::size_type, white_spaces.size()> spaceStarts;
		for ( std::size_t ii = 0; ii < white_spaces.size(); ++ii ) {
			spaceStarts[ii] = str.find( white_spaces[ii], pos );
		}

		const auto begin = std::min_element( std::cbegin( spaceStarts ), std::cend( spaceStarts ) );
		auto end = *begin;
		while ( skip_whitespace( str, end ) ) {
			++numSpaces;
		}

		return { *begin, end };
	}

	std::size_t count_line_breaks( const std::string& str, std::string::size_type start, const std::string::size_type end ) noexcept {
		constexpr auto line_end = std::string_view { "`01`" };
		std::size_t count = 0;
		while ( start < end ) {
			if ( str.find( line_end, start ) == start ) {
				++count;
				start += 4;
			} else {
				++start;
			}
		}
		return count;
	}

	std::uint32_t count_trailing_spaces( const std::string& str, std::string::size_type start, const std::string::size_type end ) noexcept {
		constexpr auto line_end = std::string_view { "`01`" };
		constexpr auto space = std::string_view { " " };
		std::uint32_t count = 0;
		while ( start < end ) {
			if ( str.find( line_end, start ) == start ) {
				count = 0;
				start += 4;
				continue;
			}

			if ( str.find( space, start ) == start ) {
				++count;
			}
			++start;
		}
		return count;
	}

	auto find_line_end( const std::string& str, const std::string::size_type pos ) noexcept {
		constexpr auto line_end = std::array<std::string_view, 2> {
			std::string_view { "`01`" }, // new line
			std::string_view { "`00`" } // terminate
		};

		std::array<std::string::size_type, line_end.size()> lineEnds;
		for ( std::size_t ii = 0; ii < line_end.size(); ++ii ) {
			lineEnds[ii] = str.find( line_end[ii], pos );
		}

		return *std::min_element( std::cbegin( lineEnds ), std::cend( lineEnds ) );
	}

	std::string::size_type find_terminal( const std::string& str, std::string::size_type pos ) noexcept {
		static constexpr auto period = '.';
		static constexpr auto exclaim = '!';
		static constexpr auto question = '?';
		static constexpr auto quote = '"';

		while ( pos < str.size() ) {
			if ( ( pos && str[pos] == period && str[pos - 1] != period && str[pos + 1] != quote ) || str[pos] == exclaim || str[pos] == question ) {
				++pos;
				if ( str[pos] == quote || is_alpha_numeric( str[pos] ) ) {
					return pos;
				}
			} else {
				++pos;
			}
		}

		return std::string::npos;
	}

	int remove_lines( std::string& str, int count ) noexcept {
		static constexpr auto terminate = std::string_view { "`00`" };
		static constexpr auto new_line = std::string_view { "`01`" };
		static constexpr auto new_line2 = std::string_view { "`nl`" };
		static constexpr auto box_break = std::string_view { "`bx`" };

		const auto terminates = std::string_view( &str[str.size() - 4], 4 ) == terminate;

		auto pos = str.find( new_line );
		pos = std::min( pos, str.find( new_line2 ) );
		while ( pos != std::string::npos ) {
			if ( str.substr( pos, 4 ) == new_line ) {
				str.erase( pos, new_line.size() );
			} else {
				pos += 4;
			}
			
			++count;
			if ( !terminates && count % 4 == 0 ) {
				str.insert( pos, box_break );
			}

			pos = str.find( new_line, pos );
			pos = std::min( pos, str.find( new_line2, pos ) );
		}

		const auto code = std::string_view( &str[str.size() - 4], 4 );
		if ( !terminates && code != box_break ) {
			str += new_line;
		}

		return count;
	}

	void remove_whitespace( std::string& str ) noexcept {
		static constexpr auto space = ' ';
		static constexpr auto control = '`';
		static constexpr auto colon = ':';
		static constexpr auto ellipsis = std::string_view { ".." };

		static constexpr auto bartz = std::string_view { "`02`" };
		static constexpr auto gil = std::string_view { "`10`" };
		static constexpr auto item = std::string_view { "`11`" };
		static constexpr auto ability = std::string_view { "`12`" };

		// Erase start
		while ( str[0] == space ) {
			str.erase( 0, 1 );
		}

		// Erase multiples
		auto start = str.find( space );
		while ( start != std::string::npos ) {
			auto end = start;
			while ( str[end] == space ) {
				end++;
			}
			const auto length = end - start - 1;
			str.erase( start, length );

			start = str.find( space, start + 1 );
		}

		// Erase either side of controls
		start = str.find( space );
		while ( start != std::string::npos ) {
			auto end = start + 1;
			if ( str[end] == control ) {
				// Check for bartz, gil, etc
				const auto code = std::string_view( &str[end], 4 );
				if ( code != bartz && code != gil && code != item && code != ability ) {
					do {
						end++;
					} while ( str[end] != control );

					if ( str[end + 1] == space ) {
						str.erase( end + 1, 1 );
					}
				}
			}

			start = str.find( space, start + 1 );
		}

		// Erase pre-grammar
		start = str.find( space );
		while ( start != std::string::npos ) {
			if ( str[start - 1] != colon && !is_alpha_numeric( str[start - 1] ) && !is_alpha_numeric( str[start + 1] ) ) {
				if ( str[start - 1] == control ) {
					const auto code = std::string_view( &str[start - 4], 4 );
					if ( code != bartz && code != gil && code != item && code != ability ) {
						str.erase( start, 1 );
						--start;
					}
				}
			}
			start = str.find( space, start + 1 );
		}

		// Erase after ellipsis
		start = str.find( ellipsis );
		while ( start != std::string::npos ) {
			if ( str[start + ellipsis.size()] == space ) {
				str.erase( start + ellipsis.size(), 1 );
			}

			start = str.find( ellipsis, start + 1 );
		}
	}

	void uppercase_grammar( std::string& str ) noexcept {
		static constexpr auto period = '.';

		auto start = str.find( period );
		while ( start != std::string::npos ) {
			if ( is_lower( str[start + 1] ) ) {
				str[start + 1] = std::toupper( str[start + 1] );
			}
			start = str.find( period, start + 1 );
		}
	}

	void grammar_line( std::string& str ) noexcept {
		static constexpr auto new_line = std::string_view { "`01`" };

		auto pos = find_terminal( str, 0 );
		while ( pos != std::string::npos ) {
			str.insert( pos, new_line );

			pos = find_terminal( str, pos + 4 );
		}
	}

} // detail

constexpr auto valid_name_chars = std::array<char, 73> {
	'A', 'B', 'C', 'D', 'E', '0', '1', '2', '3', '4',
	'F', 'G', 'H', 'I', 'J', '5', '6', '7', '8', '9',
	'K', 'L', 'M', 'N', 'O', ' ',
	'P', 'Q', 'R', 'S', 'T', '!', '?', '-', '+', '/',
	'U', 'V', 'W', 'X', 'Y',
	'Z',
	'a', 'b', 'c', 'd', 'e',
	'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y',
	'z'
};

static constexpr auto line_widths = std::array<std::uint32_t, 3> { 217, 217, 212 };
static constexpr auto new_line = std::string_view( "`01`" );

text_mutator::text_mutator( const std::vector<std::byte>& data, const text_table::type& textTable, const gba::font_table& fontTable, const std::size_t itemAdvance, const std::size_t abilityAdvance ) noexcept : m_textTable( textTable ), m_fontTable( fontTable ), m_itemAdvance( itemAdvance ), m_abilityAdvance( abilityAdvance ) {
	static constexpr auto terminate = std::string_view { "`00`" };

	std::stringstream buffer;

	auto first = std::cbegin( data );
	const auto last = std::cend( data );
	while ( first != last ) {
		auto begin = first;
		const auto it = m_textTable.find( first, last );
		++first;

		if ( it->value().has_value() ) {
			const auto value = it->value().value();
			if ( value == terminate ) {
				buffer << terminate;
				m_lines.push_back( buffer.str() );
				buffer.str( std::string() );
			} else {
				buffer << value;
			}
		} else [[unlikely]] {
			std::cout << "Warning: Missing SFC character for code ";
			while ( begin != first ) {
				std::cout << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( *begin++ );
			}
			std::cout << '\n';
		}
	}

	m_lines.shrink_to_fit();
	m_dialogMarks.resize( m_lines.size() );
}

void text_mutator::find_replace( const std::string_view& find, const std::string_view& replace ) {
	auto lowerFind = detail::to_lower( std::string { find } );
	const auto lastAlphabet = detail::last_alphabet( lowerFind );

	for ( auto& line : m_lines ) {
		auto lowerLine = detail::to_lower( line );

		std::size_t find = 0;
		while ( true ) {
			find = lowerLine.find( lowerFind, find );
			if ( find == std::string::npos ) {
				break;
			}

			if ( find == 0 || !detail::is_alphabetic( lowerLine[find - 1] ) && !detail::is_alphabetic( lowerLine[find + lastAlphabet] ) ) {
				const auto replacing = line.substr( find, lowerFind.size() );
				line.erase( find, lowerFind.size() );
				line.insert( find, detail::transform_casing( replace, replacing ) );
				find += replace.size();

				lowerLine = detail::to_lower( line );
			} else {
				++find;
			}
		}
	}
}

bool text_mutator::target_find_replace( const std::uint32_t index, const std::string_view& find, const std::string_view& replace ) {
	const auto lastAlphabet = detail::last_alphabet( find );

	auto& line = m_lines[index];

	if ( find.empty() ) {
		line.insert( 0, replace );
		return true;
	}

	bool replacedAny = false;
	std::size_t cur = 0;
	while ( true ) {
		cur = line.find( find, cur );
		if ( cur == std::string::npos ) {
			break;
		}

		if ( cur == 0 || !detail::is_alphabetic( line[cur - 1] ) && !detail::is_alphabetic( line[cur + lastAlphabet] ) ) {
			line.erase( cur, find.size() );
			line.insert( cur, replace );
			cur += replace.size();
			
			replacedAny |= true;
		} else {
			++cur;
		}
	}

	return replacedAny;
}

void text_mutator::name_case( const std::string_view& name ) {
	auto lowerFind = detail::to_lower( std::string { name } );
	const auto nameCased = detail::name_casing( name );
	const auto lastAlphabet = detail::last_alphabet( lowerFind );

	for ( auto& line : m_lines ) {
		auto lowerLine = detail::to_lower( line );

		std::size_t find = 0;
		while ( true ) {
			find = lowerLine.find( lowerFind, find );
			if ( find == std::string::npos ) {
				break;
			}

			if ( find == 0 || !detail::is_alphabetic( lowerLine[find - 1] ) && !detail::is_alphabetic( lowerLine[find + lastAlphabet] ) ) {
				const auto replacing = line.substr( find, lowerFind.size() );
				if ( !detail::is_name_case( replacing ) ) {
					line.erase( find, lowerFind.size() );
					line.insert( find, nameCased );
					lowerLine = detail::to_lower( line );
				}
				find += nameCased.size();
			} else {
				++find;
			}
		}
	}
}

std::uint32_t text_mutator::bartz_advance() const {
	std::uint32_t widestChar = 0;
	for ( const auto c : valid_name_chars ) {
		const auto key = m_textTable.rfind( std::string { c } );
		if ( key.size() == 1 ) {
			const auto advance = m_fontTable.glyphs[static_cast<int>( key[0] )].advance;
			if ( advance > widestChar ) {
				widestChar = advance;
			}
		}
	}
	return 6 * widestChar;
}

std::uint32_t text_mutator::gil_advance() const {
	std::uint32_t widestChar = 0;
	for ( const auto c : detail::numbers() ) {
		const auto key = m_textTable.rfind( std::string { c } );
		if ( key.size() == 1 ) {
			const auto advance = m_fontTable.glyphs[static_cast<int>( key[0] )].advance;
			if ( advance > widestChar ) {
				widestChar = advance;
			}
		}
	}
	return widestChar * 7;
}

std::uint32_t text_mutator::measure( const std::string& line, const std::size_t start, const std::size_t end ) const {
	const auto bartz = m_textTable.rfind( "`02`" );
	const auto gil = m_textTable.rfind( "`10`" );
	const auto item = m_textTable.rfind( "`11`" );
	const auto ability = m_textTable.rfind( "`12`" );

	const auto bartzAdvance = bartz_advance();
	const auto gilAdvance = gil_advance();

	std::uint32_t width = 0;
	auto begin = std::cbegin( line ) + start;
	const auto stop = std::cbegin( line ) + end;
	while ( begin < stop ) {
		auto end = begin + 1;
		auto key = m_textTable.rfind( std::string( begin, end ) );
		while ( key.empty() ) {
			if ( end == std::cend( line ) ) {
				break;
			}
			++end;
			key = m_textTable.rfind( std::string( begin, end ) );
		}

		if ( key == bartz ) {
			width += bartzAdvance;
		} else if ( key == gil ) {
			width += gilAdvance;
		} else if ( key == item ) {
			width += m_itemAdvance;
		} else if ( key == ability ) {
			width += m_abilityAdvance;
		} else if ( key.size() == 1 ) {
			width += m_fontTable.glyphs[static_cast<int>( key[0] )].advance;
		}

		begin = end;
	}

	return width;
}

text_range_type text_mutator::find_dialog( const std::string& str, const text_range_type& range, const int markIndex ) const noexcept {
	if ( detail::is_upper( str[range.second] ) ) {
		const auto end = detail::dialog_end( str, range.second + 1 );
		return { range.second, end };
	}

	static constexpr auto colon = ':';

	auto start = str.find( colon, range.second + 1 );
	if ( start == std::string::npos ) {
		if ( markIndex >= 0 ) {
			const auto marks = m_dialogMarks[markIndex];
			for ( const auto& m : marks ) {
				start = std::min( start, str.find( m, range.second ) );
			}

			if ( start != std::string::npos ) {
				auto end = detail::dialog_end( str, start + 1 );
				for ( const auto& m : marks ) {
					end = std::min( end, str.find( m, start + 1 ) - 5 );
				}
				return { start, end };
			}
		}

		return { std::string::npos, 0 };
	}

	auto end = detail::dialog_end( str, start + 1 );
	start = detail::dialog_start( str, start );

	return { start, end };
}

void text_mutator::text_reflow() {
	constexpr auto box_break = std::string_view { "`bx`" };
	constexpr auto enforced_line_break = std::string_view { "`nl`" };

	const auto newLine = m_textTable.rfind( std::string( new_line ) );
	const auto spaceWidth = m_fontTable.glyphs[static_cast<int>( m_textTable.rfind( " " )[0] )].advance;

	int ll = 0;
	for ( auto& line : m_lines ) {
		const auto dialog = find_dialog( line, { 0, 0 }, -1 );
		if ( dialog.first < dialog.second ) {
			continue;
		}

		bool valign = false;
		std::vector<bool> alignments;
		std::size_t start = 0;
		while ( start < line.size() ) {
			int count = 0;
			const auto ws = detail::find_white_space( line, start, count );
			if ( ws.first == std::string::npos ) {
				break;
			}

			const auto newLines = detail::count_line_breaks( line, ws.first, ws.second );
			const auto spaces = detail::count_trailing_spaces( line, ws.first, ws.second );
			if ( ws.first == 0 || newLines ) {
				alignments.push_back( spaces > 2 );
			}

			if ( ws.first == 0 && newLines ) {
				valign = true;
			}

			const auto length = ws.second - ws.first;
			if ( ws.first == 0 ) {
				// Starts with whitespace, destroy all
				line.erase( 0, ws.second );
			} else if ( count > 1 ) {
				line.erase( ws.first, length );

				// More than 1 whitespace
				if ( newLines ) {
					// Has new lines (assumption)
					line.insert( ws.first, new_line );
					start = ws.first + new_line.size();
				} else {
					// Only spaces
					line.insert( ws.first, " " );
					start = ws.first + 1;
				}
			} else {
				start = ws.second;
			}
		}

		alignments.resize( detail::count_line_breaks( line, 0, line.size() ) + 1 );

		std::size_t lineIndex = 0;
		start = 0;
		while ( start < line.size() ) {
			const auto end = detail::find_line_end( line, start );
			if ( alignments[lineIndex] ) {
				const auto width = measure( line, start, end );
				const auto spare = ( line_widths[lineIndex % line_widths.size()] - width ) / 2;
				const auto padding = spare / spaceWidth;

				line.insert( start, padding, ' ' );
				start = end + padding + 4;
			} else {
				start = end + 4;
			}

			++lineIndex;
		}

		if ( valign && alignments.size() == 1 ) {
			line.insert( 0, new_line );
		}

		lineIndex = 0;
		auto begin = std::cbegin( line );
		while ( begin != std::cend( line ) ) {
			auto end = begin + 1;
			auto key = m_textTable.rfind( std::string( begin, end ) );
			while ( key.empty() ) {
				if ( end == std::cend( line ) ) {
					break;
				}
				++end;
				key = m_textTable.rfind( std::string( begin, end ) );
			}

			if ( key.empty() ) {
				if ( std::string( begin, begin + box_break.size() ) == box_break ) {
					auto index = std::distance( std::cbegin( line ), begin );
					line.erase( index, box_break.size() );

					auto breakCount = 3 - lineIndex;
					while ( breakCount-- ) {
						line.insert( index, new_line );
						index += 4;
					}
					end = std::cbegin( line ) + index;

					lineIndex = 0;
				} else {
					throw new std::runtime_error( "OMG FIX THIS" );
				}
			} else if ( key == newLine ) {
				++lineIndex;
			}

			if ( lineIndex == 3 ) {
				lineIndex = 0;
			}

			begin = end;
		}
	}
}

void text_mutator::dialog_reflow() {
	constexpr auto box_break = std::string_view { "`bx`" };
	constexpr auto enforced_line_break = std::string_view { "`nl`" };

	const auto newLine = m_textTable.rfind( std::string( new_line ) );
	const auto bartz = m_textTable.rfind( "`02`" );
	const auto gil = m_textTable.rfind( "`10`" );

	const auto bartzAdvance = bartz_advance();
	const auto gilAdvance = gil_advance();

	int markIndex = 0;
	for ( auto& line : m_lines ) {
		int lineCount = 0;
		text_range_type pos { 0, 0 };
		while ( true ) {
			pos = find_dialog( line, pos, markIndex );
			if ( pos.first > pos.second ) {
				break;
			}

			const auto oldLength = ( pos.second - pos.first ) + 1;
			const auto oldStr = line.substr( pos.first, oldLength );
			line.erase( pos.first, oldLength );

			auto newStr = oldStr;

			lineCount = detail::remove_lines( newStr, lineCount );

			detail::grammar_line( newStr );
			detail::remove_whitespace( newStr );

			line.insert( pos.first, newStr );
			pos.second = pos.first + newStr.size() - 1;
		}
		++markIndex;

		auto elb = line.find( enforced_line_break );
		while ( elb != std::string::npos ) {
			line.erase( elb, enforced_line_break.size() );
			line.insert( elb, new_line );
			elb = line.find( enforced_line_break, elb );
		}

		int lineIndex = 0;
		std::uint32_t width = 0;
		auto begin = std::cbegin( line );
		while ( begin != std::cend( line ) ) {
			auto end = begin + 1;
			auto key = m_textTable.rfind( std::string( begin, end ) );
			while ( key.empty() ) {
				if ( end == std::cend( line ) ) {
					break;
				}
				++end;
				key = m_textTable.rfind( std::string( begin, end ) );
			}

			if ( key.empty() ) {
				if ( std::string( begin, begin + box_break.size() ) == box_break ) {
					auto index = std::distance( std::cbegin( line ), begin );
					line.erase( index, box_break.size() );

					auto breakCount = 3 - lineIndex;
					while ( breakCount-- ) {
						line.insert( index, new_line );
						index += 4;
					}
					end = std::cbegin( line ) + index;

					lineIndex = 0;
					width = 0;
				} else {
					throw new std::runtime_error( "OMG FIX THIS" );
				}
			} else if ( key == newLine ) {
				++lineIndex;
				width = 0;
			} else if ( key.size() == 1 || key == bartz || key == gil ) {
				if ( key == bartz ) {
					width += bartzAdvance;
				} else if ( key == gil ) {
					width += gilAdvance;
				} else {
					width += m_fontTable.glyphs[static_cast<int>( key[0] )].advance;
				}

				if ( width > line_widths[lineIndex] ) {
					while ( *begin != ' ' ) {
						--begin;
					}

					auto index = std::distance( std::cbegin( line ), begin );
					if ( index > 2 && line[index - 1] != ' ' && line[index - 2] == ' ' ) {
						index -= 2;
					}

					line.erase( index, 1 );
					line.insert( index, new_line );
					line.insert( index + 4, " " );
					end = std::cbegin( line ) + index + 4;

					++lineIndex;
					width = 0;
				}
			}

			if ( lineIndex == 3 ) {
				lineIndex = 0;
			}

			begin = end;
		}
	}
}
