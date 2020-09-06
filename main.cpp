#include <fstream>
#include <iomanip>
#include <sstream>

#include "ffv/gba_rom.hpp"
#include "ffv/rom.hpp"
#include "ffv/text_table.hpp"

struct rpge_constants {
	static constexpr std::uint32_t crc32 = 0xf11f1026;
	static constexpr auto terminate = std::byte { 0 };
};

int main( int argc, char * argv[] ) {
	const auto ipsRom = ffv::rom::read_ips( std::ifstream( argv[1], std::ios::binary ) );
	if ( ipsRom.hash() != rpge_constants::crc32 ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not RPGe v1.1" );
	}

	const auto textTable = ffv::text_table::read( std::ifstream( argv[2] ) );
	if ( textTable.empty() ) [[unlikely]] {
		throw std::invalid_argument( "Invalid or corrupt text table" );
	}

	auto address = std::stoul( argv[3], nullptr, 16 );
	const auto end = std::stoul( argv[4], nullptr, 16 );
	if ( end < address ) [[unlikely]] {
		throw std::invalid_argument( "Specified text start address greater than text end address" );
	}

	const auto gbaRom = ffv::gba_rom::load( std::ifstream( argv[5], std::ios::binary ) );

	auto file = std::ofstream( argv[6] );
	std::size_t count = 0;

	const auto& rom = ipsRom.data();
	std::optional<std::string> value;
	while ( address <= end ) {
		std::stringstream buffer;
		while ( rom[address] != rpge_constants::terminate ) {
			const auto start = address;
			for ( auto it = textTable.find( rom[address++] ); it != textTable.cend(); it.find_next( rom[address++] ) ) {
				value = it->value();
			}

			if ( value.has_value() ) {
				// Write string
				buffer << value.value();
				--address;
				value = {};
			} else {
				// Write hex
				buffer << '{' << std::setfill( '0' ) << std::setw( 2 ) << std::hex << static_cast<int>( rom[address - 1] ) << '}';
				address = start + 1;
			}
		}

		if ( buffer.rdbuf()->in_avail() != 0 ) {
			file << count++ << ',' << std::quoted( buffer.str(), '\"', '\"' ) << std::endl;
		}

		++address;
	}

	return 0;
}
