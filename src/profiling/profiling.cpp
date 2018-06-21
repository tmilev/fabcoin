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
    UniValue result;
    result.setObject();
    UniValue numCalls;
    numCalls.setObject();
    for (auto iterator = this->functionStats.begin(); iterator != this->functionStats.end(); iterator ++)
        numCalls.pushKV(iterator->second->name, iterator->second->numCalls);
    result.pushKV("numCalls", numCalls);
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
        threadStacks[this->threadId] = std::make_shared<std::vector<std::string> >();
        iterator = threadStacks.find(this->threadId);
    }
    if (iterator == threadStacks.end())
    {
        LoggerSession::logProfiling() << "Fatal error while creating new thread stack. " << LoggerSession::endL;
        assert(false);
    }
    std::vector<std::string>& theStack = *(iterator->second);
    if (theStack.size() > 0)
        this->extendedName = theStack[theStack.size() - 1] + "->";
    this->extendedName += name;
    theStack.push_back(this->extendedName);
    if (Profiling::theProfiler().functionStats.find(this->extendedName) == Profiling::theProfiler().functionStats.end())
    {
        Profiling::theProfiler().functionStats[this->extendedName] = std::make_shared<FunctionStats>();
        Profiling::theProfiler().functionStats[this->extendedName]->initialize(this->extendedName);
    }
    Profiling::theProfiler().functionStats[this->extendedName]->numCalls ++;
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
    std::vector<std::string>& theStack = *(iterator->second);
    theStack.pop_back();
}
