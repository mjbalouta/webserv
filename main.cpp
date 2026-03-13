#include "ServerManager/ServerManager.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cout << "Missing a config file." << std::endl;
		return 1;
	}
	try
	{
		ServerManager manager(av);
		manager.runEventLoop();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}