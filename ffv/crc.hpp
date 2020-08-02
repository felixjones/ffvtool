#ifndef FFV_CRC_HPP
#define FFV_CRC_HPP

#include <array>
#include <cstdint>

namespace ffv {
namespace detail {

	template <typename Type>
	struct crc_constants {};

	template <>
	struct crc_constants<std::uint32_t> {
		static constexpr std::uint32_t polynomial = 0xedb88320;
		static constexpr std::uint32_t xor_mask = 0xffffffff;
	};

	template <>
	struct crc_constants<std::uint16_t> {
		static constexpr std::uint16_t polynomial = 0xa001;
		static constexpr std::uint16_t xor_mask = 0;
	};

	template <typename Type>
	static constexpr auto make_polynomial_table() noexcept {
		std::array<Type, 256> table = {};
		for ( auto jj = 0; jj < 256; jj++ ) {
			Type b = jj;
			for ( auto kk = 0; kk < 8; kk++ ) {
				b = ( b & 1 ? crc_constants<Type>::polynomial ^ ( b >> 1 ) : b >> 1 );
			}
			table[jj] = b;
		}
		return table;
	}

} // detail

template <typename UIntType, std::enable_if_t<std::is_unsigned_v<UIntType>, int> = 0>
class crc {
protected:
	static constexpr auto polynomial_table = detail::make_polynomial_table<UIntType>();
	static constexpr auto xor_mask = detail::crc_constants<UIntType>::xor_mask;

	template <class Iter>
	constexpr void digest( Iter first, Iter last ) noexcept {
		auto c = xor_mask ^ m_value;
		for ( ; first != last; ++first ) {
			c = polynomial_table[( c ^ *first ) % polynomial_table.size()] ^ ( c >> 8 );
		}
		m_value = xor_mask ^ c;
	}

public:
	using value_type = UIntType;

	constexpr crc() noexcept : m_value( 0 ) {}

	template <class Container>
	constexpr auto write( const Container& container ) noexcept -> std::enable_if_t<sizeof( typename Container::value_type ) == 1, crc&> {
		digest( std::cbegin( container ), std::cend( container ) );
		return *this;
	}

	template <unsigned Size>
	constexpr crc& write( const char ( &str )[Size] ) noexcept {
		digest( std::cbegin( str ), std::cend( str ) - 1 );
		return *this;
	}

	template <class Type>
	constexpr crc& operator <<( const Type& value ) noexcept {
		return write( value );
	}

	constexpr operator value_type() const noexcept {
		return m_value;
	}

protected:
	value_type	m_value;

};

using crc16 = crc<std::uint16_t>;
using crc32 = crc<std::uint32_t>;

} // ffv

#endif // define FFV_CRC_HPP
