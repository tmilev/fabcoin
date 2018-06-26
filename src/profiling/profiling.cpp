#include "profiling.h"
#include <iostream>
#include "../logging.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

bool Profiling::fAllowProfiling = false;
bool Profiling::fAllowFinishTimeProfiling = false;
unsigned int Profiling::nMaxNumberFinishTimes = 1000;

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

UniValue FunctionStats::toUniValue() const
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("numCalls", this->numCalls);
    result.pushKV("runTimeTotalInMicroseconds", (int64_t) this->timeTotalRunTime);
    result.pushKV("runTimeSubordinatesInMicroseconds", (int64_t) this->timeSubordinates);
    result.pushKV("runTimeExcludingSubordinatesInMicroseconds", (int64_t) (this->timeTotalRunTime - this->timeSubordinates));
    if (!Profiling::fAllowFinishTimeProfiling) {
        return result;
    }
    if (this->recordFinishTimesEveryNCalls <=0) {
        return result;
    }
    auto iteratorNumCalls = this->finishTimeNumCalls.begin();
    auto iteratorTime = this->finishTimes.begin();
    UniValue timeStats;
    timeStats.setObject();
    for (;iteratorTime != this->finishTimes.end(); iteratorTime++, iteratorNumCalls++) {
        std::stringstream timeWithMsStream;
        //converting time point to date string including milliseconds: milliseconds don't work out of
        //the box in C++11:
        std::time_t timeNoMilliseconds = std::chrono::system_clock::to_time_t(*iteratorTime);
        //<- this conversion should lose the milliseconds
        std::chrono::system_clock::time_point timePointNoMs = std::chrono::system_clock::from_time_t(timeNoMilliseconds);
        //<- this time point has milliseconds rounded down.
        int milliseconds = (std::chrono::duration_cast<std::chrono::milliseconds>(*iteratorTime - timePointNoMs)).count();
        //<- now we have our milliseconds.
        std::tm timeGM = *std::gmtime(&timeNoMilliseconds);
        //<-Not thread safe. Not leaking, the pointer points to a statically allocated (shared) object.
        timeWithMsStream << std::put_time(&timeGM, "%Y.%m.%d.%H.%M.%S") << "." << milliseconds;
        //timeWithMsStream.str() finally contains our date/time with milliseconds.
        timeStats.pushKV(std::to_string(*iteratorNumCalls), timeWithMsStream.str());
    }
    result.pushKV("finishTimes", timeStats);
    return result;
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
    for (auto iterator = this->functionStats.begin(); iterator != this->functionStats.end(); iterator ++) {
        functionStats.pushKV(iterator->first, iterator->second->toUniValue());
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

void FunctionStats::accountFinishTime(long inputDuration, long inputRunTimeSubordinates, const std::chrono::system_clock::time_point& timeEnd)
{
    this->timeTotalRunTime += inputDuration;
    this->timeSubordinates += inputRunTimeSubordinates;
    if (!Profiling::fAllowFinishTimeProfiling) {
        return;
    }
    if (this->recordFinishTimesEveryNCalls <= 0) {
        return;
    }
    if (this->numCalls % this->recordFinishTimesEveryNCalls != 0) {
        return;
    }
    this->finishTimes.push_back(timeEnd);
    this->finishTimeNumCalls.push_back(this->numCalls);
    while (this->finishTimes.size() > Profiling::nMaxNumberFinishTimes) {
        //<- this loop should not run more than once.
        this->finishTimes.pop_front();
        this->finishTimeNumCalls.pop_front();
    }
}

void FunctionStats::initialize(const std::string& inputName, int inputRecordFinishTimesEveryNCalls)
{
    this->name = inputName;
    this->numCalls = 0;
    this->timeSubordinates = 0;
    this->timeTotalRunTime = 0;
    this->recordFinishTimesEveryNCalls = inputRecordFinishTimesEveryNCalls;
}

FunctionProfile::FunctionProfile(const std::string& name, int recordFinishTimesEveryNCalls)
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
        Profiling::theProfiler().functionStats[currentFunctionProfile.extendedName]->initialize(currentFunctionProfile.extendedName, recordFinishTimesEveryNCalls);
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
    long duration = (std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - last.timeStart) ).count();
    currentStats.accountFinishTime(duration, last.timeSubordinates, timeEnd);
    if (theStack.size() > 1)
    {
        FunctionProfileData& secondToLast = theStack[theStack.size() - 2];
        secondToLast.timeSubordinates += duration;
    }
    //LoggerSession::logProfiling() << LoggerSession::colorRed
    //<< currentStats.timeInFunctionBody.count() << LoggerSession::endL;
    theStack.pop_back();
}
