#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>

namespace celesos {

   class [[eosio::contract("eosio.msig")]] multisig : public eosio::contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void propose(eosio::ignore<eosio::name> proposer, eosio::ignore<eosio::name> proposal_name,
               eosio::ignore<std::vector<eosio::permission_level>> requested, eosio::ignore<eosio::transaction> trx);
         [[eosio::action]]
         void approve( eosio::name proposer, eosio::name proposal_name, eosio::permission_level level,
                       const eosio::binary_extension<eosio::checksum256>& proposal_hash );
         [[eosio::action]]
         void unapprove( eosio::name proposer, eosio::name proposal_name, eosio::permission_level level );
         [[eosio::action]]
         void cancel( eosio::name proposer, eosio::name proposal_name, eosio::name canceler );
         [[eosio::action]]
         void exec( eosio::name proposer, eosio::name proposal_name, eosio::name executer );
         [[eosio::action]]
         void invalidate( eosio::name account );

      private:
         struct [[eosio::table]] proposal {
            eosio::name                           proposal_name;
            std::vector<char>               packed_transaction;

            uint64_t primary_key()const { return proposal_name.value; }
         };

         typedef eosio::multi_index< "proposal"_n, proposal > proposals;

         struct [[eosio::table]] old_approvals_info {
            eosio::name                           proposal_name;
            std::vector<eosio::permission_level>   requested_approvals;
            std::vector<eosio::permission_level>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef eosio::multi_index< "approvals"_n, old_approvals_info > old_approvals;

         struct approval {
            eosio::permission_level level;
            eosio::time_point       time;
         };

         struct [[eosio::table]] approvals_info {
            uint8_t                 version = 1;
            eosio::name                   proposal_name;
            //requested approval doesn't need to cointain time, but we want requested approval
            //to be of exact the same size ad provided approval, in this case approve/unapprove
            //doesn't change serialized data size. So, we use the same type.
            std::vector<approval>   requested_approvals;
            std::vector<approval>   provided_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef eosio::multi_index< "approvals2"_n, approvals_info > approvals;

         struct [[eosio::table]] invalidation {
            eosio::name        account;
            eosio::time_point   last_invalidation_time;

            uint64_t primary_key() const { return account.value; }
         };

         typedef eosio::multi_index< "invals"_n, invalidation > invalidations;
   };

} /// celesos eosio