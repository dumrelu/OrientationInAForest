#pragma once

#include "forest/index.hpp"
#include "forest/zone.hpp"
#include "forest/area.hpp"
#include "forest/map.hpp"
#include "forest/pattern.hpp"

#include <array>
#include <chrono>
#include <ostream>
#include <string>
#include <vector>

#pragma warning(push, 0)

#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#pragma warning(pop)

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
			QUERY  = 0b0001'000, 
			MOVE   = 0b0010'000, 
			VERIFY = 0b0100'000,
			STATS  = 0b1000'000
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

	//! Used to send a new pattern which includes the grouth direction.
	using PatternGrowth = std::pair<Pattern, Direction>;

	//! The result type for location finding.
	using LocationOrientationPair = std::pair<index_pair, Direction>;

#pragma warning(push)	//VS implementation limitation warning
#pragma warning(disable : 4592)

	//! A list of all the directions for easy iteration.
	static const auto g_directions = { FORWARD, RIGHT, BACKWARDS, LEFT };

	//! Dummies used to send empty messages.
	template <typename D>
	static D dummy{};

#pragma warning(pop)

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

	//! Returns a direction given an offset.
	constexpr auto get_direction(const std::pair<int, int>& offset)
	{
		if (offset == std::make_pair(1, 0))
		{
			return RIGHT;
		}
		else if (offset == std::make_pair(-1, 0))
		{
			return LEFT;
		}
		else if (offset == std::make_pair(0, 1))
		{
			return BACKWARDS;
		}
		else /*if (offset == std::make_pair(0, -1))*/
		{
			return FORWARD;
		}
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

	//! Holds statistical information, including benchmarks.
	struct Statistics
	{
		index_type numOfProcessors;
		std::string startupOptions;
		index_type mapHeight;
		index_type mapWidth;
		std::chrono::milliseconds totalRunTime;

		index_pair startingLocation;
		Direction startingOrientation;
		index_pair identifiedLocation;
		Direction identifiedOrientation;
		bool identifiedSolutionValid;
		index_type numOfLocationFindingIterations;
		std::chrono::milliseconds locationFindingTime;

		bool pathFinding;
		index_type numOfPathFindingMoves;
		std::chrono::milliseconds pathFindingTime;

		index_type totalNumberOfMoves;
		index_type totalNumberOfQueries;
	};

	//! Writes the statistics to the given stream.
	std::ostream& operator<<(std::ostream& stream, const Statistics& stats);

	//! Splits the given area vertically into numOfAreas areas.
	std::vector<Area> split(const Area& main, const index_type numOfAreas);

	//! Splits the given area so that the last row has n * factor, the element before the last
	//one has (n - n * factor) * factor, etc. Where n is the number of rows.
	
	//! Splits the given area unevenly into numOfAreas areas, so that the last element area
	//has the most number of elements.
	/*
		Factor is a positive real number so that 0 < factor < 1.
		Let n be the number of rows. Splitting is done by first taking an area of (n * factor) rows, 
	then another one of ((n - n * factor) * factor) rows, etc.
		It is guaranteed that the resulting areas satisfy: area[0].height <= area[1].height <= ... <= area[numOfAreas-1].height.
	*/
	std::vector<Area> factor_split(const Area& main, const index_type numOfAreas, const float factor);

	//! Holds the process world id in printable form.
	extern std::string g_worldID;
}

//! Logs the given message using boost::log and the printable name defined by g_worldID.
/*!
	trace < debug < info < warning < error < fatal.
*/
#define PPC_LOG(severity) BOOST_LOG_TRIVIAL(severity) << ppc::g_worldID

