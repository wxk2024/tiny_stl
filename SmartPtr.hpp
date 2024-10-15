#include <atomic>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>
#include <concepts>
#include <memory>

template<class T>
T exchange(T &dst,T val) {
    T tmp = std::move(dst);
    dst = std::move(val);
    return tmp;
}

template <class T>
struct DefaultDeleter {
    void operator()(T* p) const{
        delete p;
    }
};

template<>
struct DefaultDeleter<FILE> {
    void operator() (FILE* p) const{
        fclose(p);
    }
};

template<class T>
struct DefaultDeleter<T[]> {
    void operator()(T* p) const {
        delete[] p;
    }
};

struct FileDeleter {
    void operator() (FILE* p) const{
        fclose(p);
    }
};

template<class T,class Deleter = DefaultDeleter<T>>
class UniquePtr {
private:
    T *p_ = nullptr;
    template <class U,class UDeleter>
    friend class UniquePtr;
public:
    // 默认构造
    UniquePtr(std::nullptr_t dummy = nullptr) : p_(nullptr) {}
    // 自定义构造
    explicit UniquePtr(T *p) : p_(p) {}
    ~UniquePtr() {
        if (p_ != nullptr) {
            Deleter{}(p_);
        }
        //puts("Destructor called");
    }
    UniquePtr(UniquePtr const &that) = delete;
    UniquePtr &operator=(UniquePtr const &that) = delete;
    UniquePtr(UniquePtr &&that) noexcept {p_ = std::exchange(that.p_,nullptr);}
    UniquePtr &operator=(UniquePtr &&that) noexcept {
        if(this != &that) {
            if(p_) {
                Deleter{}(p_);
                p_ = std::exchange(that.p_,nullptr);
            }
        }
        return *this;
    }
    T *get()const {
        return p_;
    }
    Deleter get_deleter()const {
        return Deleter{};
    }

    T *release() {
        return std::exchange(p_,nullptr);
    }
    void reset(T *p = nullptr) {
        if(p_) {
            Deleter{}(p_);
        }
        p_ = p;
    }
    T *operator->()const {
        return p_;
    }
    T &operator*()const {
        return *p_;
    }
    // 多态构造
    template<class U,class UDeleter>
        requires(std::convertible_to<U *,T *>)
    UniquePtr(UniquePtr<U,UDeleter> &&that) {
        p_ = std::exchange(that.p_,nullptr);
    }
};

// 偏特化不能带默认值
template<class T,class Deleter>
class UniquePtr <T[],Deleter>:public UniquePtr<T,Deleter>{
    using UniquePtr<T, Deleter>::UniquePtr; // 继承构造函数
    std::add_lvalue_reference_t<T> operator[](std::size_t idx)const {
        return this->get()[idx];
    };
};



template<class T,class...Args>
UniquePtr<T> makeUnique(Args&&...args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...)); // C++20 支持圆括号
}
template<class T,class...Args>
UniquePtr<T> makeUniqueForOverWrite() {
    return UniquePtr<T>(new T); //
}

struct Animal {
    virtual void speak() = 0;
    virtual ~Animal() = default;
};
struct Dog final :public Animal {
    void speak() override {
        puts("Dog speak");
    };
};
struct Cat final :public Animal {
    void speak() override {
        puts("Cat speak");
    }
};

//-------------------------------
// 控制块
struct SpControlBlock {
    std::atomic_long refcnt_;
    explicit SpControlBlock() :refcnt_(1) {}
    SpControlBlock(SpControlBlock &&that) = delete;
    virtual ~SpControlBlock() = default;
    void incref() {
        refcnt_.fetch_add(1,std::memory_order_relaxed);
    }
    void deref() {
        if(refcnt_.fetch_sub(1,std::memory_order_relaxed) == 1) {
            delete this;
        }
    }
    long cntref() const noexcept {
        return refcnt_.load(std::memory_order_relaxed);
    }
};
template<class T,class Deleter=DefaultDeleter<T>>
struct SpControlBlockImpl final: public SpControlBlock{
    T *data_;
    Deleter deleter_;
    explicit SpControlBlockImpl(T *ptr)noexcept:data_(ptr){}
    explicit SpControlBlockImpl(T *ptr,Deleter deleter)noexcept:data_(ptr),deleter_(std::move(deleter)){}
    ~SpControlBlockImpl() noexcept override{
        deleter_(this->data_);
    }
};
template<class T,class Deleter=DefaultDeleter<T>>
struct SpControlBlockImplFuse final:public SpControlBlock {
    T *data_;
    void *mem_;     // 还有指向自己内存的指针
    [[no_unique_address]]Deleter deleter_;

