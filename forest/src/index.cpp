#include "index.hpp"

namespace ppc
{
	namespace detail
	{
		std::ostream& operator<<(std::ostream& out, const StdPair& pair)
		{
			out << "{ x = " << pair.first << ", y = " << pair.second << " }";
			return out;
		}
	}
}