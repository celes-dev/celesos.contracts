#include <celesos.wrap/celesos.wrap.hpp>

namespace celesos {

void wrap::exec( eosio::ignore<eosio::name>, eosio::ignore<eosio::transaction> ) {
   require_auth( _self );

   eosio::name executer;
   _ds >> executer;

   require_auth( executer );

   send_deferred( (uint128_t(executer.value) << 64) | current_time(), executer.value, _ds.pos(), _ds.remaining() );
}

} /// namespace celesos

EOSIO_DISPATCH( celesos::wrap, (exec) )