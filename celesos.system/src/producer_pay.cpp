#include <celesos.system/celesos.system.hpp>
#include <celes.token/celes.token.hpp>
#include <math.h>

namespace celesos
{

const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600; // half seconds per year
const uint32_t blocks_per_day = 2 * 24 * 3600;
const uint32_t blocks_per_hour = 2 * 3600;
const int64_t useconds_per_day = 24 * 3600 * int64_t(1000000);
const int64_t useconds_per_year = seconds_per_year * 1000000ll;

void system_contract::onblock(ignore<block_header>)
{
    using namespace eosio;

    block_timestamp timestamp;
    name producer;
    _ds >> timestamp >> producer;

    require_auth(_self);

    uint32_t head_block_number = get_chain_head_num();

    if (_gstate.is_network_active)
    {
        auto prod = _producers.find(producer.value);
        if (prod != _producers.end())
        {
            {
                asset btoken_balance = celes::token::get_balance(token_account, bpaypool_account, core_symbol().code());
                uint32_t bhalftime = static_cast<uint32_t>(log(BPAY_POOL_FULL / btoken_balance.amount) / log(2));
                int64_t bpayamount = static_cast<uint32_t>(ORIGIN_REWARD_NUMBER_BPAY * pow(0.5, bhalftime));

                INLINE_ACTION_SENDER(celes::token, transfer)
                (token_account, {{bpaypool_account, active_permission}, {bpay_account, active_permission}}, {bpaypool_account, bpay_account, asset(bpayamount, core_symbol()), "block pay pool"});

                _gstate.total_unpaid_block_fee = _gstate.total_unpaid_block_fee - prod->unpaid_block_fee;
                _producers.modify(prod, eosio::same_payer, [&](auto &p) {
                    p.unpaid_block_fee = p.unpaid_block_fee + bpayamount;
                });
            }

            {
                asset wtoken_balance = celes::token::get_balance(token_account, wpaypool_account, core_symbol().code());
                uint32_t whalftime = static_cast<uint32_t>(log(WPAY_POOL_FULL / wtoken_balance.amount) / log(2));
                int64_t wpayamount = static_cast<uint32_t>(ORIGIN_REWARD_NUMBER_WPAY * pow(0.5, whalftime));
                INLINE_ACTION_SENDER(celes::token, transfer)
                (token_account, {{wpaypool_account, active_permission}, {wpay_account, active_permission}}, {wpaypool_account, wpay_account, asset(wpayamount, core_symbol()), "wood pay pool"});
            }
        }
    }

    {
        asset dtoken_balance = celes::token::get_balance(token_account, dpaypool_account, core_symbol().code());
        uint32_t dhalftime = static_cast<uint32_t>(log(DPAY_POOL_FULL / dtoken_balance.amount) / log(2));
        int64_t dpayamount = static_cast<uint32_t>(ORIGIN_REWARD_NUMBER_DPAY * pow(0.5, dhalftime));

        INLINE_ACTION_SENDER(celes::token, transfer)
        (token_account, {{dpaypool_account, active_permission}, {dpay_account, active_permission}}, {dpaypool_account, dpay_account, asset(dpayamount, core_symbol()), "dbp pay pool"});
    }

    if (head_block_number % (uint32_t)forest_space_number() == 1)
    {
        set_difficulty(calc_diff(head_block_number));
        clean_diff_stat_history(head_block_number);
    }

    // 即将开始唱票，提前清理数据
    // ready to singing the voting
    if (_gstate.last_producer_schedule_block + SINGING_TICKER_SEP <= head_block_number + 30 - 1)
    {
        uint32_t guess_modify_block = _gstate.last_producer_schedule_block + SINGING_TICKER_SEP;
        if (head_block_number > guess_modify_block)
            guess_modify_block = head_block_number; // Next singing blocktime
        clean_dirty_stat_producers(guess_modify_block, 5);
    }

    /// only update block producers once every minute, block_timestamp is in half seconds
    if (head_block_number - _gstate.last_producer_schedule_block >= SINGING_TICKER_SEP)
    {
        update_elected_producers(head_block_number);

        if (_gstate.is_network_active)
        {
            if ((timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day)
            {
                name_bid_table bids(_self, _self.value);
                auto idx = bids.get_index<"highbid"_n>();
                auto highest = idx.lower_bound(std::numeric_limits<uint64_t>::max() / 2);
                if (highest != idx.end() &&
                    highest->high_bid > 0 &&
                    (current_time_point() - highest->last_bid_time) > microseconds(useconds_per_day))
                {
                    _gstate.last_name_close = timestamp;
                    idx.modify(highest, eosio::same_payer, [&](auto &b) {
                        b.high_bid = -b.high_bid;
                    });
                }
            }
        }
    }

    ramattenuator();
}

using namespace eosio;
void system_contract::claimrewards(const name owner)
{
    require_auth(owner);

    // block fee and wood fee
    if (_gstate.is_network_active)
    {
        int64_t bpay = 0;
        int64_t wpay = 0;
        int64_t dpay = 0;

        const auto ct = current_time_point();
        auto prod = _producers.find(owner.value);
        auto dbp = _dbps.find(owner.value);
        auto bppunish_info = _dbpunishs.find(owner.value);

        int32_t punishCount = (bppunish_info == _dbpunishs.end()) ? 0 : bppunish_info->punish_count;

        if (prod != _producers.end() && prod->is_active)
        {
            if (ct - prod->last_claim_time <= eosio::microseconds((1 + punishCount) * useconds_per_day / 4))
            {
                eosio_assert(false, "already claimed rewards within past 6 hours or punished");
            }
            else
            {
                // block fee
                if (prod->unpaid_block_fee > 0)
                {
                    if (prod->unpaid_block_fee > 0)
                    {
                        bpay = prod->unpaid_block_fee;
                    }
                }

                // wood fee
                if (_gstate2.total_unpaid_wood > 0 && prod->unpaid_wood > 0)
                {
                    asset token_balance = celes::token::get_balance(token_account, wpay_account, core_symbol().code());
                    wpay = token_balance.amount * prod->unpaid_wood / _gstate2.total_unpaid_wood;
                }
            }
        }

        // dapppay
        if (dbp != _dbps.end())
        {
            if (ct - dbp->last_claim_time <= eosio::microseconds((1 + punishCount) * REWARD_TIME_SEP))
            {
                eosio_assert(false, "already claimed rewards within past 6 hours or punished");
            }
            else
            {
                asset token_balance = celes::token::get_balance(token_account, dpay_account, core_symbol().code());
                if (_gstate2.is_dbp_active)
                {
                    int64_t all_unpaid_resouresweight = total_unpaid_resouresweight();
                    int64_t this_unpaid_resouresweight = unpaid_resouresweight(owner.value);
                    if (all_unpaid_resouresweight > 0 && this_unpaid_resouresweight > 0 && all_unpaid_resouresweight >= this_unpaid_resouresweight)
                    {
                        dpay = token_balance.amount * this_unpaid_resouresweight / all_unpaid_resouresweight;
                    }
                }
                else
                {
                    if (token_balance.amount >= DAPP_PAY_UNACTIVE)
                    {
                        dpay = DAPP_PAY_UNACTIVE;
                    }
                }
            }
        }

        if (bpay + wpay + dpay < REWARD_GET_MIN)
        {
            bpay = 0;
            wpay = 0;
            dpay = 0;
        }

        if (bpay > 0)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{bpay_account, active_permission}, {owner, active_permission}}, {bpay_account, owner, asset(bpay, core_symbol()), "unallocated inflation"});
            _gstate.total_unpaid_block_fee = _gstate.total_unpaid_block_fee - bpay;
        }