    explicit SpControlBlockImplFuse(T *ptr,void *mem,Deleter deleter)noexcept:
        data_(ptr),mem_(mem),deleter_(std::move(deleter)) {}
    ~SpControlBlockImplFuse() noexcept override {
        deleter_(this->data_);
#if __cpp_aligned_new
        // 释放整个分配块 : 这个地方同样是使用 std::align_val_t T 和 控制块 的最大值，因为我们就是这样分配的
        ::operator delete(mem_,std::align_val_t(std::max(alignof(T),alignof(SpControlBlockImplFuse))));
#else
        ::operator delete(mem_);
#endif
    }
    // 相当于 = delete,我们必须要通过析构函数(该控制块作为成员的方式)来释放，而不是被 delete
    void operator delete(void*)noexcept{}
};

// EnableFrom
template<class T>
struct SharedPtr;

template<class Derived>
struct EnableSharedFromThis {
private:
    SpControlBlock *cb_{nullptr};  // 分侵入式的加入 控制块 的 思想！
    template<class T>
    friend struct SharedPtr;
public:
    EnableSharedFromThis() {}
    SharedPtr<Derived> shared_from_this() noexcept{
        static_assert(std::is_base_of_v<EnableSharedFromThis,Derived>,"must be derived class");
        if(!cb_) throw std::bad_weak_ptr();  // 根据标准如果不被 shared_ptr 管理则抛出一个该异常
        cb_->incref();
        return _makeSharedSpCounterOnce(static_cast<Derived*>(this),cb_);
    }
    // 为了防止 Derived 已经有了 const 所以我们要使用 std::add_const_t
    SharedPtr<std::add_const_t<Derived>> shared_from_this() const noexcept{
        static_assert(std::is_base_of_v<EnableSharedFromThis,Derived>,"must be derived class");
        if(!cb_) throw std::bad_weak_ptr();
        cb_->incref();
        return _makeSharedSpCounterOnce(static_cast<std::add_const_t<Derived>*>(this),cb_);
    }
    template<class T>
    friend inline void setEnableSharedFromThis(EnableSharedFromThis<T>*,SpControlBlock*);
};

// SharedPtr
template<class T>
struct SharedPtr {
private:
    T *ptr_ = nullptr;
    SpControlBlock *cb_ = nullptr;
    // 声明友元类
    template<class>
    friend class SharedPtr;
    // 支持分配到一块内存,内联函数直接定义在头文件中
    template<class Y>
    friend inline SharedPtr<Y> _makeSharedSpCounterOnce(Y *ptr,SpControlBlock *cb);

    explicit SharedPtr(T *ptr,SpControlBlock *cb)noexcept:ptr_(ptr),cb_(cb) {}

    template<class Y>
    friend inline void _setEnableSharedFromThisOwner(EnableSharedFromThis<Y>*,SpControlBlock*);
public:
    using element_type = T;
    using pointer = T*;
    SharedPtr(std::nullptr_t = nullptr)noexcept:cb_(nullptr){}

    template<class Y,std::enable_if_t<!std::is_base_of_v<EnableSharedFromThis<Y>,Y>,int> = 0,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    explicit SharedPtr(Y *ptr):ptr_(ptr) ,cb_(new SpControlBlockImpl<Y,DefaultDeleter<Y>>(ptr)){}
    template<class Y,std::enable_if_t<std::is_base_of_v<EnableSharedFromThis<Y>,Y>,int> = 0,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    explicit SharedPtr(Y *ptr):ptr_(ptr) ,cb_(new SpControlBlockImpl<Y,DefaultDeleter<Y>>(ptr)) {
        ptr_->cb_ = this->cb_;
    }

    template<class Y,class Deleter,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    explicit SharedPtr(Y *ptr,Deleter deleter):ptr_(ptr),cb_(new SpControlBlockImpl<Y,Deleter>(ptr,std::move(deleter))) {}
    // 支持从 UniquePtr 构造
    template<class Y,class Deleter,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    explicit SharedPtr(UniquePtr<Y,Deleter> &&ptr):SharedPtr(ptr.get(),ptr->get_deleter()) {}

