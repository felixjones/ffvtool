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

	using dialog_range_type = std::pair<std::string::size_type, std::string::size_type>;

	dialog_range_type find_dialog( const std::string& str, const dialog_range_type& range = { 0, 0 } ) noexcept {
		if ( is_upper( str[range.second] ) ) {
			const auto end = dialog_end( str, range.second + 1 );
			return { range.second, end };
		}

		static constexpr auto colon = ':';

		auto start = str.find( colon, range.second + 1 );
		if ( start == std::string::npos ) {
			return { std::string::npos, 0 };
		}

		auto end = dialog_end( str, start + 1 );
		start = dialog_start( str, start );

		return { start, end };
	}

	std::string::size_type find_terminal( const std::string& str, std::string::size_type pos ) noexcept {
		static constexpr auto period = '.';
		static constexpr auto exclaim = '!';
		static constexpr auto question = '?';
		static constexpr auto quote = '"';

		while ( pos < str.size() ) {
			if ( ( str[pos] == period && str[pos - 1] != period ) || str[pos] == exclaim || str[pos] == question ) {
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
		static constexpr auto box_break = std::string_view { "`BX`" };

		const auto terminates = std::string_view( &str[str.size() - 4], 4 ) == terminate;

		auto pos = str.find( new_line );
		while ( pos != std::string::npos ) {
			str.erase( pos, new_line.size() );
			
			++count;
			if ( !terminates && count % 4 == 0 ) {
				str.insert( pos, box_break );
			}

			pos = str.find( new_line, pos );
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
				str.erase( start, 1 );
				--start;
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

			pos = find_terminal( str, pos + 1 );
		}
	}

} // detail

text_mutator::text_mutator( const std::vector<std::byte>& data, const text_table::type& textTable, const gba::font_table& fontTable) noexcept : m_textTable( textTable ), m_fontTable( fontTable ) {
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

void text_mutator::dialog_reflow() {
	for ( auto& line : m_lines ) {
		int lineCount = 0;
		detail::dialog_range_type pos { 0, 0 };
		while ( true ) {
			pos = detail::find_dialog( line, pos );
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
	}
}
