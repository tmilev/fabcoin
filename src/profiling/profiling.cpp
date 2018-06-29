#include "profiling.h"
#include <iostream>
#include "../logging.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <cmath>
#include <iomanip>

bool Profiling::fAllowProfiling = false;
bool Profiling::fAllowFinishTimeProfiling = false;
unsigned int Profiling::nMaxNumberFinishTimes = 1000;
unsigned int Profiling::nMaxNumberTxsToAccount = 20000;
bool Profiling::fAllowTxIdReceiveTimeLogging = false;

Statistic::Statistic()
{
    this->initialize("", 100);
}

double Statistic::computeMean()
{
    return ((double) this->total) / ((double) this->numSamples);
}

void Statistic::updateHistogram(int value) {
    //std::cout << "Calling update histogram for: " << this->name << std::endl;
    this->updateHistogramRecursively(value, 0, this->histogramIntervals.size(), 0);
}

bool Statistic::isInHistogramBucketRangeLeft(int value, unsigned int firstPossibleBucketIndex)
{
    if (firstPossibleBucketIndex == 0)
        return true;
    return value > this->histogramIntervals[firstPossibleBucketIndex - 1];
}

bool Statistic::isInHistogramBucketRangeRight(int value, unsigned int lastPossibleBucketIndex)
{
    if (lastPossibleBucketIndex == this->histogramIntervals.size())
        return true;
    return value <= this->histogramIntervals[lastPossibleBucketIndex];
}

bool Statistic::isInHistogramBucketRange(int value, unsigned int firstPossibleBucketIndex, unsigned int lastPossibleBucketIndex)
{
    if (!this->isInHistogramBucketRangeLeft(value, firstPossibleBucketIndex))
        return false;
    if (!this->isInHistogramBucketRangeRight(value, lastPossibleBucketIndex))
        return false;
    return true;
}

void Statistic::accountToHistogram(unsigned int index)
{
    if (this->histogram.find(index) == this->histogram.end()) {
        this->histogram[index] = 0;
    }
    this->histogram[index] ++;
}

void Statistic::updateHistogramRecursively(int value, unsigned int firstPossibleBucketIndex, unsigned int lastPossibleBucketIndex, int recursionDepth)
{
    this->numCallsUpdateHistogramRecursively ++;
    if (recursionDepth > 100) {
        std::cout << "Too deeply nested. Value: " << value << ". First possible index: "
                  << firstPossibleBucketIndex << ". Last possible index: " << lastPossibleBucketIndex
                  << "." << std::endl;
        assert(false);
    }
    recursionDepth ++;
    if (lastPossibleBucketIndex < firstPossibleBucketIndex) {
        std::cout << "This should not happen: bucket indices out of order. ";
        assert(false);
    }
    if (firstPossibleBucketIndex == lastPossibleBucketIndex) {
        this->accountToHistogram(firstPossibleBucketIndex);
        return;
    }
    unsigned int intermediateIndexLeft = (firstPossibleBucketIndex + lastPossibleBucketIndex) / 2;
    unsigned int intermediateIndexRight = intermediateIndexLeft + 1;
    if (intermediateIndexLeft == lastPossibleBucketIndex) {
        intermediateIndexLeft --;
    }
    if (intermediateIndexRight > lastPossibleBucketIndex) {
        intermediateIndexRight = lastPossibleBucketIndex;
    }
    if (this->isInHistogramBucketRange(value, firstPossibleBucketIndex, intermediateIndexLeft)) {
        this->updateHistogramRecursively(value, firstPossibleBucketIndex, intermediateIndexLeft, recursionDepth);
        return;
    }
    if (this->isInHistogramBucketRange(value, intermediateIndexRight, lastPossibleBucketIndex)) {
        this->updateHistogramRecursively(value, intermediateIndexRight, lastPossibleBucketIndex, recursionDepth);
        return;
    }
    std::cout << "This piece of code should be unreachable. " << std::endl;
    assert(false);
}

