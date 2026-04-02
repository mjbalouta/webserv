MAKE			= make

NAME 			= webserv

_SUCCESS 		= $(GRN)SUCCESS$(D)

SRC_PATH		= .
INC_PATH		= .
BUILD_PATH		= .build

FILES			= main.cpp \
				  CGI/CGIHandler.cpp \
				  config/parser/ConfigParser.cpp \
				  config/parser/ConfigUtils.cpp \
				  config/parser/LocationParser.cpp \
				  config/parser/ServerParser.cpp \
				  config/LocationConfig.cpp \
				  config/ServerConfig.cpp \
				  Utils.cpp \
				  ServerManager/Request.cpp \
				  ServerManager/ServerManager.cpp \
				  ServerManager/ServerManagerCgi.cpp \
				  ServerManager/ServerManagerChunked.cpp \
				  ServerManager/ServerManagerClient.cpp \
				  ServerManager/ServerManagerLoop.cpp \
				  ServerManager/ServerManagerRequest.cpp \
				  ServerManager/ServerManagerRequestRead.cpp \
				  ServerManager/ServerManagerSetup.cpp \
				  routing/ConfigResolved.cpp \
				  fileResourceManagement/ErrorPageGenerator.cpp \
				  fileResourceManagement/FileSystemHandler.cpp \
				  fileResourceManagement/MimeTypeResolver.cpp \
				  fileResourceManagement/PathResolver.cpp \
				  Response/ResponseBuilder.cpp \
				  Response/HtmlTestsResponse.cpp \

SRC				= $(addprefix $(SRC_PATH)/, $(FILES))

# Compiler (C++).
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -g

INC			= -I $(INC_PATH)

RM			= rm -rf
MKDIR_P		= mkdir -p

# Out-of-source build directory and object list
OBJS			= $(SRC:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)

.DEFAULT_GOAL := all

help:
	@echo "\n"
	@echo "\t$(BCYAN)Available targets:$(D)"
	@echo "\t$(BGRN)  all$(D)       - Build the project"
	@echo "\t$(BGRN)  clean$(D)     - Remove object files"
	@echo "\t$(BGRN)  fclean$(D)    - Remove object files and executable"
	@echo "\t$(BGRN)  re$(D)        - Rebuild the project"
	@echo "\t$(BGRN)  run$(D)       - Build and run the project with a default config file"
	@echo "\t$(BGRN)  help$(D)      - Show this help message"
	@echo "\n"

all: $(NAME) 	## Compile

$(NAME): $(OBJS) ## Link
	@echo "\t$(BGOLD)Linking ...$(D)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "\t  → Created executable:\t$(B)$(NAME)$(D)"
	@echo "\n"

# Compile into build directory (creates directory as needed)
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.cpp
	@$(MKDIR_P) $(dir $@)
#	@echo -n "\t$(GRN)█$(D)"
	@$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean: 				## Remove object files and build dir
	@echo "\n"
	@echo "\t$(BGOLD)Cleaning build folder ...$(D)"
	@$(RM) $(BUILD_PATH)
	@echo "\t$(GRN)  ✓ Clean complete!$(D)"

fclean: clean			## Remove executable and build artifacts
	@echo "\t$(BGOLD)Removing executable ...$(D)"
	@$(RM) $(NAME)
	@echo "\t$(GRN)  ✓ Full clean complete!$(D)"
	@echo "\n"

re: fclean all	## Purge & Recompile

run: $(NAME)
	@./$(NAME) configurations/default.conf

.PHONY: help all clean fclean re run

# Colors

B  		= $(shell tput bold)
RED		= $(shell tput setaf 1)
GRN		= $(shell tput setaf 2)
BGRN		= $(shell tput bold; tput setaf 2)
BLU		= $(shell tput setaf 4)

BCYAN		= $(shell tput bold; tput setaf 6)
CYAN		= $(shell tput setaf 6)

# Extended colors
BGOLD    = $(shell tput bold; tput setaf 220)
GOLD    = $(shell tput setaf 220)
BSILV    = $(shell tput bold; tput setaf 250)
SILV    = $(shell tput setaf 250)

# Reset
D       = $(shell tput sgr0)