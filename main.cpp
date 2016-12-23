#include <iostream>
#include <sstream>
#include <string>

#include "forest/map.hpp"
#include "forest/area.hpp"


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
	
	ppc::Map map;

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

	std::cin.get();

	

	/*mpi::environment environment;
	mpi::communicator world;

	std::cout << world.rank() << " / " << world.size() << std::endl;
	std::cin.get();*/
}