        if (wpay > 0)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{wpay_account, active_permission}, {owner, active_permission}}, {wpay_account, owner, asset(wpay, core_symbol()), "wood pay"});
        }

        if (prod != _producers.end())
        {
            _producers.modify(prod, eosio::same_payer, [&](auto &p) {
                p.last_claim_time = ct;
                if (wpay > 0)
                {
                    p.unpaid_wood = 0;
                }
                if (bpay > 0)
                {
                    p.unpaid_block_fee = 0;
                }
            });
        }

        if (dpay > 0)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{dpay_account, active_permission}, {owner, active_permission}}, {dpay_account, owner, asset(dpay, core_symbol()), "dapp pay"});

            setclaimed(owner.value);

            if (dbp != _dbps.end())
            {
                _dbps.modify(dbp, eosio::same_payer, [&](auto &p) {
                    p.last_claim_time = ct;
                });
            }
        }

        if (punishCount > 0)
        {
            _dbpunishs.erase(bppunish_info);
        }
    }
} // namespace celesos

void system_contract::limitbp(const name &owner_name)
{
    require_auth(_self);
    auto bppunish_info = _dbpunishs.find(owner_name.value);
    if (bppunish_info == _dbpunishs.end())
    {
        _dbpunishs.emplace(_self, [&](auto &s) {
            s.owner = owner_name;
            s.punish_count = 1;
        });
    }
    else
    {
        _dbpunishs.modify(bppunish_info, eosio::same_payer, [&](bp_punish_info &info) {
            info.punish_count = info.punish_count + 1;
        });
    };
}

void system_contract::unlimitbp(const name &owner_name)
{
    require_auth(_self);
    auto bppunish_info = _dbpunishs.find(owner_name.value);
    eosio_assert(bppunish_info != _dbpunishs.end(), "account  is not punished");

    if (bppunish_info->punish_count > 1)
    {
        _dbpunishs.modify(bppunish_info, eosio::same_payer, [&](bp_punish_info &info) {
            info.punish_count = info.punish_count - 1;
        });
    }
    else
    {
        _dbpunishs.erase(bppunish_info);
    }
}

} //namespace celesos
