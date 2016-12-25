#include <iostream>
#include <sstream>
#include <string>

#include "forest/map.hpp"
#include "forest/area.hpp"
#include "forest/pattern.hpp"

#include "common.hpp"
#include "location_finder_master.hpp"
#include "location_finder_worker.hpp"
#include "orientee.hpp"

std::string ppc::g_worldID;

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

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;

	ppc::g_worldID = "World#" + std::to_string(world.rank()) + ": ";
	PPC_LOG(info) << "World initialized";

	ppc::Map map;
	PPC_LOG(info) << "Reading map...";
	iss >> map;
	PPC_LOG(info) << "Map read(height = " << map.height() << ", width = " << map.width() << ")";

	auto workers = world.split(world.rank() != 1);
	auto orientee = world.split(world.rank() <= 1);

	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workers, orientee };
		auto location = locationMaster.run(map);
	}
	else if (world.rank() == 1)
	{

	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workers };
		auto location = locationWorker.run(map);
	}
}