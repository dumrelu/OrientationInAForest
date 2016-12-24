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
		"5 5\n"
		"COROC\n"
		"CTRTC\n"
		"COROC\n"
		"COROC\n"
		"CCCCC\n"
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

	if (world.rank() == 0)
	{
		auto master = ppc::Master{ world, world };
		ppc::Map map;
		iss >> map;

		master.run(map);
	}
	else
	{
		ppc::AreaZones zone;
		ppc::mpi::scatter(world, zone, 0);

		auto tempMap = ppc::create_map(zone);
		BOOST_LOG_TRIVIAL(debug) << "Worker #" << world.rank() << std::endl << tempMap;
	}
}