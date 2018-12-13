#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>

namespace celesos {

   class [[eosio::contract("eosio.wrap")]] wrap : public eosio::contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void exec( eosio::ignore<eosio::name> executer, eosio::ignore<eosio::transaction> trx );

   };

} /// namespace celesos