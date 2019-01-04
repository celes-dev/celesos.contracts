#include "eosio.unregd/eosio.unregd.hpp"
#include <eosiolib/crypto.h>
#include "eosio.unregd/exchange_state.hpp"

#include "abieos_numeric.hpp"

// import sha3
#include "sha3/byte_order.h"
#include "sha3/sha3.h"

// import uecc
// #define uECC_SUPPORTS_secp160r1 0
// #define uECC_SUPPORTS_secp192r1 0
// #define uECC_SUPPORTS_secp224r1 0
// #define uECC_SUPPORTS_secp256r1 0
// #define uECC_SUPPORTS_secp256k1 1
// #define uECC_SUPPORT_COMPRESSED_POINT 1
#include "ecc/uECC.h"

// import utils
#include "utils/inline_calls_helper.hpp"
#include "utils/snapshot.hpp"

using celesos::ethaddress;
using celesos::rammarket;
using eosio::address;
using eosio::asset;
using eosio::datastream;
using eosio::fixed_bytes;
using eosio::name;
using eosio::settings;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unregd;
using std::function;
using std::string;
using std::vector;

namespace celesos {

static uint8_t hex_char_to_uint(char character) {
    const int x = character;
    return (x <= 57) ? x - 48 : (x <= 70) ? (x - 65) + 0x0a : (x - 97) + 0x0a;
}

static fixed_bytes<32> compute_ethaddress_key256(const ethaddress &ethaddress) {
    uint8_t ethereum_key[20];
    const char *characters = ethaddress.c_str();

    // The ethereum address starts with 0x, let's skip those by starting at i =
    // 2
    for (uint64_t i = 2; i < ethaddress.length(); i += 2) {
        const uint64_t index = (i / 2) - 1;

        ethereum_key[index] = 16 * hex_char_to_uint(characters[i]) +
                              hex_char_to_uint(characters[i + 1]);
    }

    const uint32_t *p32 = reinterpret_cast<const uint32_t *>(&ethereum_key);
    return fixed_bytes<32>::make_from_word_sequence<uint32_t>(
        p32[0], p32[1], p32[2], p32[3], p32[4]);
}

static fixed_bytes<32> compute_ethaddress_key256(uint8_t *ethereum_key) {
    const uint32_t *p32 = reinterpret_cast<const uint32_t *>(ethereum_key);
    return fixed_bytes<32>::make_from_word_sequence<uint32_t>(
        p32[0], p32[1], p32[2], p32[3], p32[4]);
}

}  // namespace celesos

// implement struct address methods
fixed_bytes<32> address::by_ethaddress() {
    return compute_ethaddress_key256(ethaddress);
}

// implement struct settings methods
uint64_t settings::primary_key() { return id; }

// implement class unregd methods
unregd::unregd(name s, name code, datastream<const char *> ds)
    : eosio::contract{s, code, ds},
      addresses{_self, _self.value},
      settings{_self, _self.value} {}

unregd::~unregd() {}

void unregd::get_core_symbol(const name system_account = "celes"_n) {
    rammarket market{system_account, system_acount.value};
    return unregd::get_core_symbol(market);
}

void unregd::get_core_symbol(rammarket &market) {
    static const auto ramcore_symbol = symbol{symbol_code{"RAMCODE"}, 4};
    auto itr = market.find(ramcore_symbol.raw());
    eosio_assert(itr != market.end(), "RAMCORE market not found");
    return tmp->quote.balance.symbol;
}

/**
 * Add a mapping between an ethaddress and an initial EOS token balance.
 */
