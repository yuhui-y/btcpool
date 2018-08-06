/*
 The MIT License (MIT)

 Copyright (c) [2016] [BTC.COM]

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
#ifndef STRATUM_H_
#define STRATUM_H_

#include "Common.h"
#include "Utils.h"
#include "Network.h"

// default worker name
#define DEFAULT_WORKER_NAME "__default__"

inline uint32_t jobId2Time(uint64_t jobId)
{
  return (uint32_t)((jobId >> 32) & 0x00000000FFFFFFFFULL);
}

string filterWorkerName(const string &workerName);

inline string filterWorkerName(const char *workerName)
{
  return filterWorkerName(std::string(workerName));
}

////////////////////////////////// FoundBlock //////////////////////////////////
class FoundBlock
{
public:
  uint64_t jobId_;
  int64_t workerId_; // found by who
  int32_t userId_;
  int32_t height_;
  uint8_t header80_[80];
  char workerFullName_[40]; // <UserName>.<WorkerName>

  FoundBlock() : jobId_(0), workerId_(0), userId_(0), height_(0)
  {
    memset(header80_, 0, sizeof(header80_));
    memset(workerFullName_, 0, sizeof(workerFullName_));
  }
};

//////////////////////////////// StratumError ////////////////////////////////
class StratumStatus
{
public:
  enum
  {
    // make ACCEPT and SOLVED be two singular value,
    // so code bug is unlikely to make false ACCEPT shares

    // share reached the job target (but may not reached the network target)
    ACCEPT = 1798084231, // bin(01101011 00101100 10010110 10000111)

    // share reached the job target but the job is stale
    // if uncle block is allowed in the chain, share can be accept as this status
    ACCEPT_STALE = 950395421, // bin(00111000 10100101 11100010 00011101)

    // share reached the network target
    SOLVED = 1422486894, // bin(‭01010100 11001001 01101101 01101110‬)

    // share reached the network target but the job is stale
    // if uncle block is allowed in the chain, share can be accept as this status
    SOLVED_STALE = 1713984938, // bin(01100110 00101001 01010101 10101010)

    REJECT_NO_REASON = 0,

    JOB_NOT_FOUND = 21,
    DUPLICATE_SHARE = 22,
    LOW_DIFFICULTY = 23,
    UNAUTHORIZED = 24,
    NOT_SUBSCRIBED = 25,

    ILLEGAL_METHOD = 26,
    ILLEGAL_PARARMS = 27,
    IP_BANNED = 28,
    INVALID_USERNAME = 29,
    INTERNAL_ERROR = 30,
    TIME_TOO_OLD = 31,
    TIME_TOO_NEW = 32,

    UNKNOWN = 2147483647 // bin(01111111 11111111 11111111 11111111)
  };

  static const char *toString(int err);
  
  inline static bool isAccepted(int status) {
    return (status == ACCEPT) || (status == ACCEPT_STALE) ||
           (status == SOLVED) || (status == SOLVED_STALE);
  }

  inline static bool isStale(int status) {
    return (status == ACCEPT_STALE) || (status == SOLVED_STALE);
  }

  inline static bool isSolved(int status) {
    return (status == SOLVED) || (status == SOLVED_STALE);
  }
};

///////////////////////////////////// ShareBase ////////////////////////////////////
// Class ShareBase should not be used directly.
// Use a derived class of class ShareBase (example: ShareBitcoin, ShareEth).
// Also, keep class ShareBase plain, don't add any virtual functions.
// Otherwise, derived classes will not be able to use byte-based
// object serialization (because of the addition of a virtual function table).
class ShareBase
{
public:
  
  uint32_t  version_      = 0;
  uint32_t  checkSum_     = 0;
  int64_t   workerHashId_ = 0;
  int32_t   userId_       = 0;
  int32_t   status_       = 0;
  int64_t   timestamp_    = 0;
  IpAddress ip_           = 0;

protected:
  ShareBase() = default;
  ShareBase(const ShareBase &r) = default;
  ShareBase &operator=(const ShareBase &r) = default;
};

//////////////////////////////// StratumWorker ////////////////////////////////
class StratumWorker
{
public:
  int32_t userId_;
  int64_t workerHashId_; // substr(0, 8, HASH(wokerName))

  string fullName_; // fullName = username.workername
  string userName_;
  string workerName_; // workername, max is: 20

  void reset();

public:
  StratumWorker();
  void setUserIDAndNames(const int32_t userId, const string &fullName);
  string getUserName(const string &fullName) const;

  static int64_t calcWorkerId(const string &workerName);
};

////////////////////////////////// StratumJob //////////////////////////////////
//
// Stratum Job
//
// https://slushpool.com/help/#!/manual/stratum-protocol
//
// "mining.notify"
//
// job_id   - ID of the job. Use this ID while submitting share generated
//             from this job.
// prevhash - Hash of previous block.
// coinb1   - Initial part of coinbase transaction.
// coinb2   - Final part of coinbase transaction.
// merkle_branch - List of hashes, will be used for calculation of merkle root.
//                 This is not a list of all transactions, it only contains
//                 prepared hashes of steps of merkle tree algorithm.
// version    - Bitcoin block version.
// nbits      - Encoded current network difficulty
// ntime      - Current ntime
// clean_jobs - When true, server indicates that submitting shares from previous
//              jobs don't have a sense and such shares will be rejected. When
//              this flag is set, miner should also drop all previous jobs,
//              so job_ids can be eventually rotated.
//
//
class StratumJob
{
public:
  // jobId: timestamp + gbtHash, hex string, we need to make sure jobId is
  // unique in a some time, jobId can convert to uint64_t
  uint64_t jobId_;
  int32_t height_;
  int32_t nVersion_;
  uint32_t nTime_;
protected:
  StratumJob(); //  protected so cannot create it.
public:
  virtual ~StratumJob();

  virtual string serializeToJson() const = 0;
  virtual bool unserializeFromJson(const char *s, size_t len) = 0;
  virtual uint32 jobTime() const { return jobId2Time(jobId_); }

};

#endif
