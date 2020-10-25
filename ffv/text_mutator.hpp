#ifndef FFV_TEXT_MUTATOR_HPP
#define FFV_TEXT_MUTATOR_HPP

#include <string_view>
#include <vector>

#include "text_table.hpp"
#include "gba.hpp"

namespace ffv {

using text_range_type = std::pair<std::string::size_type, std::string::size_type>;

class text_mutator {
public:
	text_mutator( const std::vector<std::byte>& data, const text_table::type& textTable, const gba::font_table& fontTable) noexcept;

	void	find_replace( const std::string_view& find, const std::string_view& replace );
	bool	target_find_replace( const std::uint32_t index, const std::string_view& find, const std::string_view& replace );
	void	name_case( const std::string_view& name );
	void	dialog_reflow();
	void	text_reflow();

	bool mark_dialog( const int lineId, const std::string_view& search ) {
		const auto pos = m_lines[lineId].find( search );
		if ( pos == std::string::npos ) {
			return false;
		}

		m_dialogMarks[lineId].push_back( pos );
		return true;
	}

	const auto& lines() const noexcept {
		return m_lines;
	}

protected:
	text_range_type	find_dialog( const std::string& str, const text_range_type& range, const int markIndex = -1 ) noexcept;

	std::uint32_t measure( const std::string& line, const std::size_t start, const std::size_t end ) const;

	std::uint32_t bartz_advance() const;
	std::uint32_t gil_advance() const;

	std::vector<std::string>	m_lines;
	const text_table::type&		m_textTable;
	const gba::font_table&		m_fontTable;

	std::vector<std::vector<std::size_t>>	m_dialogMarks;
};

} // ffv

#endif // define FFV_TEXT_MUTATOR_HPP