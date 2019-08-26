#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

namespace celesos
{
  using contract::contract;


  struct [[eosio::table("stake_global"), eosio::contract("celes.stake")]] global_attribute {
        asset           all_stake;
        uint32_t        last_settlement;
        uint128_t       all_coefficient;
        asset           all_unreceived_reward;
        asset           all_reward;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE_DERIVED( global_attribute,
                                (all_stake)(last_settlement)(all_coefficient)
                                (all_unreceived_reward)(all_reward) )
      };
  typedef eosio::singleton< "stake_global"_n, global_attribute >   global_state_singleton;

  struct [[eosio::table]] s_stake {
            eosio::name                   s_stake_name;
            asset                         s_stake_amount;
            uint32_t                      s_settlement;

            uint64_t primary_key()const { return s_stake_name.value; }
         };

   typedef eosio::multi_index< "singlestake"_n, s_stake > s_stake_table;

  class[[eosio::contract("celesos.stake")]] stake : public eosio::contract
  {
    private:
         global_state_singleton  _stake_global;
         global_attribute      _stake_gstate;
         static global_attribute get_default_parameters();
    public:
      [[eosio::action]]
      void stake(const asset& quantity,eosio::name from);

      [[eosio::action]]
      void unstake(const asset& quantity,eosio::name to);

      using stake_action = eosio::action_wrapper<"stake"_n, &stake::stake>;
      using unstake_action = eosio::action_wrapper<"stake"_n, &stake::unstake>;
  };

} // namespace celesos