#include "path_finder.hpp"

#include <algorithm>

namespace ppc
{
	PathFinder::PathFinder(mpi::communicator workers, mpi::communicator orientee, const index_type workerID, const index_type numOfWorkers)
		: m_workers{ std::move(workers) }, m_orientee{ std::move(orientee) }, m_workerID{ workerID }, m_numOfWorkers{ numOfWorkers }
	{
	}

	namespace
	{
		using TableRow = std::vector<std::pair<int, std::int8_t>>;

		struct MinOffsetCompare
		{
			MinOffsetCompare(const Map& map, const Area& area, const TableRow& prevRow, const index_type x)
				: m_map{ map }, m_area{ area }, m_prevRow{ prevRow }, m_x{ x }
			{
			}

			const bool operator()(const int offset1, const int offset2) const
			{
				if (m_x == 0 || m_x == m_area.width - 1)
				{
					const auto invalidOffset = (m_x == 0) ? -1 : 1;
					if (offset1 == invalidOffset)
					{
						return false;
					}
					else if (offset2 == invalidOffset)
					{
						return true;
					}
				}

				auto val1 = m_prevRow[m_x + offset1].first;
				auto val2 = m_prevRow[m_x + offset2].first;
				if (val1 < 0)
				{
					return false;
				}
				else if (val2 < 0)
				{
					return true;
				}

				return val1 < val2;
			}

			void x(const index_type value)
			{
				m_x = value;
			}

		private:
			const Map& m_map;
			const Area& m_area;
			const TableRow& m_prevRow;
			index_type m_x;
		};

		struct ZoneValidator
		{
			ZoneValidator(const Map& map, const Area& area)
				: m_map{ map }, m_area{ area }
			{
			}

			const bool operator()(const int x, const int y) const
			{
				const auto zone = m_map[m_area.y + y][m_area.x + x];
				return zone == OPEN || zone == ROAD;
			}

		private:
			const Map& m_map;
			const Area& m_area;
		};

		auto create_auxiliary_row(const ZoneValidator& validator, const index_type width, const index_type startX)
		{
			std::vector<int> auxiliaryRow(width, -1);
			auxiliaryRow[startX] = 0;
			if (startX > 0)
			{
				for (auto x = startX - 1; x >= 0 && validator(x, -1); --x)
				{
					auxiliaryRow[x] = auxiliaryRow[x + 1] + 1;
				}
			}

			for (auto x = startX + 1; x < width && validator(x, -1); ++x)
			{
				auxiliaryRow[x] = auxiliaryRow[x - 1] + 1;
			}

			return auxiliaryRow;
		}

		auto find_entrance_point(const std::vector<int>& pointRow, const TableRow& row, MinOffsetCompare& compare)
		{
			index_type minX = 0;
			int minValue = -1;
			
			for (auto x = 0u; x < pointRow.size(); ++x)
			{
				if (pointRow[x] < 0)
				{
					continue;
				}

				compare.x(x);
				auto minDir = std::min({ 0, -1, 1 }, compare) * -1;

				if (row[x + minDir].first < 0)
				{
					continue;
				}

				const auto val = pointRow[x] + row[x + minDir].first + (minDir == 0 ? 1 : 2);
				if (minValue < 0 || val < minValue)
				{
					minX = x;
					minValue = val;
				}
			}

			assert(minValue >= 0);
			if (minValue < 0)
			{
				PPC_LOG(info) << "Failed to find entrance point...";
				std::exit(1);
			}
			return minX;
		}

		//TODO: For some reason ostream << index_pair doesn't work with boost::log...
		inline auto to_string(const index_pair& pair)
		{
			return static_cast<const std::ostringstream&>(std::ostringstream{} << "( x = " << pair.first << ", y = " << pair.second << " )").str();
		}
	}

