// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_CHAINPARAMS_H
#define FABCOIN_CHAINPARAMS_H

#include "chainparamsbase.h"
#include "consensus/params.h"
#include "primitives/block.h"
#include "protocol.h"
#include "validation.h"

#include <memory>
#include <vector>

struct CDNSSeedData {
    std::string host;
    bool supportsServiceBitsFiltering;
    CDNSSeedData(const std::string &strHost, bool supportsServiceBitsFilteringIn) : host(strHost), supportsServiceBitsFiltering(supportsServiceBitsFilteringIn) {}
};

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;
};

struct ChainTxData {
    int64_t nTime;
    int64_t nTxCount;
    double dTxRate;
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Fabcoin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    int GetDefaultPort() const { return nDefaultPort; }

    const CBlock& GenesisBlock() const { return genesis; }
    CBlock& GenesisBlockNON_CONST() { return genesis; }
    /** Makes script error messages more descriptive.
     * Use in regtest and testnetNoDNS only.
     * Rationale: mallicious user input may end up in the descriptive error messages.
     * This may end up being a security vulnerability for a non-careful user of the
     * outputs of the rpc calls.
     */
    int DefaultDescendantLimit() const {
        if (this->nDefaultDescendantLimit > 0)
            return this->nDefaultDescendantLimit;
        return DEFAULT_DESCENDANT_LIMIT;
    }
    int DefaultDescendantSizeLimit() const {
        if (this->nDefaultDescendantSizeLimit > 0)
            return this->nDefaultDescendantSizeLimit * 1000;
        return DEFAULT_DESCENDANT_SIZE_LIMIT * 1000;
    }
    bool TxIdReceiveTimeLoggingAllowed() const {return this->fTxIdReceiveTimeLoggingAllowed; }
    bool ProfilingRecommended() const {return this->fProfilingRecommended; }
    bool AllowDebugInfo() const {return this->fDebugInfoRegtestAndTestNetNoDNSOnly; }
    bool AllowExtraErrorStreamUseInRegTestAndTestNetNoDNSOnly() const {return this->fAllowExtraErrorStreamUseInRegtestAndTestNetNoDNSOnly; }
    /** Make miner wait to have peers to avoid wasting work */
    bool MiningRequiresPeers() const { return fMiningRequiresPeers; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    unsigned int EquihashN() const { return nEquihashN; }
    unsigned int EquihashK() const { return nEquihashK; }
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::vector<SeedSpec6>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const ChainTxData& TxData() const { return chainTxData; }
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout);
protected:
    CChainParams() {}

    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    int nDefaultPort;
    uint64_t nPruneAfterHeight;
    unsigned int nEquihashN = 0;
    unsigned int nEquihashK = 0;
    /** If positive, nDefaultDescendantLimit overrides DEFAULT_DESCENDANT_LIMIT
     *  In turn, nDefaultDescendantLimit is overriden by gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
     *  See also validation.cpp
     */
    int nDefaultDescendantLimit = 0;
    int nDefaultDescendantSizeLimit = 0;
    std::vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string strNetworkID;
    CBlock genesis;
    std::vector<SeedSpec6> vFixedSeeds;
    bool fMiningRequiresPeers;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool fMineBlocksOnDemand;
    bool fAllowExtraErrorStreamUseInRegtestAndTestNetNoDNSOnly;
    bool fDebugInfoRegtestAndTestNetNoDNSOnly;
    bool fProfilingRecommended;
    bool fTxIdReceiveTimeLoggingAllowed;
    CCheckpointData checkpointData;
    ChainTxData chainTxData;
};

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 * @returns a CChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();
const CChainParams &GetParams();
bool ParamsAreInitialized();
/**
 * Sets the params returned by Params() to those for the given BIP70 chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);

/**
 * Allows modifying the Version Bits regtest parameters.
 */
void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout);

#endif // FABCOIN_CHAINPARAMS_H
