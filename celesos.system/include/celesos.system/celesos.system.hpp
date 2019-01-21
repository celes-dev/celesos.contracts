/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <celesos.system/native.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <celesos.system/exchange_state.hpp>

#include <string>

#define DPAY_POOL_FULL static_cast<uint64_t>(21 * 10000 * 10000) * 3000
#define BPAY_POOL_FULL static_cast<uint64_t>(21 * 10000 * 10000) * 1500
#define WPAY_POOL_FULL static_cast<uint64_t>(21 * 10000 * 10000) * 1500

// origin reward number (初始出块奖励，折半衰减）
#define ORIGIN_REWARD_NUMBER_BPAY 5000
#define ORIGIN_REWARD_NUMBER_WPAY 5000
#define ORIGIN_REWARD_NUMBER_DPAY 5000

// #ifdef DEBUG

#define TARGET_WOOD_NUMBER 300

#define DAPP_PAY_UNACTIVE 1000 * 10000
// number of bp,BP个数
#define BP_COUNT 9
// when the bp count is ok cycle for this number,the active the network(主网启动条件，BP个数达标轮数）
#define ACTIVE_NETWORK_CYCLE 24
// reward get min（if smaller than this number，you can't get the reward）最小奖励领取数，低于此数字将领取失败
#define REWARD_GET_MIN 1000000
// get reward time sep(奖励领取间隔时间，单位：秒）
#define REWARD_TIME_SEP 6 * 60 * 60 * uint64_t(1000000)
// singing ticker sep（唱票间隔期，每隔固定时间进行唱票）
#define SINGING_TICKER_SEP BP_COUNT * 6 * 60

#define DBP_ACTIVE_SEP 3 * 24 * 60 * 60 * 2

// #else

// #define TARGET_WOOD_NUMBER 300

//#define DAPP_PAY_UNACTIVE 1000 * 10000
// // number of bp,BP个数
// #define BP_COUNT 21
// // when the bp count is ok cycle for this number,the active the network(主网启动条件，BP个数达标轮数）
// #define ACTIVE_NETWORK_CYCLE 24
// // reward get min（if smaller than this number，you can't get the reward）最小奖励领取数，低于此数字将领取失败
// #define REWARD_GET_MIN 1000000
// // get reward time sep(奖励领取间隔时间，单位：秒）
// #define REWARD_TIME_SEP 6 * 60 * 60 * uint64_t(1000000)
// // singing ticker sep（唱票间隔期，每隔固定时间进行唱票）
// #define SINGING_TICKER_SEP BP_COUNT * 6 * 60
// #define DBP_ACTIVE_SEP 180 * 24 * 60 * 60 * 2


// #endif

namespace celesos
{

using eosio::asset;
using eosio::block_timestamp;
using eosio::const_mem_fun;
using eosio::datastream;
using eosio::indexed_by;
using eosio::microseconds;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::time_point;

struct [[ eosio::table, eosio::contract("celesos.system") ]] name_bid
{
    name newname;
    name high_bidder;
    int64_t high_bid = 0; ///< negative high_bid == closed auction waiting to be claimed
    time_point last_bid_time;

    uint64_t primary_key() const { return newname.value; }
    uint64_t by_high_bid() const { return static_cast<uint64_t>(-high_bid); }
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] bid_refund
{
    name bidder;
    asset amount;

    uint64_t primary_key() const { return bidder.value; }
};

typedef eosio::multi_index<"namebids"_n, name_bid,
                           indexed_by<"highbid"_n, const_mem_fun<name_bid, uint64_t, &name_bid::by_high_bid>>>
    name_bid_table;

typedef eosio::multi_index<"bidrefunds"_n, bid_refund> bid_refund_table;

struct[[ eosio::table("global"), eosio::contract("celesos.system") ]] eosio_global_state : eosio::blockchain_parameters
{
    uint64_t free_ram() const { return max_ram_size - total_ram_bytes_reserved; }

    uint64_t max_ram_size = 64ll * 1024 * 1024 * 1024;
    uint64_t total_ram_bytes_reserved = 0;
    int64_t total_ram_stake = 0;

    uint32_t last_producer_schedule_block;
    uint64_t total_unpaid_block_fee = 0; /// all blocks which have been produced but not paid
    uint32_t total_unpaid_wood = 0;
    uint16_t last_producer_schedule_size = 0;
    block_timestamp last_name_close;
    bool is_network_active = false;
    uint16_t active_touch_count = 0;
    uint64_t last_account = 0;
    uint32_t network_active_block = 0;

