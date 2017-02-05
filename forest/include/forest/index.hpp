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

	//! Can't overload operator<< because it's in namespace std.
	namespace detail
	{
		struct StdPair : std::pair<index_type, index_type>
		{
			using std::pair<index_type, index_type>::pair;

		private:
			friend class boost::serialization::access;

			template <typename Archive>
			void serialize(Archive& ar, const unsigned int /*version*/)
			{
				ar & first;
				ar & second;
			}
		};

		//! Debug purpose output stream operator.
		std::ostream& operator<<(std::ostream& out, const StdPair& pair);
	}

	//! Index pair(x, y).
	using index_pair = detail::StdPair;
}