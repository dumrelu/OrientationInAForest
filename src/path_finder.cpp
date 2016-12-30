#include "path_finder.hpp"

#include <algorithm>

namespace ppc
{
	PathFinder::PathFinder(mpi::communicator workers, mpi::communicator orientee)
		: m_workers{ std::move(workers) }, m_orientee{ std::move(orientee) }
	{
	}

	void PathFinder::run(const Map& map, const Area& area)
	{
		if (area.height == 0 || area.width == 0)
		{
			return;
		}

		auto isPassable = [&map,&area](const auto x, const auto y) 
		{
			const auto zone = map[area.y + y][area.x + x];
			return zone == OPEN || zone == ROAD; 
		};

		//2D matrix of pair of { weight, turn }
		std::vector<std::vector<std::pair<int, std::int8_t>>> table{ area.height,{ area.width, std::make_pair(0, 0) } };
		for (auto x = 0u; x < area.width; ++x)
		{
			table[0][x].first = isPassable(x, 0) ? 0 : -1;
		}

		for (auto y = 1u; y < area.height; ++y)
		{
			for (auto x = 0u; x < area.width; ++x)
			{
				auto& row = table[y];
				const auto& prev = table[y - 1];

				if (!isPassable(x, y))
				{
					row[x].first = -1;
					continue;
				}

				auto minDir = std::min({ 0, -1, 1 }, [&](int offset1, int offset2)
				{
					if (x == 0 || x == area.width - 1)
					{
						const auto invalidOffset = (x == 0) ? -1 : 1;
						if (offset1 == invalidOffset)
						{
							return false;
						}
						else if (offset2 == invalidOffset)
						{
							return true;
						}
					}

					auto val1 = prev[x + offset1].first;
					auto val2 = prev[x + offset2].first;
					if (val1 < 0)
					{
						return false;
					}
					else if (val2 < 0)
					{
						return true;
					}

					return val1 < val2;
				});

				const auto prevVal = prev[x + minDir].first;
				if (prevVal >= 0)
				{
					const auto addedWeight = minDir == 0 ? 1 : 2;
					row[x] = { prevVal + addedWeight, minDir };
				}
				else
				{
					row[x] = { -1, 0 };
				}
			}
		}
	}
}