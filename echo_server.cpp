#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;



// 한개의 접속만 허용,
// 입력한 문자 그대로 return하는 Echo서버
struct Server
{
  asio::io_context io;
  tcp::acceptor acceptor;
  tcp::socket socket;

  Server()
    : acceptor(io, tcp::endpoint(tcp::v4(), 8888)),
      socket(io)
  {
  }

  void wait_connection()
  {
    std::cout << "Wait for New connection...\n";
    // 접속 올때까지 return 안함
    acceptor.accept(socket);
    std::cout << "New Peer!\n";
  }

  // peer에서 보낸 데이터 확인
  // 그대로 다시 전송
  void run()
  {
    int n;
    std::vector<char> data;
    while( 1 )
    {
      boost::system::error_code error;
      std::cout << "Wait for data...\n";
      socket.read_some( asio::buffer(&n,sizeof(n)), error );
      if( error == boost::asio::error::eof )
      {
        std::cout << "Connection End\n";
        break;
      }
      std::cout << "Received " << n << "bytes : ";
      data.resize(n);
      socket.read_some( asio::buffer(data.data(),n) );

      std::cout.write( data.data(), n );
      std::cout << "\n";

      std::cout << "Send Data...\n";
      socket.write_some( asio::buffer(&n,sizeof(n)) );
      socket.write_some( asio::buffer(data.data(),n) );
    }
  }
};


// 루프백에 연결해서
// getline으로 받은거 계속 send, 답신 wait
struct Client
{
  asio::io_context io;
  tcp::socket socket;

  MessageTcpClient()
    : socket(io)
  {
  }

  void connect()
  {
    std::cout << "Connecting...\n";
    socket.connect( tcp::endpoint(tcp::v4(),8888) );
    std::cout << "End\n";
  }
  void run()
  {
    std::string str;
    std::vector<char> data;
    while( 1 )
    {
      std::cout << "Enter Message : ";
      std::getline( std::cin, str );

      int n = str.size();
      socket.write_some( asio::buffer(&n,sizeof(n)) );
      socket.write_some( asio::buffer(str.data(),n) );

      socket.read_some( asio::buffer(&n,sizeof(n)) );
      std::cout << "Received " << n << " bytes : ";
      data.resize(n);
      socket.read_some( asio::buffer(data.data(),n) );
      std::cout.write( data.data(), n );
      std::cout << "\n";
    }
  }
};

int main( int argc, char **argv )
{
  if( argv[1][0] == 's' )
  {
    // 서버
    Server server;
    server.wait_connection();
    server.run();
  }else
  {
    // 클라이언트
    Client client;
    client.connect();
    client.run();
  }
}
