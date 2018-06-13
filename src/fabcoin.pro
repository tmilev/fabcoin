TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES+=HAVE_CONFIG_H
INCLUDEPATH += \
univalue/include/ \
leveldb/include/ \
leveldb/helpers/memenv/ \
secp256k1/include/

LIBS+= \
-lpthread \
-lboost_filesystem \
-lboost_system \
-lboost_thread


LIBS += \
-Lleveldb/ -lleveldb \
-L$$PWD/leveldb/ -lmemenv



#qmake command:
#qmake fabcoin.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug


#-L./leveldb/ -llibleveldb_sse42 \

#-lboost_system -lboost_chrono  -lboost_timer


#    fabcoin-tx.cpp \
#    fabcoin-cli.cpp \
SOURCES += \
    addrdb.cpp \
    addrman.cpp \
    arith_uint256.cpp \
    base58.cpp \
    blockencodings.cpp \
    bloom.cpp \
    chain.cpp \
    chainparams.cpp \
    chainparamsbase.cpp \
    checkpoints.cpp \
    clientversion.cpp \
    coins.cpp \
    compressor.cpp \
    core_read.cpp \
    core_write.cpp \
    dbwrapper.cpp \
    fabcoind.cpp \
    fs.cpp \
    hash.cpp \
    httprpc.cpp \
    httpserver.cpp \
    init.cpp \
    key.cpp \
    keystore.cpp \
    merkleblock.cpp \
    miner.cpp \
    net_processing.cpp \
#    net.cpp \
    netaddress.cpp \
    netbase.cpp \
    noui.cpp \
    pow.cpp \
    protocol.cpp \
    pubkey.cpp \
    random.cpp \
    rest.cpp \
    scheduler.cpp \
    sync.cpp \
    threadinterrupt.cpp \
    timedata.cpp \
    torcontrol.cpp \
    txdb.cpp \
    txmempool.cpp \
    ui_interface.cpp \
    uint256.cpp \
    util.cpp \
    utilmoneystr.cpp \
    utilstrencodings.cpp \
    utiltime.cpp \
    validation.cpp \
    validationinterface.cpp \
    versionbits.cpp \
    warnings.cpp \
    crypto/aes.cpp \
    crypto/chacha20.cpp \
    crypto/equihash.cpp \
    crypto/equihash.tcc \
    crypto/hmac_sha256.cpp \
    crypto/hmac_sha512.cpp \
    crypto/ripemd160.cpp \
    crypto/sha1.cpp \
    crypto/sha256_sse4.cpp \
    crypto/sha256.cpp \
    crypto/sha512.cpp \
    support/cleanse.cpp \
    support/lockedpool.cpp \
    consensus/merkle.cpp \
    consensus/tx_verify.cpp \
    script/fabcoinconsensus.cpp \
    script/interpreter.cpp \
    script/ismine.cpp \
    script/script_error.cpp \
    script/script.cpp \
    script/sigcache.cpp \
    script/sign.cpp \
    script/standard.cpp \
    primitives/block.cpp \
    primitives/transaction.cpp \
    univalue/lib/univalue_read.cpp \
    univalue/lib/univalue_write.cpp \
    univalue/lib/univalue.cpp \
    rpc/blockchain.cpp \
    rpc/client.cpp \
    rpc/mining.cpp \
    rpc/misc.cpp \
    rpc/net.cpp \
#    rpc/protocol.cpp \
    rpc/rawtransaction.cpp \
    rpc/server.cpp \
    genesis.cpp \
    miscellaneous.cpp


SUBDIRS += \
    fabcoin.pro

HEADERS += \
    addrdb.h \
    addrman.h \
    amount.h \
    arith_uint256.h \
    base58.h \
    blockencodings.h \
    bloom.h \
    chain.h \
    chainparams.h \
    chainparamsbase.h \
    chainparamsseeds.h \
    checkpoints.h \
    checkqueue.h \
    clientversion.h \
    coins.h \
    compat.h \
    compressor.h \
    core_io.h \
    core_memusage.h \
    cuckoocache.h \
    dbwrapper.h \
    fabcoin-cli-res.rc \
    fabcoin-tx-res.rc \
    fabcoind-res.rc \
    fs.h \
    hash.h \
    httprpc.h \
    httpserver.h \
    indirectmap.h \
    init.h \
    key.h \
    keystore.h \
    limitedmap.h \
    memusage.h \
    merkleblock.h \
    miner.h \
    net_processing.h \
    net.h \
    netaddress.h \
    netbase.h \
    netmessagemaker.h \
    noui.h \
    pow.h \
    prevector.h \
    protocol.h \
    pubkey.h \
    random.h \
    reverse_iterator.h \
    reverselock.h \
    scheduler.h \
    serialize.h \
    streams.h \
    sync.h \
    threadinterrupt.h \
    threadsafety.h \
    timedata.h \
    tinyformat.h \
    torcontrol.h \
    txdb.h \
    txmempool.h \
    ui_interface.h \
    uint256.h \
    undo.h \
    util.h \
    utilmoneystr.h \
    utilstrencodings.h \
    utiltime.h \
    validation.h \
    validationinterface.h \
    version.h \
    versionbits.h \
    warnings.h \
    crypto/aes.h \
    crypto/chacha20.h \
    crypto/common.h \
    crypto/equihash.h \
    crypto/hmac_sha256.h \
    crypto/hmac_sha512.h \
    crypto/ripemd160.h \
    crypto/sha1.h \
    crypto/sha256.h \
    crypto/sha512.h \
    support/cleanse.h \
    support/events.h \
    support/lockedpool.h \
    consensus/consensus.h \
    consensus/merkle.h \
    consensus/params.h \
    consensus/tx_verify.h \
    consensus/validation.h \
    script/fabcoinconsensus.h \
    script/interpreter.h \
    script/ismine.h \
    script/script_error.h \
    script/script.h \
    script/sigcache.h \
    script/sign.h \
    script/standard.h \
    primitives/block.h \
    primitives/transaction.h \
    univalue/lib/univalue_escapes.h \
    univalue/lib/univalue_utffilter.h \
    leveldb/db/builder.h \
    leveldb/db/db_impl.h \
    leveldb/db/db_iter.h \
    leveldb/db/dbformat.h \
    leveldb/db/filename.h \
    leveldb/db/log_format.h \
    leveldb/db/log_reader.h \
    leveldb/db/log_writer.h \
    leveldb/db/memtable.h \
    leveldb/db/skiplist.h \
    leveldb/db/snapshot.h \
    leveldb/db/table_cache.h \
    leveldb/db/version_edit.h \
    leveldb/db/version_set.h \
    leveldb/db/write_batch_internal.h \
    rpc/blockchain.h \
    rpc/client.h \
    rpc/mining.h \
    rpc/protocol.h \
    rpc/register.h \
    rpc/server.h \
    genesis.h \
    miscellaneous.h
