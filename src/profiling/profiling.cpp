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

struct KeyNames
{
    static std::string functionStats;
    static std::string timesPastStarts;
    static std::string timesPastSamplings;
    static std::string numberOfSamples;
    static std::string totalRunTime;
    static std::string desiredNumberOfHistogramsMinusOne;
    static std::string meanUsedToCenterHistogram;
    static std::string histogram;
    static std::string runTime;
    static std::string runTimeSubordinates;
    static std::string histogramContent;
    static std::string numberOfRecursiveHistogramUpdateCalls;
    static std::string numberOfSamplesLoadedFromHD;
};

std::string KeyNames::functionStats = "functionStats";
std::string KeyNames::timesPastStarts = "timePastStarts";
std::string KeyNames::timesPastSamplings = "timePastSamplings";
std::string KeyNames::totalRunTime = "totalRunTime";
std::string KeyNames::numberOfSamples = "numberOfSamples";
std::string KeyNames::desiredNumberOfHistogramsMinusOne = "desiredNumberOfHistograms";
std::string KeyNames::meanUsedToCenterHistogram = "meanUsedToCenterHistogram";
std::string KeyNames::histogram = "histogram";
std::string KeyNames::runTime = "runTime";
std::string KeyNames::runTimeSubordinates = "runTimeSubordinates";
std::string KeyNames::histogramContent = "histogramContent";
std::string KeyNames::numberOfRecursiveHistogramUpdateCalls = "numberOfRecursiveHistogramUpdateCalls";
std::string KeyNames::numberOfSamplesLoadedFromHD = "numberOfStatisticsLoadedFromHD";

Statistic::Statistic()
{
    this->initialize("", 100);
}

double Statistic::computeMean()
{
    return ((double) this->total) / ((double) this->numberOfSamples);
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

bool checkForKeys(const std::vector<std::string>& requiredKeys, const UniValue& input)
{
    for (unsigned i = 0; i < requiredKeys.size(); i ++) {
        if (!input.exists(requiredKeys[i])) {
            LoggerSession::logProfiling()
            << LoggerSession::colorRed
            << "Could not find required key: " << requiredKeys[i] << LoggerSession::colorNormal
            << " in input: "
            << input.write()
            << LoggerSession::endL;
            return false;
        }
    }
    return true;
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
        if (currentBucketIndex > 0 && currentBucketIndex - 1 >= this->histogramIntervals.size()) {
            std::stringstream errorStream;
            errorStream << "Bad bucket index " << currentBucketIndex;
            currentBucketDescription.push_back(errorStream.str());
        } else {
            if (currentBucketIndex > 0 && currentBucketIndex - 1 < this->histogramIntervals.size()) {
                currentBucketDescription.push_back((int64_t) this->histogramIntervals[currentBucketIndex - 1]);
            } else {
                currentBucketDescription.push_back((int64_t) - 1);
            }
            if (currentBucketIndex < this->histogramIntervals.size() ) {
                currentBucketDescription.push_back((int64_t) this->histogramIntervals[currentBucketIndex]);
            }
        }
        bucketDescriptions.pushKV(std::to_string(iterator->first), currentBucketDescription);
    }
    result.pushKV(KeyNames::histogramContent, content);
    result.pushKV("bucketDescriptions", bucketDescriptions);
    result.pushKV(KeyNames::numberOfRecursiveHistogramUpdateCalls, (int64_t) this->numCallsUpdateHistogramRecursively);
    return result;
}

UniValue Statistic::toUniValueHistogramForStorage() const
{
    UniValue result;
    result.setObject();
    UniValue content;
    content.setObject();
    for (auto iterator = this->histogram.begin(); iterator != this->histogram.end(); iterator ++) {
        unsigned int currentBucketIndex = iterator->first;
        content.pushKV(std::to_string(currentBucketIndex), std::to_string(iterator->second));
    }
    result.pushKV(KeyNames::histogramContent, content);
    result.pushKV(KeyNames::numberOfRecursiveHistogramUpdateCalls, (int64_t) this->numCallsUpdateHistogramRecursively);
    return result;
}

bool Statistic::fromUniValueForStorageHistogram(const UniValue& inpuT)
{
    if (!checkForKeys({KeyNames::histogramContent, KeyNames::numberOfRecursiveHistogramUpdateCalls}, inpuT)) {
        return false;
    }
    //LoggerSession::logProfiling() << LoggerSession::colorYellow << "DEBUG: input: " << inpuT.write() << LoggerSession::endL;
    const UniValue& histogramContent = inpuT[KeyNames::histogramContent];

    for (unsigned i = 0; i < histogramContent.size(); i ++) {
        std::string currentValue, currentKey;
        try {
            std::string currentKey = histogramContent.getKeys()[i];
            std::string currentValue = histogramContent.getValues()[i].get_str();
            int currentBucketIndex = std::stoi(currentKey);
            int currentValueInt = std::stoi(currentValue);
            this->histogram[currentBucketIndex] = currentValueInt;
        } catch (...) {
            LoggerSession::logProfiling() << LoggerSession::colorRed
            << "Failed to convert pair: " << currentKey << ", "
            << currentValue << " (from " << histogramContent.getValues()[i].write() << ")"
            << " to a pair of integers. " << LoggerSession::endL;
            return false;
        }
    }
    this->numCallsUpdateHistogramRecursively = histogramContent[KeyNames::numberOfRecursiveHistogramUpdateCalls].get_int64();
    return true;
}