	void PathFinder::run(const Map& map, const Area& area, boost::optional<LocationOrientationPair> locationResult)
	{
		if (area.height == 0 || area.width == 0)
		{
			return;
		}

		PPC_LOG(info) << "Path finding for area: " << area;

		auto isPassable = ZoneValidator{ map, area };

		//2D matrix of pair of { weight, turn }
		std::vector<TableRow> table{ area.height,{ area.width, std::make_pair(0, 0) } };
		for (auto x = 0u; x < area.width; ++x)
		{
			table[area.height - 1][x].first = isPassable(x, area.height - 1) ? 0 : -1;
		}

		for (auto y = static_cast<int>(area.height - 2); y >= 0; --y)
		{
			for (auto x = 0u; x < area.width; ++x)
			{
				auto& row = table[y];
				const auto& prev = table[y + 1];

				if (!isPassable(x, y))
				{
					row[x].first = -1;
					continue;
				}

				auto minDir = std::min({ 0, -1, 1 }, MinOffsetCompare{map, area, prev, x});

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
		PPC_LOG(info) << "Pathing table for the area computed.";

		

		index_type startX;
		auto currentOrientation = BACKWARDS;
		if (locationResult)
		{
			startX = locationResult->first.first;
			currentOrientation = locationResult->second;
		}
		else
		{
			m_workers.recv(mpi::any_source, mpi::any_tag, startX);
			m_workers.recv(mpi::any_source, mpi::any_tag, currentOrientation);
		}

		PPC_LOG(info) << "Starting location: " << to_string(index_pair{ startX, area.y - 1 });
		PPC_LOG(info) << "Searching for an entering location...";
		auto auxiliaryRow = create_auxiliary_row(isPassable, area.width, startX);
		MinOffsetCompare compare{ map, area, table[0], 0 };
		auto entranceX = find_entrance_point(auxiliaryRow, table[0], compare);
		PPC_LOG(info) << "Entrance found: " << to_string(index_pair{ entranceX, area.y });

		//Move to entrance
		auto currentX = startX;
		auto offsetX = currentX < entranceX? 1 : -1;
		PPC_LOG(info) << "Moving to entrance. Starting pos = " << index_pair{ startX, area.y - 1 };
		for (; currentX != entranceX; currentX += offsetX)
		{
			currentOrientation = moveTo(currentOrientation, { offsetX, 0 });
			PPC_LOG(info) << "Current pos = " << index_pair{ currentX + offsetX, area.y - 1 };
		}

		PPC_LOG(info) << "Moving down to entrace";
		currentOrientation = moveTo(currentOrientation, { 0, 1 });

		PPC_LOG(info) << "Moving down. Starting pos = " << index_pair{ currentX, area.y };
		for (auto i = 0u; i < area.height - 1; ++i)
		{
			auto xOffset = table[i][currentX].second;
			currentX += xOffset;
			currentOrientation = moveTo(currentOrientation, { xOffset, 1 });
			PPC_LOG(info) << "Current pos = " << index_pair{ currentX, area.y + i + 1};
		}


		if (m_workerID != m_numOfWorkers - 1)
		{
			PPC_LOG(info) << "Sending current x and current orientation forward...";
			m_workers.send(m_workerID + 1, 0, currentX);
			m_workers.send(m_workerID + 1, 0, currentOrientation);
		}
	}

	namespace
	{
		constexpr auto ensure_direction(Direction currentOrientation, Direction moveDir)
		{
			return static_cast<Direction>(((moveDir + 4) - currentOrientation) % 4);
		}
	}

	Direction PathFinder::moveTo(Direction currentOrientation, const std::pair<int, int>& offsets)
	{
		const auto offsetX = offsets.first;
		const auto offsetY = offsets.second;
		
		if(offsetX != 0)
		{
			auto moveDir = get_direction({ offsetX, 0 });
			auto realDir = ensure_direction(currentOrientation, moveDir);
			m_orientee.send(1, tags::MOVE, realDir);
			currentOrientation = moveDir;
		}
		 
		if (offsetY != 0)
		{
			auto moveDir = BACKWARDS;
			auto realDir = ensure_direction(currentOrientation, moveDir);
			m_orientee.send(1, tags::MOVE, realDir);
			currentOrientation = moveDir;
		}

		return currentOrientation;
	}
}