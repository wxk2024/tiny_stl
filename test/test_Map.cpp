//
// Created by wxk on 2024/11/9.
//
#include <Map.hpp>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <iostream>
using namespace std;
TEST_CASE("insert and find","[set]") {
    Set s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    REQUIRE(s.insert(1).second == false);  //插入重复的键返回 false
    REQUIRE(s.insert(6).second == true);  //
    REQUIRE(s.find(3)!=nullptr);
    REQUIRE(s.find(4)==nullptr);
    auto it = s.begin();
    auto it_end = s.end();
    for(it; it != it_end; ++it) {
        std::cout << *it << std::endl;
    }
    ++it_end;
    --it;

}

TEST_CASE("iterator","[set]") {
    Set m_s;
    m_s.insert(1);
    m_s.insert(2);
    m_s.insert(3);
    m_s.insert(4);
    auto m_s_it = m_s.begin();
    auto m_it_end = m_s.end();

    REQUIRE(m_s_it != m_it_end);

    REQUIRE(1 == *m_s_it);
    ++m_s_it;
    REQUIRE(2 == *m_s_it);
    ++m_s_it;
    REQUIRE(3 == *m_s_it);
    ++m_s_it;
    REQUIRE(4 == *m_s_it);

    ++m_s_it;
    REQUIRE(m_it_end == m_s_it);     // 判断是否可以相等

    --m_s_it;
    REQUIRE(4 == *m_s_it);
    --m_s_it;
    REQUIRE(3 == *m_s_it);
    for(auto i:m_s) {
        std::cout<<"for:"<<i<<std::endl;
    }
    //---------逆向迭代器-------
    auto r_it = m_s.rbegin();
    auto r_it_end = m_s.rend();
    REQUIRE(r_it != r_it_end);
    REQUIRE(4 == *r_it);
    ++r_it;
    REQUIRE(3 == *r_it);
    ++r_it;
    REQUIRE(2 == *r_it);

    REQUIRE(m_s.contains(1));
}