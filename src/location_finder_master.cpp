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
		auto find_next_direction(const query_result& result, const bool useRandom)
		{
			static std::default_random_engine g_randomEngine{ std::random_device{}() };

			std::array<Direction, 4> directions = { FORWARD, LEFT, RIGHT, BACKWARDS };
			if (useRandom)
			{
				std::shuffle(directions.begin() + 1, directions.begin() + 3, g_randomEngine);
			}

			for (const auto dir : directions)
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

		auto rotate_position(const index_pair& position, const index_type width)
		{
			return index_pair{ position.second, width - 1 - position.first };
		}

		auto rotate_position(index_pair position, int rotationCount, index_type height, index_type width)
		{
			for (auto i = 0; i < rotationCount; ++i)
			{
				position = rotate_position(position, width);
				std::swap(height, width);
			}

			return position;
		}

		auto compute_final_orientation(Direction orientation, int rotationCount)
		{
			for (; rotationCount; --rotationCount)
			{
				orientation = combine_directions(orientation, LEFT);
			}
			return orientation;
		}
	}

	LocationOrientationPair LocationFinderMaster::run(const Map& map, const bool randomized)
	{
		PPC_LOG(info) << "Location finder master started.";

		const auto numOfWorkers = m_workers.size() - 1;
		assert(numOfWorkers >= 1);
		PPC_LOG(info) << "Number of workers available for the location finding phase: " << numOfWorkers;

		auto areas = split({ 0, 0, map.height(), map.width() }, numOfWorkers);
		areas.insert(areas.cbegin(), Area{});	//Add an empty area at the begining for the master
		assert(areas.size() == static_cast<decltype(areas.size())>(numOfWorkers + 1));
		mpi::scatter(m_workers, areas, areas.front(), 0);

		Pattern pattern{};
		Direction orientation = FORWARD;
		index_pair patternPosition{ 1, 1 };
		query_result queryResult{};

		std::tie(queryResult, pattern) = initialQuery(orientation);

		auto numOfIterations = 0u;
		do
		{
			PatternGrowth patternGrowth{ pattern, orientation };
			mpi::broadcast(m_workers, patternGrowth, 0);

			int totalNumberOfMatches = 0;
			mpi::reduce(m_workers, 0, totalNumberOfMatches, std::plus<int>(), 0);
			assert(totalNumberOfMatches >= 1);
			PPC_LOG(info) << "Number of matches at iteration #" << numOfIterations++ << ": " << totalNumberOfMatches;

			if (totalNumberOfMatches == 1)
			{
				PPC_LOG(info) << "Location found!";
				break;
			}
			else
			{
				auto moveDirection = find_next_direction(queryResult, randomized);
				queryResult = query(moveDirection);

				orientation = combine_directions(orientation, moveDirection);
				extend_pattern(pattern, patternPosition, orientation, queryResult);
			}
		} while (true);	//TODO: maybe set a max number of iteration in case there is no solution?

		PPC_LOG(info) << "Solution found in " << numOfIterations << " iterations!";
		PPC_LOG(info) << "Preparing to shutdown the workers...";
		mpi::broadcast(m_workers, dummy<PatternGrowth>, 0);

		auto solution = computeSolution(patternPosition, pattern.height, pattern.width, orientation);
		const auto isCorrect = validateSolution(solution);
		if (isCorrect)
		{
			PPC_LOG(info) << "The identified solution is valid!";
		}
		else
		{
			PPC_LOG(info) << "The identified solution is invalid!";
			assert(false);
			solution = { {}, {} };
		}

		mpi::broadcast(m_workers, solution, 0);

		return solution;
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

	LocationOrientationPair LocationFinderMaster::computeSolution(const index_pair& patternPosition, const index_type height, const index_type width, Direction orientation)
	{
		index_pair matchedPatternLocation;
		auto status = m_workers.recv(mpi::any_source, mpi::any_tag, matchedPatternLocation);
		PPC_LOG(info) << "Received the final pattern match position from worker#" << status.source() << ": " << matchedPatternLocation;

		int rotationCount = 0;
		status = m_workers.recv(mpi::any_source, mpi::any_tag, rotationCount);
		PPC_LOG(info) << "The pattern was rotated by worker#" << status.source() << " " << rotationCount << " times.";

		auto rotatedPatternPosition = rotate_position(patternPosition, rotationCount, height, width);
		const index_pair finalLocation{ matchedPatternLocation.first + rotatedPatternPosition.first,
			matchedPatternLocation.second + rotatedPatternPosition.second
		};
		PPC_LOG(info) << "Computed final location: " << finalLocation;

		const auto finalOrientation = compute_final_orientation(orientation, rotationCount);
		PPC_LOG(info) << "Compute final orientation: " << finalOrientation;

		return { finalLocation, finalOrientation };
	}

	const bool LocationFinderMaster::validateSolution(const LocationOrientationPair& solution)
	{
		PPC_LOG(info) << "Validiating solution...";

		LocationOrientationPair realSolution;
		m_orientee.send(1, tags::VERIFY, dummy<Direction>);
		m_orientee.recv(mpi::any_source, mpi::any_tag, realSolution);
		PPC_LOG(info) << "Received real location: " << realSolution.first;
		PPC_LOG(info) << "Received real orientation: " << realSolution.second;

		return solution == realSolution;
	}
}