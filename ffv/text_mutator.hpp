#ifndef FFV_TEXT_MUTATOR_HPP
#define FFV_TEXT_MUTATOR_HPP

#include <string_view>
#include <vector>

#include "text_table.hpp"
#include "gba.hpp"

namespace ffv {

class text_mutator {
public:
	text_mutator( const std::vector<std::byte>& data, const text_table::type& textTable, const gba::font_table& fontTable) noexcept;

	void	find_replace( const std::string_view& find, const std::string_view& replace );
	bool	target_find_replace( const std::uint32_t index, const std::string_view& find, const std::string_view& replace );
	void	name_case( const std::string_view& name );
	void	dialog_reflow();

protected:
	std::vector<std::string>	m_lines;
	const text_table::type&		m_textTable;
	const gba::font_table&		m_fontTable;

};

} // ffv

#endif // define FFV_TEXT_MUTATOR_HPP