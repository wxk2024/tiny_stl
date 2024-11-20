//
// Created by wxk on 2024/11/19.
//
/// 自定义了析构函数，就需要将 拷贝构造函数和赋值运算符函数删掉，否则会双重析构
#ifndef VARIANT_HPP
#define VARIANT_HPP
#include <functional>
#include <type_traits>
struct BadVariantAccess : std::exception {
    BadVariantAccess() = default;

    virtual ~BadVariantAccess() = default;

    const char *what() const noexcept override {
        return "BadVariantAccess";
    }
};

template<typename, typename> // typename -> size_t
struct VariantIndex;

template<typename, size_t> // size_t -> typename
struct VariantAlternative;


template<typename... Ts>
struct Variant {
private:
    size_t index_;
    alignas(std::max({alignof(Ts)...})) char union_[std::max({sizeof(Ts)...})];

    template<class Lambda>
    using VisitorFunction = std::common_type<typename std::invoke_result<Lambda, Ts &>::type...>::type(*)(
        char *, Lambda &&);

    template<class Lambda>
    static VisitorFunction<Lambda> *visitors_table() noexcept {
        static VisitorFunction<Lambda> function_ptrs[sizeof...(Ts)] = {
            [](char *union_p,
               Lambda &&lambda) -> std::common_type<typename std::invoke_result<Lambda, Ts &>::type...>::type {
                return std::invoke(std::forward<Lambda>(lambda),
                                   *reinterpret_cast<Ts *>(union_p));
            }...
        };
        return function_ptrs;
    }

public:
    template<typename T, std::enable_if_t<(std::is_same_v<T, Ts> || ...), int>  = 0>
    Variant(T value) : index_(VariantIndex<Variant, T>::value) {
        T *p = reinterpret_cast<T *>(union_);
        new(p) T(value);
    }

    template <class Lambda>
    std::common_type<typename std::invoke_result<Lambda, Ts &>::type...>::type visit(Lambda &&lambda) {
        return visitors_table<Lambda>()[index()](union_, std::forward<Lambda>(lambda));
    }

    constexpr size_t index() const noexcept {
        return index_;
    }

    // 检查当前变体对象中是否持有指定类型的值
    template<typename T>
    constexpr bool holds_alternative() const noexcept {
        // 使用VariantIndex traits来获取指定类型T在变体Variant中的索引值，并与当前对象的索引值进行比较
        // 如果两者相等，则表示当前对象中持有的是指定类型的值
        return VariantIndex<Variant, T>::value == index();
    }

    template<size_t I>
    typename VariantAlternative<Variant, I>::type &get() {
        static_assert(I < sizeof...(Ts), "I out of range!");
        if (index_ != I)
            throw BadVariantAccess();
        return *reinterpret_cast<typename VariantAlternative<Variant, I>::type *>(union_);
    }

    template<typename T>
    T &get() {
        return get<VariantIndex<Variant, T>::value>();
    }

    template<size_t I>
    typename VariantAlternative<Variant, I>::type const &get() const {
        static_assert(I < sizeof...(Ts), "I out of range!");
        if (index_ != I)
            throw BadVariantAccess();
        return *reinterpret_cast<typename VariantAlternative<Variant, I>::type const *>(union_);
    }

    template<typename T>
    T const &get() const {
        return get<VariantIndex<Variant, T>::value>();
    }

    template<size_t I>
    typename VariantAlternative<Variant, I>::type *get_if() {
        static_assert(I < sizeof...(Ts), "I out of range!");
        if (index_ != I)
            return nullptr;
        return reinterpret_cast<typename VariantAlternative<Variant, I>::type *>(union_);
    }

    template<typename T>
    T *get_if() {
        return get_if<VariantIndex<Variant, T>::value>();
    }

    template<size_t I>
    typename VariantAlternative<Variant, I>::type const *get_if() const {
        static_assert(I < sizeof...(Ts), "I out of range!");
        if (index_ != I)
            return nullptr;
        return reinterpret_cast<typename VariantAlternative<Variant, I>::type const *>(union_);
    }

    template<typename T>
    T const *get_if() const {
        return get_if<VariantIndex<Variant, T>::value>();
    }
};

template<typename Fn,typename...Ts>
auto Visit(Fn &&fn,Variant<Ts...> &variant) {
    return variant.visit(std::forward<Fn>(fn));
}

template<typename T, typename... Ts>
struct VariantAlternative<Variant<T, Ts...>, 0> {
    using type = T;
};

template<typename T, typename... Ts, size_t I>
struct VariantAlternative<Variant<T, Ts...>, I> {
    using type = typename VariantAlternative<Variant<Ts...>, I - 1>::type;
};

template<typename T, typename... Ts>
struct VariantIndex<Variant<T, Ts...>, T> {
    static constexpr size_t value = 0;
};

template<typename T0, typename T, typename... Ts>
struct VariantIndex<Variant<T0, Ts...>, T> {
    static constexpr size_t value = VariantIndex<Variant<Ts...>, T>::value + 1;
};
#endif //VARIANT_HPP
