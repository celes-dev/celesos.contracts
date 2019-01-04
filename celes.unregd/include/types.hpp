#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/permission.hpp>

namespace eosio {
struct permission_level_weight {
    eosio::permission_level permission;
    uint16_t weight;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
};

struct key_weight {
    eosio::public_key key;
    uint16_t weight;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(key_weight, (key)(weight))
};

struct wait_weight {
    uint32_t wait_sec;
    uint16_t weight;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
};

struct authority {
    uint32_t threshold = 0;
    std::vector<key_weight> keys;
    std::vector<permission_level_weight> accounts;
    std::vector<wait_weight> waits;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
};
}  // namespace eosio

namespace celesos {

struct [[ eosio::table, eosio::contract("celesos.system") ]] exchange_state {
    eosio::asset supply;

    struct connector {
        eosio::asset balance;
        double weight = .5;

        EOSLIB_SERIALIZE(connector, (balance)(weight))
    };

    connector base;
    connector quote;

    uint64_t primary_key() const { return supply.symbol.raw(); }

    eosio::asset convert_to_exchange(connector & c, eosio::asset in);
    eosio::asset convert_from_exchange(connector & c, eosio::asset in);
    eosio::asset convert(asset from, const symbol &to);

    EOSLIB_SERIALIZE(exchange_state, (supply)(base)(quote))
};

typedef eosio::multi_index<"rammarket"_n, exchange_state> rammarket;

}  // namespace celesos