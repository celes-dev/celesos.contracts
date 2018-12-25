#include <celesos.msig/celesos.msig.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/crypto.hpp>
#include <eosiolib/privileged.hpp>

namespace celesos
{

eosio::time_point current_time_point()
{
   const static eosio::time_point ct{eosio::microseconds{static_cast<int64_t>(current_time())}};
   return ct;
}

void multisig::propose(eosio::ignore<eosio::name> proposer,
                       eosio::ignore<eosio::name> proposal_name,
                       eosio::ignore<std::vector<eosio::permission_level>> requested,
                       eosio::ignore<eosio::transaction> trx)
{
   eosio::name _proposer;
   eosio::name _proposal_name;
   std::vector<eosio::permission_level> _requested;
   eosio::transaction_header _trx_header;

   _ds >> _proposer >> _proposal_name >> _requested;

   const char *trx_pos = _ds.pos();
   size_t size = _ds.remaining();
   _ds >> _trx_header;

   require_auth(_proposer);
   eosio_assert(_trx_header.expiration >= eosio::time_point_sec(current_time_point()), "transaction expired");
   //eosio_assert( trx_header.actions.size() > 0, "transaction must have at least one action" );

   proposals proptable(_self, _proposer.value);
   eosio_assert(proptable.find(_proposal_name.value) == proptable.end(), "proposal with the same name exists");

   auto packed_requested = pack(_requested);
   auto res = ::check_transaction_authorization(trx_pos, size,
                                                (const char *)0, 0,
                                                packed_requested.data(), packed_requested.size());
   eosio_assert(res > 0, "transaction authorization failed");

   std::vector<char> pkd_trans;
   pkd_trans.resize(size);
   memcpy((char *)pkd_trans.data(), trx_pos, size);
   proptable.emplace(_proposer, [&](auto &prop) {
      prop.proposal_name = _proposal_name;
      prop.packed_transaction = pkd_trans;
   });

   approvals apptable(_self, _proposer.value);
   apptable.emplace(_proposer, [&](auto &a) {
      a.proposal_name = _proposal_name;
      a.requested_approvals.reserve(_requested.size());
      for (auto &level : _requested)
      {
         a.requested_approvals.push_back(approval{level, eosio::time_point{eosio::microseconds{0}}});
      }
   });
}

void multisig::approve(eosio::name proposer, eosio::name proposal_name, eosio::permission_level level,
                       const eosio::binary_extension<eosio::checksum256> &proposal_hash)
{
   require_auth(level);

   if (proposal_hash)
   {
      proposals proptable(_self, proposer.value);
      auto &prop = proptable.get(proposal_name.value, "proposal not found");
      assert_sha256(prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash);
   }

   approvals apptable(_self, proposer.value);
   auto apps_it = apptable.get(proposal_name.value, "proposal not found");
   auto itr = std::find_if(apps_it.requested_approvals.begin(), apps_it.requested_approvals.end(), [&](const approval &a) { return a.level == level; });
   eosio_assert(itr != apps_it.requested_approvals.end(), "approval is not on the list of requested approvals");

   apptable.modify(apps_it, proposer, [&](auto &a) {
      a.agree_approvals.push_back(approval{level, current_time_point()});
      a.abstain_approvals.erase(itr);
      a.disagree_approvals.erase(itr);
   });
}

void multisig::abstain(eosio::name proposer, eosio::name proposal_name, eosio::permission_level level)
{
   require_auth(level);

   approvals apptable(_self, proposer.value);
   auto apps_it = apptable.get(proposal_name.value, "proposal not found");
   auto itr = std::find_if(apps_it.requested_approvals.begin(), apps_it.requested_approvals.end(), [&](const approval &a) { return a.level == level; });
   eosio_assert(itr != apps_it.requested_approvals.end(), "approval is not on the list of requested approvals");

   apptable.modify(apps_it, proposer, [&](auto &a) {
      a.agree_approvals.erase(itr);
      a.abstain_approvals.push_back(approval{level, current_time_point()});
      a.disagree_approvals.erase(itr);
   });
}

void multisig::unapprove(eosio::name proposer, eosio::name proposal_name, eosio::permission_level level)
{
   require_auth(level);

   approvals apptable(_self, proposer.value);
   auto apps_it = apptable.get(proposal_name.value, "proposal not found");
   auto itr = std::find_if(apps_it.agree_approvals.begin(), apps_it.agree_approvals.end(), [&](const approval &a) { return a.level == level; });
   eosio_assert(itr != apps_it.agree_approvals.end(), "no approval previously granted");
   apptable.modify(apps_it, proposer, [&](auto &a) {
      a.agree_approvals.erase(itr);
      a.abstain_approvals.erase(itr);
      a.disagree_approvals.push_back(approval{level, current_time_point()});
   });
}

void multisig::cancel(eosio::name proposer, eosio::name proposal_name, eosio::name canceler)
{
   require_auth(canceler);

   proposals proptable(_self, proposer.value);
   auto &prop = proptable.get(proposal_name.value, "proposal not found");

   if (canceler != proposer)
   {
      eosio_assert(eosio::unpack<eosio::transaction_header>(prop.packed_transaction).expiration < eosio::time_point_sec(current_time_point()), "cannot cancel until expiration");
   }
   proptable.erase(prop);

   //remove from new table
   approvals apptable(_self, proposer.value);
   auto apps_it = apptable.get(proposal_name.value, "proposal not found");
   apptable.erase(apps_it);
}

void multisig::exec(eosio::name proposer, eosio::name proposal_name, eosio::name executer)
{
   require_auth(executer);

   proposals proptable(_self, proposer.value);
   auto &prop = proptable.get(proposal_name.value, "proposal not found");
   eosio::transaction_header trx_header;
   eosio::datastream<const char *> ds(prop.packed_transaction.data(), prop.packed_transaction.size());
   ds >> trx_header;
   eosio_assert(trx_header.expiration >= eosio::time_point_sec(current_time_point()), "eosio::transaction expired");

   approvals apptable(_self, proposer.value);
   auto apps_it = apptable.get(proposal_name.value, "proposal not found");
   std::vector<eosio::permission_level> approvals;

   invalidations inv_table(_self, _self.value);
   approvals.reserve(apps_it.agree_approvals.size());
   for (auto &p : apps_it.agree_approvals)
   {
      auto it = inv_table.find(p.level.actor.value);
      if (it == inv_table.end() || it->last_invalidation_time < p.time)
      {
         approvals.push_back(p.level);
      }
   }

   auto requested_approvals = apps_it.requested_approvals;
   auto agree_approvals = apps_it.agree_approvals;
   auto abstain_approvals = apps_it.abstain_approvals;
   auto disagree_approvals = apps_it.disagree_approvals;

   apptable.erase(apps_it);

   auto packed_agree_approvals = pack(approvals);
   auto res = ::check_transaction_authorization(prop.packed_transaction.data(), prop.packed_transaction.size(),
                                                (const char *)0, 0,
                                                packed_agree_approvals.data(), packed_agree_approvals.size());
   eosio_assert(res > 0, "eosio::transaction authorization failed");

   send_deferred((uint128_t(proposer.value) << 64) | proposal_name.value, executer.value,
                 prop.packed_transaction.data(), prop.packed_transaction.size());

   proptable.erase(prop);

   if (is_systemaccount_transaction(prop.packed_transaction.data(), prop.packed_transaction.size()))
   {
      std::vector<eosio::permission_level> this_punishs;

      for (auto p : requested_approvals)
      {
         if (!approvalsearch(agree_approvals, p) && !approvalsearch(abstain_approvals, p) && !approvalsearch(disagree_approvals, p))
         {
            this_punishs.emplace_back(std::move(p.level));
         }
      }

      bp_punish_table bp_punishs(_self, _self.value);
      auto itr = bp_punishs.begin();

      if (itr != bp_punishs.end())
      {
         for (auto p : this_punishs)
         {
            auto last = itr->last_punish_bps;
            auto result = find(last.begin(), last.end(), p);
            if (result != last.end())
            {
               // INLINE_ACTION_SENDER(celesos::system_contract, limitbp)
               // (
               //     config::system_account_name, {{config::system_account_name, active_permission}},
               //     {});
               eosio::action(
                   eosio::permission_level{_self, "active"_n},
                   "celes"_n, "limitbp"_n, //调用 eosio.token 的 Transfer 合约
                   std::make_tuple(result->actor))
                   .send();
            }
         }

         bp_punishs.erase(itr);
      }

      bp_punishs.emplace(_self, [&](auto &a) {
         a.proposal_name = proposal_name;
         a.last_punish_bps = this_punishs;
      });
   }
}

void multisig::invalidate(eosio::name account)
{
   require_auth(account);
   invalidations inv_table(_self, _self.value);
   auto it = inv_table.find(account.value);
   if (it == inv_table.end())
   {
      inv_table.emplace(account, [&](auto &i) {
         i.account = account;
         i.last_invalidation_time = current_time_point();
      });
   }
   else
   {
      inv_table.modify(it, account, [&](auto &i) {
         i.last_invalidation_time = current_time_point();
      });
   }
}

bool multisig::approvalsearch(std::vector<approval> approvals, approval thisapproval)
{
   for (auto temp : approvals)
   {
      if (temp.level == thisapproval.level)
      {
         return true;
      }
      else
      {
         continue;
      }
   }

   return false;
} // namespace celesos

} // namespace celesos

EOSIO_DISPATCH(celesos::multisig, (propose)(approve)(unapprove)(cancel)(exec)(invalidate))