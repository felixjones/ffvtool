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
