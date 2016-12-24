#pragma once

#include <utility>

namespace ppc
{
	//! Unsigned integral type used as an index.
	using index_type = std::size_t;

	//! Index pair.
	using index_pair = std::pair<index_type, index_type>;
}