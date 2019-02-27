#include <celesos.system/celesos.system.hpp>
#include <celes.token/celes.token.hpp>
#include <math.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

namespace celesos
{
const int64_t useconds_per_day = 24 * 3600 * int64_t(1000000);

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
                if (btoken_balance.amount > 0)
                {
                    uint32_t bhalftime = static_cast<uint32_t>(log(BPAY_POOL_FULL / btoken_balance.amount) / log(2));
                    int64_t bpayamount = MAX(1, static_cast<int64_t>(ORIGIN_REWARD_NUMBER_BPAY * pow(0.5, bhalftime)));

                    INLINE_ACTION_SENDER(celes::token, transfer)
                    (token_account, {{bpaypool_account, active_permission}, {bpay_account, active_permission}}, {bpaypool_account, bpay_account, asset(bpayamount, core_symbol()), "block pay pool"});

                    _gstate.total_unpaid_block_fee = _gstate.total_unpaid_block_fee + bpayamount;
                    _producers.modify(prod, eosio::same_payer, [&](auto &p) {
                        p.unpaid_block_fee = p.unpaid_block_fee + bpayamount;
                    });
                }
            }

            {
                asset wtoken_balance = celes::token::get_balance(token_account, wpaypool_account, core_symbol().code());
                if (wtoken_balance.amount > 0)
                {
                    uint32_t whalftime = static_cast<uint32_t>(log(WPAY_POOL_FULL / wtoken_balance.amount) / log(2));
                    int64_t wpayamount = MAX(1, static_cast<int64_t>(ORIGIN_REWARD_NUMBER_WPAY * pow(0.5, whalftime)));
                    INLINE_ACTION_SENDER(celes::token, transfer)
                    (token_account, {{wpaypool_account, active_permission}, {wpay_account, active_permission}}, {wpaypool_account, wpay_account, asset(wpayamount, core_symbol()), "wood pay pool"});
                }
            }
        }

        {
            asset dtoken_balance = celes::token::get_balance(token_account, dpaypool_account, core_symbol().code());
            if (dtoken_balance.amount > 0)
            {
                uint32_t dhalftime = static_cast<uint32_t>(log(DPAY_POOL_FULL / dtoken_balance.amount) / log(2));
                int64_t dpayamount = MAX(1, static_cast<int64_t>(ORIGIN_REWARD_NUMBER_DPAY * pow(0.5, dhalftime)));
                INLINE_ACTION_SENDER(celes::token, transfer)
                (token_account, {{dpaypool_account, active_permission}, {dpay_account, active_permission}}, {dpaypool_account, dpay_account, asset(dpayamount, core_symbol()), "dbp pay pool"});
            }
        }
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
            if ((timestamp.slot - _gstate.last_name_close.slot) >= SINGING_TICKER_SEP * 6)
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

    if (!_gstate.is_dbp_active && _gstate.is_network_active)
    {
        if (head_block_number - _gstate.network_active_block >= DBP_ACTIVE_SEP)
        {
            activedbp();
        }
    }
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

        asset bpay_balance = celes::token::get_balance(token_account, bpay_account, core_symbol().code());
        asset wpay_balance = celes::token::get_balance(token_account, wpay_account, core_symbol().code());
        asset dpay_balance = celes::token::get_balance(token_account, dpay_account, core_symbol().code());

        int32_t punishCount = (bppunish_info == _dbpunishs.end()) ? 0 : bppunish_info->punish_count;

        if (prod != _producers.end() && prod->is_active)
        {
            if (ct - prod->last_claim_time <= eosio::microseconds((1 + punishCount) * REWARD_TIME_SEP))
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
                if (_gstate.total_unpaid_wood > 0 && prod->unpaid_wood > 0)
                {
                    if (wpay_balance.amount > 0)
                    {
                        wpay = MAX(1, wpay_balance.amount * prod->unpaid_wood / _gstate.total_unpaid_wood);
                    }
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
                if (_gstate.is_dbp_active)
                {
                    int64_t all_unpaid_resouresweight = total_unpaid_resouresweight();
                    int64_t this_unpaid_resouresweight = unpaid_resouresweight(owner.value);
                    if (all_unpaid_resouresweight > 0 && this_unpaid_resouresweight > 0 && all_unpaid_resouresweight >= this_unpaid_resouresweight)
                    {
                        if (dpay_balance.amount > 0)
                        {
                            dpay = MAX(1, dpay_balance.amount * this_unpaid_resouresweight / all_unpaid_resouresweight);
                        }
                    }
                }
                else
                {
                    dpay = DAPP_PAY_UNACTIVE;
                }
            }
        }

        if (bpay + wpay + dpay < REWARD_GET_MIN)
        {
            bpay = 0;
            wpay = 0;
            dpay = 0;
        }

        if (bpay > 0 && bpay_balance.amount > bpay)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{bpay_account, active_permission}, {owner, active_permission}}, {bpay_account, owner, asset(bpay, core_symbol()), "unallocated inflation"});
            _gstate.total_unpaid_block_fee = _gstate.total_unpaid_block_fee - bpay;
        }

        if (wpay > 0 && wpay_balance.amount > wpay)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{wpay_account, active_permission}, {owner, active_permission}}, {wpay_account, owner, asset(wpay, core_symbol()), "wood pay"});
            _gstate.total_unpaid_wood = _gstate.total_unpaid_wood - prod->unpaid_wood;
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

        if (dpay > 0 && dpay_balance.amount > dpay)
        {
            INLINE_ACTION_SENDER(celes::token, transfer)
            (token_account, {{dpay_account, active_permission}, {owner, active_permission}}, {dpay_account, owner, asset(dpay, core_symbol()), "dapp pay"});

            setclaimed(owner.value);
        }

        if (dbp != _dbps.end())
        {
            _dbps.modify(dbp, eosio::same_payer, [&](auto &p) {
                p.last_claim_time = ct;
            });
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

void system_contract::activedbp()
{
    require_auth(_self);

    eosio_assert(!_gstate.is_dbp_active, "dbp is actived");
    eosio_assert(_gstate.is_network_active, "network is not actived");
    _gstate.is_dbp_active = true;
    _gstate.dbp_active_block = get_chain_head_num();

    asset dtoken_balance = celes::token::get_balance(token_account, dpay_account, core_symbol().code());
    if (dtoken_balance.amount > 0)
    {
        INLINE_ACTION_SENDER(celes::token, transfer)
        (token_account, {{dpay_account, active_permission}}, {dpay_account, dpaypool_account, asset(dtoken_balance.amount, core_symbol()), "dbp pay pool"});
    }
}

} //namespace celesos
