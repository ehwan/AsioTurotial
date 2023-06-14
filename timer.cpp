#include <boost/asio.hpp>
#include <iostream>
#include <functional>
#include <thread>

namespace asio = boost::asio;

void synchronous_timer()
{
  asio::io_service io;
  asio::steady_timer t( io, asio::chrono::seconds(5) );

  // not return until timer ends
  t.wait();

  std::cout << "Hello World\n" << std::endl;
}
void asynchronous_timer()
{
  asio::io_context io;
  asio::steady_timer t( io, asio::chrono::seconds(5) );

  // return right back
  t.async_wait(
      []( boost::system::error_code error )
      {
        std::cout << "Timer Ends\n";
      } );
  std::cout << "async_wait called\n";

  // wait until all events end
  io.run();
  std::cout << "io.run() called\n";
}

struct Counter
{
  int *count;
  asio::steady_timer timer;

  Counter( int *c, asio::io_context &io )
    : count(c),
      timer(io, asio::chrono::seconds(1) )
  {
    timer.async_wait( std::bind( &Counter::print, this ) );
  }

  //void print( boost::system::error_code )
  void print()
  {
    std::cout << "Count : " << *count << 
      " / Thread : " << std::this_thread::get_id() << "\n";

    if( *count > 0 )
    {
      --*count;
      timer.expires_at( timer.expiry() + asio::chrono::seconds(1) );
      timer.async_wait( std::bind( &Counter::print, this ) );
    }
  }
};
void singlethread_counter()
{
  asio::io_context io;

  int count = 10;
  asio::io_context::strand strand( io );
  Counter counter( &count, io );
  Counter counter2( &count, io );

  // io.run() 을 부른 현재 쓰레드에서 모든 callback함수 호출
  io.run();
}

void multithread_counter()
{
  asio::io_context io;

  int count = 10;
  asio::io_context::strand strand( io );
  Counter counter1( &count, io );
  Counter counter2( &count, io );

  // io.run()을 두개의 쓰레드에서 실행
  // 같은callback이 중복실행될 위험은 없음
  // 근데 동시실행될 위험은 있음
  std::thread t(
      [&]()
      {
        io.run();
      } 
      );
  io.run();
  t.join();

  /* 
   * 실행결과
Count : 10 / Thread : Count : 0x100d20580
10 / Thread : 0x16f2bb000
Count : Count : 8 / Thread : 0x100d20580
8 / Thread : 0x16f2bb000
Count : Count : 6 / Thread : 6 / Thread : 0x100d20580
0x16f2bb000
Count : Count : 44 / Thread : 0x16f2bb000
 / Thread : 0x100d20580
Count : Count : 2 / Thread : 0x100d20580
2 / Thread : 0x16f2bb000
Count : Count : 0 / Thread : 00x100d20580 / Thread : 
0x16f2bb000

  counter1의 callback이 cout중인 동시에 counter2의 cout도 진행됨
  */
}



struct StrandCounter
{
  int *count;
  asio::steady_timer timer;
  asio::io_context::strand *strand;

  StrandCounter( int *c, asio::io_context &io, asio::io_context::strand *strand_ )
    : count(c),
      timer(io, asio::chrono::seconds(1) ),
      strand(strand_)
  {
    timer.async_wait(
        asio::bind_executor(
          *strand,
          std::bind( &StrandCounter::print, this )
        ) );
  }

  void print()
  {
    std::cout << "Count : " << *count << 
      " / Thread : " << std::this_thread::get_id() << "\n";

    if( *count > 0 )
    {
      --*count;
      timer.expires_at( timer.expiry() + asio::chrono::seconds(1) );
      timer.async_wait(
          asio::bind_executor(
            *strand,
            std::bind( &StrandCounter::print, this )
          ) );
    }
  }
};
void multithread_counter2()
{
  asio::io_context io;

  asio::io_context::strand strand(io);
  int count = 10;

  StrandCounter counter1( &count, io, &strand );
  StrandCounter counter2( &count, io, &strand );

  // io.run()을 두개의 쓰레드에서 실행
  // 같은callback이 중복실행될 위험은 없음
  // 같은 strand상에서 실행된 callback의 동시실행 위험 X
  std::thread t(
      [&]()
      {
        io.run();
      }
      );
  io.run();
  t.join();

  /*
   * 실행 결과
Count : 10 / Thread : 0x104984580
Count : 9 / Thread : 0x104984580
Count : 8 / Thread : 0x16b883000
Count : 7 / Thread : 0x16b883000
Count : 6 / Thread : 0x104984580
Count : 5 / Thread : 0x104984580
Count : 4 / Thread : 0x16b883000
Count : 3 / Thread : 0x16b883000
Count : 2 / Thread : 0x104984580
Count : 1 / Thread : 0x104984580
Count : 0 / Thread : 0x16b883000
Count : 0 / Thread : 0x16b883000
   */
}


void multithread_counter3()
{
  // 복수개의 strand를 사용하는 경우?
  asio::io_context io;

  asio::io_context::strand strand1(io);
  asio::io_context::strand strand2(io);
  int count = 10;

  StrandCounter counter1( &count, io, &strand1 );
  StrandCounter counter2( &count, io, &strand2 );

  // strand없이 io.run()을 부르는경우 각기 다른 strand를 사용하는것과 동일
  // io.run()을 두개의 쓰레드에서 실행
  // 같은callback이 중복실행될 위험은 없음
  // 다른 strand위에서 callback이 실행되므로 동시실행 위험 O
  std::thread t(
      [&]()
      {
        io.run();
      }
      );
  io.run();
  t.join();


  /*
   * 실행 결과
Count : Count : 10 / Thread : 10 / Thread : 0x10461c580
0x16bb37000
Count : Count : 8 / Thread : 8 / Thread : 0x16bb37000
0x10461c580
Count : Count : 6 / Thread : 6 / Thread : 0x16bb37000
0x10461c580
Count : Count : 4 / Thread : 4 / Thread : 0x10461c5800x16bb37000

Count : 2 / Thread : 0x16bb37000
Count : 1 / Thread : 0x10461c580
Count : Count : 0 / Thread : 0 / Thread : 0x10461c580
0x16bb37000
   */
}

int main()
{
  multithread_counter3();
}
