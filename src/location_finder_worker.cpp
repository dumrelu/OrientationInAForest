#include "location_finder_worker.hpp"
#include "forest/pattern.hpp"
#include "forest/index.hpp"

#include <algorithm>

namespace ppc
{
	LocationFinderWorker::LocationFinderWorker(mpi::communicator workers)
		: m_workers{ std::move(workers) }
	{
	}

	namespace
	{
		auto rotate_pattern(const Pattern& pattern)
		{
			Pattern rotated{ pattern.width, pattern.height, {pattern.width, {pattern.height, UNKNOWN}} };
			for (index_type i = 0; i < pattern.width; ++i)
			{
				for (index_type j = 0; j < pattern.height; ++j)
				{
					rotated.zones[i][j] = pattern.zones[j][i];
				}
			}

			std::reverse(rotated.zones.begin(), rotated.zones.end());
			return rotated;
		}
	}

	index_pair LocationFinderWorker::run(const Map& map)
	{
		PPC_LOG(info) << "Location finder worker #" << m_workers.rank() << " started.";

		Area area;
		mpi::scatter(m_workers, area, 0);
		PPC_LOG(info) << "Received area: " << area;

		std::vector<std::vector<index_pair>> prevMatches;
		std::vector<index_pair> prevSizes;
		prevMatches.reserve(4);		//Store the previous matches for each rotation of the pattern
		prevSizes.reserve(4);

		int numOfMatches = 0;
		PatternGrowth patternGrowth;
		Pattern& pattern = patternGrowth.first;
		Direction& growthDirection = patternGrowth.second;

		auto initialPatternMatching = [&]()
		{
			prevMatches.clear();
			prevSizes.clear();

			auto matchCount = 0;
			for (auto i = 0u; i < 4; ++i)
			{
				auto matches = match_pattern(map, pattern, area);
				matchCount += matches.size();

				prevMatches.push_back(std::move(matches));
				prevSizes.push_back({ pattern.height, pattern.width });

				if (i != 3)
				{
					pattern = rotate_pattern(pattern);
				}
			}

			return matchCount;
		};

		auto informedPatterhMatching = [&]()
		{
			auto matchCount = 0;
			for (auto i = 0u; i < 4; ++i)
			{
				auto& prevMatch = prevMatches[i];
				auto& prevSize = prevSizes[i];
				std::vector<index_pair> currentMatch;
				currentMatch.reserve(prevMatch.size() / 3);

				const bool hasGrown = pattern.height != prevSize.first
					|| pattern.width != prevSize.second;
				
				auto growthOffset = get_offset(growthDirection);
				if (!hasGrown || growthDirection == BACKWARDS || growthDirection == RIGHT)
				{
					growthOffset = {};
				}

				for (const auto& match : prevMatch)
				{
					//Make sure the new pattern still fits inside the map
					if ((growthOffset.first != -1 || match.first != 0) 
						&& (growthOffset.second != -1 || match.second != 0))
					{
						index_pair offsetPosition{ match.first + growthOffset.first, match.second + growthOffset.second };

						if (match_pattern(map, pattern, offsetPosition.first, offsetPosition.second))
						{
							currentMatch.push_back(offsetPosition);
							++matchCount;
						}
					}
				}

				prevMatch = std::move(currentMatch);
				prevSize = { pattern.height, pattern.width };

				if (i != 3)
				{
					pattern = rotate_pattern(pattern);
					growthDirection = combine_directions(growthDirection, LEFT);
				}
			}

			return matchCount;
		};


		do
		{
			mpi::broadcast(m_workers, patternGrowth, 0);
			const bool shouldStop = pattern.zones.empty();
			if (shouldStop)
			{
				PPC_LOG(info) << "Shutdown signal received";
				break;
			}

			PPC_LOG(info) << "Pattern received.";
			PPC_LOG(debug) << "Received pattern: " << pattern;
			
			const bool isFirstRun = prevMatches.empty();
			if (isFirstRun)
			{
				numOfMatches = initialPatternMatching();
			}
			else
			{
				numOfMatches = informedPatterhMatching();
			}

			PPC_LOG(info) << "Number of matches: " << numOfMatches;
			mpi::reduce(m_workers, numOfMatches, std::plus<int>(), 0);
		} while (true);

		if (numOfMatches == 1)
		{
			PPC_LOG(info) << "Sending solution...";
			auto matchIt = std::find_if(prevMatches.cbegin(), prevMatches.cend(), [](const auto& match)
			{
				return !match.empty();
			});
			assert(matchIt != prevMatches.cend());
			m_workers.send(0, tags::OK, matchIt->front());
		}

		index_pair finalLocation;
		mpi::broadcast(m_workers, finalLocation, 0);
		PPC_LOG(info) << "Received final location. Work done.";

		return finalLocation;
	}
}