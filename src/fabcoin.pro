TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES+=HAVE_CONFIG_H


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
    fabcoin-cli.cpp \
    fabcoin-tx.cpp \
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
    net.cpp \
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
    warnings.cpp

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
    warnings.h
