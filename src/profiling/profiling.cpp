#include "profiling.h"
#include <iostream>
#include "../logging.h"
#include "../util.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

LoggerSession& LoggerSession::logProfiling()
{
    static LoggerSession theLogger(GetDataDir().string() + "/log_profiling.log", "[profiler] ");
    return theLogger;
}

UniValue Profiling::toUniValue()
{
    boost::lock_guard<boost::mutex> lockGuard (*this->centralLock);
    UniValue result(UniValue::VOBJ);
    UniValue functionStats(UniValue::VOBJ);
    for (auto iterator = this->functionStats.begin(); iterator != this->functionStats.end(); iterator ++)
    {
        UniValue currentStat(UniValue::VOBJ);
        currentStat.pushKV("numCalls", iterator->second->numCalls);
        currentStat.pushKV("runTimeTotalInMicroseconds", iterator->second->timeTotalRunTime);
        currentStat.pushKV("runTimeSubordinatesInMicroseconds", iterator->second->timeSubordinates);
        currentStat.pushKV("runTimeExcludingSubordinatesInMicroseconds", iterator->second->timeTotalRunTime - iterator->second->timeSubordinates);
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
    LoggerSession::logProfiling() << LoggerSession::colorGreen << "Profiler created. " << LoggerSession::endL;
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
