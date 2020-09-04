#ifndef FFV_TREE_HPP
#define FFV_TREE_HPP

#include <algorithm>
#include <optional>
#include <vector>

namespace ffv {

template <class KeyType, class ValueType>
class tree {
protected:
	class node;
public:
	using size_type = std::uint32_t;
	using key_type = KeyType;
	using value_type = ValueType;

	class iterator {
		friend tree;
	public:
		explicit iterator( tree& owner, const size_type pos ) noexcept : m_owner { owner }, m_pos { pos } {}

		bool operator ==( const iterator& other ) const noexcept {
			return &m_owner == &other.m_owner && m_pos == other.m_pos;
		}

		bool operator !=( const iterator& other ) const noexcept {
			return &m_owner != &other.m_owner || m_pos != other.m_pos;
		}

		node& operator *() noexcept {
			return m_owner.m_nodes[m_pos];
		}

		node * operator ->() noexcept {
			return &m_owner.m_nodes[m_pos];
		}

		iterator& find_next( const key_type& key ) noexcept {
			const auto node = m_owner.m_nodes[m_pos].find( key );
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

		bool operator ==( const iterator& other ) const noexcept {
			return &m_owner == &other.m_owner && m_pos == other.m_pos;
		}

		bool operator !=( const const_iterator& other ) const noexcept {
			return &m_owner != &other.m_owner || m_pos != other.m_pos;
		}

		bool operator !=( const iterator& other ) const noexcept {
			return &m_owner != &other.m_owner || m_pos != other.m_pos;
		}

		const node& operator *() const noexcept {
			return m_owner.m_nodes[m_pos];
		}

		const node * operator ->() const noexcept {
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

	explicit tree() noexcept : m_nodes( 1, node( this, static_cast<size_type>( 0 ), key_type() ) ) {}

	tree( const tree& other ) noexcept {
		m_nodes.reserve( other.m_nodes.size() );
		std::transform( std::cbegin( other.m_nodes ), std::cend( other.m_nodes ), std::back_inserter( m_nodes ),
			[this]( const node& n ) {
				return node( this, n );
			}
		);
	}

	bool empty() const noexcept {
		return m_nodes.size() == 1; // only root
	}

	iterator begin() const noexcept {
		return iterator( *this, 0 );
	}

	iterator end() noexcept {
		return iterator( *this, static_cast<size_type>( -1 ) );
	}

	const_iterator cbegin() const noexcept {
		return const_iterator( *this, 0 );
	}

	const_iterator cend() const noexcept {
		return const_iterator( *this, static_cast<size_type>( -1 ) );
	}

	iterator find( const key_type& key ) noexcept {
		return m_nodes.front().find( key );
	}

	const_iterator find( const key_type& key ) const noexcept {
		return m_nodes.front().find( key );
	}

	template <class Iter>
	auto insert( Iter first, Iter last, const value_type& value ) noexcept -> std::enable_if_t<std::is_same_v<typename std::iterator_traits<Iter>::value_type, KeyType>, void> {
		size_type nodeIndex = 0;
		while ( first != last ) {
			const auto next = m_nodes[nodeIndex].find( *first );
			if ( next == cend() ) {
				const auto index = static_cast<size_type>( m_nodes.size() );
				m_nodes[nodeIndex].m_children.push_back( index );
				m_nodes.emplace_back( this, index, *first );
				nodeIndex = index;
			} else {
				nodeIndex = next.m_pos;
			}
			++first;
		}

		m_nodes[nodeIndex].m_value = value;
	}

protected:
	class node {
		friend tree;
	public:
		explicit node( tree * const owner, const size_type index, const key_type& key ) noexcept : m_key { key }, m_value {}, m_owner { owner }, m_index { index }, m_children {} {}

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
		explicit node( tree * const owner, const node& other ) noexcept : m_key { other.m_key }, m_value { other.m_value }, m_owner { owner }, m_index { other.m_index }, m_children { other.m_children } {}

		const key_type				m_key;
		std::optional<value_type>	m_value;

	private:
		tree * const			m_owner;
		const size_type			m_index;
		std::vector<size_type>	m_children;

	};

	std::vector<node>	m_nodes;

};

} // ffv

#endif // define FFV_TREE_HPP
