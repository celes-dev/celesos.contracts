#pragma once

#include <cmath>
#include <functional>
#include <string>

#include <eosiolib/transaction.h>
#include <eosiolib/types.h>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/fixed_bytes.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/public_key.hpp>
#include "types.hpp"

namespace celes {
typedef std::string ethaddress;

struct [[ eosio::table, eosio::contract("celes.unregd") ]] address {
    uint64_t id;
    celes::ethaddress ethaddress;
    eosio::asset balance;

    uint64_t primary_key() const { return id; }

    eosio::fixed_bytes<32> by_ethaddress() const;
};

typedef eosio::multi_index<
    "addresses"_n, address,
    eosio::indexed_by<"ethaddress"_n,
                      eosio::const_mem_fun<address, eosio::fixed_bytes<32>,
                                           &address::by_ethaddress>>>
    addresses_index;

struct [[ eosio::table, eosio::contract("celes.unregd") ]] settings {
    uint64_t id;
    eosio::asset max_eos_for_8k_of_ram;

    uint64_t primary_key() const;
};
typedef eosio::multi_index<"settings"_n, settings> settings_index;

class[[eosio::contract("celes.unregd")]] unregd : public eosio::contract {
   public:
    using contract::contract;
    static constexpr eosio::name active_permission{"active"_n};
    static constexpr eosio::name system_account{"celes"_n};

    unregd(eosio::name s, eosio::name code, eosio::datastream<const char *> ds);

    [[eosio::action]] void add(const celes::ethaddress &ethereum_address,
                               const eosio::asset &balance);
    [[eosio::action]] void chngaddress(const celes::ethaddress &old_address,
                                       const celes::ethaddress &new_address);
    [[eosio::action]] void setmaxeos(const eosio::asset &maxeos);
    [[eosio::action]] void regaccount(const std::vector<char> &signature,
                                      const std::string &account,
                                      const std::string &eos_pubkey_str);

   private:
    celes::addresses_index addresses;
    celes::settings_index settings;

    void update_address(const celes::ethaddress &ethereum_address,
                        const std::function<void(celes::address &)> updater);

    static eosio::symbol get_core_symbol(celesos::rammarket & market);
    static eosio::symbol get_core_symbol(const eosio::name system_account = celes::unregd::system_account);
};
}  // namespace celes