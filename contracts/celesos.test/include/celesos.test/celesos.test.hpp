#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

namespace celesos
{

  class[[eosio::contract("celesos.test")]] test : public eosio::contract
  {
    public:
      using contract::contract;

      [[eosio::action]]
      void helloworld(eosio::name from);

      [[eosio::action]]
      void sayhello(eosio::name from, eosio::name to);

      [[eosio::action]]
      void blockrandom(const uint32_t block_number);

      using helloworld_action = eosio::action_wrapper<"helloworld"_n, &test::helloworld>;
      using sayhello_action = eosio::action_wrapper<"sayhello"_n, &test::sayhello>;
      using blockrandom_action = eosio::action_wrapper<"blockrandom"_n, &test::blockrandom>;
  };

} // namespace celesos