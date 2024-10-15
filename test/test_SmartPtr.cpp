//
// Created by wxk on 2024/10/14.
//
#include "SmartPtr.hpp"
#include <catch2/catch_test_macros.hpp>

struct MyClass {
    int a,b,c;
};

struct Student:EnableSharedFromThis<Student> {
    const char *name_;
    int age_;
    explicit Student(const char *name,int age):name_(name),age_(age) {

    }
    void func() {
        std::cout << (void *)shared_from_this().get()<<std::endl;
        //global(sp);
    }
    Student(Student &&)=delete;
    ~Student() {
        puts("Student destructor");
    }
};
struct StudentDerived: Student {
    explicit StudentDerived(const char *name,int age):Student(name,age) {
        puts("StudentDerived destructor");
    }
    ~StudentDerived() {
        puts("StudentDerived destructor");
    }
};

TEST_CASE("Constructor that receive a pointer","[UniquePtr]") {
    // 最好是设置成绝对路径
    auto p = UniquePtr<FILE, FileDeleter>(fopen("a.txt","r"));
    CHECK(p.get()!=nullptr);
}

TEST_CASE("Constructor that receive a new array","[UniquePtr]") {
    auto p = UniquePtr<int[]>(new int[2]);
    CHECK(p.get()!=nullptr);
}
TEST_CASE("get member funciton ","[UniquePtr]") {
    auto arr = UniquePtr<int[]>(new int[2]);
    arr.get()[0] = 1;
    REQUIRE(arr.get()[0] == 1);
}

TEST_CASE("makeUnique fun","[UniquePtr]") {
    auto p = makeUnique<MyClass>(41,42,43);
    CHECK(p->a == 41);
    CHECK(p->b == 42);
    CHECK(p->c == 43);
}

TEST_CASE("makeUniqueForOverWrite fun","[UniquePtr]") {
    auto p2 = makeUniqueForOverWrite<int>();
    CHECK(p2.get()!=nullptr);
}

TEST_CASE("polymorphic","[UniquePtr]") {
    std::vector<UniquePtr<Animal>> vec;
    vec.push_back(makeUnique<Cat>());
    vec.push_back(makeUnique<Dog>());
    for(auto const& animal:vec) {
        animal->speak();
    }
}

TEST_CASE("makeShared and enable_shared_from_this","[SharePtr]") {
    SharedPtr<Student> sp1 = makeShared<Student>("wxk",23);
    SharedPtr<Student> sp2(new Student("wxk",23));
    sp2->func();
    sp1->func();
}

TEST_CASE("Constructor that receive a pointer","[SharedPtr]") {
    SharedPtr<Student> sp(new StudentDerived("wxk",23));
    REQUIRE(sp.get()!=nullptr);
}

TEST_CASE("implicit conversion","[SharedPtr]") {
    SharedPtr<Student> sp3(new StudentDerived("wxk",23));
    SharedPtr<Student const> sp5 = sp3;
    REQUIRE(sp3->age_ == sp5->age_);
    REQUIRE(sp3->name_ == sp5->name_);
}

TEST_CASE("explicit translation","[SharedPtr]") {
    SharedPtr<Student> sp3(new StudentDerived("wxk",23));
    SharedPtr<Student const> sp5 = sp3;
    auto sp4 = staticPointerCast<StudentDerived>(sp3);
    REQUIRE(sp4->name_ == "wxk");
    REQUIRE(sp4->age_ == 23);
    sp3 = constPointerCast<Student>(sp5);
    REQUIRE(sp3->name_ == "wxk");
    REQUIRE(sp3->age_ == 23);
}

/*
int main() {


    // 支持 shared_ptr
    SharedPtr<Student> sp1 = makeShared<Student>("wxk",23);
    SharedPtr<Student> sp2(new Student("wxk",23));
    sp2->func();
    sp1->func();

    SharedPtr<Student> sp3(new StudentDerived("wxk",23));
    auto sp4 = staticPointerCast<StudentDerived>(sp3);
    SharedPtr<Student const> sp5 = sp3;
    //sp3 = sp5;
    sp3 = constPointerCast<Student>(sp5);

    return 0;
}
*/