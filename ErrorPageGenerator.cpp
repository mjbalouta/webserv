#include "ErrorPageGenerator.hpp"

/**
 * @brief Generate a simple HTML error page based on the provided status code and message.
 * The generated page includes a title, a heading with the status code and message, and a link to return to the home page.
 * 
 * @param statusCode 
 * @param message 
 * @return std::string 
 */

std::string ErrorPageGenerator::getReasonPhrase(int status_code){
    switch (status_code)
    {
        case 200: return ("OK");
        case 201: return ("Created");
        case 204: return ("No Content");
    }
}
