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
	namespace expr = logging::expressions;
	//TODO: stop using g_worldID and insert it here
	logging::add_console_log(std::cout,
		keywords::format = (
			expr::stream << "["
			<< expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
			<< "] : <" << logging::trivial::severity
			<< "> " << expr::smessage
		),
		keywords::auto_flush = true
	);
	logging::add_common_attributes();
	/*logging::core::get()->set_filter
	(
		logging::trivial::severity == logging::trivial::trace
	);*/

	const std::string testMap{
		"20 20\n"
		"CCCCCCCCCCCCCCCCCCCC\n"
		"CCCCCCCCCRCCCCCCCCCC\n"
		"CCCTCCCCCRCCCCCCCCCC\n"
		"CCCCCCCCCRCCCCCCCTCC\n"
		"CCCCCCCCCRCCCCCCCCCC\n"
		"CCCCCCCCCRCCCTOCCCCC\n"
		"CCTOCCCCCRCCCCRRCCCC\n"
		"CCCCCCCCRCRCCCCCCCCC\n"
		"CCCCCCCRCCCRCCCCCCCC\n"
		"CCCCCCRCCCCCRCCCCCCC\n"
		"CCCCCRCCCCCCCRCCCCCC\n"
		"CCCCRCCCCCCCCCRCCCCC\n"
		"CCCCCRCCCCCTCCCRCCCC\n"
		"CCCCCCRCCCCCCCCCRCCC\n"
		"CCCCCCRCCCCCCCCCCRCC\n"
		"CCTCCCRCCCCCCCCCCRCC\n"
		"CCCCCCRRCCCTCCCCOOOC\n"
		"CCCCCCRCCCCCCCCCOOOC\n"
		"CCCCCCRCCCTOCCCCOOOC\n"
		"RRRRRRRRRRRRRRRRRRRR\n"
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
		orienteeComm.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
		ppc::mpi::broadcast(workers, ppc::dummy<ppc::Pattern>, 0);
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