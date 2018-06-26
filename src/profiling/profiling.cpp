#include "profiling.h"
#include <iostream>
#include "../logging.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

bool Profiling::fAllowProfiling = false;

LoggerSession& LoggerSession::logProfiling()
{
    if (LoggerSession::baseFolderComputedRunTime == "") {
        std::cout << "This should not happen: profiler is running before the base folder is initialized. Please call LogMain() "
        << "before you start the system's profiling. ";
        assert(false);
    }
    static LoggerSession theLogger(LoggerSession::baseFolderComputedRunTime + "/log_profiling.log", "[profiler] ");
    return theLogger;
}

UniValue Profiling::toUniValue()
{
    UniValue result(UniValue::VOBJ);
    if (!Profiling::fAllowProfiling) {
        result.pushKV ("error", "Profiling is off. Default for testnet and mainnet. "
                                "Override by running fabcoind with -profilingon option. "
                                "Please note that profiling is a introduces timing attacks security risks. "
                                "DO NOT USE profiling on mainnet unless you know what you are doing. ");
        return result;
    }
    boost::lock_guard<boost::mutex> lockGuard (*this->centralLock);
    UniValue functionStats(UniValue::VOBJ);
    for (auto iterator = this->functionStats.begin(); iterator != this->functionStats.end(); iterator ++)
    {
        UniValue currentStat(UniValue::VOBJ);
        currentStat.pushKV("numCalls", iterator->second->numCalls);
        currentStat.pushKV("runTimeTotalInMicroseconds", (int64_t) iterator->second->timeTotalRunTime);
        currentStat.pushKV("runTimeSubordinatesInMicroseconds", (int64_t) iterator->second->timeSubordinates);
        currentStat.pushKV("runTimeExcludingSubordinatesInMicroseconds", (int64_t) (iterator->second->timeTotalRunTime - iterator->second->timeSubordinates));
        functionStats.pushKV(iterator->second->name, currentStat);
    }
    result.pushKV("functionStats", functionStats);
    UniValue theThreads;
    theThreads.setArray();
    for (auto iterator = this->threadStacks.begin(); iterator != this->threadStacks.end(); iterator ++) {
        std::stringstream printToString;
        printToString << iterator->first;
        theThreads.push_back(printToString.str());
    }
    result.pushKV("threads", theThreads);
    return result;
}

Profiling::Profiling()
{
    this->centralLock = std::make_shared<boost::mutex>();
    if (this->fAllowProfiling) {
        LoggerSession::logProfiling() << LoggerSession::colorGreen
        << "Profiler on [default for testnetnodns and regtest]."
        << LoggerSession::colorRed << " To turn off (overriding defaults) use option -profilingoff";
    } else {
        LoggerSession::logProfiling() << LoggerSession::colorRed << "Profiler off [default for testnet and mainnet]."
        << LoggerSession::colorGreen << " To turn on (overriding defaults) use option -profilingon";
    }
    LoggerSession::logProfiling() << LoggerSession::endL;
    LoggerSession::logProfiling() << LoggerSession::colorYellow
    << "DO NOT use profiling on mainnet unless you know what are doing. " << LoggerSession::endL;
}

Profiling& Profiling::theProfiler()
{
    static Profiling theProfiler;
    return theProfiler;
}

unsigned long getThreadId()
{
    //taken from https://stackoverflow.com/questions/4548395/how-to-retrieve-the-thread-id-from-a-boostthread
    std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long threadNumber = 0;
    sscanf(threadId.c_str(), "%lx", &threadNumber);
    return threadNumber;
}

void FunctionStats::initialize(const std::string& inputName)
{
    this->name = inputName;
    this->numCalls = 0;
    this->timeSubordinates = 0;
    this->timeTotalRunTime = 0;
}

FunctionProfile::FunctionProfile(const std::string& name)
{
    if (!Profiling::fAllowProfiling)
        return;
    auto lockSharedPointer = Profiling::theProfiler().centralLock;
    boost::lock_guard<boost::mutex> lockGuard (*lockSharedPointer);
    auto& threadStacks = Profiling::theProfiler().threadStacks;
    this->threadId = getThreadId();
    auto iterator = threadStacks.find(this->threadId);
    if (iterator == threadStacks.end())
    {
        threadStacks[this->threadId] = std::make_shared<std::vector<FunctionProfileData> >();
        iterator = threadStacks.find(this->threadId);
    }
    if (iterator == threadStacks.end())
    {
        LoggerSession::logProfiling() << "Fatal error while creating new thread stack. " << LoggerSession::endL;
        assert(false);
    }
    std::vector<FunctionProfileData>& theStack = *(iterator->second);
    FunctionProfileData currentFunctionProfile;
    if (theStack.size() > 0)
        currentFunctionProfile.extendedName = theStack[theStack.size() - 1].extendedName + "->";
    currentFunctionProfile.extendedName += name;
    currentFunctionProfile.timeStart = std::chrono::system_clock::now();
    currentFunctionProfile.timeSubordinates = 0;
    theStack.push_back(currentFunctionProfile);
    if (Profiling::theProfiler().functionStats.find(currentFunctionProfile.extendedName) == Profiling::theProfiler().functionStats.end())
    {
        Profiling::theProfiler().functionStats[currentFunctionProfile.extendedName] = std::make_shared<FunctionStats>();
        Profiling::theProfiler().functionStats[currentFunctionProfile.extendedName]->initialize(currentFunctionProfile.extendedName);
    }
    Profiling::theProfiler().functionStats[currentFunctionProfile.extendedName]->numCalls ++;
}

FunctionProfile::~FunctionProfile()
{
    if (!Profiling::fAllowProfiling)
        return;
    auto lockSharedPointer = Profiling::theProfiler().centralLock;
    boost::lock_guard<boost::mutex> lockGuard (*lockSharedPointer);
    auto& threadStacks = Profiling::theProfiler().threadStacks;
    auto iterator = threadStacks.find(this->threadId);
    if (iterator == threadStacks.end())
    {
        LoggerSession::logProfiling() << "Fatal error in function profile destructor. " << LoggerSession::endL;
        assert(false);
    }
    std::vector<FunctionProfileData>& theStack = *(iterator->second);
    FunctionProfileData& last = theStack[theStack.size() - 1];
    FunctionStats& currentStats = *Profiling::theProfiler().functionStats[last.extendedName];
    auto timeEnd = std::chrono::system_clock::now();
    long currentTotalRunTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - last.timeStart).count();
    currentStats.timeTotalRunTime += currentTotalRunTime;
    currentStats.timeSubordinates += last.timeSubordinates;
    if (theStack.size() > 1)
    {
        FunctionProfileData& secondToLast = theStack[theStack.size() - 2];
        secondToLast.timeSubordinates += currentTotalRunTime;
    }
    //LoggerSession::logProfiling() << LoggerSession::colorRed
    //<< currentStats.timeInFunctionBody.count() << LoggerSession::endL;
    theStack.pop_back();
}
