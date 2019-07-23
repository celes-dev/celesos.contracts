#pragma once

#include <eosio/binary_extension.hpp>
#include <eosio/eosio.hpp>
#include <eosio/ignore.hpp>
#include <eosio/transaction.hpp>

namespace celesos {

   using eosio::contract;
   using eosio::name;
   using eosio::permission_level;
   using eosio::transaction;
   using eosio::binary_extension;
   using eosio::action_wrapper;
   using eosio::ignore;
   using eosio::unpack;
   using eosio::check;
   using eosio::transaction_header;
   using eosio::datastream;
   using eosio::checksum256;

   /**
    * @defgroup eosiomsig celes.msig
    * @ingroup eosiocontracts
    * celes.msig contract defines the structures and actions needed to manage the proposals and approvals on blockchain.
    * @{
    */
   class [[eosio::contract("celesos.msig")]] multisig : public contract {
      public:
         using contract::contract;

         /**
          * Create proposal
          *
          * @details Creates a proposal containing one transaction.
          * Allows an account `proposer` to make a proposal `proposal_name` which has `requested`
          * permission levels expected to approve the proposal, and if approved by all expected
          * permission levels then `trx` transaction can we executed by this proposal.
          * The `proposer` account is authorized and the `trx` transaction is verified if it was
          * authorized by the provided keys and permissions, and if the proposal eosio::name doesn’t
          * already exist; if all validations pass the `proposal_name` and `trx` trasanction are
          * saved in the proposals table and the `requested` permission levels to the
          * approvals table (for the `proposer` context).
          * Storage changes are billed to `proposer`.
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be unique for proposer)
          * @param requested - Permission levels expected to approve the proposal
          * @param trx - Proposed transaction
          */
         [[eosio::action]]
         void propose(ignore<eosio::name> proposer, ignore<eosio::name> proposal_name,
               ignore<std::vector<permission_level>> requested, ignore<transaction> trx,
               eosio::ignore<std::string> memo);
         /**
          * Approve proposal
          *
          * @details Approves an existing proposal
          * Allows an account, the owner of `level` permission, to approve a proposal `proposal_name`
          * proposed by `proposer`. If the proposal's requested approval list contains the `level`
          * permission then the `level` permission is moved from internal `requested_approvals` list to
          * internal `provided_approvals` list of the proposal, thus persisting the approval for
          * the `proposal_name` proposal.
          * Storage changes are billed to `proposer`.
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be unique for proposer)
          * @param level - Permission level approving the transaction
          * @param proposal_hash - Transaction's checksum
          */
         [[eosio::action]]
         void approve( eosio::name proposer, eosio::name proposal_name, permission_level level,
                       const eosio::binary_extension<eosio::checksum256  >& proposal_hash );
         /**
          * Revoke proposal
          *
          * @details Revokes an existing proposal
          * This action is the reverse of the `approve` action: if all validations pass
          * the `level` permission is erased from internal `provided_approvals` and added to the internal
          * `requested_approvals` list, and thus un-approve or revoke the proposal.
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be an existing proposal)
          * @param level - Permission level revoking approval for proposal
          */
         [[eosio::action]]
         void unapprove( eosio::name proposer, eosio::name proposal_name, permission_level level );
         /**
          * abstain proposal
          *
          * @details Abstain an existing proposal
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be an existing proposal)
          * @param level - Permission level revoking approval for proposal
          */
         [[eosio::action]]
         void abstain( eosio::name proposer, eosio::name proposal_name, eosio::permission_level level );
         /**
          * Cancel proposal
          *
          * @details Cancels an existing proposal
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be an existing proposal)
          * @param canceler - The account cancelling the proposal (only the proposer can cancel an unexpired transaction, and the canceler has to be different than the proposer)
          *
          * Allows the `canceler` account to cancel the `proposal_name` proposal, created by a `proposer`,
          * only after time has expired on the proposed transaction. It removes corresponding entries from
          * internal proptable and from approval (or old approvals) tables as well.
          */
         [[eosio::action]]
         void cancel( eosio::name proposer, eosio::name proposal_name, eosio::name canceler );
         /**
          * Execute proposal
          *
          * @details Allows an `executer` account to execute a proposal.
          *
          * Preconditions:
          * - `executer` has authorization,
          * - `proposal_name` is found in the proposals table,
          * - all requested approvals are received,
          * - proposed transaction is not expired,
          * - and approval accounts are not found in invalidations table.
          *
          * If all preconditions are met the transaction is executed as a deferred transaction,
          * and the proposal is erased from the proposals table.
          *
          * @param proposer - The account proposing a transaction
          * @param proposal_name - The eosio::name of the proposal (should be an existing proposal)
          * @param executer - The account executing the transaction
          */
         [[eosio::action]]
         void exec( eosio::name proposer, eosio::name proposal_name, eosio::name executer );
         /**
          * Invalidate proposal
          *
          * @details Allows an `account` to invalidate itself, that is, its eosio::name is added to
          * the invalidations table and this table will be cross referenced when exec is performed.
          *
          * @param account - The account invalidating the transaction
          */
         [[eosio::action]]
         void invalidate( eosio::name account );

         using propose_action = eosio::action_wrapper<"propose"_n, &multisig::propose>;
         using approve_action = eosio::action_wrapper<"approve"_n, &multisig::approve>;
         using unapprove_action = eosio::action_wrapper<"unapprove"_n, &multisig::unapprove>;
         using abstain_action = eosio::action_wrapper<"abstain"_n, &multisig::abstain>;
         using cancel_action = eosio::action_wrapper<"cancel"_n, &multisig::cancel>;
         using exec_action = eosio::action_wrapper<"exec"_n, &multisig::exec>;
         using invalidate_action = eosio::action_wrapper<"invalidate"_n, &multisig::invalidate>;

      private:
         struct [[eosio::table]] proposal {
            eosio::name                            proposal_name;
            std::vector<char>               packed_transaction;

            uint64_t primary_key()const { return proposal_name.value; }
         };

         typedef eosio::multi_index< "proposal"_n, proposal > proposals;

         struct approval {
            permission_level level;
            eosio::time_point       time;
         };

         struct [[eosio::table]] approvals_info {
            uint8_t                 version = 1;
            eosio::name                    proposal_name;
            //requested approval doesn't need to cointain time, but we want requested approval
            //to be of exact the same size ad provided approval, in this case approve/unapprove
            //doesn't change serialized data size. So, we use the same type.
            std::vector<approval>   requested_approvals;
            std::vector<approval>   agree_approvals;
            std::vector<approval>   abstain_approvals;
            std::vector<approval>   disagree_approvals;

            uint64_t primary_key()const { return proposal_name.value; }
         };
         typedef eosio::multi_index< "approvals"_n, approvals_info > approvals;

         struct [[eosio::table]] invalidation {
            eosio::name         account;
            eosio::time_point   last_invalidation_time;

            uint64_t primary_key() const { return account.value; }
         };

         typedef eosio::multi_index< "invals"_n, invalidation > invalidations;

         struct [[eosio::table]] bp_punish_info {
            eosio::name                   proposal_name;
            std::vector<eosio::permission_level>   last_punish_bps;

            uint64_t primary_key()const { return proposal_name.value; }
         };

         typedef eosio::multi_index< "bppunish"_n, bp_punish_info > bp_punish_table;

         bool approvalsearch(std::vector<approval>  approvals, approval thisapproval);
   };
   /** @}*/ // end of @defgroup celesosmsig celesos.msig
} /// namespace celesos
