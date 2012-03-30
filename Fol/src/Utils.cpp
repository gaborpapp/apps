#include <ctime>
#include <sstream>
#include <iomanip>

#include "Utils.h"

using namespace std;

namespace cinder
{

std::string timeStamp()
{
    struct tm tm;
    time_t ltime;
    static int last_sec = 0;
    static int index = 0;

    time(&ltime);
    localtime_r(&ltime, &tm);
    if (last_sec != tm.tm_sec)
        index = 0;

	stringstream ss;
    ss << setfill('0') << setw(2) << tm.tm_year - 100 <<
        setw(2) << tm.tm_mon + 1 << setw(2) << tm.tm_mday <<
        setw(2) << tm.tm_hour << setw(2) << tm.tm_min <<
        setw(2) << tm.tm_sec << setw(2) << index;

    index++;
    last_sec = tm.tm_sec;

    return ss.str();
}

} // namespace cinder

