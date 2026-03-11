#include "Includes.hpp"

int main(int ac, char **av)
{
	//tem de receber obrigatoriamente 1 config file como argumento?

	/* try catch para o parsing, depois de passar as validacoes do parsing,
	nao pode haver try catch porque o servidor tem de estar sempre aberto*/
	try
	{
		ConfigParser config(av[1]);
		// config.getServers(); vai devolver um std::vector<ServerConfig> _servers!
		//podem passar o objeto config para as restantes partes do projeto 
		//porque ja tem a info toda que precisam
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}