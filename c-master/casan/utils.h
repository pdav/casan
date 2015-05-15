/**
 * @file utils.h
 * @brief Some random utilities
 */

#ifndef CASAN_UTILS_H
#define CASAN_UTILS_H

namespace casan {

int random_value (int n) ;
duration_t random_timeout (int maxmilli) ;
std::vector <std::string> split_path (const std::string &s) ;
std::string join_path (const std::vector <std::string> &v) ;

}					// end of namespace casan
#endif
