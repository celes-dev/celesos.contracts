#include <celes.stake/celes.stake.hpp>
#include <celesos.system/celesos.system.hpp>

#define ALL_REWARD 10000000
#define HALF_LIFT_PERIOD 8000000

namespace celesos {

   stake::stake_contract( name s, name code, datastream<const char*> ds )
   :eosio::contract(s,code,ds),
   _stake_global(get_self(), get_self().value)
   {
      _stake_gstate  = _stake_global.get();
   };
   stake::~stake_contract()
   {
      _stake_global.set( _stake_gstate, get_self() );
   };

   asset calculate_reword()
   {
      uint32_t current_block_number = get_chain_head_num();
      if(current_block_number - _stake_global.last_settlement > 0)
      {
         uint128_t all_new_coefficient = (current_block_number - _stake_global.last_settlement) * _stake_global.all_stake.amount;
         _stake_global.all_coefficient += all_new_coefficient;
         asset unreceived_reward = _stale_global.all_reward - _stake_global.all_unreceived_reward;
         int half_life_period = (int)(log(ALL_REWARD/unreceived_reward.amount)/log(2));
         uint64_t increased_reward = (uint64_t)((ALL_REWARD/2/HALF_LIFT_PERIOD)*(current_block_number - _stake_global.last_settlement)*pow(0.5,half_life_period));
         _stake_global.all_unreceived_reward += asset(increased_reward, system_contract::get_core_symbol());
      }

      s_stake_table single_stake(get_self(), get_self().value);
      uint64_t reward = (uint64_t)(single_stake.s_stake_amount.amount*(current_block_number - single_stake.s_settlement)/(_stake_global.all_stake.amount*_stake_global.all_coefficient));
      _stale_global.all_unreceived_reward -= asset(reward, system_contract::get_core_symbol());
      _stale_global.all_reward -= asset(reward, system_contract::get_core_symbol());
      _stake_global.last_settlement = current_block_number;
      single_stake.s_settlement = current_block_number;

      return reward;
   }

   void stake::stake(const asset& quantity,eosio::name from) {
      require_auth( from );

      uint32_t current_block_number = get_chain_head_num();
      if(_stake_global.last_settlement == 0)
      {
         asset init_quant = asset(0, system_contract::get_core_symbol());
         _stake_global.all_stake = init_quant;
         _stake_global.last_settlement = current_block_number;
         _stake_global.all_unreceived_reward = init_quant;
         _stake_global.all_coefficient = 0;
         _stale_global.all_reward = asset(ALL_REWARD, system_contract::get_core_symbol());
      }
      else
      {
         asset reward_value = calculate_reword();
         //transfer reward to from account
         if(reward_value.amount > 0)
         {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (
               get_self(), {{get_self(), active_permission}},
               {get_self(),from, reward_value, ""}
            );
         }
         s_stake_table single_stake(get_self(), get_self().value);
         single_stake.s_stake_amount += quantity;
         single_stake.s_settlement = current_block_number;
         _stake_global.all_stake += quantity;
      }
   }

   void stake::unstake(const asset& quantity,eosio::name to) {
      require_auth( to );
      eosio_assert(quantity.symbol == system_contract::get_core_symbol(), "unexpected asset symbol input");
      eosio_assert(_stake_global.last_settlement > 0, "stake contract not init!");
      is_stake_table single_stake(get_self(), get_self().value);
      eosio_assert(quantity.amount <= single_stake.s_stake_amount, "unstake amount is bigger than stake amount!");

      uint32_t current_block_number = get_chain_head_num();
      single_stake.s_stake_amount -= quantity;
      single_stake.s_settlement = current_block_number;
      _stake_global.all_stake -= quantity;

      //delay unstake quantity to stake account

   }

   void stake::onTransfer(const currency::transfer &transfer)
   {
      if (transfer.to != get_self())
      {
         return;
      }
      eosio_assert(transfer.quantity.symbol == system_contract::get_core_symbol(), "must pay with EOS token");
      

      stake::stake(transfer, transfer.from);
   }

   void stake::apply(account_name contract, account_name act)
   {
      if (contract == N(eosio.token) && act == N(transfer))
      {
         onTransfer(unpack_action_data<currency::transfer>());
         return;
      }

      if (contract != _self)
         return;
   }

   extern "C"
   {
      [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
         print("apply called!!!\n");
         // if( code == N(eosio) && action == N(onerror) ) {
         //    apply_onerror( receiver, onerror::from_current_action() );
         // }

         croncontract c( receiver );
         c.apply(code, action);
         eosio_exit(0);
      }
   }
} /// namespace celesos