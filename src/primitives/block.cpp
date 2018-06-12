// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "chainparams.h"
#include "consensus/params.h"
#include "crypto/common.h"

uint256 CBlockHeader::GetHash(const Consensus::Params& params) const
{
    int version;
    std::cout << "DEBUG: Getting hash. " << std::endl;

    if ((uint32_t) -1 == (uint32_t)params.FABHeight ) {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }
    else if (nHeight >= (uint32_t)params.FABHeight) {
        version = PROTOCOL_VERSION;
    } else {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }
    CHashWriter writer(SER_GETHASH, version);
    ::Serialize(writer, *this);
    return writer.GetHash();
}

uint256 CBlockHeader::GetHash() const
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    return GetHash(consensusParams);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    std::cout << "DEBUG: GOT TO before solution" << std::endl;
    std::string solutionString = HexStr(nSolution.begin(), nSolution.end());
    std::cout << "DEBUG: GOT TO HERE" << std::endl;

    std::string hashString = "";
    if (ParamsAreInitialized())
        hashString = GetHash().ToString();
    else
        hashString = "(params not initialized)";
    std::cout << "DEBUG: computed hash string:" << hashString << std::endl;

    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nHeight=%u, nTime=%u, nBits=%08x, nNonce=%s, nSolution=%s, vtx=%u)\n",
        hashString,
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nHeight, nTime, nBits, nNonce.GetHex(),
        solutionString,
        vtx.size());
    std::cout << "DEBUG: and got t TO HERE" << std::endl;
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    std::cout << "DEBUG: about to return :" << s.str() << std::endl;
    return s.str();
}

