#ifndef PROFILING_H_header
#define PROFILING_H_header

#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/thread.hpp>
#include "../univalue/include/univalue.h"
#include <chrono>
#include <deque>

class Statistic
{
public:
    //Assumptions:
    //We are making statistics for non-negative integers.
    //Original use case: statistics in microseconds.
    std::string name;
    int numberOfSamples;
    long total;
    bool fHistogramInitialized;
    unsigned int desiredNumberOfSampleMeasurementsBeforeWeSetupTheHistogram;
    unsigned int desiredNumberOfHistogramBucketsMinusOne;
    unsigned int numCallsUpdateHistogramRecursively;
    double meanUsedToComputeBuckets;
    int desiredIntervalSize;
    std::vector<int> sampleMeasurements;
    std::vector<int> histogramIntervals;
    //<- If the elements of histogramIntervals are x_0, x_2, ... , x_{N-1},
    // then the buckets of the histogram are:
    // (-\infty, x_0], (x_0, x_1], (x_1, x_2], ..., (x_{N-2}, x_{N-1}], (x_{N-1}, \infty)
    std::unordered_map<int, long> histogram;
    double standardDeviationNoBesselCorrection();
    UniValue toUniValue() const;
    UniValue toUniValueHistogram() const;
    UniValue toUniValueHistogramForStorage() const;
    UniValue toUniValueForStorage() const;
    bool fromUniValueForStorage(const UniValue& input);
    bool fromUniValueForStorageHistogram(const UniValue& inpuT);
    double computeMean();
    bool isInHistogramBucketRange(int value, unsigned int firstPossibleBucketIndex, unsigned int lastPossibleBucketIndex);
    bool isInHistogramBucketRangeLeft(int value, unsigned int firstPossibleBucketIndex);
    bool isInHistogramBucketRangeRight(int value, unsigned int lastPossibleBucketIndex);
    void updateHistogram(int value);
    void updateHistogramRecursively(
        int value,
        unsigned int firstPossibleBucketIndex,
        unsigned int lastPossibleBucketIndex,
        int recursionDepth
    );
    void accountToHistogram(unsigned int index);
    void accountStatistic(int value);
    void initialize(const std::string& inputName, int inputDesiredNumberOfSamplesBeforeWeSetupHistogram);
    void initializeHistogramIfPossible();
    void initializeHistogramFromMean(double inputMean);
    Statistic();
};

class FunctionStats
{
public:
    std::string name;
    int recordFinishTimesEveryNCalls;
    std::deque<std::chrono::system_clock::time_point> finishTimes;
    std::deque<int> finishTimeNumCalls;
    Statistic timeSubordinates;
    Statistic timeTotalRunTime;
    void initialize(const std::string& inputName, int inputRecordFinishTimesEveryNCalls, int numSamplesTomComputeMean);
    void accountFinishTime(long inputDuration, long inputRunTimeSubordinates, const std::chrono::system_clock::time_point& timeEnd);
    //Not thread safe:
    UniValue toUniValue() const;
    UniValue toUniValueForStorage() const;
    bool fromUniValueForStorageNoLock(const std::string &inputName, const UniValue& input);
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
    /**
     * Name is the string used to construct an identifier of the function.
     * Each FunctionProfile is identified via:
     *
     * name0->name1->...->nameN->name,
     *
     * where name is the input given in the constructor, and
     * name0, ..., nameN are the names of all FunctionProfiles are still exist on the stack.
     * The stack names name0, ..., nameN names are fetched from the Profiling::Profiling() global object.
     *
     * Different calls of FunctionProfile are expected to use different names.
     * Names are expected to not contain the - or > symbols.
     *
     *
     * recordFinishTimesEveryNCalls tells the profiler to record the universal time
     * during which the function call was completed every recordFinishTimesEveryNCalls calls of the function.
     *
     * If Profiling::fAllowFinishTimeProfiling is set to false
     * (defaults: false on mainnet and testnet, true on testnetnodns and regtest),
     * then recordFinishTimesEveryNCalls will be ignored.
     * If Profiling::fAllowFinishTimeProfiling is set to true, then recordFinishTimesEveryNCalls
     * will be processed as follows.
     *
     * If recordFinishTimesEveryNCalls is zero or negative, no times are recorded.
     * If recordFinishTimesEveryNCalls is 1, all function call times will be recorded (with old recordings pruned).
     * If recordFinishTimesEveryNCalls is, say, 10, the function call times will be recorded on
     * call 10th, 20th, 30th, ... calls (old recordings pruned).
     *
     * Only the latest Profiling::nMaxNumberFinishTimes finish times are recorded,
     * all earlier finish times will be discarded.
     *
     * Original use case: we are using this to time actions accross a network: say, compare the
     * completion times of the 1000th call of AcceptToMemoryPoolWorker
     * on one machine to the 1000th call of AcceptToMemoryPoolWorker on
     * a different machine in the network.
     */
    FunctionProfile(const std::string& name, int recordFinishTimesEveryNCalls, int numSamplesToComputeMean);
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
    static bool fAllowFinishTimeProfiling;
    static bool fAllowTxIdReceiveTimeLogging;
    static unsigned int nMaxNumberFinishTimes;
    static unsigned int nMaxNumberTxsToAccount;
    /** Returns a global profiling object.
     * Avoids the static initalization order fiasco.
     */
    static Profiling& theProfiler();
    std::shared_ptr<boost::mutex> centralLock;
    UniValue statisticsPersistent;
    std::deque<std::string> memoryPoolAcceptanceTimeKeys;
    std::string fileNameStatistics;
    std::vector<int64_t> timeStartsInThePast;
    std::vector<int64_t> timeSamplingsInPast;
    std::chrono::time_point<std::chrono::system_clock> timeStart;
    std::unordered_map<std::string, std::chrono::time_point<std::chrono::system_clock> > memoryPoolAcceptanceTimes;
    long numberOfStatisticsTakenCurrentSession;
    int numberStatisticsSinceLastStorage;
    int numberOfStatisticsLoaded;
    int nWriteStatisticsToHDEveryThisNumberOfCalls;

    //map from thread id to a stack containing the names of the profiled functions.
    std::unordered_map<std::string, std::shared_ptr<FunctionStats> > functionStats;
    std::unordered_map<unsigned long, std::shared_ptr<std::vector<FunctionProfileData> > > threadStacks;
    bool ReadStatistics(const std::string& input);
    bool WriteStatistics();
    void AccountStat();
    UniValue toUniValueForBrowser();
    bool fromUniValueForStorageNoLock(const UniValue& input);
    UniValue toUniValueForStorageNoLock();
    UniValue toUniValueMemoryPoolAcceptanceTimes();
    void RegisterReceivedTxId(const std::string& txId);
};

#endif // PROFILING_H_header

