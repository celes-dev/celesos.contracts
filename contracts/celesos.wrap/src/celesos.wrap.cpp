#include <celesos.wrap/celesos.wrap.hpp>

namespace celesos {

void wrap::exec( ignore<eosio::name>, ignore<transaction> ) {
   require_auth( get_self() );

   eosio::name executer;
   _ds >> executer;

   require_auth( executer );

   send_deferred( (uint128_t(executer.value) << 64) | (uint64_t)eosio::current_time_point().time_since_epoch().count(),
                  executer, _ds.pos(), _ds.remaining() );
}

} /// namespace celesos
