# celes.contracts

## Version : 1.0.0

The design of the EOSIO blockchain calls for a number of smart contracts that are run at a privileged permission level in order to support functions such as block producer registration and voting, token staking for CPU and network bandwidth, RAM purchasing, multi-sig, etc.  These smart contracts are referred to as the system, token, msig and wrap (formerly known as sudo) contracts.

This repository contains examples of these privileged contracts that are useful when deploying, managing, and/or using an EOSIO blockchain.  They are provided for reference purposes:

   * [celesos.system](https://github.com/eosio/celes.contracts/tree/master/celesos.system)
   * [celes.msig](https://github.com/eosio/celes.contracts/tree/master/celes.msig)
   * [celes.wrap](https://github.com/eosio/celes.contracts/tree/master/celes.wrap)

The following unprivileged contract(s) are also part of the system.
   * [celes.token](https://github.com/eosio/celes.contracts/tree/master/celes.token)

Dependencies:
* [celesos v1.0.0](https://github.com/celes-dev/celesos/releases/tag/v1.0.0)
* [celesos.cdt v1.0.0](https://github.com/celes-dev/celesos/releases/tag/v1.0.0)

To build the contracts and the unit tests:
* First, ensure that your __celesos__ is compiled to the core symbol for the EOSIO blockchain that intend to deploy to.
* Second, make sure that you have ```sudo make install```ed __celesos__.
* Then just run the ```build.sh``` in the top directory to build all the contracts and the unit tests for these contracts.

After build:
* The contracts are built into a _bin/\<contract name\>_ folder in their respective directories.
* Finally, simply use __cleos__ to _set contract_ by pointing to the previously mentioned directory.
