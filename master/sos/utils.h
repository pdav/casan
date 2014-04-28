/**
 * @file utils.h
 * @brief Some random utilities
 */

#ifndef SOS_UTILS_H
#define SOS_UTILS_H

namespace sos {

int random_value (int n) ;
duration_t random_timeout (int maxmilli) ;
std::vector <std::string> split_path (const std::string &s) ;
std::string join_path (const std::vector <std::string> &v) ;

}					// end of namespace sos
#endif
