#ifndef FFV_IPS_EXT_HPP
#define FFV_IPS_EXT_HPP

#include "crc.hpp"
#include "ips.hpp"

namespace ffv {
namespace detail {

	template <class Type>
	constexpr auto byte_swap( Type value ) noexcept -> std::enable_if_t<std::is_fundamental_v<Type>, Type> {
		auto bytes = std::bit_cast<std::array<char, sizeof( Type )>>( value );
		std::reverse( std::begin( bytes ), std::end( bytes ) );
		return std::bit_cast<Type>( bytes );
	}

} // detail

template <typename UIntType>
constexpr crc<UIntType>& operator <<( crc<UIntType>& crc, const ips::record& record ) noexcept {
	const auto offset = record.offset;
	auto offsetBytes = std::bit_cast<std::array<char, sizeof( offset )>>( offset );
	if constexpr ( ips::endianness != std::endian::native ) {
		std::reverse( std::begin( offsetBytes ), std::begin( offsetBytes ) + ips::record::offset_bytes );
	}

	crc.write( offsetBytes.begin(), offsetBytes.begin() + ips::record::offset_bytes );

	if ( std::holds_alternative<ips::record::copy_type>( record.data ) ) {
		const auto& copy = std::get<ips::record::copy_type>( record.data );

		auto sizeBytes = std::bit_cast<std::array<char, sizeof( ips::record::copy_type::size_type )>>( copy.size() );
		if constexpr ( ips::endianness != std::endian::native ) {
			std::reverse( std::begin( sizeBytes ), std::begin( sizeBytes ) + ips::record::size_bytes );
		}

		crc.write( sizeBytes.begin(), sizeBytes.begin() + ips::record::size_bytes );
		crc.write( copy.begin(), copy.end() );
	} else if ( std::holds_alternative<ips::record::fill_type>( record.data ) ) {
		crc.write( std::uint16_t { 0 } );

		const auto& fill = std::get<ips::record::fill_type>( record.data );
		if constexpr ( ips::endianness != std::endian::native ) {
			crc.write( detail::byte_swap( fill.size ) );
		} else {
			crc.write( fill.size );
		}

		crc.write( fill.data );
	}

	return crc;
}

} // ffv

#endif // define FFV_IPS_EXT_HPP
