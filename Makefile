MAKE			= make

NAME 			= webserver

_SUCCESS 		= $(GRN)SUCCESS$(D)

SRC_PATH		= .
INC_PATH		= .
BUILD_PATH		= .build

FILES			= main.cpp

SRC				= $(addprefix $(SRC_PATH)/, $(FILES))

# Compiler (C++).
CXX			= c++
CXXFLAGS		= -Wall -Wextra -Werror -std=c++98 -g

INC			= -I $(INC_PATH)

RM			= rm -rf
MKDIR_P		= mkdir -p

# Out-of-source build directory and object list
OBJS			= $(SRC:$(SRC_PATH)/%.cpp=$(BUILD_PATH)/%.o)

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
	@echo "\t$(BGRN)✓ Build complete!$(D)"
	@echo "\t  Run ./$(NAME) to start the server"
	@echo "\n"

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
	@./$(NAME) config.file

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