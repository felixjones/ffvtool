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
			constexpr auto upper = upper_alphabet();

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
		constexpr auto upper = upper_alphabet();

		auto result = std::string { in };
		if ( !result.empty() ) [[likely]] {
			result[0] = std::toupper( result[0] );

			for ( int ii = 1; ii < result.size(); ++ii ) {
				result[ii] = std::tolower( result[ii] );
			}
		}
		return result;
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
