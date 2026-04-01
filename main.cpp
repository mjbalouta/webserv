#include "ServerManager/ServerManager.hpp"

volatile sig_atomic_t running = 1;

void HandleSigInt(int sig)
{
	(void)sig;
	running = 0;
}

int main(int ac, char **av)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, HandleSigInt);
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