#pragma once

#include <utility>
#include <ostream>

#pragma warning(push)
#pragma warning(disable : 4714)

#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#pragma warning(pop)

namespace ppc
{
	//! Unsigned integral type used as an index.
	using index_type = std::size_t;

	//! Index pair(x, y).
	using index_pair = std::pair<index_type, index_type>;

	//! Debug purpose output stream operator.
	//TODO: for some reason this doesn't work...
	/*static std::ostream& operator<<(std::ostream& out, const index_pair& indexPair)
	{
		out << "( x = " << indexPair.first << ", y = " << indexPair.second << " )";
		return out;
	}*/
}