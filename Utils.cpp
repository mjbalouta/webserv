#include "Utils.hpp"

/**
 * @brief Prints a colored message to standard output
 * @param message The message string to print
 * @param color ANSI color code to apply to the message
 * @return void
 */
void printMessage(std::string message, std::string color)
{
	std::cout << color << message << D << std::endl;
}

/**
 * @brief Converts an integer to a string (C++98 compatible)
 * @param value The integer value to convert
 * @return std::string The string representation of the integer
 */
std::string itostr(int value){
	std::ostringstream oss;
	oss << value;
	return oss.str();
}