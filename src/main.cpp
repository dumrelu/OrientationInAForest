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
#include "path_finder.hpp"

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

	boost::optional<ppc::index_pair> startingPos;
	if (argc >= 4)
	{
		ppc::index_pair pos{};
		pos.first = std::stoi(argv[2]);
		pos.second = std::stoi(argv[3]);

		startingPos = pos;
	}

	boost::optional<ppc::Direction> startingDirection;
	if (argc >= 5)
	{
		int asInteger = std::stoi(argv[4]);
		if (asInteger >= 0 && asInteger <= 3)
		{
			startingDirection = static_cast<ppc::Direction>(asInteger);
		}
	}

	PPC_LOG(info) << "Reading the map...";
	ppc::Map map;
	inputFile >> map;
	PPC_LOG(info) << "Map read(height=" << map.height() << ", width=" << map.width() << ").";

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

	ppc::LocationOrientationPair solution;
	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workers, orienteeComm };
		solution = locationMaster.run(map);
	}
	else if (world.rank() == 1)
	{
		ppc::Orientee orientee{ orienteeComm };
		ppc::path path = orientee.run(map, startingPos, startingDirection);

		std::ofstream pathFile{ "path.path" };
		for (const auto& point : path)
		{
			pathFile << point.first << " " << point.second << "\n";
		}
	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workers };
		solution = locationWorker.run(map);
	}

	//location = { 3, 1 };
	if (world.rank() == 0 && solution.first.second < map.height() - 2)	//TODO: (world.rank() != 1
	{
		const auto areas = ppc::split({ 1, solution.first.second + 1, map.height() - 2 - solution.first.second - 1, map.width() - 2 }, 1);	//TODO: world.size() - 1
		const auto workerID = static_cast<ppc::index_type>(world.rank() == 0 ? 0 : world.rank() - 1);
		const auto myArea = areas[workerID];
		const auto numOfWorkers = static_cast<ppc::index_type>(std::count_if(areas.cbegin(), areas.cend(), [](const ppc::Area& area) { return area.height != 0; }));

		ppc::PathFinder pathFinder{ workers, orienteeComm, workerID, numOfWorkers };
		if (world.rank() == 0)
		{
			pathFinder.run(map, myArea, solution );
		}
		else
		{
			pathFinder.run(map, myArea);
		}
	}

	if (world.rank() == 0)
	{
		orienteeComm.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
	}

	PPC_LOG(info) << "Shutting down...";
	
}