#include "location_finder_master.hpp"
#include "forest/index.hpp"

#include <cassert>
#include <cmath>
#include <functional>
#include <sstream>

namespace ppc
{
	LocationFinderMaster::LocationFinderMaster(mpi::communicator workers, mpi::communicator orientee)
		: m_workers{ std::move(workers) }, m_orientee{ std::move(orientee) }
	{
		assert(m_workers.rank() == 0);
		assert(m_orientee.rank() == 0);
	}

	namespace
	{
		//! For now splitting is done only vertically.
		auto split(const Map& map, index_type numOfAreas)
		{
			std::vector<Area> areas;
			areas.reserve(numOfAreas + 1);	// + 1 so it can be used for scatter
			areas.push_back({});			//master doesn't need an area

			const auto numOfRowsPerArea = static_cast<index_type>(std::ceil(map.height() / static_cast<double>(numOfAreas)));
			PPC_LOG(debug) << "Number of rows per area: " << numOfRowsPerArea;
			for (index_type i = 0; i < numOfAreas; ++i)
			{
				auto area = Area{ 0, i * numOfRowsPerArea, numOfRowsPerArea, map.width() };
				if (i == numOfAreas - 1)
				{
					area.height = map.height() - (numOfAreas - 1) * numOfRowsPerArea;
				}

				areas.push_back(std::move(area));
			}

			return areas;
		}

		auto find_next_direction(const query_result& result)
		{
			for (const auto dir : { FORWARD, LEFT, RIGHT, BACKWARDS })
			{
				if (result[dir] == OPEN || result[dir] == ROAD)
				{
					return dir;
				}
			}

			PPC_LOG(fatal) << "Unable to find next direction";
			assert(false);
			return FORWARD;
		}

		void extend_pattern(Pattern& pattern, index_pair& position, const Direction orientation, const query_result& qResult)
		{
			auto dir = get_offset(orientation);
			auto growX = false;
			auto growY = false;
			auto xOffset = 0;
			auto yOffset = 0;

			if (position.first + dir.first == 0)	//Shift the whole matrix, so there's no need to change the position.
			{
				growX = true;
				xOffset = 1;
			}
			else if (position.first + dir.first == pattern.width - 1)	//The matrix remains unchanged(just grows), so change the position.
			{
				growX = true;
				++position.first;
			}
			
			if (position.second + dir.second == 0)
			{
				growY = true;
				yOffset = 1;
			}
			else if (position.second + dir.second == pattern.height - 1)
			{
				growY = true;
				++position.second;
			}

			Pattern newPattern;
			newPattern.height = pattern.height + (growY ? 1 : 0);
			newPattern.width = pattern.width + (growX ? 1 : 0);
			if (!growX && !growY)
			{
				newPattern.zones = std::move(pattern.zones);
				position = get_position(position, orientation);
			}
			else
			{
				newPattern.zones = { newPattern.height, {newPattern.width, UNKNOWN} };
				for (index_type y = 0; y < pattern.height; ++y)
				{
					for (index_type x = 0; x < pattern.width; ++x)
					{
						newPattern.zones[y + yOffset][x + xOffset] = pattern.zones[y][x];
					}
				}
			}

			for (const auto direction : g_directions)
			{
				const auto orientated = combine_directions(orientation, direction);
				const auto orientatedPos = get_position(position, orientated);
				newPattern.zones[orientatedPos.second][orientatedPos.first] = qResult[direction];
			}
			pattern = std::move(newPattern);
		}

		//TODO: For some reason ostream << index_pair doesn't work with boost::log...
		inline auto to_string(const index_pair& pair)
		{
			return static_cast<const std::ostringstream&>(std::ostringstream{} << "( x = " << pair.first << ", y = " << pair.second << " )").str();
		}
	}

