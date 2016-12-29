#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <fstream>
#include <thread>

#include "forest/map.hpp"
#include "forest/area.hpp"
#include "forest/pattern.hpp"

#include "common.hpp"
#include "location_finder_master.hpp"
#include "location_finder_worker.hpp"
#include "orientee.hpp"

std::string ppc::g_worldID;

int main(int argc, char *argv[])
{
	//http://www.boost.org/doc/libs/1_54_0/libs/log/doc/html/boost/log/add_console_lo_idp21543664.html
	namespace logging = boost::log;
	namespace keywords = logging::keywords;
	namespace expr = logging::expressions;
	namespace sinks = logging::sinks;

	auto format = expr::stream << "["
		<< expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		<< "] <" << logging::trivial::severity
		<< "> - " << expr::smessage;

	//TODO: stop using g_worldID and insert it here
	logging::add_console_log(
		std::cout,
		keywords::format = format,
		keywords::auto_flush = true
	);
	logging::add_file_log(
		keywords::file_name = "sample_%N.log",
		keywords::rotation_size = 10 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::format = format
	);
	logging::add_common_attributes();

#ifdef NDEBUG
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::info
	);
#endif

	if (argc < 2)
	{
		PPC_LOG(fatal) << "The program requires an input map as its first argument.";
		return 1;
	}

	std::ifstream inputFile{ argv[1] };
	if (!inputFile)
	{
		PPC_LOG(fatal) << "Invalid map filename(" << argv[1] << ").";
		return 1;
	}

	PPC_LOG(info) << "Reading the map...";
	ppc::Map map;
	inputFile >> map;

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;

	ppc::g_worldID = "World#" + std::to_string(world.rank()) + ": ";
	PPC_LOG(info) << "World initialized(" << world.size() << " processes).";
	assert(world.size() >= 3);

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
	}
	else if (world.rank() == 1)
	{
		ppc::Orientee orientee{ orienteeComm };
		ppc::path path = orientee.run(map);

		std::ofstream pathFile{ "path.path" };
		for (const auto& point : path)
		{
			pathFile << point.first << " " << point.second << "\n";
		}
	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workers };
		auto location = locationWorker.run(map);
	}

	PPC_LOG(info) << "Shutting down...";
	
}