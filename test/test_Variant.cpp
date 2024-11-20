//
// Created by wxk on 2024/11/20.
//
#include <catch2/catch_test_macros.hpp>
#include <Variant.hpp>

TEST_CASE("ctor","[variant]")
{
    Variant<int,float> v(3);
}

TEST_CASE("index","[variant]")
{
    Variant<int,float> v(3);
    REQUIRE(v.index() == 0);
}

TEST_CASE("hold_alternate","[variant]") {
    Variant<int,float> v(3);
    REQUIRE(v.holds_alternative<int>());
}

TEST_CASE("get","[variant]") {
    Variant<int,float> v(3);
    REQUIRE(v.get<int>()==3);
    v.get<int>()=2;
    REQUIRE(v.get<int>()==2);
    REQUIRE(v.get<0>()==2);
    REQUIRE_THROWS_AS(v.get<float>(),BadVariantAccess);
}
TEST_CASE("get_if","[variant]") {
    Variant<int,float> v(3);
    REQUIRE(v.get_if<int>()!=nullptr);
    REQUIRE(v.get_if<float>()==nullptr);
    REQUIRE(v.get_if<0>()!=nullptr);
    REQUIRE(v.get_if<1>()==nullptr);
    REQUIRE(*v.get_if<0>()==3);
}
TEST_CASE("visit","[variant]") {
    Variant<int,float> v(3);
    REQUIRE(v.visit([](auto&& arg){return arg;})==3);
    REQUIRE(Visit([](auto&& arg){return arg;},v)==3);
}