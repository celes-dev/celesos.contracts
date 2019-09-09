#pragma once

#include <eosio/privileged.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include<eosio/system.hpp>
#include <cmath>
#include <eosio/transaction.hpp>

namespace celesos
{
  using eosio::contract;
  using eosio::symbol;
  using eosio::symbol_code;


  struct [[eosio::table("stakeglobal"), eosio::contract("poolcontract")]] stake_global_state {
        stake_global_state(){}

        eosio::asset    all_stake;
        uint32_t        last_settlement = 0;
        uint128_t       all_coefficient;
        eosio::asset    all_unreceived_reward;
        eosio::asset    all_reward;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( stake_global_state,
                                (all_stake)(last_settlement)(all_coefficient)
                                (all_unreceived_reward)(all_reward))
      };
  typedef eosio::singleton< "stakeglobal"_n, stake_global_state> global_stakestate_singleton;

  struct [[eosio::table]] s_stake {
            eosio::name                   s_stake_name;
            eosio::asset                  s_stake_amount;
            uint32_t                      s_settlement;

            uint64_t primary_key()const { return s_stake_name.value; }
         };

  typedef eosio::multi_index< "singlestake"_n, s_stake > s_stake_table;

  class[[eosio::contract("poolcontract")]] stake : public eosio::contract
  {
    private:
        global_stakestate_singleton  _stake_global;
        stake_global_state           _stake_gstate;

        eosio::asset calculate_reward(eosio::name from);

        
    public:
      using contract::contract;
      static constexpr eosio::name active_permission{"active"_n};
      static constexpr eosio::name token_account{"celes.token"_n};
      static constexpr eosio::symbol core_symbol = eosio::symbol{eosio::symbol_code{"CELES"}, 4};

      [[eosio::action]]
      void staketoken(const eosio::asset& quantity,eosio::name from);

      [[eosio::action]]
      void unstake(const eosio::asset& quantity,eosio::name to);

      [[eosio::action]]
      void apply( eosio::name from, eosio::name to, eosio::asset quantity);
      // using stake_action = eosio::action_wrapper<"stake"_n, &stake::stake>;
      using unstake_action = eosio::action_wrapper<"stake"_n, &stake::unstake>;
  };

} // namespace celesos