void unregd::add(const ethaddress &ethaddress, const asset &balance) {
    require_auth(_self);

    auto symbol = balance.symbol;
    eosio_assert(symbol.is_valid() && symbol == unregd::get_core_symbol(), "balance must be
    EOS token");

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
void unregd::setmaxeos(const asset &maxeos) {
    require_auth(_self);

    auto symbol = maxeos.symbol;
    eosio_assert(symbol.is_valid() && symbol == unreg::get_core_symbol(), "maxeos invalid
    symbol");

    auto itr = settings.find(1);
    if (itr == settings.end()) {
        settings.emplace(_self, [&](auto &s) {
            s.id = 1;
            s.max_eos_for_8k_of_ram = maxeos;
        });
    } else {
        settings.modify(itr, _self,
                        [&](auto &s) { s.max_eos_for_8k_of_ram = maxeos; });
    }
}

/**
 * Register an EOS account using the stored information (address/balance)
 * verifying an ETH signature
 */
void unregd::regaccount(const vector<char> &signature, const string &account,
                        const string &eos_pubkey_str) {
    eosio_assert(signature.size() == 66, "Invalid signature");
    eosio_assert(account.size() == 12, "Invalid account length");

    // Verify that the destination account name is valid
    for (const auto &c : account) {
        if (!((c >= 'a' && c <= 'z') || (c >= '1' && c <= '5')))
            eosio_assert(false, "Invalid account name");
    }

    auto naccount = eosio::name(account.c_str());

    // Verify that the account does not exists
    eosio_assert(!is_account(naccount), "Account already exists");

    // Rebuild signed message based on current TX block num/prefix, pubkey and
    // name
    const abieos::public_key eos_pubkey =
        abieos::string_to_public_key(eos_pubkey_str);

    char tmpmsg[128];
    sprintf(tmpmsg, "%u,%u,%s,%s", tapos_block_num(), tapos_block_prefix(),
            eos_pubkey_str.c_str(), account.c_str());

    // Add prefix and length of signed message
    char message[128];
    sprintf(message, "%s%s%d%s", "\x19", "Ethereum Signed Message:\n",
            strlen(tmpmsg), tmpmsg);

    // Calculate sha3 hash of message
    sha3_ctx shactx;
    capi_checksum256 msghash;
    rhash_keccak_256_init(&shactx);
    rhash_keccak_update(&shactx, (const uint8_t *)message, strlen(message));
    rhash_keccak_final(&shactx, msghash.hash);

    // Recover compressed pubkey from signature
    uint8_t pubkey[64];
    uint8_t compressed_pubkey[34];
    auto res = recover_key(&msghash, signature.data(), signature.size(),
                           (char *)compressed_pubkey, 34);

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
    asset max_eos_for_8k_of_ram = asset(0, symbol{symbol_code{"CELES"}, 4});
    auto sitr = settings.find(1);
    if (sitr != settings.end()) {
        max_eos_for_8k_of_ram = sitr->max_eos_for_8k_of_ram;
    }

    // Calculate the amount of EOS to purchase 8k of RAM
    auto amount_to_purchase_8kb_of_RAM = buyrambytes(8 * 1024);
    eosio_assert(amount_to_purchase_8kb_of_RAM <= max_eos_for_8k_of_ram,
                 "price of RAM too high");

    // Build authority with the pubkey passed as parameter
    const auto auth = authority{
        1, {{{(uint8_t)eos_pubkey.type, eos_pubkey.data}, 1}}, {}, {}};

    // Create account with the same key for owner/active
    INLINE_ACTION_SENDER(call::eosio, newaccount)
    ("eosio"_n, {{"eosio.unregd"_n, "active"_n}},
     {"eosio.unregd"_n, naccount, auth, auth});

    // Buy RAM for this account (8k)
    INLINE_ACTION_SENDER(call::eosio, buyram)
    ("eosio"_n, {{"eosio.regram"_n, "active"_n}},
     {"eosio.regram"_n, naccount, amount_to_purchase_8kb_of_RAM});

    // Delegate bandwith
    INLINE_ACTION_SENDER(call::eosio, delegatebw)
    ("eosio"_n, {{"eosio.unregd"_n, "active"_n}},
     {"eosio.unregd"_n, naccount, balances[0], balances[1], 1});

    // Transfer remaining if any (liquid EOS)
    if (balances[2] != asset{}) {
        INLINE_ACTION_SENDER(call::token, transfer)
        ("eosio.token"_n, {{"eosio.unregd"_n, "active"_n}},
         {"eosio.unregd"_n, naccount, balances[2], ""});
    }

    // Remove information for the ETH address from the eosio.unregd DB
    index.erase(itr);
}

void unregd::update_address(const ethaddress &ethaddress,
                            const function<void(address &)> updater) {
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

EOSIO_DISPATCH(eosio::unregd, (add)(chngaddress)(setmaxeos)(regaccount))