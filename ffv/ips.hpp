#ifndef FFV_IPS_HPP
#define FFV_IPS_HPP

#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <type_traits>
#include <variant>

namespace ffv {
namespace ips {

struct eof {
	static constexpr auto magic = std::to_array( { 'E', 'O', 'F' } );

	using data_type = std::remove_const_t<decltype( magic )>;

	data_type	data;

	constexpr bool is_end() const noexcept {
		return data == magic;
	}
};

namespace magic {
	static constexpr auto id = std::to_array( { 'P', 'A', 'T', 'C', 'H' } );
} // magic

struct record {
	static constexpr auto offset_bytes = 3;
	static constexpr auto size_bytes = 2;

	using copy_type = std::vector<char>;
	struct fill_type {
		std::uint16_t	size;
		char			data;
	};

	using data_type = std::variant<copy_type, fill_type>;	

	std::uint32_t	offset : 24;
	data_type		data;
};

} // ips

inline ips::eof& operator <<( ips::eof& eof, std::istream& streamSource ) noexcept {
	auto itor = eof.data.begin();
	while ( streamSource.good() && itor != eof.data.end() ) {
		streamSource >> *itor++;
	}

	return eof;
}

inline ips::record& operator <<( ips::record& record, std::istream& streamSource ) noexcept {
	// TODO : Endianness byte swap
	std::array<char, sizeof( std::uint32_t )> offsetBytes {};
	auto offsetItor = offsetBytes.begin();
	const auto offsetEnd = offsetItor + ips::record::offset_bytes;
	while ( offsetItor != offsetEnd ) {
		streamSource >> *offsetItor++;
	}

	record.offset = std::bit_cast<std::uint32_t>( offsetBytes );

	// TODO : Endianness byte swap
	std::array<char, ips::record::size_bytes> sizeBytes {};
	auto sizeItor = sizeBytes.begin();
	while ( sizeItor != sizeBytes.end() ) {
		streamSource >> *sizeItor++;
	}

	const auto size = std::bit_cast<std::uint16_t>( sizeBytes );
	if ( size == 0 ) {
		// Fill
		auto& fill = std::get<ips::record::fill_type>( record.data );
		
		// TODO : Endianness byte swap
		sizeItor = sizeBytes.begin();
		while ( sizeItor != sizeBytes.end() ) {
			streamSource >> *sizeItor++;
		}
		fill.size = std::bit_cast<std::uint16_t>( sizeBytes );

		streamSource >> fill.data;
	} else {
		// Copy
		auto& copy = std::get<ips::record::copy_type>( record.data );
		copy.resize( size, 0 );

		auto copyItor = copy.begin();
		while ( copyItor != copy.end() ) {
			streamSource >> *copyItor++;
		}
	}

	return record;
}

} // ffv

#endif // define FFV_IPS_HPP
