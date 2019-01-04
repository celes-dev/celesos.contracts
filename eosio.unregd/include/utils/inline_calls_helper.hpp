#pragma once

#include "authority.hpp"

namespace celes {

// Helper struct for inline calls.
// Only the prototype of the functions are used for
// the serialization of the action's payload.
struct call {
    struct token {
        void issue(eosio::name to, eosio::asset quantity, std::string memo);
        void transfer(eosio::name from, eosio::name to, eosio::asset quantity,
                      std::string memo);
    };

    struct eosio {
        void newaccount(eosio::name creator, eosio::name name,
                        eosio::authority owner, eosio::authority active);
        void delegatebw(eosio::name from, eosio::name receiver,
                        eosio::asset stake_net_quantity,
                        eosio::asset stake_cpu_quantity, bool transfer);
        void buyram(eosio::name buyer, eosio::name receiver,
                    eosio::asset tokens);
    };
};
}  // namespace celes
