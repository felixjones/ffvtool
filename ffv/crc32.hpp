#ifndef FFV_CRC32_HPP
#define FFV_CRC32_HPP

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace ffv {
namespace detail {

	static constexpr auto make_polynomial_table() noexcept {
		using table_type = std::array<std::uint32_t, 256>;

		constexpr auto polynomial = 0xedb88320;

		table_type table = {};
		for ( auto jj = 0; jj < 256; jj++ ) {
			std::uint32_t b = jj;
			for ( auto kk = 0; kk < 8; kk++ ) {
				b = ( b & 1 ? polynomial ^ ( b >> 1 ) : b >> 1 );
			}
			table[jj] = b;
		}

		return table;
	}

} // detail

class crc32 {
protected:
	static constexpr auto polynomial_table = detail::make_polynomial_table();

public:
	constexpr crc32() noexcept : m_value( 0 ) {}

	template <typename Type, unsigned Size, std::enable_if_t<std::is_integral_v<Type> && sizeof( Type ) == 1, int> = 0>
	constexpr crc32& write( const std::array<Type, Size>& buffer ) noexcept {
		constexpr auto value_mask = std::numeric_limits<std::uint32_t>::max();

		auto c = value_mask ^ m_value;
		for ( const auto& element : buffer ) {
			c = polynomial_table[( c ^ element ) & 0xff] ^ ( c >> 8 );
		}
		m_value = value_mask ^ c;

		return *this;
	}

	template <unsigned Size>
	constexpr crc32& write( const char ( &str )[Size] ) noexcept {
		constexpr auto value_mask = std::numeric_limits<std::uint32_t>::max();

		auto c = value_mask ^ m_value;
		for ( auto ii = 0; ii < Size - 1; ii++ ) {
			c = polynomial_table[( c ^ str[ii] ) & 0xff] ^ ( c >> 8 );
		}
		m_value = value_mask ^ c;

		return *this;
	}

	template <class Type, class Traits, std::enable_if_t<std::is_integral_v<Type> && sizeof( Type ) == 1, int> = 0>
	constexpr crc32& write( const std::basic_string<Type, Traits>& str ) noexcept {
		constexpr auto value_mask = std::numeric_limits<std::uint32_t>::max();

		auto c = value_mask ^ m_value;
		for ( const auto& element : str ) {
			c = polynomial_table[( c ^ element ) & 0xff] ^ ( c >> 8 );
		}
		m_value = value_mask ^ c;

		return *this;
	}

	template <class Type, std::enable_if_t<std::is_integral_v<Type> && sizeof( Type ) == 1, int> = 0>
	constexpr crc32& write( const std::vector<Type>& vec ) noexcept {
		constexpr auto value_mask = std::numeric_limits<std::uint32_t>::max();

		auto c = value_mask ^ m_value;
		for ( const auto& element : vec ) {
			c = polynomial_table[( c ^ element ) & 0xff] ^ ( c >> 8 );
		}
		m_value = value_mask ^ c;

		return *this;
	}

	template <unsigned Size>
	constexpr crc32& operator <<( const char ( &str )[Size] ) noexcept {
		return write( str );
	}

	template <typename Type, unsigned Size>
	constexpr crc32& operator <<( const std::array<Type, Size>& buffer ) noexcept {
		return write( buffer );
	}

	template <class CharT, class Traits>
	constexpr crc32& operator <<( const std::basic_string<CharT, Traits>& str ) noexcept {
		return write( str );
	}

	template <class Type>
	constexpr crc32& operator <<( const std::vector<Type>& vec ) noexcept {
		return write( vec );
	}

	constexpr auto operator ==( const std::uint32_t value ) const noexcept {
		return m_value == value;
	}

	constexpr auto operator !=( const std::uint32_t value ) const noexcept {
		return m_value != value;
	}

	constexpr auto operator ==( const crc32& o ) const noexcept {
		return m_value == o.m_value;
	}

	constexpr auto operator !=( const crc32& o ) const noexcept {
		return m_value != o.m_value;
	}

protected:
	std::uint32_t	m_value;

};

} // ffv

#endif // define FFV_CRC32_HPP
