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

	std::ostream& operator<<(std::ostream& stream, const Statistics& stats)
	{
		stream << "Number of processors used: " << stats.numOfProcessors << std::endl;
		stream << "Startup options: " << stats.startupOptions << std::endl;
		stream << "Map size: height = " << stats.mapHeight << ", width = " << stats.mapWidth << std::endl;
		stream << "Total time: " << stats.totalTime.count() << " ms" << std::endl;
		stream << "Initialization time: " << stats.initializationTime.count() << " ms" << std::endl;
		stream << "Total run time: " << (stats.totalTime - stats.initializationTime).count() << " ms" << std::endl;

		stream << "Starting location: " << stats.startingLocation << std::endl;
		stream << "Starting orientation: " << stats.startingOrientation << std::endl;
		stream << "Identified location: " << stats.identifiedLocation << std::endl;
		stream << "Identified direction: " << stats.identifiedOrientation << std::endl;
		stream << "Was the identified solution valid: " << (stats.identifiedSolutionValid ? "YES" : "NO") << std::endl;
		stream << "Number of iterations to find the location: " << stats.numOfLocationFindingIterations << std::endl;
		stream << "Location finding time: " << stats.locationFindingTime.count() << " ms" << std::endl;

		stream << "Path finding: " << (stats.pathFinding ? "ON" : "OFF") << std::endl;
		if (stats.pathFinding)
		{
			stream << "Number of moves for path finding: " << stats.numOfPathFindingMoves << std::endl;
			stream << "Path finding time: " << stats.pathFindingTime.count() << " ms" << std::endl;
		}

		stream << "Total number of moves: " << stats.totalNumberOfMoves << std::endl;
		stream << "Total number of queries: " << stats.totalNumberOfQueries << std::endl;

		return stream;
	}
}