    SharedPtr &operator=(SharedPtr const &that) noexcept{
        if(this == &that) {
            return *this;
        }
        if(cb_)cb_->deref();
        ptr_ = that.ptr_;
        cb_ = that.cb_;
        if(cb_) cb_->incref();
        return *this;
    }
    SharedPtr &operator=(SharedPtr &&that) noexcept{
        if(this == &that) {
            return *this;
        }
        if(cb_) cb_->deref();
        ptr_ = that.ptr_;
        cb_ = that.cb_;
        that.ptr_ = nullptr;
        that.cb_ = nullptr;
        return *this;
    }
    template<class Y,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    SharedPtr &operator=(SharedPtr<Y> const&that)noexcept {
        if(this == &that) { return *this;}
        if(cb_) cb_->deref();
        ptr_ = that.ptr_;
        cb_ = that.cb_;
        if(cb_) cb_->incref();
        return *this;
    }
    template<class Y,std::enable_if_t<std::is_convertible_v<Y*,T*>,int> = 0>
    SharedPtr &operator=(SharedPtr<Y> &&that)noexcept {
        if(this == &that) { return *this;}
        if(cb_) cb_->deref();
        ptr_ = that.ptr_;
        cb_ = that.cb_;
        that.ptr_ = nullptr;
        that.cb_ = nullptr;
        return *this;
    }
    // 添加隐式转换函数
    template<class Y,std::enable_if_t<std::convertible_to<Y*,T*>,int> = 0>
    SharedPtr(SharedPtr<Y> const &that) noexcept:ptr_(that.ptr_),cb_(that.cb_) {
        if(cb_) cb_->incref();
    }
    template<class Y,std::enable_if_t<std::convertible_to<Y*,T*>,int> = 0>
    SharedPtr(SharedPtr<Y> &&that) noexcept:ptr_(that.ptr_),cb_(that.cb_) {
        that.ptr_ = nullptr;
        that.cb_ = nullptr;
    }
    template<class Y>
    bool operator<(SharedPtr<Y> const &that) const noexcept {
        return ptr_ < that.ptr_;
    }
    template<class Y>
    bool operator==(SharedPtr<T> const &that)const noexcept {
        return ptr_ == that.ptr_;
    }
    template<class Y>
    bool owner_before(SharedPtr<Y> const &that) noexcept {
        return cb_ < that.cb_;
    }
    template<class Y>
    bool owner_equal(SharedPtr<Y> const &that)const noexcept {
        return cb_ == that.cb_;
    }
    void swap(SharedPtr &that) {
        std::swap(ptr_, that.ptr_);
        std::swap(cb_, that.cb_);
    }
    void reset()noexcept {
        if(cb_)cb_->deref();
        cb_=nullptr;ptr_=nullptr;
    }
    template<class Y>
    void reset(Y *ptr) {
        if(cb_)
            cb_->deref();
        cb_ = nullptr;
        ptr_ = nullptr;
        ptr_ = ptr;
        cb_ = new SpControlBlockImpl<Y,DefaultDeleter<Y>>(ptr);
    }
    template<class Y,class Deleter>
    void reset(Y *ptr,Deleter deleter) {
        if(cb_)
            cb_->deref();
        cb_ = nullptr;
        ptr_ = nullptr;
        ptr_ = ptr;
        cb_ = new SpControlBlockImpl<Y,Deleter>(ptr,std::move(deleter));
    }
    ~SharedPtr() {
        if(cb_)
            cb_->deref();
    }
    SharedPtr(SharedPtr const &that)noexcept:ptr_(that.ptr_),cb_(that.cb_) {
        if(cb_)cb_->incref();
    }
    SharedPtr(SharedPtr &&that)noexcept:ptr_(that.ptr_),cb_(that.cb_) {
        that.ptr_=nullptr;
        that.cb_=nullptr;
    }
    T* get()const {
        return ptr_;
    }
    // 为了防止 void& 的出现编译不通过
    std::add_lvalue_reference<T> operator*()const {
        return *ptr_;
    }
    T* operator->()const {
        return ptr_;
    }
    long use_count()const {
        return cb_?cb_->cntref():0;
    }
    bool unique()noexcept {
        return cb_?cb_->refcnt_==1:false;
    }
    // 支持 staticPointerCast 函数,加上 std::enable_if_t<std::convertible_to<Y*,T*>,int> = 0 负责隐式转换
    template<class Y>
    SharedPtr(SharedPtr<Y> const &that,T *ptr) noexcept:ptr_(ptr),cb_(that.cb_) {
        if(cb_)cb_->incref();
    }
    //加上 std::enable_if_t<std::convertible_to<Y*,T*>,int> = 0 负责隐式转换
    template<class Y>
    SharedPtr(SharedPtr<Y> &&that,T *ptr) noexcept:ptr_(ptr),cb_(that.cb_) {
        that.ptr_=nullptr;
        that.cb_=nullptr;
    }

};
//支持数组
template<class T>
class SharedPtr<T[]>:public SharedPtr<T>{
    using SharedPtr<T>::UniquePtr; // 继承构造函数
    std::add_lvalue_reference_t<T> operator[](std::size_t idx)const {
        return this->get()[idx];
    };
};
// 支持实现 make_shared
template<class T>
SharedPtr<T> _makeSharedSpCounterOnce(T *ptr,SpControlBlock *cb) {
    return SharedPtr<T>(ptr,cb);
}
// 支持 enable_shared_from_this: SFIAE
template<class T>
inline void setEnableSharedFromThis(EnableSharedFromThis<T>*ptr,SpControlBlock*cb) {
    ptr->cb_ = cb;
}
template<class T,std::enable_if_t<std::is_base_of_v<EnableSharedFromThis<T>,T>,int> = 0>
inline void setupEnableSharedFromThis(T* ptr,SpControlBlock* cb) {
    setEnableSharedFromThis(static_cast<EnableSharedFromThis<T>*>(ptr),cb);
}
template<class T,std::enable_if_t<!std::is_base_of_v<EnableSharedFromThis<T>,T>,int> = 0>
void setupEnableSharedFromThis(T *ptr,SpControlBlock *cb) {}