UniValue Statistic::toUniValue() const
{
    UniValue result;
    result.setObject();
    result.pushKV(KeyNames::numberOfSamples, (int64_t) this->numberOfSamples);
    result.pushKV(KeyNames::totalRunTime, (int64_t) this->total);
    result.pushKV("desiredHistogramIntervalSize", (int64_t) this->desiredIntervalSize);
    if (this->fHistogramInitialized) {
        std::stringstream meanStream;
        meanStream << this->meanUsedToComputeBuckets;
        result.pushKV(KeyNames::meanUsedToCenterHistogram, meanStream.str());
        result.pushKV(KeyNames::histogram, this->toUniValueHistogram());
    }
    return result;
}

bool Statistic::fromUniValueForStorage(const UniValue& input)
{
    if (!checkForKeys({KeyNames::totalRunTime, KeyNames::numberOfSamples}, input)) {
        return false;
    }
    this->numberOfSamples = (int) input[KeyNames::numberOfSamples].get_int64();
    //LoggerSession::logProfiling()
    //<< "DEBUG: loaded: " << this->numberOfSamples << " samples from: "
    //<< input[KeyNames::numberOfSamples].write() << LoggerSession::endL;
    this->total = input[KeyNames::totalRunTime].get_int64();
    //LoggerSession::logProfiling()
    //<< "DEBUG: loaded: " << this->total << " total from: "
    //<< input[KeyNames::totalRunTime].write() << LoggerSession::endL;
    this->histogram.clear();
    this->histogramIntervals.clear();
    this->meanUsedToComputeBuckets = 0;
    //LoggerSession::logProfiling() << "DEBUG: here I am. " << LoggerSession::endL;
    if (!input.exists(KeyNames::meanUsedToCenterHistogram)) {
        //LoggerSession::logProfiling() << "Histogram not initialized. " << LoggerSession::endL;
        this->fHistogramInitialized = false;
        return true;
    }
    //LoggerSession::logProfiling() << "DEBUG: about to read mean from: " << input[KeyNames::meanUsedToCenterHistogram].write() << LoggerSession::endL;
    this->meanUsedToComputeBuckets = std::stod(input[KeyNames::meanUsedToCenterHistogram].get_str());
    //LoggerSession::logProfiling() << "DEBUG: Read mean: " << this->meanUsedToComputeBuckets << LoggerSession::endL;
    this->initializeHistogramFromMean(this->meanUsedToComputeBuckets);
    if (input.exists(KeyNames::histogram)) {
        if (!this->fromUniValueForStorageHistogram(input[KeyNames::histogram])) {
            return false;
        }
    }
    return true;
}

UniValue Statistic::toUniValueForStorage() const
{
    UniValue result;
    result.setObject();
    result.pushKV(KeyNames::numberOfSamples, (int64_t) this->numberOfSamples);
    result.pushKV(KeyNames::totalRunTime, (int64_t) this->total);
    if (this->fHistogramInitialized) {
        std::stringstream meanStream;
        meanStream << this->meanUsedToComputeBuckets;
        result.pushKV(KeyNames::meanUsedToCenterHistogram, meanStream.str());
        result.pushKV(KeyNames::desiredNumberOfHistogramsMinusOne, (int64_t) this->desiredNumberOfHistogramBucketsMinusOne);
        result.pushKV(KeyNames::histogram, this->toUniValueHistogramForStorage());
    }
    return result;
}

void Statistic::initializeHistogramIfPossible()
{
    if (this->fHistogramInitialized)
        return;
    if (this->sampleMeasurements.size() < this->desiredNumberOfSampleMeasurementsBeforeWeSetupTheHistogram)
        return;
    this->initializeHistogramFromMean(this->computeMean());
}

void Statistic::initializeHistogramFromMean(double inputMean)
{
    this->meanUsedToComputeBuckets = inputMean;
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
    this->numberOfSamples = 0;
    this->total = 0;
    this->numCallsUpdateHistogramRecursively = 0;
    this->desiredIntervalSize = 0;
    this->meanUsedToComputeBuckets = 0;
}