UniValue Statistic::toUniValueHistogram() const
{
    UniValue result;
    result.setObject();
    UniValue content;
    content.setObject();
    UniValue bucketDescriptions;
    bucketDescriptions.setObject();
    for (auto iterator = this->histogram.begin(); iterator != this->histogram.end(); iterator ++) {
        unsigned int currentBucketIndex = iterator->first;
        content.pushKV(std::to_string(currentBucketIndex), std::to_string(iterator->second));
        UniValue currentBucketDescription;
        currentBucketDescription.setArray();
        if (currentBucketIndex > 0) {
            currentBucketDescription.push_back((int64_t) this->histogramIntervals[currentBucketIndex - 1]);
        } else {
            currentBucketDescription.push_back((int64_t) - 1);
        }
        if (currentBucketIndex < this->histogramIntervals.size() ) {
            currentBucketDescription.push_back((int64_t) this->histogramIntervals[currentBucketIndex]);
        }
        bucketDescriptions.pushKV(std::to_string(iterator->first), currentBucketDescription);
    }
    result.pushKV("content", content);
    result.pushKV("bucketDescriptions", bucketDescriptions);
    result.pushKV("numCallsUpdateHistogramRecursively", (int64_t) this->numCallsUpdateHistogramRecursively);
    return result;
}

UniValue Statistic::toUniValue() const
{
    UniValue result;
    result.setObject();
    result.pushKV("numSamples", (int64_t) this->numSamples);
    result.pushKV("total", (int64_t) this->total);
    std::stringstream meanStream;
    meanStream << this->meanUsedToComputeBuckets;
    result.pushKV("meanUsedToCenterHistogram", meanStream.str());
    result.pushKV("desiredHistogramIntervalSize", (int64_t) this->desiredIntervalSize);
    if (this->fHistogramInitialized) {
        result.pushKV("histogram", this->toUniValueHistogram());
    }
    return result;
}


void Statistic::initializeHistogramIfPossible()
{
    if (this->fHistogramInitialized)
        return;
    if (this->sampleMeasurements.size() < this->desiredNumberOfSampleMeasurementsBeforeWeSetupTheHistogram)
        return;
    this->meanUsedToComputeBuckets = this->computeMean();
    // we want approximately 500 buckets to the left of the mean and 500 buckets to the right.
    unsigned int halfTheBuckets = this->desiredNumberOfHistogramBucketsMinusOne / 2;
    double desiredIntervalSizeDouble = this->meanUsedToComputeBuckets / halfTheBuckets;

    int meanInteger = (int) this->meanUsedToComputeBuckets;
    this->desiredIntervalSize = (int) std::floor(desiredIntervalSizeDouble);
    if (this->desiredIntervalSize <= 0) {
        this->desiredIntervalSize = 1;
    }
    int currentBucket = meanInteger - halfTheBuckets * this->desiredIntervalSize;
    this->histogramIntervals.reserve(this->desiredNumberOfHistogramBucketsMinusOne + 1);
    for (unsigned i = 0; i < this->desiredNumberOfHistogramBucketsMinusOne; i++) {
        if (currentBucket > 0)
            this->histogramIntervals.push_back(currentBucket);
        currentBucket += this->desiredIntervalSize;
    }
    for (unsigned i = 0; i< this->sampleMeasurements.size(); i++) {
        this->updateHistogram(this->sampleMeasurements[i]);
    }
    this->fHistogramInitialized = true;
}

void Statistic::initialize(const std::string& inputName, int inputDesiredNumberOfSamplesBeforeWeSetupHistogram)
{
    this->name = inputName;
    this->desiredNumberOfSampleMeasurementsBeforeWeSetupTheHistogram = inputDesiredNumberOfSamplesBeforeWeSetupHistogram;
    this->fHistogramInitialized = false;
    this->desiredNumberOfHistogramBucketsMinusOne = 100;
    this->numSamples = 0;
    this->total = 0;
    this->numCallsUpdateHistogramRecursively = 0;
    this->desiredIntervalSize = 0;
    this->meanUsedToComputeBuckets = 0;
}

void Statistic::accountStatistic(int value)
{
    this->numSamples ++;
    if (this->sampleMeasurements.size() < this->desiredNumberOfSampleMeasurementsBeforeWeSetupTheHistogram) {
        this->sampleMeasurements.push_back(value);
    }
    this->total += value;
    this->initializeHistogramIfPossible();
    if (this->fHistogramInitialized) {
        this->updateHistogram(value);
    }
}

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
    result.pushKV("runTime", this->timeTotalRunTime.toUniValue());
    if (this->timeSubordinates.total > 0) {
        result.pushKV("runTimeSubordinates", this->timeSubordinates.toUniValue());
    }
    result.pushKV("runTimeExcludingSubordinatesInMicroseconds", (int64_t) (this->timeTotalRunTime.total - this->timeSubordinates.total));
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
        timeWithMsStream << std::put_time(&timeGM, "%Y.%m.%d.%H.%M.%S") << "." << std::setw(3) << std::setfill('0') << milliseconds;
        //timeWithMsStream.str() finally contains our date/time with milliseconds.
        timeStats.pushKV(std::to_string(*iteratorNumCalls), timeWithMsStream.str());
    }
    result.pushKV("finishTimes", timeStats);
    return result;
}

