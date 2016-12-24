#pragma once

#include "index.hpp"

#include <cstdint>

namespace ppc
{
	//! Indicates a direction.
	enum Direction : std::uint8_t
	{
		NORTH, 
		SOUTH, 
		EAST, 
		WEST
	};

	//! Offset used to add to indices.
	using offset = std::pair<int, int>;

	//! Converts a direction to an index offset.
	const offset get_offset(const Direction direction)
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