void Statistic::accountStatistic(int value)
{
    this->numberOfSamples ++;
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

bool FunctionStats::fromUniValueForStorageNoLock(const std::string& inputName, const UniValue& input)
{
    if (!checkForKeys({KeyNames::runTime}, input))
        return false;
    this->name = inputName;
    //LoggerSession::logProfiling()
    //<< "DEBUG: about to read fun stat with name: " << inputName << " from: "
    //<< input.write() << LoggerSession::endL;

    if (!this->timeTotalRunTime.fromUniValueForStorage(input[KeyNames::runTime])) {
        LoggerSession::logProfiling()
        << "Failed to load run time from: "
        << input[KeyNames::runTime].write() << LoggerSession::endL;
        return false;
    }
    //LoggerSession::logProfiling()
    //<< "DEBUG: read stat with name: " << this->name << ". Total samples: "
    //<< this->timeTotalRunTime.numberOfSamples << LoggerSession::endL;

    //LoggerSession::logProfiling()
    //<< "DEBUG: Got to here!" << LoggerSession::endL;
    if (!input.exists(KeyNames::runTimeSubordinates)) {
        this->timeSubordinates.total = 0;
        this->timeSubordinates.numberOfSamples = 0;
        //LoggerSession::logProfiling()
        //<< "DEBUG: zeroed time subordinates. " << LoggerSession::endL;
        return true;
    }
    if (!this->timeSubordinates.fromUniValueForStorage(input[KeyNames::runTimeSubordinates]))
        return false;
    return true;
}

UniValue FunctionStats::toUniValueForStorage() const
{
    UniValue result(UniValue::VOBJ);
    result.pushKV(KeyNames::runTime, this->timeTotalRunTime.toUniValueForStorage());
    if (this->timeSubordinates.total > 0) {
        result.pushKV(KeyNames::runTimeSubordinates, this->timeSubordinates.toUniValueForStorage());
    }
    return result;
}

UniValue FunctionStats::toUniValue() const
{
    UniValue result(UniValue::VOBJ);
    result.pushKV(KeyNames::runTime, this->timeTotalRunTime.toUniValue());
    if (this->timeSubordinates.total > 0) {
        result.pushKV(KeyNames::runTimeSubordinates, this->timeSubordinates.toUniValue());
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
    for (; iteratorTime != this->finishTimes.end(); iteratorTime++, iteratorNumCalls++) {
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

int64_t convertTimePointToIntMilliseconds(const std::chrono::time_point<std::chrono::system_clock>& input)
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

UniValue Profiling::toUniValueForBrowser()
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
    result.pushKV(KeyNames::numberOfSamplesLoadedFromHD, this->numberOfStatisticsLoaded);
    result.pushKV(KeyNames::functionStats, functionStats);
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

void Profiling::AccountStat()
{
    this->numberOfStatisticsTakenCurrentSession ++;
    this->numberStatisticsSinceLastStorage ++;
    if (this->numberOfStatisticsTakenCurrentSession < this->nWriteStatisticsToHDEveryThisNumberOfCalls) {
        this->numberOfStatisticsTakenCurrentSession ++;
        return;
    }
    this->numberOfStatisticsTakenCurrentSession = 0;
    if (!this->fAllowProfiling)
        return;
    UniValue theValue = this->toUniValueForStorageNoLock();
    //LoggerSession::logProfiling() << "DEBUG: About to write: " << theValue.write();
    LoggerSession::logProfiling()
    << "Storing current profing stats to file: "
    << LoggerSession::colorBlue << this->fileNameStatistics << LoggerSession::endL;
    std::ofstream fileOut(this->fileNameStatistics);
    fileOut << theValue.write();
}

bool Profiling::fromUniValueForStorageNoLock(const UniValue& input)
{
    if (!checkForKeys({KeyNames::functionStats, KeyNames::timesPastSamplings, KeyNames::timesPastStarts}, input)) {
        return false;
    }
    const UniValue& pastStarts = input[KeyNames::timesPastStarts];
    this->timeStartsInThePast.resize(pastStarts.size());
    for (unsigned i = 0; i < this->timeStartsInThePast.size(); i ++) {
        this->timeStartsInThePast[i] = pastStarts[i].get_int64();
    }
    const UniValue& pastSamplings = input[KeyNames::timesPastSamplings];
    this->timeSamplingsInPast.resize(pastSamplings.size());
    for (unsigned i = 0; i < this->timeSamplingsInPast.size(); i ++) {
        this->timeSamplingsInPast[i] = pastStarts[i].get_int64();
    }
    const UniValue& theStats = input[KeyNames::functionStats];
    bool result = true;
    for (unsigned i = 0; i < theStats.size(); i ++) {
        const std::string& currentName = theStats.getKeys()[i];
        const UniValue& currentStatJSON = theStats.getValues()[i];
        std::shared_ptr<FunctionStats> currentFun = std::make_shared<FunctionStats>();
        if (!currentFun->fromUniValueForStorageNoLock(currentName, currentStatJSON)) {
            result = false;
            //LoggerSession::logProfiling() << LoggerSession::colorRed
            //<< "DEBUG: failed to load: " << currentName << " from: "
            //<< currentStatJSON.write() << LoggerSession::endL;
        } else {
            this->functionStats[currentName] = currentFun;
            //LoggerSession::logProfiling()
            //<< "DEBUG: Function num samples for: " << currentName << ": "
            //<< this->functionStats[currentName]->timeTotalRunTime.numberOfSamples << LoggerSession::endL;
        }
        this->numberOfStatisticsLoaded += currentFun->timeTotalRunTime.numberOfSamples;
    }
    return result;
}

UniValue Profiling::toUniValueForStorageNoLock()
{   //DO not lock please, this function is called from already locking functions.
    //boost::lock_guard<boost::mutex> lockGuard (*this->centralLock);
    UniValue result(UniValue::VOBJ);
    if (!Profiling::fAllowProfiling) {
        result.pushKV ("error", "Profiling is off. Default for testnet and mainnet. "
                                "Override by running fabcoind with -profilingon option. "
                                "Please note that profiling is a introduces timing attacks security risks. "
                                "DO NOT USE profiling on mainnet unless you know what you are doing. ");
        return result;
    }
    UniValue functionStats(UniValue::VOBJ);
    for (auto iterator = this->functionStats.begin(); iterator != this->functionStats.end(); iterator ++) {
        functionStats.pushKV(iterator->first, iterator->second->toUniValueForStorage());
    }
    UniValue timesStart, timesRecordingInPast;
    timesStart.setArray();
    timesRecordingInPast.setArray();
    for (unsigned i = 0; i < this->timeStartsInThePast.size(); i ++) {
        timesStart.push_back(this->timeStartsInThePast[i]);
        timesRecordingInPast.push_back(this->timeSamplingsInPast[i]);
    }
    timesStart.push_back(convertTimePointToIntMilliseconds(this->timeStart));
    timesRecordingInPast.push_back(convertTimePointToIntMilliseconds(std::chrono::system_clock::now()));
    result.pushKV(KeyNames::timesPastStarts, timesStart);
    result.pushKV(KeyNames::timesPastSamplings, timesRecordingInPast);
    result.pushKV(KeyNames::functionStats, functionStats);
    return result;
}

Profiling::Profiling()
{
    this->numberOfStatisticsTakenCurrentSession = 0;
    this->numberStatisticsSinceLastStorage = 0;
    this->numberOfStatisticsLoaded = 0;
    this->nWriteStatisticsToHDEveryThisNumberOfCalls = 1000;
    this->centralLock = std::make_shared<boost::mutex>();
    if (this->fAllowProfiling) {
        LoggerSession::logProfiling() << LoggerSession::colorGreen
        << "Profiler on [default for testnetnodns and regtest]."
        << LoggerSession::colorRed << " To turn off (overriding defaults) use option -profilingoff";
    } else {
        LoggerSession::logProfiling() << LoggerSession::colorRed << "Profiler off [default for testnet and mainnet]."
        << LoggerSession::colorGreen << " To turn on (overriding defaults) use option -profilingon";
        return;
    }
    LoggerSession::logProfiling() << LoggerSession::endL;
    this->timeStart = std::chrono::system_clock::now();
    LoggerSession::logProfiling() << LoggerSession::colorYellow
    << "DO NOT use profiling on mainnet unless you know what are doing. " << LoggerSession::endL;
    this->fileNameStatistics = LoggerSession::baseFolderComputedRunTime + "/profiling_statistics.json";
    std::ifstream statsFile(this->fileNameStatistics);
    LoggerSession::logProfiling()
    << LoggerSession::colorBlue << "Reading profiling stats from previous runs from file: " << this->fileNameStatistics
    << LoggerSession::colorNormal << LoggerSession::endL;
    std::string statisticsRead((std::istreambuf_iterator<char>(statsFile)), std::istreambuf_iterator<char>());
    this->ReadStatistics(statisticsRead);
}

bool Profiling::ReadStatistics(const std::string& input)
{
    UniValue readFromHD;
    if (!readFromHD.read(input)){
        LoggerSession::logProfiling()
        << LoggerSession::colorRed << "Statistics JSON read failed. " << LoggerSession::colorNormal
        << input << LoggerSession::endL;
        return false;
    }
    return this->fromUniValueForStorageNoLock(readFromHD);
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
    if (this->timeTotalRunTime.numberOfSamples % this->recordFinishTimesEveryNCalls != 0) {
        return;
    }
    this->finishTimes.push_back(timeEnd);
    this->finishTimeNumCalls.push_back(this->timeTotalRunTime.numberOfSamples);
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
    Profiling::theProfiler().AccountStat();
}
