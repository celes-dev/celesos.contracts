#pragma once

#include <eosiolib/asset.hpp>

namespace celesos
{
using eosio::asset;
using eosio::symbol;

typedef double real_type;

/**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
struct [[ eosio::table, eosio::contract("celesos.system") ]] exchange_state
{
   eosio::asset supply;

   struct connector
   {
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

} // namespace celesos