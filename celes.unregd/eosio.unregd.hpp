#include <cmath>
#include <functional>
#include <string>

#include <eosiolib/transaction.h>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/fixed_bytes.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/public_key.hpp>

#include "ram/exchange_state.cpp"

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

using namespace eosio;
using namespace std;

#include "utils/snapshot.hpp"

// Typedefs
typedef std::string ethaddress;

// Namespaces
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::fixed_bytes;
using std::function;
using std::string;

namespace eosio {

class[[eosio::contract("eosio::unregd")]] unregd : public contract {
   public:
    unregd(name s, name code, datastream<const char*> ds) : eosio::contract{s, code, ds},
                                                            addresses(_self, _self.value),
                                                            settings(_self, _self.value) {}

    // Actions
    void add(const ethaddress& ethaddress, const asset& balance);
    void regaccount(const std::vector<char>& signature, const string& account, const string& eos_pubkey);
    void setmaxeos(const asset& maxeos);
    void chngaddress(const ethaddress& old_address, const ethaddress& new_address);

   private:
    static uint8_t hex_char_to_uint(char character) {
        const int x = character;

        return (x <= 57) ? x - 48 : (x <= 70) ? (x - 65) + 0x0a : (x - 97) + 0x0a;
    }

    static fixed_bytes<32> compute_ethaddress_key256(const ethaddress& ethaddress) {
        uint8_t ethereum_key[20];
        const char* characters = ethaddress.c_str();

        // The ethereum address starts with 0x, let's skip those by starting at i = 2
        for (uint64_t i = 2; i < ethaddress.length(); i += 2) {
            const uint64_t index = (i / 2) - 1;

            ethereum_key[index] = 16 * hex_char_to_uint(characters[i]) + hex_char_to_uint(characters[i + 1]);
        }

        const uint32_t* p32 = reinterpret_cast<const uint32_t*>(&ethereum_key);
        return fixed_bytes<32>::make_from_word_sequence<uint32_t>(p32[0], p32[1], p32[2], p32[3], p32[4]);
    }

    static fixed_bytes<32> compute_ethaddress_key256(uint8_t * ethereum_key) {
        const uint32_t* p32 = reinterpret_cast<const uint32_t*>(ethereum_key);
        return fixed_bytes<32>::make_from_word_sequence<uint32_t>(p32[0], p32[1], p32[2], p32[3], p32[4]);
    }

    //@abi table addresses i64
    struct [[eosio::table]] address {
        uint64_t id;
        ethaddress ethaddress;
        asset balance;

        uint64_t primary_key() const { return id; }
        fixed_bytes<32> by_ethaddress() const { return unregd::compute_ethaddress_key256(ethaddress); }
    };

    typedef eosio::multi_index<
        "addresses"_n, address,
        indexed_by<"ethaddress"_n, const_mem_fun<address, fixed_bytes<32>, &address::by_ethaddress>>>
        addresses_index;

    //@abi table settings i64
    struct [[eosio::table]] settings {
        uint64_t id;
        asset max_eos_for_8k_of_ram;

        uint64_t primary_key() const { return id; }
    };

    typedef eosio::multi_index<"settings"_n, settings> settings_index;

    void update_address(const ethaddress& ethaddress, const function<void(address&)> updater);

    addresses_index addresses;
    settings_index settings;
};

}  // namespace eosio
