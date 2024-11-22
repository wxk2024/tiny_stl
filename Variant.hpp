//
// Created by wxk on 2024/11/19.
//
///
#ifndef VARIANT_HPP
#define VARIANT_HPP
#include <functional>
#include <type_traits>

template <size_t I>
struct InPlaceIndex {
    explicit InPlaceIndex() = default;
};

// 变量模板
template <size_t I>
constexpr InPlaceIndex<I> inPlaceIndex;

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

    using DestructorFunction = void(*)(char *) noexcept;

    static DestructorFunction *destructors_table() noexcept {
        static DestructorFunction function_ptrs[sizeof...(Ts)] = {
            [] (char *union_) noexcept {
                reinterpret_cast<Ts *>(union_)->~Ts();
            }...
        };
        return function_ptrs;
    }

    using MoveConstructorFunction = void(*)(char *, char *) noexcept;

    static MoveConstructorFunction *move_constructors_table() noexcept {
        static MoveConstructorFunction function_ptrs[sizeof...(Ts)] = {
            [] (char *union_dst, char *union_src) noexcept {
                new (union_dst) Ts(std::move(*reinterpret_cast<Ts *>(union_src)));
            }...
        };
        return function_ptrs;
    }

    using CopyAssignmentFunction = void(*)(char *, char const *) noexcept;

    static CopyAssignmentFunction *copy_assigment_functions_table() noexcept {
        static CopyAssignmentFunction function_ptrs[sizeof...(Ts)] = {
            [] (char *union_dst, char const *union_src) noexcept {
                *reinterpret_cast<Ts *>(union_dst) = *reinterpret_cast<Ts const*>(union_src);
            }...
        };
        return function_ptrs;
    }

    using MoveAssignmentFunction = void(*)(char *, char *) noexcept;

    static MoveAssignmentFunction *move_assigment_functions_table() noexcept {
        static MoveAssignmentFunction function_ptrs[sizeof...(Ts)] = {
            [] (char *union_dst, char *union_src) noexcept {
                *reinterpret_cast<Ts *>(union_dst) = std::move(*reinterpret_cast<Ts *>(union_src));
            }...
        };
        return function_ptrs;
    }


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
    Variant(T&& value) : index_(VariantIndex<Variant, T>::value) {
        T *p = reinterpret_cast<T *>(union_);
        new(p) T(std::forward<T>(value));
    }
    Variant() = default;
    Variant(const Variant &that) = default;
    Variant &operator=(Variant const &that) noexcept{
        index_ = that.index_;
        copy_assigment_functions_table()[index()](union_, that.union_);
        return *this;
    }
    Variant &operator=(Variant &&that) {
        index_ = that.index_;
        move_assigment_functions_table()[index()](union_, that.union_);
        return *this;
    }
    Variant(Variant &&that) : index_(that.index_) {
        move_constructors_table()[index()](union_, that.union_);
    }

    ~Variant() noexcept {
        destructors_table()[index()](union_);
    }

    template <size_t I, typename ...Args>
    explicit Variant(InPlaceIndex<I>, Args &&...value_args) : index_(I) {
        new (union_) typename VariantAlternative<Variant, I>::type
            (std::forward<Args>(value_args)...);
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
