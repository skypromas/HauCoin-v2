// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_H
#define BITCOIN_TXDB_H

#include <coins.h>
#include <dbwrapper.h>
#include <chain.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CBlockIndex;
class CCoinsViewDBCursor;
class uint256;

//! No need to periodic flush if at least this much space still available.
static constexpr int MAX_BLOCK_COINSDB_USAGE = 10;
//! -dbcache default (MiB)
static  int64_t nDefaultDbCache = 450;
//! -dbbatchsize default (bytes)
static  int64_t nDefaultDbBatchSize = 16 << 20;
//! max. -dbcache (MiB)
static  int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static  int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static  int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
static  int64_t nMaxBlockDBAndTxIndexCache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static  int64_t nMaxCoinsDBCache = 8;

struct CDiskTxPos : public CDiskBlockPos
{
    unsigned int nTxOffset; // after header

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CDiskBlockPos*)this);
        READWRITE(VARINT(nTxOffset));
    }

    CDiskTxPos( CDiskBlockPos &blockIn, unsigned int nTxOffsetIn) : CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
        nTxOffset = 0;
    }
};

/** CCoinsView backed by the coin database (chainstate/) */
class CCoinsViewDB final : public CCoinsView
{
protected:
    CDBWrapper db;
public:
    explicit CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoin( COutPoint &outpoint, Coin &coin)  override;
    bool HaveCoin( COutPoint &outpoint)  override;
    uint256 GetBestBlock()  override;
    std::vector<uint256> GetHeadBlocks()  override;
    bool BatchWrite(CCoinsMap &mapCoins,  uint256 &hashBlock) override;
    CCoinsViewCursor *Cursor()  override;

    //! Attempt to update from an older database format. Returns whether an error occurred.
    bool Upgrade();
    size_t EstimateSize()  override;
};

/** Specialization of CCoinsViewCursor to iterate over a CCoinsViewDB */
class CCoinsViewDBCursor: public CCoinsViewCursor
{
public:
    ~CCoinsViewDBCursor() {}

    bool GetKey(COutPoint &key)  override;
    bool GetValue(Coin &coin)  override;
    unsigned int GetValueSize()  override;

    bool Valid()  override;
    void Next() override;

private:
    CCoinsViewDBCursor(CDBIterator* pcursorIn,  uint256 &hashBlockIn):
        CCoinsViewCursor(hashBlockIn), pcursor(pcursorIn) {}
    std::unique_ptr<CDBIterator> pcursor;
    std::pair<char, COutPoint> keyTmp;

    friend class CCoinsViewDB;
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CDBWrapper
{
public:
    explicit CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CBlockTreeDB( CBlockTreeDB&) = delete;
    CBlockTreeDB& operator=( CBlockTreeDB&) = delete;

    bool WriteBatchSync( std::vector<std::pair<int,  CBlockFileInfo*> >& fileInfo, int nLastFile,  std::vector< CBlockIndex*>& blockinfo);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &info);
    bool ReadLastBlockFile(int &nFile);
    bool WriteReindexing(bool fReindexing);
    bool ReadReindexing(bool &fReindexing);
    bool ReadTxIndex( uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex( std::vector<std::pair<uint256, CDiskTxPos> > &vect);
    bool WriteFlag( std::string &name, bool fValue);
    bool ReadFlag( std::string &name, bool &fValue);
    bool LoadBlockIndexGuts( Consensus::Params& consensusParams, std::function<CBlockIndex*( uint256&)> insertBlockIndex);
};

#endif // BITCOIN_TXDB_H
