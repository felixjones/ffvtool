#include "gba_rom.hpp"

#include "agb_huff.hpp"
#include "tree.hpp"

using namespace ffv;

namespace detail {

template <typename... Ts>
constexpr auto make_byte_array( Ts&&... args ) noexcept -> std::array<std::byte, sizeof...( Ts )> {
	return { std::byte( std::forward<Ts>( args ) )... };
}

} // detail

static bool check_nintendo_logo( std::istream& streamSource ) noexcept {
	static constexpr auto nintendo_logo_tree = make_huff(
		0x40, 0x00, 0x00, 0x00, 0x01, 0x81, 0x82, 0x82, 0x83, 0x0F, 0x83, 0x0C, 0xC3, 0x03, 0x83, 0x01,
		0x83, 0x04, 0xC3, 0x08, 0x0E, 0x02, 0xC2, 0x0D, 0xC2, 0x07, 0x0B, 0x06, 0x0A, 0x05, 0x09
	);

	const auto decompressed = nintendo_logo_tree.decompress4( streamSource, 156 );

	std::array<std::byte, 4> word;
	std::copy( std::cbegin( decompressed ), std::cbegin( decompressed ) + word.size(), std::begin( word ) );
	auto size = std::bit_cast<std::uint32_t>( word ) >> 8;

	static constexpr auto nintendo_logo_tiles_width = std::size_t { 13 };
	static constexpr auto nintendo_logo_bitmap = detail::make_byte_array(
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x3c, 0xf0, 0x3c, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x21, 0x7c, 0xf0, 0x3c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00,
		0xc0, 0x03, 0x80, 0x4e, 0x7c, 0xf0, 0x3c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80,
		0x52, 0xfc, 0xf0, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80, 0x4e, 0xfc, 0xf1,
		0x3c, 0xef, 0xf1, 0x8f, 0x1f, 0xde, 0x03, 0xde, 0xc3, 0x8f, 0x52, 0xbc, 0xf1, 0x3c, 0xff, 0xc7,
		0xc3, 0x39, 0xfe, 0x0f, 0xff, 0xe3, 0x1c, 0x21, 0xbc, 0xf3, 0x3c, 0x1f, 0xcf, 0xe3, 0x70, 0x3e,
		0x9e, 0xc7, 0x73, 0x38, 0x1e, 0x3c, 0xf7, 0x3c, 0x0f, 0xcf, 0xf3, 0xf0, 0x1e, 0xde, 0xc3, 0x7b,
		0x78, 0x00, 0x3c, 0xf6, 0x3c, 0x0f, 0xcf, 0xf3, 0xff, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c,
		0xfe, 0x3c, 0x0f, 0xcf, 0xf3, 0x00, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c, 0xfc, 0x3c, 0x0f,
		0xcf, 0xf3, 0xf0, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c, 0xf8, 0x3c, 0x0f, 0xcf, 0xe3, 0xf0,
		0x1e, 0x9e, 0xc3, 0x73, 0x38, 0x00, 0x3c, 0xf8, 0x3c, 0x0f, 0xcf, 0xc3, 0x79, 0x1e, 0x1e, 0xe7,
		0xe3, 0x1c, 0x00, 0x3c, 0xf0, 0x3c, 0x0f, 0xcf, 0x83, 0x1f, 0x1e, 0x1e, 0xfe, 0xc3, 0x0f, 0x00
	);

	std::size_t tileStart = 0;
	std::size_t tileY = 0;

	std::uint16_t sum = 0;
	auto read = std::cbegin( decompressed ) + 4;
	while ( size ) {
		std::array<std::byte, 2> s16;
		std::copy( read, read + 2, std::begin( s16 ) );
		sum += std::bit_cast<std::uint16_t>( s16 );

		s16 = std::bit_cast<std::array<std::byte, 2>>( sum );

		if ( nintendo_logo_bitmap[tileStart + tileY * nintendo_logo_tiles_width] != s16[0] ) {
			return false;
		}
		++tileY;

		if ( nintendo_logo_bitmap[tileStart + tileY * nintendo_logo_tiles_width] != s16[1] ) {
			return false;
		}
		++tileY;

		if ( tileY == 8 ) {
			tileY = 0;
			++tileStart;
			if ( ( tileStart % nintendo_logo_tiles_width ) == 0 ) {
				tileStart += nintendo_logo_tiles_width * 7;
			}
		}

		read += 2;
		size -= 2;
	}

	return true;
}

gba_rom gba_rom::load( std::istream& streamSource ) {
	const auto start = static_cast<std::size_t>( streamSource.tellg() );

	streamSource.seekg( start + 0x004 );

	const auto hasLogo = check_nintendo_logo( streamSource );

	std::array<char, 12> title;
	streamSource.read( title.data(), title.size() );

	std::array<char, 6> gameCode;
	streamSource.read( gameCode.data(), gameCode.size() );

	char constant96;
	streamSource >> constant96;
	if ( constant96 != static_cast<char>( 0x96 ) ) {
		throw std::runtime_error( "Missing constant value 0x96" );
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

	return gba_rom { hasLogo, title, std::bit_cast<gba_rom::game_code>( gameCode ), version };
}
