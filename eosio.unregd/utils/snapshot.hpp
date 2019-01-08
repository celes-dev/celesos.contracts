#pragma once

namespace eosio {

asset buyrambytes(uint32_t bytes) {
    eosiosystem::rammarket market{"eosio"_n, "eosio"_n.value};
    auto ramcore_symbol = symbol{symbol_code{"RAMCODE"}, 4};
    auto itr = market.find(ramcore_symbol.raw());
    eosio_assert(itr != market.end(), "RAMCORE market not found");
    auto tmp = *itr;
    auto ram_symbol = symbol{symbol_code{"RAM"}, 0};
    auto core_symbol = tmp.quote.balance.symbol;
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

}  // namespace eosio