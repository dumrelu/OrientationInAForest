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

void init_logger();
bool parse_args(int argc, char* argv[], ppc::Map& map, boost::optional<ppc::index_pair>& startingPosition, boost::optional<ppc::Direction>& startingDirection);
bool init_mpi(ppc::mpi::environment& environment, ppc::mpi::communicator& world, ppc::mpi::communicator& workersComm, ppc::mpi::communicator& orienteeComm);
void write_path(const ppc::path& path, const std::string& filename);

int main(int argc, char *argv[])
{
	init_logger();

	ppc::Map map;
	boost::optional<ppc::index_pair> startingPosition;
	boost::optional<ppc::Direction> startingDirection;

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;
	ppc::mpi::communicator workersComm;
	ppc::mpi::communicator orienteeComm;
	if (!init_mpi(environment, world, workersComm, orienteeComm))
	{
		PPC_LOG(info) << "Program is shutting down...";
		return 1;
	}

	if (!parse_args(argc, argv, map, startingPosition, startingDirection))
	{
		PPC_LOG(info) << "Program is shutting down...";
		return 1;
	}

	//Find the current location.
	ppc::LocationOrientationPair locationSolution;
	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workersComm, orienteeComm };
		locationSolution = locationMaster.run(map);
	}
	else if (world.rank() == 1)
	{
		ppc::Orientee orientee{ orienteeComm };
		ppc::path path = orientee.run(map, startingPosition, startingDirection);

		write_path(path, "path.path");
	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workersComm };
		locationSolution = locationWorker.run(map);
	}

	if (world.rank() == 0)
	{
		orienteeComm.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
	}

	PPC_LOG(info) << "Shutting down...";
}

void init_logger()
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
}

bool parse_args(int argc, char* argv[], ppc::Map& map, boost::optional<ppc::index_pair>& startingPosition, boost::optional<ppc::Direction>& startingDirection)
{
	if (argc < 2)
	{
		PPC_LOG(fatal) << "The program required the map filename as its first argument.";
		return false;
	}

	std::ifstream inputFile{ argv[1] };
	if (inputFile)
	{
		PPC_LOG(info) << "Reading the map...";
		inputFile >> map;
		PPC_LOG(info) << "Map successfully read. Map height: " << map.height() << ", Map width: " << map.width() << ".";
	}
	else
	{
		PPC_LOG(fatal) << "Invalid map file(" << argv[1] << ").";
		return false;
	}

	if (argc >= 4)
	{
		ppc::index_pair pos{ std::stoi(argv[2]), std::stoi(argv[3]) };
		startingPosition = pos;

		PPC_LOG(info) << "Starting position overriden: { x = " << pos.first << ", y = " << pos.second << " }.";
	}

	if (argc >= 5)
	{
		int asInteger = std::stoi(argv[4]);
		if (asInteger >= 0 && asInteger <= 3)
		{
			startingDirection = static_cast<ppc::Direction>(asInteger);
			PPC_LOG(info) << "Starting direction overriden: " << *startingDirection;
		}
	}

	return true;
}

bool init_mpi(ppc::mpi::environment& environment, ppc::mpi::communicator& world, ppc::mpi::communicator& workersComm, ppc::mpi::communicator& orienteeComm)
{
	PPC_LOG(info) << "Initializing mpi...";

	ppc::g_worldID = "World#" + std::to_string(world.rank()) + ": ";
	PPC_LOG(info) << "World initialized! World size: " << world.size();
	if (world.size() < 3)
	{
		PPC_LOG(fatal) << "The program requires at least 3 processes.";
		return false;
	}

	workersComm = world.split(world.rank() != 1);
	PPC_LOG(info) << "Workers communicator initialized!";

	orienteeComm = world.split(world.rank() <= 1);
	PPC_LOG(info) << "Orientee communicator initialized.";

	world.barrier();
	PPC_LOG(info) << "Program starting!";

	return true;
}

void write_path(const ppc::path& path, const std::string& filename)
{
	std::ofstream pathFile{ filename };
	if (!pathFile)
	{
		PPC_LOG(fatal) << "Cannot write path file(" << filename << ")";
		return;
	}

	for (const auto& point : path)
	{
		pathFile << point.first << " " << point.second << "\n";
	}
}