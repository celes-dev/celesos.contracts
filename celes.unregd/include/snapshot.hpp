#pragma once

#include <eosiolib/asset.hpp>

namespace celes {

eosio::asset buyrambytes(uint32_t bytes);

std::vector<eosio::asset> split_snapshot(const eosio::asset& balance);

std::vector<eosio::asset> split_snapshot_abp(const eosio::asset& balance);

}  // namespace celes