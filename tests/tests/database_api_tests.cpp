/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/app/database_api.hpp>
#include <graphene/chain/hardfork.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(database_api_tests, database_fixture)

  BOOST_AUTO_TEST_CASE(is_registered) {
      try {
          /***
           * Arrange
           */
          auto nathan_private_key = generate_private_key("nathan");
          public_key_type nathan_public = nathan_private_key.get_public_key();

          auto dan_private_key = generate_private_key("dan");
          public_key_type dan_public = dan_private_key.get_public_key();

          auto unregistered_private_key = generate_private_key("unregistered");
          public_key_type unregistered_public = unregistered_private_key.get_public_key();


          /***
           * Act
           */
          create_account("dan", dan_private_key.get_public_key()).id;
          create_account("nathan", nathan_private_key.get_public_key()).id;
          // Unregistered key will not be registered with any account


          /***
           * Assert
           */
          graphene::app::database_api db_api(db);

          BOOST_CHECK(db_api.is_public_key_registered((string) nathan_public));
          BOOST_CHECK(db_api.is_public_key_registered((string) dan_public));
          BOOST_CHECK(!db_api.is_public_key_registered((string) unregistered_public));

      } FC_LOG_AND_RETHROW()
  }

  BOOST_AUTO_TEST_CASE( get_potential_signatures_owner_and_active )
{
   try {
      fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
      public_key_type pub_key_active( nathan_key1.get_public_key() );
      public_key_type pub_key_owner( nathan_key2.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key1.get_public_key() );

      try {
         account_update_operation op;
         op.account = nathan.id;
         op.active = authority(1, pub_key_active, 1);
         op.owner = authority(1, pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, nathan_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.clear();
      } FC_CAPTURE_AND_RETHROW ((nathan.active))

      // this op requires active
      transfer_operation op;
      op.from = nathan.id;
      op.to = account_id_type();
      trx.operations.push_back(op);

      graphene::app::database_api db_api(db);
      set<public_key_type> pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      trx.operations.clear();

      // this op requires owner
      account_update_operation auop;
      auop.account = nathan.id;
      auop.owner = authority(1, pub_key_owner, 1);
      trx.operations.push_back(auop);

      pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() ); // active key doesn't help in this case
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

/// Testing get_potential_signatures and get_required_signatures for non-immediate owner authority issue.
/// https://github.com/bitshares/bitshares-core/issues/584
BOOST_AUTO_TEST_CASE( get_signatures_non_immediate_owner )
{
   try {
      fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
      fc::ecc::private_key ashley_key1 = fc::ecc::private_key::regenerate(fc::digest("akey1"));
      fc::ecc::private_key ashley_key2 = fc::ecc::private_key::regenerate(fc::digest("akey2"));
      fc::ecc::private_key oliver_key1 = fc::ecc::private_key::regenerate(fc::digest("okey1"));
      fc::ecc::private_key oliver_key2 = fc::ecc::private_key::regenerate(fc::digest("okey2"));
      public_key_type pub_key_active( nathan_key1.get_public_key() );
      public_key_type pub_key_owner( nathan_key2.get_public_key() );
      public_key_type a_pub_key_active( ashley_key1.get_public_key() );
      public_key_type a_pub_key_owner( ashley_key2.get_public_key() );
      public_key_type o_pub_key_active( oliver_key1.get_public_key() );
      public_key_type o_pub_key_owner( oliver_key2.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key1.get_public_key() );
      const account_object& ashley = create_account("ashley", ashley_key1.get_public_key() );
      const account_object& oliver = create_account("oliver", oliver_key1.get_public_key() );
      account_id_type nathan_id = nathan.id;
      account_id_type ashley_id = ashley.id;
      account_id_type oliver_id = oliver.id;

      try {
         account_update_operation op;
         op.account = nathan_id;
         op.active = authority(1, pub_key_active, 1, ashley_id, 1);
         op.owner = authority(1, pub_key_owner, 1, oliver_id, 1);
         trx.operations.push_back(op);
         sign(trx, nathan_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.clear();

         op.account = ashley_id;
         op.active = authority(1, a_pub_key_active, 1);
         op.owner = authority(1, a_pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, ashley_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.clear();

         op.account = oliver_id;
         op.active = authority(1, o_pub_key_active, 1);
         op.owner = authority(1, o_pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, oliver_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.clear();
      } FC_CAPTURE_AND_RETHROW ((nathan.active))

      // this transaction requires active
      signed_transaction trx_a;
      transfer_operation op;
      op.from = nathan_id;
      op.to = account_id_type();
      trx_a.operations.push_back(op);

      // get potential signatures
      graphene::app::database_api db_api(db);
      set<public_key_type> pub_keys = db_api.get_potential_signatures( trx_a );

      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_active ) != pub_keys.end() );
      // doesn't work due to https://github.com/bitshares/bitshares-core/issues/584
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_active ) != pub_keys.end() );
      // doesn't work due to https://github.com/bitshares/bitshares-core/issues/584
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) == pub_keys.end() );

      // get required signatures
      pub_keys = db_api.get_required_signatures( trx_a, { a_pub_key_owner, o_pub_key_owner } );
      BOOST_CHECK( pub_keys.empty() );

      // this op requires owner
      signed_transaction trx_o;
      account_update_operation auop;
      auop.account = nathan_id;
      auop.owner = authority(1, pub_key_owner, 1);
      trx_o.operations.push_back(auop);

      // get potential signatures
      pub_keys = db_api.get_potential_signatures( trx_o );

      // active authorities doesn't help in this case
      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) == pub_keys.end() );

      // owner authorities should be ok
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_active ) != pub_keys.end() );
      // doesn't work due to https://github.com/bitshares/bitshares-core/issues/584
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) == pub_keys.end() );

      // get required signatures
      pub_keys = db_api.get_required_signatures( trx_o, { a_pub_key_owner, o_pub_key_owner } );
      BOOST_CHECK( pub_keys.empty() );

      // go beyond hard fork
      generate_blocks( HARDFORK_1002_TIME, true );

      // for the transaction that requires active
      // get potential signatures
      pub_keys = db_api.get_potential_signatures( trx_a );

      // all authorities should be ok
      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) != pub_keys.end() );

      // get required signatures
      pub_keys = db_api.get_required_signatures( trx_a, { a_pub_key_owner } );
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) != pub_keys.end() );
      pub_keys = db_api.get_required_signatures( trx_a, { o_pub_key_owner } );
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) != pub_keys.end() );

      // for the transaction that requires owner
      // get potential signatures
      pub_keys = db_api.get_potential_signatures( trx_o );

      // active authorities doesn't help in this case
      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) == pub_keys.end() );

      // owner authorities should help
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) != pub_keys.end() );

      // get required signatures
      pub_keys = db_api.get_required_signatures( trx_o, { a_pub_key_owner, o_pub_key_owner } );
      BOOST_CHECK( pub_keys.find( a_pub_key_owner ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( o_pub_key_owner ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_potential_signatures_other )
{
   try {
      fc::ecc::private_key priv_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      public_key_type pub_key1( priv_key1.get_public_key() );

      const account_object& nathan = create_account( "nathan" );

      balance_claim_operation op;
      op.deposit_to_account = nathan.id;
      op.balance_owner_key = pub_key1;
      trx.operations.push_back(op);

      graphene::app::database_api db_api(db);
      set<public_key_type> pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key1 ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_required_signatures_owner_or_active )
{
   try {
      fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
      public_key_type pub_key_active( nathan_key1.get_public_key() );
      public_key_type pub_key_owner( nathan_key2.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key1.get_public_key() );

      try {
         account_update_operation op;
         op.account = nathan.id;
         op.active = authority(1, pub_key_active, 1);
         op.owner = authority(1, pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, nathan_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.clear();
      } FC_CAPTURE_AND_RETHROW ((nathan.active))

      graphene::app::database_api db_api(db);

      // prepare available keys sets
      flat_set<public_key_type> avail_keys1, avail_keys2, avail_keys3;
      avail_keys1.insert( pub_key_active );
      avail_keys2.insert( pub_key_owner );
      avail_keys3.insert( pub_key_active );
      avail_keys3.insert( pub_key_owner );

      set<public_key_type> pub_keys;

      // this op requires active
      transfer_operation op;
      op.from = nathan.id;
      op.to = account_id_type();
      trx.operations.push_back(op);

      // provides active, should be ok
      pub_keys = db_api.get_required_signatures( trx, avail_keys1 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );

      // provides owner, should be ok
      pub_keys = db_api.get_required_signatures( trx, avail_keys2 );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      // provides both active and owner, should return one of them
      pub_keys = db_api.get_required_signatures( trx, avail_keys3 );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() || pub_keys.find( pub_key_owner ) != pub_keys.end() );

      trx.operations.clear();

      // this op requires owner
      account_update_operation auop;
      auop.account = nathan.id;
      auop.owner = authority(1, pub_key_owner, 1);
      trx.operations.push_back(auop);

      // provides active, should return an empty set
      pub_keys = db_api.get_required_signatures( trx, avail_keys1 );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides owner, should return it
      pub_keys = db_api.get_required_signatures( trx, avail_keys2 );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      // provides both active and owner, should return owner only
      pub_keys = db_api.get_required_signatures( trx, avail_keys3 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_required_signatures_partially_signed_or_not )
{
   try {
      fc::ecc::private_key morgan_key = fc::ecc::private_key::regenerate(fc::digest("morgan_key"));
      fc::ecc::private_key nathan_key = fc::ecc::private_key::regenerate(fc::digest("nathan_key"));
      fc::ecc::private_key oliver_key = fc::ecc::private_key::regenerate(fc::digest("oliver_key"));
      public_key_type pub_key_morgan( morgan_key.get_public_key() );
      public_key_type pub_key_nathan( nathan_key.get_public_key() );
      public_key_type pub_key_oliver( oliver_key.get_public_key() );
      const account_object& morgan = create_account("morgan", morgan_key.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key.get_public_key() );
      const account_object& oliver = create_account("oliver", oliver_key.get_public_key() );

      graphene::app::database_api db_api(db);

      // prepare available keys sets
      flat_set<public_key_type> avail_keys_empty, avail_keys_m, avail_keys_n, avail_keys_o;
      flat_set<public_key_type> avail_keys_mn, avail_keys_mo, avail_keys_no, avail_keys_mno;
      avail_keys_m.insert( pub_key_morgan );
      avail_keys_mn.insert( pub_key_morgan );
      avail_keys_mo.insert( pub_key_morgan );
      avail_keys_mno.insert( pub_key_morgan );
      avail_keys_n.insert( pub_key_nathan );
      avail_keys_mn.insert( pub_key_nathan );
      avail_keys_no.insert( pub_key_nathan );
      avail_keys_mno.insert( pub_key_nathan );
      avail_keys_o.insert( pub_key_oliver );
      avail_keys_mo.insert( pub_key_oliver );
      avail_keys_no.insert( pub_key_oliver );
      avail_keys_mno.insert( pub_key_oliver );

      // result set
      set<public_key_type> pub_keys;

      // make a transaction that require 1 signature (m)
      transfer_operation op;
      op.from = morgan.id;
      op.to = oliver.id;
      trx.operations.push_back(op);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

     // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

       // provides [m,n], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // sign with n, but actually need m
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // sign with m, should be enough
      trx.clear_signatures();
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // sign with m+n, although m only should be enough, this API won't complain
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

      // make a transaction that require 2 signatures (m+n)
      trx.clear_signatures();
      op.from = nathan.id;
      trx.operations.push_back(op);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with o, but actually need m+n
      sign(trx, oliver_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m+o, but actually need m+n
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m, but actually need m+n
      trx.clear_signatures();
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m+n, should be enough
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

      // sign with m+n+o, should be enough as well
      sign(trx, oliver_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
