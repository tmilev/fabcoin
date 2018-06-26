#ifndef PROFILING_H_header
#define PROFILING_H_header

#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/thread.hpp>
#include "../univalue/include/univalue.h"
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
    /**
     * Global flag indicating whether profiling is allowed.
     * If the flag is off, no profiling will be done; all calls of
     * FunctionProfile and the Profiling class will exit promptly without doing any work.
     *
     * Please beware that profiling is a security hazard when doing ECDSA signatures,
     * as it makes timing attacks easier.
     * We recall that a timing attack is an attempt to guess a
     * private key by timing (many times)
     * the speed of generation of signatures from the same private key.
     *
     * At the same time profiling is highly desired for live systems:
     * additional monitoring is always good for security.
     *
     * So, it makes sense to run profiling on a live system that is not handling secrets,
     * for example a no-wallet node.
     *
     * The static initialization value of fAllowProfiling is false.
     * Its value is set at run time in the file fabcoind.cpp in the
     * function AppInit(), as follows.
     *
     * If the flag -profilingoff is given, profiling is turned off.
     * This overrides all other settings.
     * If the flag -profilingon is given, profiling is turned on;
     * this overrides all settings except -profilingoff.
     * If neither -profilingon or profilingoff are given, profiling is turned on
     * for regtest and testnetnodns, and is turned off for testnet and mainnet.
     */
    static bool fAllowProfiling;
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

