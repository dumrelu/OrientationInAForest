#pragma once

#include <utility>
#include <ostream>

namespace ppc
{
	//! Unsigned integral type used as an index.
	using index_type = std::size_t;

	//! Index pair(x, y).
	using index_pair = std::pair<index_type, index_type>;

	//! Debug purpose output stream operator.
	inline std::ostream& operator<<(std::ostream& out, const index_pair& indexPair)
	{
		out << "( x = " << indexPair << ", y = " << indexPair.second << " )";
		return out;
	}
}