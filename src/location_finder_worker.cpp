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
		Area area;
		mpi::scatter(m_workers, area, 0);
		PPC_LOG(debug) << "Area received: " << area;
		std::vector<std::vector<index_pair>> knownMatches;
		knownMatches.reserve(4);
		std::vector<index_pair> prevSizes;
		prevSizes.reserve(4);
		PatternGrowth patternGrowth{ {}, FORWARD };

		int patternMatches{};
		do
		{
			mpi::broadcast(m_workers, patternGrowth, 0);
			if (patternGrowth.first.zones.empty())
			{
				break;
			}

			patternMatches = 0;

			Pattern& pattern = patternGrowth.first;
			Direction growthDirection = patternGrowth.second;

			PPC_LOG(trace) << "Received pattern: " << pattern;


			
			if (knownMatches.empty())
			{
				for (auto i = 0u; i < 4; ++i)
				{
					knownMatches.push_back({});
					auto pMatches = match_pattern(map, pattern, area);
					knownMatches.back().insert(knownMatches.back().cbegin(), pMatches.cbegin(), pMatches.cend());
					prevSizes.push_back({ pattern.height, pattern.width });
					patternMatches += pMatches.size();

					if (i != 3)
					{
						pattern = rotate_pattern(pattern);
					}
				}
			}
			else
			{
				for (auto i = 0u; i < 4; ++i)
				{
					auto& prevMatches = knownMatches[i];
					auto& prevSize = prevSizes[i];
					std::vector<index_pair> currentMatches;
					currentMatches.reserve(prevMatches.size() / 2);


					const bool hasGrown = pattern.height != prevSize.first
						|| pattern.width != prevSize.second;
					auto growthOffset = get_offset(growthDirection);
					if (growthDirection == BACKWARDS || growthDirection == RIGHT)
					{
						growthOffset = {};
					}

					PPC_LOG(trace) << "Has grown: " << hasGrown;

					for (const auto& match : prevMatches)
					{
						if (!hasGrown)
						{
							PPC_LOG(trace) << "No growth search";
							if (match_pattern(map, pattern, match.first, match.second))
							{
								currentMatches.push_back(match);
								++patternMatches;
							}
						}
						else if ((growthOffset.first != -1 || match.first != 0)
							&& (growthOffset.second != -1 || match.second != 0))
						{
							//PPC_LOG(trace) << "Growth search: " << match << " -> ( x = " << match.first + growthOffset.first << ", y = " << match.second + growthOffset.second << " ) (dir = " << growthDirection << ")";
							if (match_pattern(map, pattern, match.first + growthOffset.first, match.second + growthOffset.second))
							{
								currentMatches.push_back({ match.first + growthOffset.first, match.second + growthOffset.second });
								++patternMatches;
							}
						}
					}

					prevMatches = std::move(currentMatches);
					prevSize = { pattern.height, pattern.width };

					if (i != 3)
					{
						pattern = rotate_pattern(pattern);
						growthDirection = combine_directions(growthDirection, LEFT);
					}
				}
			}

			PPC_LOG(debug) << "Number of matches: " << patternMatches;
			mpi::reduce(m_workers, patternMatches, std::plus<int>(), 0);
		} while (true);


		PPC_LOG(debug) << "Before shutdown: " << patternMatches;
		if (patternMatches == 1)
		{
			PPC_LOG(debug) << "Sending solution...";
			for (const auto& matches : knownMatches)
			{
				if (!matches.empty())
				{
					m_workers.send(0, tags::OK, matches.front());
					break;
				}
			}
		}

		return {};
	}
}