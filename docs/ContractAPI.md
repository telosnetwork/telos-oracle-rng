# Telos RNG Oracle Contract API

## ACTION init()

* string app_name
* string app_version
* name initial_admin

## ACTION setversion()

* string new_version

## ACTION setadmin()

* name new_admin

## ACTION rmvrequest()

* uint64_t request_id

## ACTION requestrand()

* uint64_t caller_id
* uint64_t seed
* name caller

## ACTION submitrand()

* uint64_t request_id
* name oracle_name
* signature sig

## 
