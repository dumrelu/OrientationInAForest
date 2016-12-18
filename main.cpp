#include <iostream>
#include <string>
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;


int main()

{
	mpi::environment environment;
	mpi::communicator world;

	std::cout << world.rank() << " / " << world.size() << std::endl;
	std::cin.get();
}