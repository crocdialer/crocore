//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "crocore/Property.hpp"

using namespace crocore;
//____________________________________________________________________________//

class TestObserver : public Property::Observer
{
public:
    TestObserver():m_triggered(false){};
    bool m_triggered;

    void update_property(const PropertyConstPtr &theProperty) override
    {
        m_triggered = true;
    };
};

// test Property::Observer methods and behaviour
BOOST_AUTO_TEST_CASE( testObserver )
{
    Property_<uint32_t>::Ptr intProp(Property_<uint32_t>::create("intProp", 5));

    std::shared_ptr<TestObserver> obs1(new TestObserver),
    obs2(new TestObserver);

    // add 2 observers
    intProp->add_observer(obs1);
    intProp->add_observer(obs2);

    // assign a value
    *intProp = 69;

    BOOST_CHECK(intProp->value() == 69);
    BOOST_CHECK(*intProp == 69);
    BOOST_CHECK(obs1->m_triggered);
    BOOST_CHECK(obs2->m_triggered);

    obs1->m_triggered = false;
    obs2->m_triggered = false;

    intProp->remove_observer(obs1);

    intProp->value(23);
    BOOST_CHECK(intProp->value() == 23);

    *intProp = 30 - 7;
    BOOST_CHECK(*intProp == 23);

    *intProp = 30;
    *intProp += 10;
    *intProp -= 17;
    BOOST_CHECK(*intProp == 23);

    *intProp = 5;
    *intProp *= 3;
    *intProp /= 5;
    BOOST_CHECK(*intProp == 3);

    BOOST_CHECK(!obs1->m_triggered);
    BOOST_CHECK(obs2->m_triggered);

    intProp->set(111);
    BOOST_CHECK(intProp->value() == 111);

    obs1->m_triggered = false;
    obs2->m_triggered = false;

    // this part tests the rather hacky way of manipulating
    // the underlying data directly via references or pointers.
    // keep in mind that no observer notifications or range checks are performed when using this
    uint32_t &int_ref = intProp->value();
    int_ref = 69;
    BOOST_CHECK(intProp->value() == 69);
    BOOST_CHECK(!obs2->m_triggered);

    uint32_t *int_ptr = intProp->get_value_ptr<uint32_t>();
    *int_ptr = 54321;
    BOOST_CHECK(*intProp == 54321);
    BOOST_CHECK(!obs2->m_triggered);

    // tests automatic observer managment with smart pointers
    obs1.reset();
    *intProp = 5;
}

//____________________________________________________________________________//

// test
BOOST_AUTO_TEST_CASE( testRangedProp )
{
    RangedProperty<int32_t>::Ptr rangeProp
        (RangedProperty<int32_t>::create("rangedProp", 5, -3, 10));

    *rangeProp = -5;
    BOOST_CHECK_EQUAL( *rangeProp, -3 );

    *rangeProp = 1001;
    BOOST_CHECK_EQUAL( rangeProp->value(), 10 );

    rangeProp->set_range(-40, 5000);
    auto rng = rangeProp->range();
    int min = rng.first, max = rng.second;
    
    BOOST_CHECK_EQUAL(min, -40);
    BOOST_CHECK_EQUAL(max, 5000);

    rangeProp->set(-999);
    BOOST_CHECK_EQUAL(*rangeProp, min);

    rangeProp->set(9999);
    BOOST_CHECK_EQUAL(*rangeProp, max);

    *rangeProp -= 9999;
    BOOST_CHECK_EQUAL(*rangeProp, min);

    *rangeProp += 9999;
    BOOST_CHECK_EQUAL(*rangeProp, max);
}

//____________________________________________________________________________//

// EOF
