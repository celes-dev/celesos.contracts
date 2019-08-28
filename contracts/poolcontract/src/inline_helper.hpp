#pragma once

#include "poolauthority.hpp"

namespace celes {

    struct stakecall {
        struct token {
            void transfer(eosio::name from,
                          eosio::name to,
                          eosio::asset quantity,
                          std::string memo);
        };
    };
}