	index_pair LocationFinderMaster::run(const Map& map)
	{
		PPC_LOG(info) << "Location finder master started.";

		const auto numOfWorkers = m_workers.size() - 1;
		assert(numOfWorkers >= 1);
		PPC_LOG(info) << "Number of workers available for the location finding phase: " << numOfWorkers;

		auto areas = split(map, numOfWorkers);
		assert(areas.size() == static_cast<decltype(areas.size())>(numOfWorkers + 1));
		mpi::scatter(m_workers, areas, areas.front(), 0);

		Pattern pattern{};
		Direction orientation = FORWARD;
		index_pair patternPosition{ 1, 1 };
		query_result queryResult{};

		std::tie(queryResult, pattern) = initialQuery(orientation);

		
		do
		{
			PatternGrowth patternGrowth{ pattern, orientation };
			mpi::broadcast(m_workers, patternGrowth, 0);

			int totalNumberOfMatches = 0;
			mpi::reduce(m_workers, 0, totalNumberOfMatches, std::plus<int>(), 0);
			assert(totalNumberOfMatches >= 1);

			if (totalNumberOfMatches == 1)
			{
				PPC_LOG(info) << "Location found!";
				break;
			}
			else
			{
				auto moveDirection = find_next_direction(queryResult);
				queryResult = query(moveDirection);

				orientation = combine_directions(orientation, moveDirection);
				extend_pattern(pattern, patternPosition, orientation, queryResult);
			}
		} while (true);	//TODO: maybe set a max number of iteration in case there is no solution?

		PPC_LOG(info) << "Preparing to shutdown the workers...";
		mpi::broadcast(m_workers, dummy<PatternGrowth>, 0);

		auto finalLocation = computeFinalLocation(patternPosition);
		const auto isCorrect = validateSolution(finalLocation);
		if (isCorrect)
		{
			PPC_LOG(info) << "The identified solution is valid!";
		}
		else
		{
			PPC_LOG(info) << "The identified solution is invalid!";
			assert(false);
		}

		mpi::broadcast(m_workers, finalLocation, 0);

		return finalLocation;
	}

	std::pair<query_result, Pattern> LocationFinderMaster::initialQuery(Direction initialDirection)
	{
		PPC_LOG(info) << "Running initial query...";
		query_result queryResult = query();
		Pattern pattern{ 3, 3, {3, {3, UNKNOWN}} };

		const index_pair middle{ 1, 1 };
		for (const auto dir : g_directions)
		{
			const auto pos = get_position(middle, combine_directions(initialDirection, dir));
			pattern.zones[pos.second][pos.first] = queryResult[dir];
		}

		PPC_LOG(info) << "Initial pattern: " << pattern;
		return { queryResult, std::move(pattern) };
	}

	query_result LocationFinderMaster::query(boost::optional<Direction> direction)
	{
		if (direction)
		{
			PPC_LOG(trace) << "Sending query and move(direction = " << *direction << ")...";
			m_orientee.send(1, tags::MOVE | tags::QUERY, *direction);
		}
		else
		{
			PPC_LOG(trace) << "Sending query...";
			m_orientee.send(1, tags::QUERY, dummy<Direction>);
		}

		query_result result;
		auto status = m_orientee.recv(mpi::any_source, mpi::any_tag, result);
		if (status.tag() & tags::ERROR)
		{
			PPC_LOG(fatal) << "Query fatal error!" << std::endl;
			assert(false);
		}
		PPC_LOG(trace) << "Received query result: " << result;

		return result;
	}

	index_pair LocationFinderMaster::computeFinalLocation(const index_pair& patternPosition)
	{
		index_pair matchedPatternLocation;
		auto status = m_workers.recv(mpi::any_source, mpi::any_tag, matchedPatternLocation);
		PPC_LOG(info) << "Received the final pattern match from worker #" << status.source() << ": " << to_string(matchedPatternLocation);

		index_pair finalLocation{ matchedPatternLocation.first + patternPosition.first,
			matchedPatternLocation.second + patternPosition.second
		};
		PPC_LOG(info) << "Computed final location: " << to_string(finalLocation);

		return finalLocation;
	}

	const bool LocationFinderMaster::validateSolution(const index_pair& finalLocation)
	{
		PPC_LOG(info) << "Validiating solution...";

		index_pair realLocation;
		m_orientee.send(1, tags::VERIFY, dummy<Direction>);
		m_orientee.recv(mpi::any_source, mpi::any_tag, realLocation);
		PPC_LOG(info) << "Received real location: " << to_string(realLocation);

		return finalLocation == realLocation;
	}
}