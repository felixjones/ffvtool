#include "text_table.hpp"

using namespace ffv;

static bool is_invalid_hex( const std::string& s ) noexcept {
	return s.find_first_not_of( "0123456789abcdefABCDEF" ) != std::string::npos;
}

text_table::const_type text_table::read( std::istream& streamSource ) {
	text_table::type tree;

	std::string line;
	while ( streamSource.good() ) {
		std::getline( streamSource, line );
		if ( line.empty() ) {
			continue;
		}

		const auto splitPos = line.find( '=' );
		if ( splitPos == std::string::npos ) {
			continue;
		}

		const auto keyStr = line.substr( 0, splitPos );
		if ( keyStr.empty() || ( keyStr.size() % 2 == 1 ) || is_invalid_hex( keyStr ) ) {
			continue;
		}

		std::vector<std::byte> key;
		key.reserve( keyStr.size() / 2 );
		for ( std::string::size_type ii = 0; ii < keyStr.size(); ii += 2 ) {
			key.push_back( std::byte { std::stoul( keyStr.substr( ii, 2 ), nullptr, 16) } );
		}

		const auto value = line.substr( splitPos + 1 );
		tree.insert( std::cbegin( key ), std::cend( key ), value );
	}

	return tree;
}
