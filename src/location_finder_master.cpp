#include "location_finder_master.hpp"
#include "forest/index.hpp"

#include <cassert>
#include <cmath>
#include <functional>


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

			PPC_LOG(trace) << "New pattern: " << pattern;
		}
	}

	index_pair LocationFinderMaster::run(const Map& map)
	{
		const auto numOfWorkers = m_workers.size() - 1;
		assert(numOfWorkers);
		PPC_LOG(info) << "Number of workers available for location finding: " << numOfWorkers;

		auto areas = split(map, numOfWorkers);
		assert(areas.size() == static_cast<decltype(areas.size())>(numOfWorkers + 1));
		mpi::scatter(m_workers, areas, areas.front(), 0);

		auto numOfPatternMatches = 0;
		/*auto orientation = FORWARD;
		Pattern pattern;*/
		PatternGrowth patternGrowth{ {}, FORWARD };
		index_pair patternPosition;
		Direction moveDir;
		
		std::tie(patternGrowth.first, patternPosition, moveDir) = initialQuery(patternGrowth.second);
		//patternGrowth.second = combine_directions(patternGrowth.second, moveDir);


		do
		{
			numOfPatternMatches = 0;

			mpi::broadcast(m_workers, patternGrowth, 0);
			patternGrowth.second = combine_directions(patternGrowth.second, moveDir);

			

			mpi::reduce(m_workers, 0, numOfPatternMatches, std::plus<int>(), 0);
			PPC_LOG(info) << "Total number of locations found: " << numOfPatternMatches;
			assert(numOfPatternMatches >= 1);

			if (numOfPatternMatches != 1)
			{
				auto queryResult = query(moveDir);

				extend_pattern(patternGrowth.first, patternPosition, patternGrowth.second, queryResult);
				moveDir = find_next_direction(queryResult);
			}

		} while (numOfPatternMatches != 1);

		PPC_LOG(info) << "Location found! Computing final location...";
		ppc::mpi::broadcast(m_workers, ppc::dummy<ppc::PatternGrowth>, 0);

		index_pair patternMatchLocation;
		auto status = m_workers.recv(mpi::any_source, mpi::any_tag, patternMatchLocation);
		//PPC_LOG(info) << "Received pattern location from process #" << status.source() + 1 << ": " << patternMatchLocation;
		std::cout << "Received pattern location from process #" << status.source() + 1 << ": " << patternMatchLocation << std::endl;

		index_pair finalLocation{ patternMatchLocation.first + patternPosition.first, patternMatchLocation.second + patternPosition.second };
		//PPC_LOG(info) << "Computed final location: " << finalLocation;
		std::cout << "Computed final location: " << finalLocation << std::endl;

		PPC_LOG(info) << "Validating solution...";
		m_orientee.send(1, tags::VERIFY, dummy<Direction>);
		index_pair realLocation;
		m_orientee.recv(mpi::any_source, mpi::any_tag, realLocation);
		std::cout << "Real location: " << realLocation << std::endl;
		//PPC_LOG(info) << "Real location: " << realLocation;
		PPC_LOG(info) << "Is solution valid: " << std::boolalpha << (finalLocation == realLocation);


		/*	numOfPatternMatches++;
		} while (numOfPatternMatches < 5);*/

		return {};
	}

	std::tuple<Pattern, index_pair, Direction> LocationFinderMaster::initialQuery(Direction orientation)
	{
		Pattern pattern{ 3, 3,{ 3,{ 3, UNKNOWN } } };
		index_pair patternPosition{ 1, 1 };
		Direction moveDirection = FORWARD;

		auto result = query();
		for (const auto dir : g_directions)
		{
			const auto pos = get_position(patternPosition, combine_directions(orientation, dir));
			pattern.zones[pos.second][pos.first] = result[dir];
		}
		moveDirection = find_next_direction(result);

		return { std::move(pattern), patternPosition, moveDirection };
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
			Direction dummy = FORWARD;
			m_orientee.send(1, tags::QUERY, dummy);
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
}