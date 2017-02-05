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

			if (area.y + area.height > main.y + main.height)
			{
				area.height = 0;
			}

			areas.push_back(std::move(area));
		}

		return areas;
	}

	std::vector<Area> factor_split(const Area& main, const index_type numOfAreas, const float factor)
	{
		assert(factor > 0.0f && factor < 1.0f);

		std::vector<index_type> heights;
		heights.reserve(numOfAreas);

		auto numOfRows = main.height;
		for (auto i = 0u; i < numOfAreas - 1; ++i)
		{
			const auto height = static_cast<index_type>(numOfRows * factor);
			heights.push_back(height);
			numOfRows -= height;
		}
		heights.push_back(numOfRows);
		std::sort(heights.begin(), heights.end());

		std::vector<Area> areas;
		areas.reserve(numOfAreas);
		auto currentY = main.y;
		for (const auto height : heights)
		{
			areas.push_back(Area{ main.x, currentY, height, main.width });
			currentY += height;
		}

		return areas;
	}
}