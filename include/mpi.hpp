#pragma once

#include "forest/index.hpp"

#include <vector>

#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

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
		UP = 0, 
		RIGHT, 
		DOWN, 
		LEFT
	};

	//! Combines two direction.
	/*!
		The function returnes the resulting orientation of after first orientating to "firstDir" and then
	reorientating, relative to "firstDir", to "secondDir".
		For example, if a person turns right once and then turns backwards, the resulting orientation of that
	person will be left.
	*/
	constexpr Direction combine_directions(const Direction firstDir, const Direction secondDir)
	{
		return static_cast<Direction>((firstDir + secondDir) % 4);
	}

	//! Converts a direction into an offset.
	constexpr offset get_offset(const Direction direction)
	{
		switch (direction)
		{
		case UP:
			return { 0, -1 };
		case DOWN:
			return { 0, 1 };
		case LEFT:
			return { -1, 0 };
		case RIGHT:
			return { 1, 0 };
		default:
			return {};
		}
	}
}

