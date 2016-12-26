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
	//http://www.boost.org/doc/libs/1_54_0/libs/log/doc/html/boost/log/add_console_lo_idp21543664.html
	namespace logging = boost::log;
	namespace keywords = logging::keywords;
	logging::add_console_log(std::cout,
		keywords::format = "[%TimeStamp%]: %Message%",
		keywords::auto_flush = true
	);
	logging::add_common_attributes();
	/*logging::core::get()->set_filter
	(
		logging::trivial::severity == logging::trivial::trace
	);*/

	const std::string testMap{
		"6 7\n"
		"CCCCCCC\n"
		"CCOROCC\n"
		"CCORTCC\n"
		"CCOROCC\n"
		"CCOROCC\n"
		"CCCCCCC\n"
	};
	std::istringstream iss{ testMap };

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;

	ppc::g_worldID = "World#" + std::to_string(world.rank()) + ": ";
	PPC_LOG(info) << "World initialized(" << world.size() << " processes)." << std::endl;
	//assert(world.size() >= 4);

	ppc::Map map;
	PPC_LOG(info) << "Reading map...";
	iss >> map;
	PPC_LOG(info) << "Map read(height = " << map.height() << ", width = " << map.width() << ")";

	auto workers = world.split(world.rank() != 1);
	PPC_LOG(info) << "Workers communicator initialized.";

	auto orienteeComm = world.split(world.rank() <= 1);
	PPC_LOG(info) << "Orientee communicator initialized.";

	world.barrier();

	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workers, orienteeComm };
		auto location = locationMaster.run(map);
	}
	else if (world.rank() == 1)
	{
		ppc::Orientee orientee{ orienteeComm };
		orientee.run(map);
	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workers };
		auto location = locationWorker.run(map);
	}

	PPC_LOG(info) << "Shutting down..." << std::endl;
	
}