#include "config/parser/ConfigParser.hpp"

//  //FOR TESTING THE PARSING
// void printServers(ConfigParser &config)
// {
//     const std::vector<ServerConfig>& servers = config.getServers();

//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         const ServerConfig& s = servers[i];
//         std::cout << "=== SERVER [" << i << "] ===" << std::endl;
        
//         // Print Ports
//         const std::vector<int>& ports = s.getPorts();
//         std::cout << "  Ports: ";
//         for (size_t p = 0; p < ports.size(); ++p)
//             std::cout << ports[p] << (p < ports.size() - 1 ? ", " : "");
//         std::cout << std::endl;

//         std::cout << "  Host: " << s.getHost() << std::endl;
//         std::cout << "  Root: " << s.getRoot() << std::endl;
//         std::cout << "  Max Body Size: " << s.getMaxBodySize() << std::endl;
//         std::cout << "  Autoindex: " << (s.getAutoIndex() ? "on" : "off") << std::endl;

//         // Print Server Names
//         const std::vector<std::string>& names = s.getServerNames();
//         std::cout << "  Server Names: ";
//         for (size_t n = 0; n < names.size(); ++n)
//             std::cout << names[n] << " ";
//         std::cout << std::endl;

//         // Print Server-level Allowed Methods
//         const std::vector<std::string>& serverMethods = s.getAllowedMethods();
//         if (!serverMethods.empty()) {
//             std::cout << "  Server Methods: ";
//             for (size_t sm = 0; sm < serverMethods.size(); ++sm)
//                 std::cout << serverMethods[sm] << " ";
//             std::cout << std::endl;
//         }

//         // Print Error Pages (Map)
//         const std::map<int, std::string>& errors = s.getErrorPages();
//         std::cout << "  Error Pages: ";
//         for (std::map<int, std::string>::const_iterator itE = errors.begin(); itE != errors.end(); ++itE)
//             std::cout << "[" << itE->first << " -> " << itE->second << "] ";
//         std::cout << std::endl;

//         // Print Index files
//         const std::vector<std::string>& indexes = s.getIndexes();
//         if (!indexes.empty()) {
//             std::cout << "  Index Files: ";
//             for (size_t idx = 0; idx < indexes.size(); ++idx)
//                 std::cout << indexes[idx] << " ";
//             std::cout << std::endl;
//         }

//         // Print Locations
//         const std::vector<LocationConfig>& locs = s.getLocations();
//         for (size_t j = 0; j < locs.size(); ++j)
//         {
//             const LocationConfig& l = locs[j];
//             std::cout << "  --- Location " << l.getPath() << " ---" << std::endl;
//             if (!l.getRoot().empty()) std::cout << "    Root: " << l.getRoot() << std::endl;
//             if (l.getAliasFlag()) std::cout << "    Alias: " << l.getAlias() << std::endl;

//             // Methods
//             std::cout << "    Methods: ";
//             const std::vector<std::string>& meths = l.getAllowedMethods();
//             for (size_t m = 0; m < meths.size(); ++m) std::cout << meths[m] << " ";
//             std::cout << std::endl;

//             // Autoindex (location-level)
//             std::cout << "    Autoindex: " << (l.getAutoIndex() ? "on" : "off") << std::endl;

//             // Return directive
//             if (l.getReturnStatusCode() != 0)
//                 std::cout << "    Return: " << l.getReturnStatusCode() << " " << l.getReturnURL() << std::endl;

//             // Upload store
//             if (!l.getUploadStore().empty())
//                 std::cout << "    Upload Store: " << l.getUploadStore() << std::endl;

//             // CGI configurations
//             const std::map<std::string, std::string>& cgis = l.getCGI();
//             if (!cgis.empty()) {
//                 std::cout << "    CGI: ";
//                 for (std::map<std::string, std::string>::const_iterator cgi = cgis.begin(); cgi != cgis.end(); ++cgi)
//                     std::cout << "[" << cgi->first << " -> " << cgi->second << "] ";
//                 std::cout << std::endl;
//             }

//             // Location index files (if different from server)
//             const std::vector<std::string>& locIndexes = l.getIndexes();
//             if (!locIndexes.empty()) {
//                 std::cout << "    Index Files: ";
//                 for (size_t lidx = 0; lidx < locIndexes.size(); ++lidx)
//                     std::cout << locIndexes[lidx] << " ";
//                 std::cout << std::endl;
//             }
//         }
//         std::cout << "==========================\n" << std::endl;
//     }
// }

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cout << "Missing a config file." << std::endl;
		return 1;
	}

	/* try catch para o parsing, depois de passar as validacoes do parsing,
	nao pode haver try catch porque o servidor tem de estar sempre aberto*/
	ConfigParser config; //has to be created outside of try catch because we need it outside of this scope
	try
	{
		config.parse(av[1]);
		// config.getServers(); vai devolver um std::vector<ServerConfig> _servers!
		//podem passar o objeto config para as restantes partes do projeto 
		//porque ja tem a info toda que precisam
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	// printServers(config);
	return 0;
}