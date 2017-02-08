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

void init_logger(const int rank);
bool parse_args(int argc, char* argv[], ppc::Map& map, 
	boost::optional<ppc::index_pair>& startingPosition, boost::optional<ppc::Direction>& startingDirection,
	bool& randomize, bool& pathFinding, float& splitFactor, bool& statistics, 
	ppc::PathFinder::EntranceSearching& searchMethod, int& numOfSearchLocations,
	const bool log);
bool init_mpi(ppc::mpi::communicator& world, ppc::mpi::communicator& workersComm, ppc::mpi::communicator& orienteeComm);
void write_path(const ppc::path& path, const std::string& filename);

int main(int argc, char *argv[])
{
	const auto startTime = std::chrono::steady_clock::now();

	ppc::Map map;
	boost::optional<ppc::index_pair> startingPosition;
	boost::optional<ppc::Direction> startingDirection;
	bool randomize = false;
	bool pathFiding = false;
	float splitFactor = 0.0f;
	bool statistics = true;
	ppc::PathFinder::EntranceSearching searchMethod = ppc::PathFinder::N_SEARCH;
	int numOfSearchLocations = 5;

	ppc::mpi::environment environment;
	ppc::mpi::communicator world;
	ppc::mpi::communicator workersComm;
	ppc::mpi::communicator orienteeComm;

	init_logger(world.rank());

	if (!init_mpi(world, workersComm, orienteeComm))
	{
		PPC_LOG(info) << "Program is shutting down...";
		return 1;
	}

	try
	{
		if (!parse_args(argc, argv, map, 
			startingPosition, startingDirection, 
			randomize, pathFiding, splitFactor, statistics, 
			searchMethod, numOfSearchLocations, 
			world.rank() == 0))
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

	//Statistics initializations
	boost::optional<ppc::Statistics> stats;
	if (statistics)
	{
		stats = ppc::Statistics{};
	}

	//Find the current location.
	ppc::LocationOrientationPair locationSolution;
	if (world.rank() == 0)
	{
		ppc::LocationFinderMaster locationMaster{ workersComm, orienteeComm };
		locationSolution = locationMaster.run(map, randomize, stats);

		if (statistics)
		{
			const auto locationFindingEndTime = std::chrono::steady_clock::now();
			stats->locationFindingTime = std::chrono::duration_cast<std::chrono::milliseconds>(locationFindingEndTime - startTime);

			orienteeComm.send(1, ppc::tags::STATS, ppc::dummy<ppc::Direction>);
			orienteeComm.recv(ppc::mpi::any_source, ppc::mpi::any_tag, stats->startingLocation);
			orienteeComm.recv(ppc::mpi::any_source, ppc::mpi::any_tag, stats->startingOrientation);
			orienteeComm.recv(ppc::mpi::any_source, ppc::mpi::any_tag, stats->totalNumberOfMoves);
			orienteeComm.recv(ppc::mpi::any_source, ppc::mpi::any_tag, stats->totalNumberOfQueries);
		}

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
	const auto pathFindingStartTime = std::chrono::steady_clock::now();
	const auto startingY = location.second + 1;
	const auto requiredY = map.height() - 3;	//-2 for the border, - 1 for 0 indexing
	const auto pathFindingRows = requiredY - startingY + 2;	//TODO: come up with a better name

	if (world.rank() == 1)
	{
		//Recreate the orientee to use the world communicator and to start from
		//the previous state.
		ppc::Orientee orientee{ world };
		auto path = orientee.run(map, location, orientation);

		write_path(path, "path_finding.path");
	}

	ppc::index_type numOfPathfindingWorkers = 0;
	if (world.rank() != 1 && location.second < requiredY)
	{
		const auto startX = 0;
		const auto width = map.width() - 2;	//+2 for the border
		const auto numOfWorkers = static_cast<ppc::index_type>(workersComm.size());
		const auto workerID = static_cast<ppc::index_type>(workersComm.rank());

		ppc::Area mainArea{ startX, startingY - 1, pathFindingRows, width };
		
		if (mainArea.height > 3 * numOfWorkers)
		{
			PPC_LOG(info) << "Starting pathfinding with " << numOfWorkers << " workers";
			numOfPathfindingWorkers = numOfWorkers;
			std::vector<ppc::Area> areas;
			if (splitFactor != 0.0f)
			{
				areas = ppc::factor_split(mainArea, numOfWorkers, splitFactor);
			}
			else
			{
				areas = ppc::split(mainArea, numOfWorkers);
			}

			ppc::PathFinder pathFinder{ workersComm, world, workerID, numOfWorkers };
			if (workerID == 0u)
			{
				pathFinder.run(map, areas[workerID], locationSolution, searchMethod, numOfSearchLocations);
			}
			else
			{
				pathFinder.run(map, areas[workerID], {}, searchMethod, numOfSearchLocations);
			}
		}
		else if(world.rank() == 0)
		{
			PPC_LOG(info) << "Path finding area too small. Will only use 1 worker.";
			numOfPathfindingWorkers = 1;
			ppc::PathFinder pathFinder{ workersComm, world, workerID, 1 };
			pathFinder.run(map, mainArea, locationSolution, searchMethod, numOfSearchLocations);
		}
	}
	else if (world.rank() == 0)
	{
		PPC_LOG(info) << "No need for path finding.";
		world.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
	}

	const bool isLastWorker = (numOfPathfindingWorkers > 1 && world.rank() == world.size() - 1) 
		|| (numOfPathfindingWorkers == 1 && world.rank() == 0);
	if (!statistics && isLastWorker)
	{
		world.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);
	}

	if (statistics)
	{
		workersComm.barrier();

		if (world.rank() == 0)
		{
			PPC_LOG(info) << "Gathering statistical data...";

			//Run time stats.
			const auto endTime = std::chrono::steady_clock::now();
			stats->totalRunTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

			//Num of moves + queries stats
			ppc::index_type additionalNumOfMoves;
			ppc::index_type additionalNumOfQueries;

			world.send(1, ppc::tags::STATS, ppc::dummy<ppc::Direction>);
			world.recv(ppc::mpi::any_source, ppc::mpi::any_tag, ppc::dummy<ppc::index_pair>);
			world.recv(ppc::mpi::any_source, ppc::mpi::any_tag, ppc::dummy<ppc::index_pair>);
			world.recv(ppc::mpi::any_source, ppc::mpi::any_tag, additionalNumOfMoves);
			world.recv(ppc::mpi::any_source, ppc::mpi::any_tag, additionalNumOfQueries);

			stats->totalNumberOfMoves += additionalNumOfMoves;
			stats->totalNumberOfQueries += additionalNumOfQueries;

			world.send(1, ppc::tags::STOP, ppc::dummy<ppc::Direction>);

			//Path finding stats
			stats->pathFinding = pathFiding;
			if (pathFiding)
			{
				const auto pathFindingEndTime = std::chrono::steady_clock::now();

				stats->numOfPathFindingMoves = additionalNumOfMoves;
				stats->pathFindingTime = std::chrono::duration_cast<std::chrono::milliseconds>(pathFindingEndTime - pathFindingStartTime);
			}

			//Other stats
			stats->numOfProcessors = world.size();
			stats->mapHeight = map.height();
			stats->mapWidth = map.width();
			for (auto i = 1; i < argc; ++i)
			{
				stats->startupOptions += argv[i];
				stats->startupOptions += " ";
			}

			std::ofstream statsFile{ "statistics.txt" };
			statsFile << *stats;
		}
	}

	PPC_LOG(info) << "Shutting down...";
	return 0;
}

void init_logger(const int rank)
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
		keywords::file_name = "process_" + std::to_string(rank) + "_log.txt",
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

bool parse_args(int argc, char* argv[], ppc::Map& map, boost::optional<ppc::index_pair>& startingPosition, 
	boost::optional<ppc::Direction>& startingDirection, 
	bool& randomize, bool& pathFinding, float& splitFactor, bool& statistics, 
	ppc::PathFinder::EntranceSearching& searchMethod, int& numOfSearchLocations,
	const bool log)
{
	namespace po = boost::program_options;

	po::options_description desc{ "Orientation In A Forest program options" };
	desc.add_options()
		("help,h", "Prints instructions on how to use the program")
		("map,m", po::value<std::string>(), "The name of the file containing the map. This is a required argument")
		("path_finding,p", "After finding the location, find the way to the bottom side of the map")
		("statistics,s", "Gathers statistics and benchmarks")
		("startingX,x", po::value<ppc::index_type>(), "Starting x position of the orientee")
		("startingY,y", po::value<ppc::index_type>(), "Starting y position of the orientee")
		("direction,d", po::value<int>(), "Starting orientation of the orientee(FORWARD=0, RIGHT=1, BACKWARDS=2, LEFT=3)")
		("random,r", "Indicates that the algorithm is allowed to use randomization in some areas, to guarantee a solution(if it exists)")
		("split_factor,f", po::value<float>(), "Factor used to when splitting the area for path finding(has to be 0 < factor < 1)")
		("entry_method,e", po::value<int>(), "0 = Any entrance, 1 = Min table cost entrance(Default value), 2 = Search n locations")
		("n_locations,n", po::value<int>(), "Number of entrances to be searched when using -e 2");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		if (log)
		{
			std::cout << desc << std::endl;
		}
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
			if (log)
			{
				PPC_LOG(fatal) << "Invalid direction";
			}
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
	}

	if (vm.count("path_finding"))
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
	}

	if (vm.count("split_factor"))
	{
		const auto factor = vm["split_factor"].as<float>();
		if (factor <= 0 || factor >= 1)
		{
			if (log)
			{
				PPC_LOG(fatal) << "Invalid split factor! Use --help for more information.";
			}
			return false;
		}
		splitFactor = factor;
		if (log)
		{
			PPC_LOG(info) << "Split factor used: " << splitFactor;
		}
	}
	else
	{
		splitFactor = 0.0f;
	}

	if (vm.count("statistics"))
	{
		statistics = true;
		if (log)
		{
			PPC_LOG(info) << "Statistics are on";
		}
	}
	else
	{
		statistics = false;
	}

	if (vm.count("entry_method"))
	{
		const auto asInteger = vm["entry_method"].as<int>();
		if (asInteger >= 0 && asInteger <= 2)
		{
			searchMethod = static_cast<ppc::PathFinder::EntranceSearching>(asInteger);
			if (log)
			{
				PPC_LOG(info) << "Entry method: " << searchMethod;
			}
		}
		else
		{
			if (log)
			{
				PPC_LOG(fatal) << "Invalid entrance method";
			}
			return false;
		}
	}
	else
	{
		searchMethod = ppc::PathFinder::MIN_ENTRANCE;
	}

	if (vm.count("n_locations"))
	{
		numOfSearchLocations = vm["n_locations"].as<int>();
		if (numOfSearchLocations <= 0)
		{
			if (log)
			{
				PPC_LOG(fatal) << "n_locations taks as input a positive integer > 1";
			}
			return false;
		}
	}
	else if (searchMethod == ppc::PathFinder::N_SEARCH)
	{
		if (log)
		{
			PPC_LOG(fatal) << "To use N_SEARCH you also need to give the n_locations options";
		}
		return false;
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