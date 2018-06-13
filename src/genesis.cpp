#include "genesis.h"
#include "crypto/equihash.h"
#include "streams.h"
#include <iostream>
#include "util.h"
#include "logging.h"
#include "miscellaneous.h"

extern LoggerSession& logBeforeInitialization();

void genesis_Scan_nNonce_nSolution(CBlock *pblock, unsigned int n, unsigned int k)
{
    logBeforeInitialization() << "Scanning for genesis block solutions." << LoggerSession::endL;
    bool cancelSolver = false;
    uint64_t nMaxTries = 0;

    //
    // Search
    //
    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
    crypto_generichash_blake2b_state stateNoNonce, currentState;
    EhInitialiseState(n, k, stateNoNonce);
    // I = the block header minus nonce and solution.
    CEquihashInput I{*pblock};
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << I;
    crypto_generichash_blake2b_update(&stateNoNonce, (unsigned char*) &ss[0], ss.size());

    while (true) {
        nMaxTries++;
        // Hash state

        // H(I||...

        // H(I||V||...
        currentState = stateNoNonce;
        crypto_generichash_blake2b_update(&currentState, pblock->nNonce.begin(), pblock->nNonce.size());

        // (x_1, x_2, ...) = A(I, V, n, k)
        //  LogPrint(BCLog::POW, "Running Equihash solver \"%s\" with nNonce = %s\n",
        //      solver, pblock->nNonce.ToString());

        std::function<bool(std::vector<unsigned char>) > validBlock =
                [&pblock, &hashTarget, &cancelSolver](std::vector<unsigned char> solution) {
                    // Write the solution to the hash and compute the result.
                    pblock->nSolution = solution;
                    arith_uint256 hashAttained = UintToArith256(pblock->GetHashBeforeParamInitialization());
                    logBeforeInitialization() << "Checking solution: "  << Miscellaneous::toStringVectorHex(solution) << LoggerSession::endL;
                    logBeforeInitialization() << "hash: target, attained:" << LoggerSession::endL;
                    logBeforeInitialization() << hashTarget.ToString() << LoggerSession::endL;
                    logBeforeInitialization() << hashAttained.ToString() << LoggerSession::endL;
                    if (hashAttained > hashTarget) {
                        logBeforeInitialization() << LoggerSession::colorYellow << "Solution rejected (too large). " << LoggerSession::endL;
                        return false;
                    }
                    // Found a solution
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    logBeforeInitialization() << LoggerSession::colorGreen
                    << "The solution is accepted. " << LoggerSession::endL;
                    return true;
                };

        std::function<bool(EhSolverCancelCheck) > cancelled = [&cancelSolver](EhSolverCancelCheck pos) {
            return cancelSolver;
        };

        try {
            // If we find a valid block, we rebuild
            bool found = EhOptimisedSolve(n, k, currentState, validBlock, cancelled);
            if (found) {
                logBeforeInitialization() << "Rebuilding block with the found solution. " << LoggerSession::endL
                << "hash: target, attained:" << LoggerSession::endL
                << hashTarget.GetHex() << LoggerSession::endL
                << pblock->GetHashBeforeParamInitialization().GetHex() << LoggerSession::endL;
                logBeforeInitialization() << "Block: "
                << pblock->ToString() << LoggerSession::endL;
                break;
            }
        } catch (EhSolverCancelledException&) {
            logBeforeInitialization() << "Equihash solver cancelled" << LoggerSession::endL;
            cancelSolver = false;
        }
        // Update nNonce
        pblock->nNonce = ArithToUint256(UintToArith256(pblock->nNonce) + 1);
    }
}
