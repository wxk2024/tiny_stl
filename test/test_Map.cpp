//
// Created by wxk on 2024/11/9.
//
#include <Map.hpp>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <iostream>
using namespace std;
TEST_CASE("insert and find","[set]") {
    Set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    REQUIRE(s.insert(1).second == false);  //插入重复的键返回 false
    REQUIRE(s.insert(6).second == true);  //
    REQUIRE(s.find(3)!=s.end());
    REQUIRE(s.find(4)==s.end()); //找不到
    auto it = s.begin();
    auto it_end = s.end();
    for(it; it != it_end; ++it) {
        std::cout << *it << std::endl;
    }
    ++it_end;
    --it;

}

TEST_CASE("iterator operator *","[set]") {
    Set<int> m_s;
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

TEST_CASE("iterator cast","[set]") {

}

TEST_CASE("low_bound upper_bound ","[test]") {
    Set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    REQUIRE(*s.lower_bound(2) == 2);
    REQUIRE(*s.upper_bound(2) == 3);
    //REQUIRE(s.equal_range(2));
}

TEST_CASE("ctor","[set]") {
    Set<int> s_source,s_destination;
    s_source.insert(1);
    s_source.insert(2);
    s_source.insert(3);
    s_source.insert(4);
    s_destination = s_source;
    REQUIRE(*s_destination.begin()==1);
    REQUIRE(*s_destination.end()==4);
}

TEST_CASE("erase","[set]") {
    Set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    REQUIRE(s.erase(1)==1);
}

TEST_CASE("count and contain","[set]") {
    Set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    REQUIRE(s.count(1)==1);
    REQUIRE(s.count(2)==1);
    REQUIRE(s.count(0)==0);
    s.erase(1);
    REQUIRE(s.count(1)==0);
}

TEST_CASE("emplace","[set]") {
    struct A {
        int a,b,c,d;
        //A(int a, int b, int c, int d) :a(a),b(b),c(c),d(d){}
        bool operator<(const A& rhs) const {
            return a<rhs.a;
        }
    };
    Set<A> s;
    s.emplace(1,2,3,4);
    REQUIRE(s.begin()->a == 1);
    REQUIRE(s.begin()->b == 2);
    REQUIRE(s.begin()->c == 3);
    REQUIRE(s.begin()->d == 4);
}

TEST_CASE("clear and assign","[set]") {
    Set<int> s;
    vector<int> v{1,2,3};
    s.insert(v.begin(),v.end());
    REQUIRE(*s.begin()==1);
    REQUIRE(*(--s.end())==3);
    s.clear();
    s.assign(v.begin(),v.end());
    REQUIRE(*s.begin()==1);
    REQUIRE(*(--s.end())==3);
}

