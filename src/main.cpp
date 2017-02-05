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

#include <boost/program_options.hpp>

std::string ppc::g_worldID;

void init_logger();
bool parse_args(int argc, char* argv[], ppc::Map& map, boost::optional<ppc::index_pair>& startingPosition, boost::optional<ppc::Direction>& startingDirection, bool& randomize, bool& pathFinding, const bool log);
bool init_mpi(ppc::mpi::communicator& world, ppc::mpi::communicator& workersComm, ppc::mpi::communicator& orienteeComm);
void write_path(const ppc::path& path, const std::string& filename);

int main(int argc, char *argv[])
{
	init_logger();

	ppc::Map map;
	boost::optional<ppc::index_pair> startingPosition;
	boost::optional<ppc::Direction> startingDirection;
	bool randomize = false;
	bool pathFiding = false;

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;
	ppc::mpi::communicator workersComm;
	ppc::mpi::communicator orienteeComm;
	if (!init_mpi(world, workersComm, orienteeComm))
	{
		PPC_LOG(info) << "Program is shutting down...";
		return 1;
	}

	try
	{
		if (!parse_args(argc, argv, map, startingPosition, startingDirection, randomize, pathFiding, world.rank() == 0))
		{
			PPC_LOG(info) << "Program is shutting down...";
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		PPC_LOG(fatal) << "Parsing error: " << e.what();
		return 1;
	}

	//Find the current location.
	ppc::LocationOrientationPair locationSolution;
	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workersComm, orienteeComm };
		locationSolution = locationMaster.run(map, randomize);

		orienteeComm.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
		orienteeComm.send(1, 0, locationSolution);
	}
	else if (world.rank() == 1)
	{
		ppc::Orientee orientee{ orienteeComm };
		ppc::path path = orientee.run(map, startingPosition, startingDirection);
		orienteeComm.recv(ppc::mpi::any_source, ppc::mpi::any_tag, locationSolution);

		write_path(path, "location_finding.path");
	}
	else
	{
		ppc::LocationFinderWorker locationWorker{ workersComm };
		locationSolution = locationWorker.run(map);
	}

	const auto location = locationSolution.first;
	const auto orientation = locationSolution.second;
	if (world.rank() != 1 && location.first == 0 && location.second == 0)
	{
		return 1;
	}

	if (!pathFiding)
	{
		PPC_LOG(info) << "Shutting down...";
		return 0;
	}

	//Path finding
	world.barrier();
	const auto startingY = location.second + 1;
	const auto requiredY = map.height() - 3;	//-2 for the border, - 1 for 0 indexing

	if (world.rank() == 1)
	{
		//Recreate the orientee to use the world communicator and to start from
		//the previous state.
		ppc::Orientee orientee{ world };
		auto path = orientee.run(map, location, orientation);

		write_path(path, "path_finding.path");
	}

	if (world.rank() != 1 && location.second < requiredY)
	{
		const auto startX = 1;	//+1 for the border
		const auto width = map.width() - 2;	//+2 for the border
		const auto numOfWorkers = static_cast<ppc::index_type>(workersComm.size());
		const auto workerID = static_cast<ppc::index_type>(workersComm.rank());
		PPC_LOG(info) << "Starting pathfinding with " << numOfWorkers << " workers";

		const auto areas = ppc::split({ startX, startingY, requiredY - startingY + 1, width }, numOfWorkers);
		
		ppc::PathFinder pathFinder{ workersComm, world, workerID, numOfWorkers };
		if (workerID == 0u)
		{
			pathFinder.run(map, areas[workerID], locationSolution);
		}
		else
		{
			pathFinder.run(map, areas[workerID]);
		}
	}

	if (world.rank() == world.size() - 1)
	{
		world.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
	}

	PPC_LOG(info) << "Shutting down...";
	return 0;
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
		keywords::rotation_size = 100 * 1024 * 1024,
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

bool parse_args(int argc, char* argv[], ppc::Map& map, boost::optional<ppc::index_pair>& startingPosition, boost::optional<ppc::Direction>& startingDirection, bool& randomize, bool& pathFinding, const bool log)
{
	namespace po = boost::program_options;

	po::options_description desc{ "Orientation In A Forst program options" };
	desc.add_options()
		("help,h", "Prints instructions on how to use the program")
		("map,m", po::value<std::string>(), "The name of the file containing the map")
		("pathfinding,p", "After finding the location, find the way to the bottom side of the map")
		("startingX,x", po::value<ppc::index_type>(), "Starting x position of the orientee")
		("startingY,y", po::value<ppc::index_type>(), "Starting y position of the orientee")
		("direction,d", po::value<int>(), "Starting orientation of the orientee(FORWARD=0, RIGHT=1, BACKWARDS=2, LEFT=3)")
		("random,r", "Indicates that the algorithm is allowed to use randomization in some areas, to guarantee a solution(if it exists)");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") && log)
	{
		std::cout << desc << std::endl;
		return false;
	}

	if (vm.count("map"))
	{
		auto mapFilename = vm["map"].as<std::string>();
		std::ifstream inputFile{ mapFilename };
		if (inputFile)
		{
			if (log)
			{
				PPC_LOG(info) << "Reading the map...";
			}
			inputFile >> map;

			if (log)
			{
				PPC_LOG(info) << "Map successfully read. Map height: " << map.height() << ", Map width: " << map.width() << ".";
			}
		}
		else
		{
			if (log)
			{
				PPC_LOG(fatal) << "Invalid map file(" << mapFilename << ").";
			}
			return false;
		}
	}
	else
	{
		if (log)
		{
			PPC_LOG(fatal) << "Map filename is required! Use --help for more information.";
		}
		return false;
	}

	if (vm.count("startingX") ^ vm.count("startingY"))
	{
		PPC_LOG(fatal) << "You have to specify both x and y if you want to set the starting position.";
		return false;
	}

	if (vm.count("startingX"))
	{
		const auto x = vm["startingX"].as<ppc::index_type>();
		const auto y = vm["startingY"].as<ppc::index_type>();

		ppc::index_pair pos{ x, y };
		startingPosition = pos;
		if (log)
		{
			PPC_LOG(info) << "Starting position overriden: " << pos << ".";
		}
	}

	if (vm.count("direction"))
	{
		const auto asInteger = vm["direction"].as<int>();
		if (asInteger >= 0 && asInteger <= 3)
		{
			startingDirection = static_cast<ppc::Direction>(asInteger);
			if (log)
			{
				PPC_LOG(info) << "Starting direction overriden: " << *startingDirection;
			}
		}
		else
		{
			PPC_LOG(fatal) << "Invalid direction";
			return false;
		}
	}

	if (vm.count("random"))
	{
		randomize = true;
		if (log)
		{
			PPC_LOG(info) << "Randomization is on";
		}
	}
	else
	{
		randomize = false;
		if (log)
		{
			PPC_LOG(info) << "Randomization is off";
		}
	}

	if (vm.count("pathfinding"))
	{
		pathFinding = true;
		if (log)
		{
			PPC_LOG(info) << "Pathfinding is on";
		}
	}
	else
	{
		pathFinding = false;
		if (log)
		{
			PPC_LOG(info) << "Pathfinding if off";
		}
	}

	return true;
}

bool init_mpi(ppc::mpi::communicator& world, ppc::mpi::communicator& workersComm, ppc::mpi::communicator& orienteeComm)
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