//
// Created by wxk on 2024/11/20.
//
#include <catch2/catch_test_macros.hpp>
#include <Variant.hpp>
#include <iostream>

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

struct Foo {
    Foo()=default;
    Foo(int i):i(i){}
    Foo(const Foo& foo):i(foo.i) {
        puts("Foo copy ctor");
    }
    Foo(Foo&& foo):i(foo.i) {
        puts("Foo move ctor");
    }
    Foo& operator=(const Foo & foo) noexcept{
        i=foo.i;
        puts("Foo copy assignment");
        return *this;
    }
    Foo& operator=(Foo && foo) noexcept{
        i=foo.i;
        puts("Foo move assignment");
        return *this;
    }
    // 添加 const 版本
    int i;
    ~Foo() {
        puts("~Foo");
    };
};
TEST_CASE("copy ctor","[variant]") {
    Variant<int,Foo> v(Foo(3));
    Variant<int,Foo> v2(v);             // 如果我们定义了 move ctor ，C ++ 会删除 copy ctor ，需要我们再次添加
    REQUIRE(v2.get<Foo>().i==(3));
    REQUIRE(v.get<Foo>().i==(3));
}
TEST_CASE("move ctor","[variant]") {
    Variant<int,Foo> v(Foo(3));
    Variant<int,Foo> v2(std::move(v));  // 为了支持 移动，需要在 variant 中添加 move ctor
    REQUIRE(v2.get<Foo>().i==(3));
}

TEST_CASE("copy assignment ctor","[variant]") {
    Variant<int,Foo> v(Foo(3));
    Variant<int,Foo> v2;
    v2=v;
}

TEST_CASE("move assignment ctor","[variant]") {
    Variant<int,Foo> v(Foo(3));
    Variant<int,Foo> v2;
    v2=std::move(v);
}

TEST_CASE("inplace ctor","[variant]") {
    Variant<int,Foo> v(inPlaceIndex<1>,Foo(3));
    REQUIRE(v.get<Foo>().i==(3));
}
