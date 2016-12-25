#include <iostream>
#include <sstream>
#include <string>

#include "forest/map.hpp"
#include "forest/area.hpp"
#include "forest/pattern.hpp"

#include "master.hpp"


int main()
{
	const std::string testMap{
		"6 7\n"
		"CCCCCCC\n"
		"CCOROCC\n"
		"CCTRTCC\n"
		"CCOROCC\n"
		"CCOROCC\n"
		"CCCCCCC\n"
	};
	std::istringstream iss{ testMap };
	
	/*ppc::Map map;

	iss >> map;
	std::cout << "Original map: " << std::endl << map;

	auto zones = *ppc::extract_zones(map, { 1, 1, 3, 3 });
	auto partialMap = ppc::create_map(zones);
	std::cout << "Partial map: " << std::endl << partialMap;

	partialMap[1][0] = ppc::ROAD;
	auto partialZones = *ppc::extract_zones(partialMap, { 0, 0, 3, 3 });
	partialZones.area.x = 1;
	partialZones.area.y = 1;
	ppc::update_map(map, partialZones);

	std::cout << "Updated map: " << std::endl << map;

	ppc::Pattern pattern{ 3, 3, { ppc::UNKNOWN, ppc::ROAD, ppc::UNKNOWN, ppc::UNKNOWN, ppc::ROAD, ppc::UNKNOWN, ppc::UNKNOWN, ppc::ROAD, ppc::UNKNOWN } };
	auto matches = ppc::match_pattern(map, pattern);


	BOOST_LOG_TRIVIAL(trace) << "Hello World!";
	std::cin.get();*/

	

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;

	BOOST_LOG_TRIVIAL(debug) << "Before split";
	auto workers = world.split(world.rank() != 1);
	auto outside = world.split(world.rank() <= 1);
	BOOST_LOG_TRIVIAL(debug) << "After split";

	if (world.rank() == 0)
	{
		auto master = ppc::Master{ workers, outside };
		//auto master = ppc::Master{ world, world };
		ppc::Map map;
		iss >> map;

		BOOST_LOG_TRIVIAL(debug) << "Workers communicator: " << workers.rank() << "/" << workers.size();
		BOOST_LOG_TRIVIAL(debug) << "Outside communicator: " << outside.rank() << "/" << outside.size();

		master.run(map);
	}
	else if (world.rank() == 1)
	{
		ppc::Map map;
		iss >> map;

		auto position = ppc::index_pair{ 3, 3 };
		auto orientation = ppc::LEFT;
		auto runQuery = [&]()
		{
			std::array<ppc::ZoneType, 4> queryResult;
			for (const auto dir : { ppc::UP, ppc::RIGHT, ppc::DOWN, ppc::LEFT })
			{
				const auto offset = ppc::get_offset(ppc::combine_directions(orientation, dir));
				auto dirPos = position;
				dirPos.first += offset.first;
				dirPos.second += offset.second;

				queryResult[dir] = map[dirPos.second][dirPos.first];
			}
			return queryResult;
		};
	}
	else
	{
		ppc::AreaZones zone;
		ppc::mpi::scatter(workers, zone, 0);

		auto tempMap = ppc::create_map(zone);
		BOOST_LOG_TRIVIAL(debug) << "Worker #" << workers.rank() << std::endl << tempMap;
	}
}