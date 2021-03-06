#include "celes.unregd.hpp"
#include <eosiolib/crypto.h>
#include <eosiolib/print.h>
#include "ram/exchange_state.cpp"
#include "utils/authority.hpp"
#include "utils/inline_calls_helper.hpp"

#define USE_KECCAK

#include "sha3/byte_order.c"
#include "sha3/sha3.c"

#include "abieos_numeric.hpp"

#define uECC_SUPPORTS_secp160r1 0
#define uECC_SUPPORTS_secp192r1 0
#define uECC_SUPPORTS_secp224r1 0
#define uECC_SUPPORTS_secp256r1 0
#define uECC_SUPPORTS_secp256k1 1
#define uECC_SUPPORT_COMPRESSED_POINT 1

#include "ecc/uECC.c"

#include "utils/snapshot.hpp"

using celes::unregd;
using celesos::rammarket;
using eosio::action;
using eosio::asset;
using eosio::authority;
using eosio::datastream;
using eosio::name;
using eosio::permission_level;
using eosio::symbol;
using eosio::symbol_code;
using std::function;
using std::string;
using std::vector;

unregd::unregd(name s, name code, datastream<const char *> ds)
        : eosio::contract{s, code, ds},
          addresses(_self, _self.value),
          settings(_self, _self.value) {}

/**
 * Add a mapping between an ethaddress and an initial EOS token balance.
 */
void unregd::add(const ethaddress &ethaddress, const asset &balance) {
    require_auth(_self);

    auto symbol = balance.symbol;
    eosio_assert(symbol.is_valid() && symbol == unregd::core_symbol,
                 "balance must be EOS token");

    eosio_assert(ethaddress.length() == 42,
                 "Ethereum address should have exactly 42 characters");

    update_address(ethaddress, [&](auto &address) {
        address.ethaddress = ethaddress;
        address.balance = balance;
    });
}

/**
 * Change the ethereum address that owns a balance
 */
void unregd::chngaddress(const ethaddress &old_address,
                         const ethaddress &new_address) {
    require_auth(_self);

    eosio_assert(old_address.length() == 42,
                 "Old Ethereum address should have exactly 42 characters");
    eosio_assert(new_address.length() == 42,
                 "New Ethereum address should have exactly 42 characters");

    auto index = addresses.template get_index<"ethaddress"_n>();
    auto itr = index.find(compute_ethaddress_key256(old_address));

    eosio_assert(itr != index.end(), "Old Ethereum address not found");

    index.modify(itr, _self,
                 [&](auto &address) { address.ethaddress = new_address; });
}

/**
 * Sets the maximum amount of EOS this contract is willing to pay when creating
 * a new account
 */
void unregd::setmaxceles(const asset &maxceles) {
    require_auth(_self);
    const auto core_symbol = unregd::core_symbol;
    auto symbol = maxceles.symbol;
    eosio_assert(symbol.is_valid() && symbol == core_symbol,
                 "maxeos invalid symbol");

    auto itr = settings.find(1);
    if (itr == settings.end()) {
        settings.emplace(_self, [&](auto &s) {
            s.id = 1;
            s.max_celes_for_8k_of_ram = maxceles;
        });
    } else {
        settings.modify(itr, _self,
                        [&](auto &s) { s.max_celes_for_8k_of_ram = maxceles; });
    }
}

/**
 * Register an CELES account using the stored information (address/balance)
 * verifying an ETH signature
 */
