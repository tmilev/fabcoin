#ifndef PROFILING_H_header
#define PROFILING_H_header

#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/thread.hpp>
#include <univalue.h>
#include <chrono>


class FunctionStats
{
public:
    std::string name;
    int numCalls;
    long timeSubordinates;
    long timeTotalRunTime;
    void initialize(const std::string& inputName);
};

class FunctionProfileData
{
public:
    std::string extendedName;
    std::chrono::time_point<std::chrono::system_clock> timeStart;
    long timeSubordinates;
    //std::chrono::microseconds timeInFunctionBody;
};

class FunctionProfile
{
public:
    unsigned long threadId;
    FunctionProfile(const std::string& name);
    ~FunctionProfile();
};

class Profiling
{
public:
    Profiling();
    /** Returns a global profiling object.
     * Avoids the static initalization order fiasco.
     */
    static Profiling& theProfiler();
    std::shared_ptr<boost::mutex> centralLock;
    //map from thread id to a stack containing the names of the profiled functions.
    std::unordered_map<std::string, std::shared_ptr<FunctionStats> > functionStats;
    std::unordered_map<unsigned long, std::shared_ptr<std::vector<FunctionProfileData> > > threadStacks;
    UniValue toUniValue();
};

#endif // PROFILING_H_header

