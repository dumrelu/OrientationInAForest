#include "common.hpp"

namespace ppc
{
	std::vector<Area> split(const Area& main, const index_type numOfAreas)
	{
		std::vector<Area> areas;
		areas.reserve(numOfAreas);

		const auto numOfRowsPerArea = static_cast<index_type>(std::ceil(main.height / static_cast<double>(numOfAreas)));
		PPC_LOG(debug) << "Number of rows per area: " << numOfRowsPerArea;
		for (index_type i = 0; i < numOfAreas; ++i)
		{
			auto area = Area{ main.x, main.y + i * numOfRowsPerArea, numOfRowsPerArea, main.width };
			if (i == numOfAreas - 1)
			{
				area.height = main.height - (numOfAreas - 1) * numOfRowsPerArea;
			}

			if (area.y + area.height > main.height)
			{
				area.height = 0;
			}

			areas.push_back(std::move(area));
		}

		return areas;
	}
}