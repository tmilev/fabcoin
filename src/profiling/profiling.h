#ifndef PROFILING_H_header
#define PROFILING_H_header

#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/thread.hpp>
#include <univalue.h>


class FunctionStats
{
public:
    std::string name;
    int numCalls;
    void initialize(const std::string& inputName);
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
    std::unordered_map<unsigned long, std::shared_ptr<std::vector<std::string> > > threadStacks;
    UniValue toUniValue();
};

class FunctionProfile
{
public:
    unsigned long threadId;
    std::string extendedName;
    FunctionProfile(const std::string& name);
    ~FunctionProfile();
};
#endif // PROFILING_H_header

