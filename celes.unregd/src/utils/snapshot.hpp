#pragma once

using eosio::asset;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using std::vector;

namespace celes {

asset buyrambytes(uint32_t bytes) {
    const static auto system_account = "celes"_n;
    const static auto ramcore_symbol = symbol{symbol_code{"RAMCORE"}, 4};
    const static auto ram_symbol = symbol{symbol_code{"RAM"}, 0};
    const static auto core_symbol = symbol{symbol_code{"CELES"}, 4};

    celesos::rammarket market{system_account, system_account.value};
    auto itr = market.find(ramcore_symbol.raw());
    eosio_assert(itr != market.end(), "RAMCORE market not found");
    auto tmp = *itr;
    return tmp.convert(asset{bytes, ram_symbol}, core_symbol);
}

vector<asset> split_snapshot(const asset& balance) {
    const auto symbol = balance.symbol;
    if (balance < asset{5000, symbol}) {
        return {};
    }

    // everyone has minimum 0.25 EOS staked
    // some 10 EOS unstaked
    // the rest split between the two

    auto cpu = asset{2500, symbol};
    auto net = asset{2500, symbol};

    auto remainder = balance - cpu - net;

    if (remainder <= asset{100000, symbol}) /* 10.0 EOS */
    {
        return {net, cpu, remainder};
    }

    remainder -= asset{100000, symbol};  // keep them floating, unstaked

    auto first_half = remainder / 2;
    cpu += first_half;
    net += remainder - first_half;

    return {net, cpu, asset{100000, symbol}};
}

vector<asset> split_snapshot_abp(const asset& balance) {
    const auto symbol = balance.symbol;
    eosio_assert(balance >= asset{1000, symbol}, "insuficient balance");

    asset floatingAmount{};

    if (balance > asset{110000, symbol}) {
        floatingAmount = asset{100000, symbol};
    } else if (balance > asset{30000, symbol}) {
        floatingAmount = asset{20000, symbol};
    } else {
        floatingAmount = asset{1000, symbol};
    }

    asset to_split = balance - floatingAmount;
    asset split_cpu = to_split / 2;
    asset split_net = to_split - split_cpu;

    return {split_net, split_cpu, floatingAmount};
}

}  // namespace celes