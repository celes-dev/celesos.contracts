#include <celesos.test/celesos.test.hpp>

namespace celesos {

void test::helloworld() {

}

void test::sayhello(eosio::name from, eosio::name to) {
   eosio::print(from.to_string()," say hello to :",to.to_string());
}

void test::blockrandom(const uint32_t block_number){
   uint64_t block_random = block_random_by_num(block_number);
    eosio::print("block_random_by_num:",block_random);
}
} /// namespace celesos

EOSIO_DISPATCH( celesos::test, (sayhello) (blockrandom))