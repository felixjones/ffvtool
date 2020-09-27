#include "rom.hpp"

#include "ips.hpp"
#include "ips_ext.hpp"

using namespace ffv;

rom rom::read_ips( std::istream& streamSource ) {
	std::array<char, ips::magic::id.size()> header;
	streamSource.read( header.data(), header.size() );

	if ( header != ips::magic::id ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not IPS file (Magic ID mismatch)" );
	}

	crc32 hash;
	hash << header;

	std::vector<std::byte> romBytes;

	ips::record record {};
	ips::eof eof {};
	while ( streamSource.good() ) {
		record << streamSource;

		if ( std::holds_alternative<ips::record::copy_type>( record.data ) ) {
			const auto& copy = std::get<ips::record::copy_type>( record.data );

			romBytes.resize( record.offset + copy.size(), std::byte { 0xff } );
			std::copy( std::cbegin( copy ), std::cend( copy ), std::begin( romBytes ) + record.offset );
		} else if ( std::holds_alternative<ips::record::fill_type>( record.data ) ) {
			const auto& fill = std::get<ips::record::fill_type>( record.data );

			romBytes.resize( static_cast<std::size_t>( record.offset ) + fill.size, std::byte { 0xff } );
			std::fill_n( std::begin( romBytes ) + record.offset, fill.size, fill.data );
		}

		hash << record;

		const auto start = streamSource.tellg();
		eof << streamSource;
		if ( eof.is_end() ) {
			hash << eof;
			break;
		}
		streamSource.seekg( start );
	}

	return rom { romBytes, hash };
}

/*
rom rom::read_gba( std::istream& streamSource ) {
	const auto start = static_cast<std::size_t>( streamSource.tellg() );

	streamSource.seekg( start + 0x0b2 );

	char constant96;
	streamSource >> constant96;
	if ( constant96 != static_cast<char>( 0x96 ) ) {
		throw std::runtime_error( "Missing constant value 0x96" );
	}


	streamSource.seekg( start + 0x004 );

	//const auto hasLogo = check_nintendo_logo( streamSource );

	std::array<char, 12> title;
	streamSource.read( title.data(), title.size() );

	std::array<char, 6> gameCode;
	streamSource.read( gameCode.data(), gameCode.size() );

	char constant96;
	streamSource >> constant96;
	if ( constant96 != static_cast<char>( 0x96 ) ) {
		throw std::invalid_argument( "Stream is not GBA ROM (constant value 0x96 not found)" );
	}

	streamSource.seekg( start + 0x0bc );

	std::uint8_t version;
	streamSource >> version;

	std::uint8_t crc;
	streamSource >> crc;

	streamSource.seekg( start + 0x0a0 );
	std::array<char, 0x0bc - 0x0a0> buf;
	streamSource.read( buf.data(), buf.size() );

	std::uint8_t chk = 0;
	for ( const auto& c : buf ) {
		chk += c;
	}
	chk = -( 0x19 + chk );

	if ( crc != crc ) {
		throw std::runtime_error( "Header checksum fail" );
	}
}

*/