template<class T,class...Args,std::enable_if_t<!std::is_unbounded_array_v<T>,int> = 0>
SharedPtr<T> makeShared(Args&&...args) {
    auto const deleter = [](T *ptr) noexcept {
        ptr->~T();
    };
    using Counter = SpControlBlockImplFuse<T,decltype(deleter)>; //控制块的类型
    // T 的开始位置： 控制块放在前面，所以开始位置
    constexpr std::size_t offset = alignof(T)>=sizeof(Counter)?alignof(T):((sizeof(Counter)+alignof(T)-1)/alignof(T))*alignof(T);
    //constexpr std::size_t offset = std::max(alignof(T),sizeof(Counter));
    constexpr std::size_t align = std::max(alignof(T),alignof(Counter)); // 整个内存的对齐标准
    constexpr std::size_t size = offset + sizeof(T);
#if __cpp_aligned_new
    void *mem = ::operator new(size,std::align_val_t(align));
    Counter *ptr = reinterpret_cast<Counter *>(mem);
#else
    void *mem = ::operator new(size + align);
    Counter *ptr = reinterpret_cast<Counter *>(mem & align);
#endif
    T *obj = reinterpret_cast<T *>(reinterpret_cast<char*>(ptr)+offset);
    try {
        new (obj) T(std::forward<Args>(args)...);
    }catch(...) {
#if __cpp_aligned_new
        ::operator delete(mem,std::align_val_t(align));
#else
        ::operator delete(mem);
#endif
        throw;
    }
    static_assert(std::is_same_v<Counter,SpControlBlockImplFuse<T,decltype(deleter)>>);
    new (ptr) Counter(obj,mem,deleter);

    setupEnableSharedFromThis(obj,ptr);
    return _makeSharedSpCounterOnce(obj,ptr);
}

template<class T,std::enable_if_t<!std::is_unbounded_array_v<T>,int> = 0>
SharedPtr<T> makeSharedForOverwrite() {
    return SharedPtr<T>(new T);
}
template<class T,class...Args,std::enable_if_t<std::is_unbounded_array_v<T>,int> = 0>
SharedPtr<T> makeShared(std::size_t len) {
    return SharedPtr<T>(new std::remove_extent_t<T>[len]());
}
template<class T,std::enable_if_t<std::is_unbounded_array_v<T>,int> = 0>
SharedPtr<T> makeSharedForOverwrite(std::size_t len) {
    return SharedPtr<T>(new std::remove_extent_t<T>[len]);
}


template<class T>
struct WeakPtr{};



template<class T,class U>
SharedPtr<T> staticPointerCast(SharedPtr<U> const &ptr) {
    return SharedPtr<T>(ptr,static_cast<T *>(ptr.get())); //拿到控制块
}

template<class T,class U>
SharedPtr<T> constPointerCast(SharedPtr<U> const &ptr) {
    return SharedPtr<T>(ptr,const_cast<T *>(ptr.get()));
}

template<class T,class U>
SharedPtr<T> reinterpretPointerCast(SharedPtr<U> const &ptr) {
    return SharedPtr<T>(ptr,reinterpret_cast<T *>(ptr.get()));
}

template<class T,class U>
SharedPtr<T> dynamicPointerCast(SharedPtr<U> const &ptr) {
    if(auto *p = dynamic_cast<T*>(ptr.get())) {
        return SharedPtr<T>(ptr,p);
    }
    return nullptr;
}



