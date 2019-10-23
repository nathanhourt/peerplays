#pragma once

#include <graphene/peerplays_sidechain/sidechain_net_handler.hpp>

#include <string>
#include <zmq.hpp>

#include <fc/signals.hpp>
#include <fc/network/http/connection.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace peerplays_sidechain {

class bitcoin_rpc_client {
public:
   bitcoin_rpc_client( std::string _ip, uint32_t _rpc, std::string _user, std::string _password) ;
   std::string receive_full_block( const std::string& block_hash );
   int32_t receive_confirmations_tx( const std::string& tx_hash );
   bool receive_mempool_entry_tx( const std::string& tx_hash );
   uint64_t receive_estimated_fee();
   void send_btc_tx( const std::string& tx_hex );
   bool connection_is_not_defined() const;

private:

   fc::http::reply send_post_request( std::string body );

   std::string ip;
   uint32_t rpc_port;
   std::string user;
   std::string password;

   fc::http::header authorization;
};

// =============================================================================

class zmq_listener {
public:
   zmq_listener( std::string _ip, uint32_t _zmq );
   bool connection_is_not_defined() const { return zmq_port == 0; }

   fc::signal<void( const std::string& )> block_received;
private:
   void handle_zmq();
   std::vector<zmq::message_t> receive_multipart();

   std::string ip;
   uint32_t zmq_port;

   zmq::context_t ctx;
   zmq::socket_t socket;
};

// =============================================================================

class sidechain_net_handler_bitcoin : public sidechain_net_handler {
public:
   sidechain_net_handler_bitcoin(const boost::program_options::variables_map& options);
   virtual ~sidechain_net_handler_bitcoin();

   void handle_block( const std::string& block_hash );
private:
   std::string ip;
   uint32_t zmq_port;
   uint32_t rpc_port;
   std::string rpc_user;
   std::string rpc_password;

   std::unique_ptr<zmq_listener> listener;
   std::unique_ptr<bitcoin_rpc_client> bitcoin_client;
   graphene::chain::database* db;

};

} } // graphene::peerplays_sidechain
