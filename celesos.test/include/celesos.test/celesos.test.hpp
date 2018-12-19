#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>

namespace celesos
{

class[[eosio::contract("celesos.test")]] test : public eosio::contract
{
 public:
   using contract::contract;

   [[eosio::action]] void sayhello(eosio::name from, eosio::name to);
};

} // namespace celesos