void unregd::regaccount(const vector<char> &signature, const string &account,
                        const string &celes_pubkey_str) {
    eosio_assert(signature.size() == 66, "Invalid signature");
    eosio_assert(account.size() == 12, "Invalid account length");

    // Verify that the destination account name is valid
    for (const auto &c : account) {
        if (!((c >= 'a' && c <= 'z') || (c >= '1' && c <= '5')))
            eosio_assert(false, "Invalid account name");
    }

    auto naccount = name(account.c_str());

    // Verify that the account does not exists
    eosio_assert(!is_account(naccount), "Account already exists");

    // Rebuild signed message based on current TX block num/prefix, pubkey and
    // name
    const abieos::public_key celes_pubkey =
            abieos::string_to_public_key(celes_pubkey_str);

    char tmpmsg[128];
    sprintf(tmpmsg, "%u,%u,%s,%s", tapos_block_num(), tapos_block_prefix(),
            celes_pubkey_str.c_str(), account.c_str());

    // Add prefix and length of signed message
    char message[128];
    sprintf(message, "%s%s%d%s", "\x19", "Ethereum Signed Message:\n",
            strlen(tmpmsg), tmpmsg);

    // Calculate sha3 hash of message
    sha3_ctx shactx;
    capi_checksum256 msghash;
    rhash_keccak_256_init(&shactx);
    rhash_keccak_update(&shactx, (const uint8_t *) message, strlen(message));
    rhash_keccak_final(&shactx, msghash.hash);

    // Recover compressed pubkey from signature
    uint8_t pubkey[64];
    uint8_t compressed_pubkey[34];
    auto res = recover_key(&msghash, signature.data(), signature.size(),
                           (char *) compressed_pubkey, 34);

    eosio_assert(res == 34, "Recover key failed");

    // Decompress pubkey
    uECC_decompress(compressed_pubkey + 1, pubkey, uECC_secp256k1());

    // Calculate ETH address based on decompressed pubkey
    capi_checksum256 pubkeyhash;
    rhash_keccak_256_init(&shactx);
    rhash_keccak_update(&shactx, pubkey, 64);
    rhash_keccak_final(&shactx, pubkeyhash.hash);

    uint8_t eth_address[20];
    memcpy(eth_address, pubkeyhash.hash + 12, 20);

    // Verify that the ETH address exists in the "addresses" eosio.unregd
    // contract table
    addresses_index addresses(_self, _self.value);
    auto index = addresses.template get_index<"ethaddress"_n>();

    auto itr = index.find(compute_ethaddress_key256(eth_address));
    eosio_assert(itr != index.end(), "Address not found");

    // Split contribution balance into cpu/net/liquid
    auto balances = split_snapshot_abp(itr->balance);
    eosio_assert(balances.size() == 3, "Unable to split snapshot");
    eosio_assert(itr->balance == balances[0] + balances[1] + balances[2],
                 "internal error");

    // Get max EOS willing to spend for 8kb of RAM
    asset max_celes_for_8k_of_ram = asset{0, unregd::core_symbol};
    auto sitr = settings.find(1);
    if (sitr != settings.end()) {
        max_celes_for_8k_of_ram = sitr->max_celes_for_8k_of_ram;
    }

    // Calculate the amount of EOS to purchase 8k of RAM
    auto amount_to_purchase_8kb_of_RAM = buyrambytes(8 * 1024);
    eosio_assert(amount_to_purchase_8kb_of_RAM <= max_celes_for_8k_of_ram,
                 "price of RAM too high");

    // Build authority with the pubkey passed as parameter
    const auto auth = authority{
            1, {{{(uint8_t) celes_pubkey.type, celes_pubkey.data}, 1}}, {}, {}
    };

    // Create account with the same key for owner/active
    INLINE_ACTION_SENDER(call::system, newaccount)
            (system_account, {{_self, active_permission}},
             {_self, naccount, auth, auth});

    // Buy RAM for this account (8k)
    INLINE_ACTION_SENDER(call::system, buyram)
            (system_account, {{_self, active_permission}},
             {_self, naccount, amount_to_purchase_8kb_of_RAM});

    // Delegate bandwith
    INLINE_ACTION_SENDER(call::system, delegatebw)
            (system_account, {{_self, active_permission}},
             {_self, naccount, balances[0], balances[1], 1});

    // Transfer remaining if any (liquid EOS)
    if (balances[2] != asset{0, unregd::core_symbol}) {
        INLINE_ACTION_SENDER(call::token, transfer)
                (token_account, {{_self, active_permission}},
                 {_self, naccount, balances[2], ""});
    }

    // Remove information for the ETH address from the celes.unregd DB
    index.erase(itr);
}

void unregd::update_address(const ethaddress &ethaddress,
                            const function<void(address & )> updater) {
    auto index = addresses.template get_index<"ethaddress"_n>();

    auto itr = index.find(compute_ethaddress_key256(ethaddress));
    if (itr == index.end()) {
        addresses.emplace(_self, [&](auto &address) {
            address.id = addresses.available_primary_key();
            updater(address);
        });
    } else {
        index.modify(itr, _self, [&](auto &address) { updater(address); });
    }
}

EOSIO_DISPATCH(celes::unregd, (add)(regaccount)
(setmaxceles)(chngaddress))