    uint16_t new_ram_per_block = 1024;
    block_timestamp last_ram_increase;
    block_timestamp last_block_num;
    double reserved = 0;
    uint8_t revision = 0; ///< used to track version updates in the future.

    uint32_t total_wood = 0;
    uint32_t total_dbp_count = 0;
    bool is_dbp_active = false;
    uint32_t dbp_active_block = 0;

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE_DERIVED(eosio_global_state, eosio::blockchain_parameters,
                             (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)
                             (last_producer_schedule_block)(total_unpaid_block_fee)(total_unpaid_wood)
                             (last_producer_schedule_size)(last_name_close)(is_network_active)(active_touch_count)(last_account)(network_active_block)
                             (new_ram_per_block)(last_ram_increase)(last_block_num)(reserved)(revision)(total_wood)(total_dbp_count)(is_dbp_active)(dbp_active_block)
    )
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] producer_info
{
    name owner;
    uint32_t valid_woods = 0;
    eosio::public_key producer_key; /// a packed public key object
    bool is_active = true;
    std::string url;
    uint16_t location = 0;
    uint32_t unpaid_block_fee = 0;
    uint32_t unpaid_wood = 0;
    time_point last_claim_time;

    uint64_t primary_key() const { return owner.value; }

    double by_votes() const { return is_active ? -1.0 * valid_woods : valid_woods; }

    bool active() const { return is_active; }

    void deactivate()
    {
        producer_key = public_key();
        is_active = false;
    }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(producer_info, (owner)(valid_woods)(producer_key)(is_active)(url)(location)(unpaid_block_fee)(unpaid_wood)(last_claim_time))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] dbp_info
{
    name owner;
    std::string url;
    std::string steemid;
    time_point last_claim_time;

    uint64_t primary_key() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(dbp_info, (owner)(url)(steemid)(last_claim_time))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] bp_punish_info
{
    name owner;
    uint16_t punish_count = 0;
    uint64_t primary_key() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(bp_punish_info, (owner)(punish_count))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] voter_info
{
    name owner;                  /// the voter
    name proxy;                  /// the proxy set by the voter, if any
    std::vector<name> producers; /// the producers approved by this voter if no proxy set
    int64_t voted = 0;

    bool is_proxy = 0; /// whether the voter is a proxy for others

    uint32_t reserved1 = 0;
    uint32_t reserved2 = 0;
    eosio::asset reserved3;

    uint64_t primary_key() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(voter_info, (owner)(proxy)(producers)(voted)(is_proxy)(reserved1)(reserved2)(reserved3))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] wood_burn_info
{ // wood burn detail
    uint64_t rowid = 0;
    name voter; /// the voter
    uint32_t block_number = 0;
    std::string wood;

    uint64_t primary_key() const { return rowid; }

    static uint64_t woodkey(std::string wood)
    {
        if (wood.length() > 16)
        {
            return hextoint64(wood.substr(wood.length() - 16, 16));
        }
        else
        {
            return hextoint64(wood);
        }
    }

    static uint64_t hextoint64(std::string str)
    {

        uint64_t result = 0;
        const char *ch = str.c_str();
        for (int i = 0; (size_t)i < strlen(ch); i++)
        {
            if (ch[i] >= '0' && ch[i] <= '9')
            {
                result = result * 16 + (uint64_t)(ch[i] - '0');
            }
            else if (ch[i] >= 'A' && ch[i] <= 'Z')
            {
                result = result * 16 + (uint64_t)(ch[i] - 'A');
            }
            else if (ch[i] >= 'a' && ch[i] <= 'z')
            {
                result = result * 16 + (uint64_t)(ch[i] - 'a');
            }
            else
            {
                result = result * 16;
            }
        }

        return result;
    }

    uint64_t get_wood_index() const
    {
        return woodkey(wood);
    }

    uint64_t get_block_number() const { return (uint64_t)block_number; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(wood_burn_info, (rowid)(voter)(block_number)(wood))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] wood_burn_producer_block_stat //  wood burn per bp and block stat(木头焚烧按照BP及block_number统计的表)
{
    uint64_t rowid = 0;
    name producer; /// the producer
    uint32_t block_number = 0;
    uint32_t stat = 0;

    uint64_t primary_key() const { return rowid; }

    static uint128_t bpblockkey(name producer, uint32_t block_number)
    {
        return (uint128_t)producer.value << 32 | block_number;
    }

    uint128_t get_producer_block() const { return bpblockkey(producer, block_number); }

    uint64_t get_block_number() const { return (uint64_t)block_number; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(wood_burn_producer_block_stat, (rowid)(producer)(block_number)(stat))
};

struct [[ eosio::table, eosio::contract("celesos.system") ]] wood_burn_block_stat
{
    uint32_t block_number = 0;
    uint32_t stat = 0;
    double diff = 0;

    uint32_t primary_key() const { return block_number; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(wood_burn_block_stat, (block_number)(stat)(diff))
}; // 按照block_number统计的表，用于难度调整

typedef eosio::multi_index<"voters"_n, voter_info> voters_table;

typedef eosio::multi_index<"dbps"_n, dbp_info> dbps_table;

typedef eosio::multi_index<"bppunish"_n, bp_punish_info> bp_punish_table;

typedef eosio::multi_index<"woodburns"_n, wood_burn_info,
                           indexed_by<"wood"_n, const_mem_fun<wood_burn_info, uint64_t, &wood_burn_info::get_wood_index>>,
                           indexed_by<"blocknumber"_n, const_mem_fun<wood_burn_info, uint64_t, &wood_burn_info::get_block_number>>>
    wood_burn_table;

typedef eosio::multi_index<"woodbpblocks"_n, wood_burn_producer_block_stat,
                           indexed_by<"prodblock"_n, const_mem_fun<wood_burn_producer_block_stat, uint128_t, &wood_burn_producer_block_stat::get_producer_block>>,
                           indexed_by<"blocknumber"_n, const_mem_fun<wood_burn_producer_block_stat, uint64_t, &wood_burn_producer_block_stat::get_block_number>>>
    wood_burn_producer_block_table;

typedef eosio::multi_index<"woodblocks"_n, wood_burn_block_stat> wood_burn_block_stat_table;

typedef eosio::multi_index<"producers"_n, producer_info,
                           indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>>>
    producers_table;

typedef eosio::singleton<"global"_n, eosio_global_state> global_state_singleton;

static constexpr uint32_t seconds_per_day = 24 * 3600;

class [[eosio::contract("celesos.system")]] system_contract : public native
{
  private:
    voters_table _voters;
    producers_table _producers;

    dbps_table _dbps;
    bp_punish_table _dbpunishs;

    global_state_singleton _global;

    wood_burn_table _burninfos;
    wood_burn_producer_block_table _burnproducerstatinfos;
    wood_burn_block_stat_table _burnblockstatinfos;

    eosio_global_state _gstate;
    rammarket _rammarket;

  public:
    static constexpr eosio::name active_permission{"active"_n};
    static constexpr eosio::name token_account{"celes.token"_n};
    static constexpr eosio::name ram_account{"celes.ram"_n};
    static constexpr eosio::name ramfee_account{"celes.ramfee"_n};
    static constexpr eosio::name stake_account{"celes.stake"_n};
    static constexpr eosio::name bpay_account{"celes.bpay"_n};
    static constexpr eosio::name wpay_account{"celes.wpay"_n};
    static constexpr eosio::name dpay_account{"celes.dpay"_n};
    static constexpr eosio::name bpaypool_account{"celes.bpayp"_n};
    static constexpr eosio::name wpaypool_account{"celes.wpayp"_n};
    static constexpr eosio::name dpaypool_account{"celes.dpayp"_n};
    static constexpr eosio::name names_account{"celes.names"_n};
    static constexpr eosio::name dbp_account{"celes.dbp"_n};
    static constexpr symbol ramcore_symbol = symbol(symbol_code("RAMCORE"), 4);
    static constexpr symbol ram_symbol = symbol(symbol_code("RAM"), 0);

    system_contract(name s, name code, datastream<const char *> ds);
    ~system_contract();

    static symbol get_core_symbol(name system_account = "celes"_n)
    {
        rammarket rm(system_account, system_account.value);
        const static auto sym = get_core_symbol(rm);
        return sym;
    }

    // Actions:
    [[eosio::action]] void init(unsigned_int version, symbol core);
    [[eosio::action]] void onblock(ignore<block_header> header);

    [[eosio::action]] void setalimits(name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
    // functions defined in delegate_bandwidth.cpp

    /**
          *  Stakes SYS from the balance of 'from' for the benfit of 'receiver'.
          *  If transfer == true, then 'receiver' can unstake to their account
          *  Else 'from' can unstake at any time.
          */
    [[eosio::action]] void delegatebw(name from, name receiver,
                                      asset stake_net_quantity, asset stake_cpu_quantity, bool transfer);

    /**
          *  Decreases the total tokens delegated by from to receiver and/or
          *  frees the memory associated with the delegation if there is nothing
          *  left to delegate.
          *
          *  This will cause an immediate reduction in net/cpu bandwidth of the
          *  receiver.
          *
          *  A transaction is scheduled to send the tokens back to 'from' after
          *  the staking period has passed. If existing transaction is scheduled, it
          *  will be canceled and a new transaction issued that has the combined
          *  undelegated amount.
          *
          *  The 'from' account loses voting power as a result of this call and
          *  all producer tallies are updated.
          */
    [[eosio::action]] void undelegatebw(name from, name receiver,
                                        asset unstake_net_quantity, asset unstake_cpu_quantity);

    /**
          * Increases receiver's ram quota based upon current price and quantity of
          * tokens provided. An inline transfer from receiver to system contract of
          * tokens will be executed.
          */
    [[eosio::action]] void buyram(name payer, name receiver, asset quant);
    [[eosio::action]] void buyrambytes(name payer, name receiver, uint32_t bytes);

    /**
          *  Reduces quota my bytes and then performs an inline transfer of tokens
          *  to receiver based upon the average purchase price of the original quota.
          */
    [[eosio::action]] void sellram(name account, int64_t bytes);

    /**
          *  This action is called after the delegation-period to claim all pending
          *  unstaked tokens belonging to owner
          */
    [[eosio::action]] void refund(name owner);

    // functions defined in voting.cpp

    [[eosio::action]] void regproducer(const name producer, const eosio::public_key &producer_key, const std::string &url, uint16_t location);

    [[eosio::action]] void unregprod(const name producer);

    [[eosio::action]] void setram(uint64_t max_ram_size);
    [[eosio::action]] void setramrate(uint16_t bytes_per_block);

    [[eosio::action]] void regproxy(const name proxy, bool isproxy);

    [[eosio::action]] void setparams(const eosio::blockchain_parameters &params);

    // functions defined in producer_pay.cpp
    [[eosio::action]] void claimrewards(const name owner);

    [[eosio::action]] void setpriv(name account, uint8_t is_priv);

    [[eosio::action]] void rmvproducer(name producer);

    [[eosio::action]] void updtrevision(uint8_t revision);

    [[eosio::action]] void bidname(name bidder, name newname, asset bid);

    [[eosio::action]] void bidrefund(name bidder, name newname);

    [[eosio::action]] void regdbp(const name dbpname, std::string url, std::string steemid);

    [[eosio::action]] void unregdbp(const name dbpname);

    [[eosio::action]] void setproxy(const name voter_name, const name proxy_name);

    [[eosio::action]] void voteproducer(const name voter_name, const name wood_owner_name, std::string wood,
                                        const uint32_t block_number, const name producer_name);

    [[eosio::action]] void limitbp(const name &owner_name);
    [[eosio::action]] void unlimitbp(const name &owner_name);

    [[eosio::action]] void activedbp();

  private:
    // Implementation details:

    static symbol get_core_symbol(const rammarket &rm)
    {
        auto itr = rm.find(ramcore_symbol.raw());
        eosio_assert(itr != rm.end(), "system contract must first be initialized");
        return itr->quote.balance.symbol;
    }

    //defined in eosio.system.cpp
    static eosio_global_state get_default_parameters();
    static time_point current_time_point();
    static block_timestamp current_block_time();

    symbol core_symbol() const;

    void update_ram_supply();

    //defined in delegate_bandwidth.cpp
    void changebw(name from, name receiver,
                  asset stake_net_quantity, asset stake_cpu_quantity, bool transfer);

    //defined in voting.hpp
    void update_elected_producers(uint32_t head_block_number);

    // defined in voting.cpp
    void propagate_weight_change(const voter_info &voter);

    bool verify(const std::string wood, const uint32_t block_number, const name wood_owner_name);

    uint32_t clean_dirty_stat_producers(uint32_t block_number, uint32_t maxline);

    void clean_diff_stat_history(uint32_t block_number);

    uint32_t clean_dirty_wood_history(uint32_t block_number, uint32_t maxline);

    double calc_diff(uint32_t block_number);

    void update_vote(const name voter_name, const name wood_owner_name,
                     const std::string wood, const uint32_t block_number, const name producer_name);

    void ramattenuator();
    void ramattenuator(name account);
};

} // namespace celesos
