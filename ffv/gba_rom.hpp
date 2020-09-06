#ifndef FFV_GBA_ROM_HPP
#define FFV_GBA_ROM_HPP

#include <array>
#include <istream>

namespace ffv {

class gba_rom {
public:
	struct game_code {
		char	type;
		char	short_title[2];
		char	language;
		char	maker_code[2];

		auto title() const noexcept {
			return std::string_view( short_title, sizeof( short_title ) );
		}

		auto maker() const noexcept {
			return std::string_view( maker_code, sizeof( maker_code ) );
		}
	};

	static gba_rom	load( std::istream& streamSource );

	bool has_logo() const noexcept {
		return m_hasLogo;
	}

	auto title() const noexcept {
		return std::string_view( m_title.data(), m_title.size() );
	}

	const auto& id() const noexcept {
		return m_id;
	}

protected:
	gba_rom( const bool hasLogo, const std::array<char, 12>& title, const game_code& id, const std::uint8_t version ) noexcept : m_hasLogo { hasLogo }, m_title { title }, m_id { id }, m_version { version } {}

	const bool					m_hasLogo;
	const std::array<char, 12>	m_title;
	const game_code				m_id;
	const std::uint8_t			m_version;

};

} // ffv

#endif // define FFV_GBA_ROM_HPP
