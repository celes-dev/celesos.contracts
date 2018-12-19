#include <celesos.test/celesos.test.hpp>

namespace celesos {

void test::sayhello(eosio::name from, eosio::name to) {

   eosio::print(from.to_string()," say hello to :",to.to_string());
}

} /// namespace celesos

EOSIO_DISPATCH( celesos::test, (sayhello) )