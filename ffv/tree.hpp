#ifndef FFV_TREE_HPP
#define FFV_TREE_HPP

#include <optional>
#include <vector>

namespace ffv {

template <class KeyType, class ValueType>
class tree {
public:
	using size_type = std::uint32_t;
	using key_type = KeyType;
	using value_type = ValueType;

	class iterator {
		friend tree;
	public:
		explicit iterator( tree& owner, size_type pos ) noexcept : m_owner { owner }, m_pos { pos } {}

		bool operator ==( const iterator& other ) const noexcept {
			return &m_owner == &other.m_owner && m_pos == other.m_pos;
		}

		bool operator !=( const iterator& other ) const noexcept {
			return &m_owner != &other.m_owner || m_pos != other.m_pos;
		}

		class node& operator *() noexcept {
			return m_owner.m_nodes[m_pos];
		}

		class node * operator->() noexcept {
			return &m_owner.m_nodes[m_pos];
		}

		iterator& find_next( const key_type& key ) noexcept {
			auto node = m_owner.m_nodes[m_pos].find( key );
			m_pos = node.m_pos;
			return *this;
		}

	protected:
		tree&		m_owner;
		size_type	m_pos;

	};

	class const_iterator {
		friend tree;
	public:
		explicit const_iterator( const tree& owner, size_type pos ) noexcept : m_owner { owner }, m_pos { pos } {}

		bool operator ==( const const_iterator& other ) const noexcept {
			return &m_owner == &other.m_owner && m_pos == other.m_pos;
		}

		bool operator !=( const const_iterator& other ) const noexcept {
			return &m_owner != &other.m_owner || m_pos != other.m_pos;
		}

		const class node& operator *() const noexcept {
			return m_owner.m_nodes[m_pos];
		}

		const class node * operator->() const noexcept {
			return &m_owner.m_nodes[m_pos];
		}

		const_iterator& find_next( const key_type& key ) noexcept {
			const auto node = m_owner.m_nodes[m_pos].find( key );
			m_pos = node.m_pos;
			return *this;
		}

	protected:
		const tree&	m_owner;
		size_type	m_pos;

	};

	class node {
		friend tree;
	public:
		auto& value() noexcept {
			return m_value;
		}

		const auto& value() const noexcept {
			return m_value;
		}

		iterator find( const key_type& key ) noexcept {
			for ( const auto& child : m_children ) {
				const auto& node = m_owner->m_nodes[child];
				if ( node.m_key == key ) {
					return iterator( *m_owner, node.m_index );
				}
			}
			return m_owner->end();
		}

		const_iterator find( const key_type& key ) const noexcept {
			for ( const auto& child : m_children ) {
				const auto& node = m_owner->m_nodes[child];
				if ( node.m_key == key ) {
					return const_iterator( *m_owner, node.m_index );
				}
			}
			return m_owner->cend();
		}

	protected:
		explicit node( tree * owner, const size_type index, const key_type& key ) noexcept : m_owner { owner }, m_index { index }, m_children {}, m_key { key }, m_value {{}} {}

	private:
		tree *					m_owner;
		const size_type			m_index;
		std::vector<size_type>	m_children;

	protected:
		const key_type				m_key;
		std::optional<value_type>	m_value;

	};

	explicit tree() noexcept : m_nodes( 1, node( this, static_cast<size_type>( 0 ), key_type() ) ) {}

	tree( const tree& other ) noexcept : m_nodes( other.m_nodes ) {
		for ( auto& node : m_nodes ) {
			node.m_owner = this;
		}
	}

	auto empty() const noexcept {
		return m_nodes.size() == 1; // only root
	}

	iterator end() noexcept {
		return iterator( *this, static_cast<size_type>( -1 ) );
	}

	const_iterator cend() const noexcept {
		return const_iterator( *this, static_cast<size_type>( -1 ) );
	}

	const_iterator cbegin() const noexcept {
		return const_iterator( *this, 0 );
	}

	const_iterator find( const key_type& key ) const noexcept {
		return m_nodes.front().find( key );
	}

	template <class Iter>
	auto insert( Iter first, Iter last, const value_type& value ) noexcept -> std::enable_if_t<std::is_same_v<typename Iter::value_type, typename key_type>, void> {
		auto nodeIndex = 0;

		while ( first != last ) {
			const auto next = m_nodes[nodeIndex].find( *first );
			if ( next == end() ) {
				const auto index = static_cast<size_type>( m_nodes.size() );
				m_nodes[nodeIndex].m_children.push_back( index );
				m_nodes.push_back( node( this, index, *first ) );
				nodeIndex = index;
			} else {
				nodeIndex = next.m_pos;
			}
			++first;
		}

		m_nodes[nodeIndex].m_value = value;
	}

protected:
	std::vector<node>	m_nodes;

};

} // ffv

#endif // define FFV_TREE_HPP