void Profiling::RegisterReceivedTxId(const std::string &txId)
{
    if (!this->fAllowTxIdReceiveTimeLogging)
        return;
    boost::lock_guard<boost::mutex> lockGuard (*this->centralLock);
    if (this->memoryPoolAcceptanceTimes.find(txId)!= this->memoryPoolAcceptanceTimes.end()) {
        return;
    }
    this->memoryPoolAcceptanceTimeKeys.push_back(txId);
    this->memoryPoolAcceptanceTimes[txId] = std::chrono::system_clock::now();
    while (this->memoryPoolAcceptanceTimeKeys.size() > this->nMaxNumberTxsToAccount) {
        //<- this loop should run only once.
        this->memoryPoolAcceptanceTimes.erase(this->memoryPoolAcceptanceTimeKeys.front());
        this->memoryPoolAcceptanceTimeKeys.pop_front();
    }
}

int64_t convertTimePointToIntMilliseconds(std::chrono::time_point<std::chrono::system_clock>& input)
{
    auto timeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(input);
    auto timeSinceEpoch = timeMs.time_since_epoch();
    int64_t receiveTime = std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceEpoch).count();
    return receiveTime;
}

UniValue Profiling::toUniValueMemoryPoolAcceptanceTimes()
{
    UniValue result(UniValue::VOBJ);
    if (!Profiling::fAllowTxIdReceiveTimeLogging) {
        result.pushKV ("error", "Profiling is off. Default for testnet and mainnet. "
                                "Override by running fabcoind with -profilingon option. "
                                "Please note that profiling is a introduces timing attacks security risks. "
                                "DO NOT USE profiling on mainnet unless you know what you are doing. ");
        return result;
    }
    boost::lock_guard<boost::mutex> lockGuard (*this->centralLock);
    UniValue arrivalTimes(UniValue::VOBJ);
    for (unsigned counter = 0; counter < this->memoryPoolAcceptanceTimeKeys.size(); counter ++) {
        const std::string& currentTxId = this->memoryPoolAcceptanceTimeKeys[counter];
        int64_t receiveTime = convertTimePointToIntMilliseconds(this->memoryPoolAcceptanceTimes[currentTxId]);
        arrivalTimes.pushKV(currentTxId, receiveTime);
    }
    result.pushKV("arrivals", arrivalTimes);
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
    result.pushKV("startTime", convertTimePointToIntMilliseconds(this->timeStart));
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
    this->timeStart = std::chrono::system_clock::now();
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
    this->timeTotalRunTime.accountStatistic(inputDuration);
    this->timeSubordinates.accountStatistic(inputRunTimeSubordinates);
    if (!Profiling::fAllowFinishTimeProfiling) {
        return;
    }
    if (this->recordFinishTimesEveryNCalls <= 0) {
        return;
    }
    if (this->timeTotalRunTime.numSamples % this->recordFinishTimesEveryNCalls != 0) {
        return;
    }
    this->finishTimes.push_back(timeEnd);
    this->finishTimeNumCalls.push_back(this->timeTotalRunTime.numSamples);
    while (this->finishTimes.size() > Profiling::nMaxNumberFinishTimes) {
        //<- this loop should not run more than once.
        this->finishTimes.pop_front();
        this->finishTimeNumCalls.pop_front();
    }
}

void FunctionStats::initialize(const std::string& inputName, int inputRecordFinishTimesEveryNCalls, int numSamplesTomComputeMean)
{
    this->name = inputName;
    this->timeSubordinates.initialize(inputName + "_subordinates", numSamplesTomComputeMean);
    this->timeTotalRunTime.initialize(inputName + "_runtime", numSamplesTomComputeMean);
    this->recordFinishTimesEveryNCalls = inputRecordFinishTimesEveryNCalls;
}

FunctionProfile::FunctionProfile(const std::string& name, int recordFinishTimesEveryNCalls, int numSamplesToComputeMean)
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
        Profiling::theProfiler().functionStats[currentFunctionProfile.extendedName]->initialize(currentFunctionProfile.extendedName, recordFinishTimesEveryNCalls, numSamplesToComputeMean);
    }
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
