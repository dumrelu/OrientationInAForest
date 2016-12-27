#include "location_finder_worker.hpp"
#include "forest/pattern.hpp"

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

		do
		{
			Pattern pattern;
			mpi::broadcast(m_workers, pattern, 0);

			if (pattern.zones.empty())
			{
				break;
			}

			PPC_LOG(trace) << "Received pattern: " << pattern;

			std::vector<Pattern> patterns;
			patterns.reserve(4);
			patterns.push_back(pattern);
			for (auto i = 0; i < 3; ++i)
			{
				pattern = rotate_pattern(pattern);
			}

			std::vector<index_pair> matches;
			decltype(matches) pMatches;
			for (const auto& pattern : patterns)
			{
				pMatches = match_pattern(map, pattern, area);
				matches.insert(matches.cend(), pMatches.cbegin(), pMatches.cend());
			}

			PPC_LOG(trace) << "Matches found: " << matches.size();

		} while (true);

		return {};
	}
}