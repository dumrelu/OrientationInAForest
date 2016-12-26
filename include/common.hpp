#pragma once

#include "forest/index.hpp"
#include "forest/zone.hpp"
#include "forest/area.hpp"
#include "forest/map.hpp"

#include <array>
#include <ostream>
#include <string>
#include <vector>

#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace ppc
{
	namespace tags
	{
		//! Common tags.
		enum Common
		{
			OK	  = 0b001, 
			ERROR = 0b010, 
			STOP  = 0b100
		};

		//! Tags used between the orientee and the masters.
		enum OrienteeTags
		{
			QUERY = 0b01000, 
			MOVE  = 0b10000
		};
	}

	//! Namespace alias for boost::mpi.
	namespace mpi = boost::mpi;

	//! A list of indices which represent a path from path.front() to path.back().
	using path = std::vector<index_pair>;

	//! Results of a positional query.
	/*!
		query_result[dir] = ZoneType.
	*/
	using query_result = std::array<ZoneType, 4>;

	//! Direction enum.
	enum Direction : std::uint8_t
	{
		FORWARD = 0, 
		RIGHT, 
		BACKWARDS, 
		LEFT
	};

	//! A list of all the directions for easy iteration.
	static const auto g_directions = { FORWARD, RIGHT, BACKWARDS, LEFT };

	//! Debug purpose output stream operator.
	inline std::ostream& operator<<(std::ostream& out, const query_result& result)
	{
		out << "( FORWARD = " << result[FORWARD] << ", BACKWARDS = " << result[BACKWARDS]
			<< ", LEFT = " << result[LEFT] << ", RIGHT = " << result[RIGHT] << " )";
		return out;
	}

	//! Returns the direction resulting from applying the given two direction consecutively.
	/*!
		For example, if a person turns RIGHT once and then turns BACKWARDS, the resulting orientation of that
	person will be LEFT.
	*/
	constexpr auto combine_directions(const Direction first, const Direction second)
	{
		return static_cast<Direction>((first + second) % 4);
	}

	//! Returns a map offset for the given direction.
	constexpr auto get_offset(const Direction direction)
	{
		auto offset = std::make_pair(0, 0);
		switch (direction)
		{
		case FORWARD:
			offset = { 0, -1 };
			break;
		case RIGHT:
			offset = { 1, 0 };
			break;
		case BACKWARDS:
			offset = { 0, 1 };
			break;
		case LEFT:
			offset = { -1, 0 };
			break;
		}

		return offset;
	}

	//! Returns the resulting position after moving on the given direction, starting
	//from the given position.
	//TODO: rename to offset_position
	constexpr auto get_position(index_pair position, const Direction direction)
	{
		const auto offset = get_offset(direction);
		position.first += offset.first;
		position.second += offset.second;

		return position;
	}

	//! Holds the process world id in printable form.
	extern std::string g_worldID;
}

//! Logs the given message using boost::log and the printable name defined by g_worldID.
/*!
	trace < debug < info < warning < error < fatal.
*/
#define PPC_LOG(severity) BOOST_LOG_TRIVIAL(severity) << ppc::g_worldID