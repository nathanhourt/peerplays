#define BOOST_TEST_MODULE Peerplays SON Tests

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(peerplays_sidechain) {
}

#include <boost/test/included/unit_test.hpp>
#include <cstdlib>
#include <iostream>

boost::unit_test::test_suite *init_unit_test_suite(int argc, char *argv[]) {
   std::srand(time(NULL));
   std::cout << "Random number generator seeded to " << time(NULL) << std::endl;

   return nullptr;
}
