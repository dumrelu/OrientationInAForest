#include <iostream>
#include <sstream>
#include <string>

#include "forest/map.hpp"

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
	std::cout << map;
	std::cin.get();

	/*mpi::environment environment;
	mpi::communicator world;

	std::cout << world.rank() << " / " << world.size() << std::endl;
	std::cin.get();*/
}