#ifndef FFV_IPS_EXT_HPP
#define FFV_IPS_EXT_HPP

#include <array>
#include <bit>

#include "crc.hpp"
#include "ips.hpp"

namespace ffv {

	template <typename UIntType>
	// TODO : Endianness byte swap
	constexpr crc<UIntType>& operator <<( crc<UIntType>& crc, const ips::record& record ) noexcept {
		const auto offset = record.offset;
		const auto offsetBytes = std::bit_cast<std::array<char, sizeof( offset )>>( offset );

		crc.write( offsetBytes );

		if ( std::holds_alternative<ips::record::copy_type>( record.data ) ) {
			const auto& copy = std::get<ips::record::copy_type>( record.data );

			// TODO : Endianness byte swap
			const auto sizeBytes = std::bit_cast<std::array<char, sizeof( ips::record::copy_type::size_type )>>( copy.size() );
			crc.write( sizeBytes.begin(), sizeBytes.begin() + ips::record::size_bytes );
			crc.write( copy.begin(), copy.end() );
		} else if ( std::holds_alternative<ips::record::fill_type>( record.data ) ) {
			crc.write( std::uint16_t { 0 } );

			const auto& fill = std::get<ips::record::fill_type>( record.data );
			// TODO : Endianness byte swap
			crc.write( fill.size );

			crc.write( fill.data );
		}
		
		return crc;
	}

} // ffv

#endif // define FFV_IPS_EXT_HPP
