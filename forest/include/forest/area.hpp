#pragma once

#include "map.hpp"

#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>

namespace ppc
{
	struct Area;

	//! Represents a rectangular area of a map.
	struct Area
	{
		index_type x;
		index_type y;
		index_type height;
		index_type width;

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar & x;
			ar & y;
			ar & width;
			ar & height;
		}
	};

	//! Debug purpose output stream operator.
	std::ostream& operator<<(std::ostream& out, const Area& area);
}