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
	assert(world.size() >= 3);

	ppc::Map map;
	if (world.rank() == 0)
	{
		PPC_LOG(info) << "Generating random map...";
		const auto height = 10000u;
		const auto width = 10000u;
		map = { height, width, {height, {width, ppc::CLIFF}} };

		std::random_device rd;
		std::default_random_engine engine{ rd() };
		std::normal_distribution<> distribution{ 3, 1 };

		//ppc::ZoneType zones[] = { ppc::OPEN, ppc::ROAD, ppc::TREE, ppc::CLIFF };
		constexpr ppc::ZoneType zones[] = { ppc::CLIFF, ppc::ROAD, ppc::OPEN, ppc::TREE, ppc::OPEN, ppc::ROAD, ppc::CLIFF };
		constexpr auto numOfZones = sizeof(zones) / sizeof(zones[0]);
		for (auto y = 1u; y < height - 1; ++y)
		{
			for (auto x = 1u; x < width - 1; ++x)
			{
				auto randomValue = distribution(engine);
				randomValue = std::min(std::max(0.0, randomValue), static_cast<double>(numOfZones - 1));

				map[y][x] = zones[static_cast<unsigned>(randomValue)];
			}
		}

		{
			std::ofstream file{ "random_map.txt" };
			assert(file);
			file << map;
		}

		world.barrier();
	}
	else
	{
		world.barrier();

		std::ifstream file{ "random_map.txt" };

		PPC_LOG(info) << "Reading map...";
		file >> map;
		PPC_LOG(info) << "Map read(height = " << map.height() << ", width = " << map.width() << ")";
	}


	PPC_LOG(info) << "Getting started...";
	std::this_thread::sleep_for(std::chrono::seconds{ 5 });
	world.barrier();	//TODO: necessary?

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
		ppc::mpi::broadcast(workers, ppc::dummy<ppc::PatternGrowth>, 0);
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