#include "genesis.h"
#include "crypto/equihash.h"
#include "streams.h"
#include <iostream>
#include "util.h"

void Scan_nNonce_nSolution(CBlock *pblock, unsigned int n, unsigned int k )
{
    bool cancelSolver = false;
    uint64_t nMaxTries = 0;

    //
    // Search
    //
    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
    uint256 hash;
    std::cout << "DEBUG: Scanning for solution. " << std::endl;
    while (true) {
        nMaxTries++;
        // Hash state
        crypto_generichash_blake2b_state state;
        EhInitialiseState(n, k, state);

        // I = the block header minus nonce and solution.
        CEquihashInput I{*pblock};
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << I;

        // H(I||...
        crypto_generichash_blake2b_update(&state, (unsigned char*) &ss[0], ss.size());

        // H(I||V||...
        crypto_generichash_blake2b_state curr_state;
        curr_state = state;
        crypto_generichash_blake2b_update(&curr_state, pblock->nNonce.begin(), pblock->nNonce.size());

        // (x_1, x_2, ...) = A(I, V, n, k)
        //  LogPrint(BCLog::POW, "Running Equihash solver \"%s\" with nNonce = %s\n",
        //      solver, pblock->nNonce.ToString());

        std::function<bool(std::vector<unsigned char>) > validBlock =
                [&pblock, &hashTarget, &cancelSolver ](std::vector<unsigned char> soln) {
                    // Write the solution to the hash and compute the result.
                    LogPrint(BCLog::POW, "- Checking solution against target\n");
                    pblock->nSolution = soln;

                    if (UintToArith256(pblock->GetHash()) > hashTarget) {
                        return false;
                    }

                    // Found a solution
                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    LogPrintf("Scan_nNonce:\n");
                    LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", pblock->GetHash().GetHex(), hashTarget.GetHex());

                    return true;
                };

        std::function<bool(EhSolverCancelCheck) > cancelled = [ &cancelSolver](EhSolverCancelCheck pos) {
            return cancelSolver;
        };

        try {
            // If we find a valid block, we rebuild
            bool found = EhOptimisedSolve(n, k, curr_state, validBlock, cancelled);
            if (found) {
                LogPrintf("FabcoinMiner:\n");
                LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", pblock->GetHash().GetHex(), hashTarget.GetHex());
                LogPrintf("Block ------------------ \n%s\n ----------------", pblock->ToString());

                break;
            }
        } catch (EhSolverCancelledException&) {
            LogPrint(BCLog::POW, "Equihash solver cancelled\n");
            cancelSolver = false;
        }

        // Update nNonce and nTime
        pblock->nNonce = ArithToUint256(UintToArith256(pblock->nNonce) + 1);
    }
}
