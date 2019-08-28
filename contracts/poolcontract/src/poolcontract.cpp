#include <poolcontract/poolcontract.hpp>
#include "inline_helper.hpp"

#define ALL_REWARD 10000000
#define HALF_LIFT_PERIOD 8000000

namespace celesos {

   stake::stake( eosio::name s, eosio::name code, eosio::datastream<const char*> ds )
   :eosio::contract(s,code,ds),
   _stake_global(get_self(), get_self().value)
   {
      _stake_gstate  = _stake_global.exists()?_stake_global.get() : stake_global_state{};
   };
   stake::~stake()
   {
      eosio::print(" ---3-- ");
      _stake_global.set( _stake_gstate, get_self() );
   };

   eosio::asset stake::calculate_reward(eosio::name from)
   {
      using namespace eosio;
      
      eosio::internal_use_do_not_use::eosio_assert(_stake_gstate.all_reward.amount > 0, "poolcontract reward amount is 0!");
      uint32_t current_block_number = eosio::internal_use_do_not_use::get_chain_head_num();
      if(current_block_number - _stake_gstate.last_settlement > 0)
      {
         eosio::print(" ---settlement reward!!!-- ");
         uint128_t all_new_coefficient = (current_block_number - _stake_gstate.last_settlement) * _stake_gstate.all_stake.amount;
         _stake_gstate.all_coefficient += all_new_coefficient;
         eosio::asset unreceived_reward = _stake_gstate.all_reward - _stake_gstate.all_unreceived_reward;
         int half_life_period = (int)(log(ALL_REWARD/unreceived_reward.amount)/log(2));
         uint64_t increased_reward = (uint64_t)((ALL_REWARD/2/HALF_LIFT_PERIOD)*(current_block_number - _stake_gstate.last_settlement)*pow(0.5,half_life_period));
         _stake_gstate.all_unreceived_reward += eosio::asset(increased_reward, core_symbol);
      }
      
      s_stake_table single_stake(get_self(), from.value);
      auto single = single_stake.find( from.value );
      if(single->s_stake_amount.amount == 0)
      {
         eosio::print(" ---reward is 0!-- ");
         return eosio::asset(0, core_symbol);
      }
      uint64_t reward = (uint64_t)((single->s_stake_amount.amount*(current_block_number - single->s_settlement))/(_stake_gstate.all_stake.amount*_stake_gstate.all_coefficient));
      _stake_gstate.all_unreceived_reward -= eosio::asset(reward, core_symbol);
      _stake_gstate.all_reward -= eosio::asset(reward, core_symbol);
      _stake_gstate.last_settlement = current_block_number;

      return eosio::asset(reward, core_symbol);
   }

   void stake::staketoken(const eosio::asset& quantity,eosio::name from) {
      require_auth( from );

      uint32_t current_block_number = eosio::internal_use_do_not_use::get_chain_head_num();
      if(_stake_gstate.last_settlement == 0)
      {
         eosio::print(" *******1-- ");
         eosio::asset init_quant = eosio::asset(0, core_symbol);
         _stake_gstate.all_stake = quantity;
         _stake_gstate.last_settlement = current_block_number;
         _stake_gstate.all_unreceived_reward = init_quant;
         _stake_gstate.all_coefficient = 0;
         _stake_gstate.all_reward = eosio::asset(ALL_REWARD, core_symbol);
      }
      else
      {
         eosio::asset reward_value = stake::calculate_reward(from);
         //transfer reward to from account
         if(reward_value.amount > 0)
         {
            INLINE_ACTION_SENDER(celes::stakecall::token, transfer)
            (get_self(), {{get_self(), "active"_n}},
               {get_self(),from, reward_value, ""});
         }

         s_stake_table single_stake(get_self(), from.value);
         auto single = single_stake.find( from.value );
         if( single == single_stake.end() ) {
            single_stake.emplace( from, [&]( auto& a ){
               //first time s_stake_amount is  quantity
               a.s_stake_amount = quantity;
               a.s_settlement = current_block_number;
            });
            eosio::print(" ------qqqqqqqq-- ");
         } else {
            single_stake.modify( single, from, [&]( auto& a ) {
               a.s_stake_amount += quantity;
               a.s_settlement = current_block_number;
            });
         }
         _stake_gstate.all_stake += quantity;
         
      }
   }

   void stake::unstake(const eosio::asset& quantity,eosio::name to) {
      require_auth( to );
      eosio::internal_use_do_not_use::eosio_assert(quantity.symbol == core_symbol, "unexpected asset symbol input");
      eosio::internal_use_do_not_use::eosio_assert(_stake_gstate.last_settlement > 0, "stake contract not init!");
      s_stake_table single_stake(get_self(), to.value);
      auto single = single_stake.find( to.value );
      eosio::internal_use_do_not_use::eosio_assert(single != single_stake.end(), "stake infomation table is not exist!");
      eosio::internal_use_do_not_use::eosio_assert(quantity <= single->s_stake_amount, "unstake amount is bigger than stake amount!");

      eosio::asset reward_value = stake::calculate_reward(to);
      if(reward_value.amount > 0)
      {
         INLINE_ACTION_SENDER(celes::stakecall::token, transfer)
         (get_self(), {{get_self(), "active"_n}},
            {get_self(),to, reward_value, ""});
      }

      uint32_t current_block_number = eosio::internal_use_do_not_use::get_chain_head_num();
      single_stake.modify( single, to, [&]( auto& a ) {
         a.s_stake_amount -= quantity;
         a.s_settlement = current_block_number;
      });
      _stake_gstate.all_stake -= quantity;

      //delay unstake quantity to stake account

   }

   void stake::apply( eosio::name from, eosio::name to, eosio::asset quantity)
   {
      eosio::internal_use_do_not_use::eosio_assert(to == get_self(), "transfer not to poolcontract contract!");
      eosio::internal_use_do_not_use::eosio_assert(from != get_self(), "from account is equal poolcontract contract!");
      stake::staketoken(quantity, from);
   }

   extern "C"
   {
      [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
         constexpr static auto token_account = "celes.token"_n;
         constexpr static auto action_name = "transfer"_n;
         if ((eosio::name(code) == token_account) && (eosio::name(action) == action_name))
         { 
            eosio::print(" ---execute_action--- ");
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &celesos::stake::apply);
         }
         
         eosio::eosio_exit(0);
      }
   }
} /// namespace celesos