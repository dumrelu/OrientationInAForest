#pragma once

#include "forest/index.hpp"

#include <vector>

#include <boost/mpi.hpp>

namespace ppc
{
	//! Namespace alias for boost::mpi.
	namespace mpi = boost::mpi;

	//! A list of indices representing a path from .front() to .back().
	using path = std::vector<index_pair>;

	//! Used as an offset for index_pair coordinates.
	using offset = std::pair<int, int>;

	//! Indicates direction.
	enum Direction : std::uint8_t
	{
		NORTH, 
		SOUTH, 
		EAST, 
		WEST
	};

	//! Converts a direction into an offset.
	constexpr offset get_offset(const Direction direction)
	{
		switch (direction)
		{
		case NORTH:
			return { 0, -1 };
		case SOUTH:
			return { 0, 1 };
		case EAST:
			return { 1, 0 };
		case WEST:
			return { -1, 0 };
		default:
			return {};
		}
	}
}