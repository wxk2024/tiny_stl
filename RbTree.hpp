//
// Created by wxk on 2024/11/16.
//

#ifndef RBTREE_HPP
#define RBTREE_HPP
#include <cassert>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "Common.hpp"

enum _RbTreeColor {
    _S_black,
    _S_red,
};

enum _RbTreeChildDir {
    _S_left,
    _S_right,
};

struct _RbTreeNode {
    _RbTreeNode *_M_left; // 左子节点指针
    _RbTreeNode *_M_right; // 右子节点指针
    _RbTreeNode *_M_parent; // 父节点指针
    _RbTreeNode **_M_pparent; // 父节点中指向本节点指针的指针
    _RbTreeColor _M_color; // 红或黑
};

// 带上实际的数据类型
template<class _Tp>
struct _RbTreeNodeImpl : _RbTreeNode {
    union {
        _Tp _M_value;
    }; // union 可以阻止里面成员的自动初始化，方便不支持 _Tp() 默认构造的类型

    template<class... _Ts>
    void _M_construct(_Ts &&... __value) noexcept {
        new(const_cast<std::remove_const_t<_Tp> *>(std::addressof(_M_value)))
                _Tp(std::forward<_Ts>(__value)...);
    }

    void _M_destruct() noexcept {
        _M_value.~_Tp();
    }

    _RbTreeNodeImpl() noexcept {
    }

    ~_RbTreeNodeImpl() noexcept {
    }
};

// 声明一个模板结构体 _RbTreeIteratorBase，用于红黑树的迭代器基础
template<bool>
struct _RbTreeIteratorBase;

// 定义当模板参数为 false 时的 _RbTreeIteratorBase 结构体
template<>
struct _RbTreeIteratorBase<false> {
public:
    // 定义遍历状态枚举，用于表示迭代器的特殊位置
    enum TraverseStatus {
        BEGINOFF = 0, // node 指向最小的节点
        RENDOFF = 0,
        ENDOFF = 1, // node 指向最大的节点
        RBEGINOFF = 1,
        NORMAL,
    };

public:
    // 构造函数，初始化 node 和 status
    _RbTreeIteratorBase(_RbTreeNode *node) noexcept: _M_node(node), status(NORMAL) {
        if (node == nullptr) {
            status = ENDOFF;
        }
    }

    // 构造函数，初始化 node 和 status
    _RbTreeIteratorBase(_RbTreeNode *node, enum TraverseStatus status): _M_node(node), status(status) {
    }
    _RbTreeNode *_M_node; // 当前迭代器指向的节点
    TraverseStatus status; // 当前迭代器的状态

public:
    // 定义迭代器类别和差值类型
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;

    // 比较两个迭代器是否相等
    bool operator==(_RbTreeIteratorBase const &that) const noexcept {
        return (status == NORMAL && that.status == NORMAL && _M_node == that._M_node) ||
               (status == BEGINOFF && that.status == BEGINOFF && _M_node == that._M_node) ||
               (status == ENDOFF && that.status == ENDOFF && _M_node == that._M_node);
    }

    // 比较两个迭代器是否不相等
    bool operator!=(_RbTreeIteratorBase const &that) const noexcept {
        return !(*this == that);
    }

    // 前缀自增运算符
    void operator++() noexcept {
        if (status == ENDOFF) {
            return; // 已经指向了最大的节点
        }
        if (status == BEGINOFF) {
            status = NORMAL;
            return;
        }
        // normal
        assert(_M_node);
        if (_M_node->_M_right != nullptr) {
            _M_node = _M_node->_M_right;
            while (_M_node->_M_left != nullptr) {
                _M_node = _M_node->_M_left;
            }
        } else {
            auto temp = _M_node;
            while (_M_node->_M_parent != nullptr && _M_node == _M_node->_M_parent->_M_right) {
                _M_node = _M_node->_M_parent;
            }
            if (_M_node->_M_parent == nullptr) {
                status = ENDOFF; // 此时 node 指向的是根节点
                _M_node = temp; // 此时 node 指向的是最大的节点
                return;
            }
            _M_node = _M_node->_M_parent;
        }
    }

    // 前缀自减运算符
    void operator--() noexcept {
        if (status == BEGINOFF) {
            return;
        }
        if (status == ENDOFF) {
            status = NORMAL;
            return;
        }
        // 为了支持 --end()
        if (_M_node->_M_left != nullptr) {
            _M_node = _M_node->_M_left;
            while (_M_node->_M_right != nullptr) {
                _M_node = _M_node->_M_right;
            }
        } else {
            auto temp = _M_node;
            while (_M_node->_M_parent != nullptr && _M_node == _M_node->_M_parent->_M_left) {
                _M_node = _M_node->_M_parent;
            }
            if (_M_node->_M_parent == nullptr) {
                status = BEGINOFF; // 此时 node 指向根节点
                _M_node = temp; // 此时 node 指向最小的节点
                return;
            }
            _M_node = _M_node->_M_parent;
        }
    }
};

// 定义当模板参数为 true 时的 _RbTreeIteratorBase 结构体，继承自 _RbTreeIteratorBase<false>
template<>
struct _RbTreeIteratorBase<true> : _RbTreeIteratorBase<false> {
protected:
    using _RbTreeIteratorBase<false>::_RbTreeIteratorBase;

public:
    // 重载前缀自增运算符，调用基类的自减运算符
    void operator++() noexcept {
        _RbTreeIteratorBase<false>::operator--();
    }

    // 重载前缀自减运算符，调用基类的自增运算符
    void operator--() noexcept {
        _RbTreeIteratorBase<false>::operator++();
    }
};

template<class _NodeImpl, class _Tp, bool _Reverse>
struct _RbTreeIterator : _RbTreeIteratorBase<_Reverse> {
public:
    using _RbTreeIteratorBase<_Reverse>::_RbTreeIteratorBase;

public:
    // const -> non const
    template<class T0 = _Tp>
    explicit operator std::enable_if_t<std::is_const_v<T0>,
        _RbTreeIterator<_NodeImpl, std::remove_const_t<T0>, _Reverse> >() const noexcept {
        return _RbTreeIterator<_NodeImpl, T0, _Reverse>(this->node, this->status);
    }

    // non const -> const
    template<class T0 = _Tp>
    operator std::enable_if_t<!std::is_const_v<T0>,
        _RbTreeIterator<_NodeImpl, std::add_const_t<T0>, _Reverse> >() const noexcept {
        return _RbTreeIterator<_NodeImpl, std::add_const_t<T0>, _Reverse>(this->node, this->status);
    }

    _RbTreeIterator &operator++() noexcept {
        // ++__it
        _RbTreeIteratorBase<_Reverse>::operator++();
        return *this;
    }

    _RbTreeIterator &operator--() noexcept {
        // --__it
        _RbTreeIteratorBase<_Reverse>::operator--();
        return *this;
    }

    _RbTreeIterator operator++(int) noexcept {
        // __it++
        _RbTreeIterator __tmp = *this;
        ++*this;
        return __tmp;
    }

    _RbTreeIterator operator--(int) noexcept {
        // __it--
        _RbTreeIterator __tmp = *this;
        --*this;
        return __tmp;
    }

    _Tp *operator->() const noexcept {
        return std::addressof(
            static_cast<_NodeImpl *>(this->_M_node)->_M_value);
    }

    _Tp &operator*() const noexcept {
        return static_cast<_NodeImpl *>(this->_M_node)->_M_value;
    }

    using value_type = std::remove_const_t<_Tp>;
    using reference = _Tp &;
    using pointer = _Tp *;
};

struct _RbTreeRoot {
    _RbTreeNode *_M_root;
};

struct _RbTreeBase {
protected:
    template<class _Tp, class _Compare, class _Alloc, class _NodeImpl,
    class>
    friend struct _RbTreeNodeHandle;
    _RbTreeRoot *_M_block;

    _RbTreeBase(_RbTreeRoot *block) noexcept: _M_block(block) {
    }

    template<class _Type, class _Alloc>
    static _Type *_M_allocate(_Alloc __alloc) {
        typename std::allocator_traits<_Alloc>::template rebind_alloc<_Type>
                __rebind_alloc(__alloc);
        return std::allocator_traits<_Alloc>::template rebind_traits<
            _Type>::allocate(__rebind_alloc, sizeof(_Type));
    }

    template<class _Type, class _Alloc>
    static void _M_deallocate(_Alloc __alloc, void *__ptr) noexcept {
        typename std::allocator_traits<_Alloc>::template rebind_alloc<_Type>
                __rebind_alloc(__alloc);
        std::allocator_traits<_Alloc>::template rebind_traits<
            _Type>::deallocate(__rebind_alloc, static_cast<_Type *>(__ptr),
                               sizeof(_Type));
    }

    static void _M_rotate_left(_RbTreeNode *__node) noexcept {
        _RbTreeNode *__right = __node->_M_right;
        __node->_M_right = __right->_M_left;
        if (__right->_M_left != nullptr) {
            __right->_M_left->_M_parent = __node;
            __right->_M_left->_M_pparent = &__node->_M_right;
        }
        __right->_M_parent = __node->_M_parent;
        __right->_M_pparent = __node->_M_pparent;
        *__node->_M_pparent = __right;
        __right->_M_left = __node;
        __node->_M_parent = __right;
        __node->_M_pparent = &__right->_M_left;
    }

    static void _M_rotate_right(_RbTreeNode *__node) noexcept {
        _RbTreeNode *__left = __node->_M_left;
        __node->_M_left = __left->_M_right;
        if (__left->_M_right != nullptr) {
            __left->_M_right->_M_parent = __node;
            __left->_M_right->_M_pparent = &__node->_M_left;
        }
        __left->_M_parent = __node->_M_parent;
        __left->_M_pparent = __node->_M_pparent;
        *__node->_M_pparent = __left;
        __left->_M_right = __node;
        __node->_M_parent = __left;
        __node->_M_pparent = &__left->_M_right;
    }

    static void _M_fix_violation(_RbTreeNode *__node) noexcept {
        while (true) {
            _RbTreeNode *__parent = __node->_M_parent;
            if (__parent == nullptr) {
                // 根节点的 __parent 总是 nullptr
                // 情况 0: __node == root
                __node->_M_color = _S_black;
                return;
            }
            if (__node->_M_color == _S_black ||
                __parent->_M_color == _S_black) {
                return;
            }
            _RbTreeNode *__uncle;
            _RbTreeNode *__grandpa = __parent->_M_parent;
            assert(__grandpa);
            _RbTreeChildDir __parent_dir =
                    __parent->_M_pparent == &__grandpa->_M_left
                        ? _S_left
                        : _S_right;
            if (__parent_dir == _S_left) {
                __uncle = __grandpa->_M_right;
            } else {
                assert(__parent->_M_pparent == &__grandpa->_M_right);
                __uncle = __grandpa->_M_left;
            }
            _RbTreeChildDir __node_dir =
                    __node->_M_pparent == &__parent->_M_left ? _S_left : _S_right;
            if (__uncle != nullptr && __uncle->_M_color == _S_red) {
                // 情况 1: 叔叔是红色人士
                __parent->_M_color = _S_black;
                __uncle->_M_color = _S_black;
                __grandpa->_M_color = _S_red;
                __node = __grandpa;
            } else if (__node_dir == __parent_dir) {
                if (__node_dir == _S_right) {
                    assert(__node->_M_pparent == &__parent->_M_right);
                    // 情况 2: 叔叔是黑色人士（RR）
                    _RbTreeBase::_M_rotate_left(__grandpa);
                } else {
                    // 情况 3: 叔叔是黑色人士（LL）
                    _RbTreeBase::_M_rotate_right(__grandpa);
                }
                std::swap(__parent->_M_color, __grandpa->_M_color);
                __node = __grandpa;
            } else {
                if (__node_dir == _S_right) {
                    assert(__node->_M_pparent == &__parent->_M_right);
                    // 情况 4: 叔叔是黑色人士（LR）
                    _RbTreeBase::_M_rotate_left(__parent);
                } else {
                    // 情况 5: 叔叔是黑色人士（RL）
                    _RbTreeBase::_M_rotate_right(__parent);
                }
                __node = __parent;
            }
        }
    }

    _RbTreeNode *_M_min_node() const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        if (__current != nullptr) {
            while (__current->_M_left != nullptr) {
                __current = __current->_M_left;
            }
        }
        return __current;
    }

    _RbTreeNode *_M_max_node() const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        if (__current != nullptr) {
            while (__current->_M_right != nullptr) {
                __current = __current->_M_right;
            }
        }
        return __current;
    }

    template<class _NodeImpl, class _Tv, class _Compare>
    _RbTreeNode *_M_find_node(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        while (__current != nullptr) {
            if (__comp(__value,
                       static_cast<_NodeImpl *>(__current)->_M_value)) {
                __current = __current->_M_left;
                continue;
            }
            if (__comp(static_cast<_NodeImpl *>(__current)->_M_value,
                       __value)) {
                __current = __current->_M_right;
                continue;
            }
            // __value == curent->_M_value
            return __current;
        }
        return nullptr;
    }

    template<class _NodeImpl, class _Tv, class _Compare>
    _RbTreeNode *_M_lower_bound(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        _RbTreeNode *__result = nullptr;
        while (__current != nullptr) {
            if (!(__comp(static_cast<_NodeImpl *>(__current)->_M_value,
                         __value))) {
                // __current->_M_value >= __value
                __result = __current;
                __current = __current->_M_left;
            } else {
                __current = __current->_M_right;
            }
        }
        return __result;
    }

    template<class _NodeImpl, class _Tv, class _Compare>
    _RbTreeNode *_M_upper_bound(_Tv &&__value, _Compare __comp) const noexcept {
        _RbTreeNode *__current = _M_block->_M_root;
        _RbTreeNode *__result = nullptr;
        while (__current != nullptr) {
            if (__comp(__value,
                       static_cast<_NodeImpl *>(__current)
                       ->_M_value)) {
                // __current->_M_value > __value
                __result = __current;
                __current = __current->_M_left;
            } else {
                __current = __current->_M_right;
            }
        }
        return __result;
    }

    template<class _NodeImpl, class _Tv, class _Compare>
    std::pair<_RbTreeNode *, _RbTreeNode *>
    _M_equal_range(_Tv &&__value, _Compare __comp) const noexcept {
        return {
            this->_M_lower_bound<_NodeImpl>(__value, __comp),
            this->_M_upper_bound<_NodeImpl>(__value, __comp)
        };
    }

    /**
     * 在红黑树中用另一节点替换当前节点。
     *
     * 该函数用于在红黑树数据结构中，用一个节点（__replace）替换另一个节点（__node）。
     * 主要进行的操作是更新被替换节点的父节点的指针，使其指向替换节点。
     * 如果替换节点不为空，还需要更新替换节点的父节点指针和双重父节点指针。
     *
     * @param __node 要被替换的节点。
     * @param __replace 替换当前节点的新节点。
     */
    static void _M_transplant(_RbTreeNode *__node,
                              _RbTreeNode *__replace) noexcept {
        // 更新被替换节点的父节点的指针，使其指向替换节点
        *__node->_M_pparent = __replace;
        // 如果替换节点不为空，更新替换节点的父节点指针和双重父节点指针
        if (__replace != nullptr) {
            __replace->_M_parent = __node->_M_parent;
            __replace->_M_pparent = __node->_M_pparent;
        }
    }

    static void _M_delete_fixup(_RbTreeNode *__node) noexcept {
        if (__node == nullptr) {
            return;
        }
        while (__node->_M_parent != nullptr && __node->_M_color == _S_black) {
            _RbTreeChildDir __dir =
                    __node->_M_pparent == &__node->_M_parent->_M_left
                        ? _S_left
                        : _S_right;
            _RbTreeNode *__sibling = __dir == _S_left
                                         ? __node->_M_parent->_M_right
                                         : __node->_M_parent->_M_left;
            if (__sibling->_M_color == _S_red) {
                __sibling->_M_color = _S_black;
                __node->_M_parent->_M_color = _S_red;
                if (__dir == _S_left) {
                    _RbTreeBase::_M_rotate_left(__node->_M_parent);
                } else {
                    _RbTreeBase::_M_rotate_right(__node->_M_parent);
                }
                __sibling = __dir == _S_left
                                ? __node->_M_parent->_M_right
                                : __node->_M_parent->_M_left;
            }
            if (__sibling->_M_left->_M_color == _S_black &&
                __sibling->_M_right->_M_color == _S_black) {
                __sibling->_M_color = _S_red;
                __node = __node->_M_parent;
            } else {
                if (__dir == _S_left &&
                    __sibling->_M_right->_M_color == _S_black) {
                    __sibling->_M_left->_M_color = _S_black;
                    __sibling->_M_color = _S_red;
                    _RbTreeBase::_M_rotate_right(__sibling);
                    __sibling = __node->_M_parent->_M_right;
                } else if (__dir == _S_right &&
                           __sibling->_M_left->_M_color == _S_black) {
                    __sibling->_M_right->_M_color = _S_black;
                    __sibling->_M_color = _S_red;
                    _RbTreeBase::_M_rotate_left(__sibling);
                    __sibling = __node->_M_parent->_M_left;
                }
                __sibling->_M_color = __node->_M_parent->_M_color;
                __node->_M_parent->_M_color = _S_black;
                if (__dir == _S_left) {
                    __sibling->_M_right->_M_color = _S_black;
                    _RbTreeBase::_M_rotate_left(__node->_M_parent);
                } else {
                    __sibling->_M_left->_M_color = _S_black;
                    _RbTreeBase::_M_rotate_right(__node->_M_parent);
                }
                // while (__node->_M_parent != nullptr) {
                //     __node = __node->_M_parent;
                // }
                break;
            }
        }
        __node->_M_color = _S_black;
    }

    static void _M_erase_node(_RbTreeNode *__node) noexcept {
        if (__node->_M_left == nullptr) {
            _RbTreeNode *__right = __node->_M_right;
            _RbTreeColor __color = __node->_M_color;
            _RbTreeBase::_M_transplant(__node, __right);
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__right);
            }
        } else if (__node->_M_right == nullptr) {
            _RbTreeNode *__left = __node->_M_left;
            _RbTreeColor __color = __node->_M_color;
            _RbTreeBase::_M_transplant(__node, __left);
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__left);
            }
        } else {
            _RbTreeNode *__replace = __node->_M_right;
            while (__replace->_M_left != nullptr) {
                __replace = __replace->_M_left;
            }
            _RbTreeNode *__right = __replace->_M_right;
            _RbTreeColor __color = __replace->_M_color;
            if (__replace->_M_parent == __node) {
                if (__right != nullptr) {
                    __right->_M_parent = __replace;
                    __right->_M_pparent = &__replace->_M_right;
                }
            } else {
                _RbTreeBase::_M_transplant(__replace, __right);
                __replace->_M_right = __node->_M_right;
                __replace->_M_right->_M_parent = __replace;
                __replace->_M_right->_M_pparent = &__replace->_M_right;
            }
            _RbTreeBase::_M_transplant(__node, __replace);
            __replace->_M_left = __node->_M_left;
            __replace->_M_left->_M_parent = __replace;
            __replace->_M_left->_M_pparent = &__replace->_M_left;
            if (__color == _S_black) {
                _RbTreeBase::_M_delete_fixup(__right);
            }
        }
    }

    template<class _NodeImpl, class _Compare>
    _RbTreeNode *_M_single_insert_node(_RbTreeNode *__node, _Compare __comp) {
        _RbTreeNode **__pparent = &_M_block->_M_root;
        _RbTreeNode *__parent = nullptr;
        while (*__pparent != nullptr) {
            __parent = *__pparent;
            if (__comp(static_cast<_NodeImpl *>(__node)->_M_value,
                       static_cast<_NodeImpl *>(__parent)->_M_value)) {
                __pparent = &__parent->_M_left;
                continue;
            }
            if (__comp(static_cast<_NodeImpl *>(__parent)->_M_value,
                       static_cast<_NodeImpl *>(__node)->_M_value)) {
                __pparent = &__parent->_M_right;
                continue;
            }
            return __parent;
        }

        __node->_M_left = nullptr;
        __node->_M_right = nullptr;
        __node->_M_color = _S_red;

        __node->_M_parent = __parent;
        __node->_M_pparent = __pparent;
        *__pparent = __node;
        _RbTreeBase::_M_fix_violation(__node);
        return nullptr;
    }

    template<class _NodeImpl, class _Compare>
    void _M_multi_insert_node(_RbTreeNode *__node, _Compare __comp) {
        _RbTreeNode **__pparent = &_M_block->_M_root;
        _RbTreeNode *__parent = nullptr;
        while (*__pparent != nullptr) {
            __parent = *__pparent;
            if (__comp(static_cast<_NodeImpl *>(__node)->_M_value,
                       static_cast<_NodeImpl *>(__parent)->_M_value)) {
                __pparent = &__parent->_M_left;
                continue;
            }
            if (__comp(static_cast<_NodeImpl *>(__parent)->_M_value,
                       static_cast<_NodeImpl *>(__node)->_M_value)) {
                __pparent = &__parent->_M_right;
                continue;
            }
            __pparent = &__parent->_M_right;
        }

        __node->_M_left = nullptr;
        __node->_M_right = nullptr;
        __node->_M_color = _S_red;

        __node->_M_parent = __parent;
        __node->_M_pparent = __pparent;
        *__pparent = __node;
        _RbTreeBase::_M_fix_violation(__node);
    }
};

// 定义一个红黑树节点句柄的模板结构体
// 该结构体用于管理和操作红黑树中的节点
// 参数：
// _Tp: 节点中存储的数据类型
// _Compare: 比较函数对象类型
// _Alloc: 内存分配策略类型
// _NodeImpl: 节点实现的类型
// void: 默认的空类型参数
template<class _Tp, class _Compare, class _Alloc, class _NodeImpl,
    class = void>
struct _RbTreeNodeHandle {
protected:
    // 节点实现类型的指针
    _NodeImpl *_M_node;
    // 内存分配策略对象，使用no_unique_address属性以节省空间
    [[no_unique_address]] _Alloc _M_alloc;

    // 构造函数，初始化节点指针和内存分配策略
    // 参数：
    // __node: 节点实现类型的指针
    // __alloc: 内存分配策略对象
    _RbTreeNodeHandle(_NodeImpl *__node, _Alloc __alloc) noexcept
        : _M_node(__node),
          _M_alloc(__alloc) {
    }

    // 友元声明，使_RbTreeImpl能够访问_RbTreeNodeHandle的保护成员
    template<class, class, class, class>
    friend struct _RbTreeImpl;

public:
    // 默认构造函数，将节点指针初始化为nullptr
    _RbTreeNodeHandle() noexcept : _M_node(nullptr) {
    }

    // 移动构造函数，将资源从另一个_RbTreeNodeHandle实例转移过来
    _RbTreeNodeHandle(_RbTreeNodeHandle &&__that) noexcept
        : _M_node(__that._M_node) {
        __that._M_node = nullptr;
    }

    // 移动赋值运算符，交换两个_RbTreeNodeHandle实例的资源
    _RbTreeNodeHandle &operator=(_RbTreeNodeHandle &&__that) noexcept {
        std::swap(_M_node, __that._M_node);
        return *this;
    }

    // 获取节点中存储的值的引用
    // 返回：
    // _Tp &: 节点中存储的数据的引用
    _Tp &value() const noexcept {
        return static_cast<_NodeImpl *>(_M_node)->_M_value;
    }

    // 析构函数，如果节点指针不为空，则释放节点资源.用在 extract 函数中有用！！
    ~_RbTreeNodeHandle() noexcept {
        if (_M_node) {
            _RbTreeBase::_M_deallocate<_NodeImpl>(_M_alloc, _M_node);
        }
    }
};


// 定义一个模板结构体_RbTreeNodeHandle，用于处理红黑树节点
// 该结构体继承自另一个_RbTreeNodeHandle模板结构体
template<class _Tp, class _Compare, class _Alloc, class _NodeImpl>
struct _RbTreeNodeHandle<
            _Tp, _Compare, _Alloc, _NodeImpl,
            decltype((void) static_cast<typename _Compare::_RbTreeIsMap *>(nullptr))>
        : _RbTreeNodeHandle<_Tp, _Compare, _Alloc, _NodeImpl, void *> {
    // 获取节点中键的引用
    // 这里的key()函数利用了红黑树节点存储的值的first属性来返回键的引用
    typename _Tp::first_type &key() const noexcept {
        return this->value().first;
    }

    // 获取节点中映射值的引用
    // 这里的mapped()函数利用了红黑树节点存储的值的second属性来返回映射值的引用
    typename _Tp::second_type &mapped() const noexcept {
        return this->value().second;
    }
};


template<class _Tp, class _Compare, class _Alloc,
    class _NodeImpl = _RbTreeNodeImpl<_Tp> >
struct _RbTreeImpl : protected _RbTreeBase {
protected:
    [[no_unique_address]] _Alloc _M_alloc;
    [[no_unique_address]] _Compare _M_comp;

public:
    _RbTreeImpl() noexcept
        : _RbTreeBase(_RbTreeBase::_M_allocate<_RbTreeRoot>(_M_alloc)) {
        _M_block->_M_root = nullptr;
    }

    ~_RbTreeImpl() noexcept {
        this->clear();
        _RbTreeBase::_M_deallocate<_RbTreeRoot>(_M_alloc, _M_block);
    }

    explicit _RbTreeImpl(_Compare __comp) noexcept
        : _RbTreeBase(_RbTreeBase::_M_allocate<_RbTreeRoot>(_M_alloc)),
          _M_comp(__comp) {
        _M_block->_M_root = nullptr;
    }

    explicit _RbTreeImpl(_Alloc alloc, _Compare __comp = _Compare()) noexcept
        : _RbTreeBase(_RbTreeBase::_M_allocate<_RbTreeRoot>(_M_alloc)),
          _M_alloc(alloc),
          _M_comp(__comp) {
        _M_block->_M_root = nullptr;
    }

    _RbTreeImpl(_RbTreeImpl &&__that) noexcept : _RbTreeBase(__that._M_block) {
        __that._M_block = _RbTreeBase::_M_allocate<_RbTreeRoot>(_M_alloc);
        __that._M_block->_M_root = nullptr;
    }

    _RbTreeImpl &operator=(_RbTreeImpl &&__that) noexcept {
        std::swap(_M_block, __that._M_block);
        return *this;
    }

protected:
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void _M_single_insert(InputIt input_iter) {
        // 调用父类的逻辑进行分配节点内存
        auto node = _RbTreeBase::_M_allocate<_NodeImpl>(_M_alloc);
        // 在节点内构造对象
        node->_M_construct(*input_iter);
        _RbTreeBase::_M_single_insert_node<_NodeImpl>(node,_M_comp);
    }
    template<_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator,
                                                    _InputIt)>
    void _M_single_insert(_InputIt __first, _InputIt __last) {
        while (__first != __last) {
            this->_M_single_insert(__first);
            ++__first;
        }
    }

    template<_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator,
                                                    _InputIt)>
    void _M_multi_insert(_InputIt __first, _InputIt __last) {
        while (__first != __last) {
            this->_M_multi_insert(*__first);
            ++__first;
        }
    }

public:
    template<_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator,
                                                    _InputIt)>
    void assign(_InputIt __first, _InputIt __last) {
        this->clear();
        this->_M_multi_insert(__first, __last);
    }

    using iterator = _RbTreeIterator<_NodeImpl, _Tp, false>;
    using reverse_iterator = _RbTreeIterator<_NodeImpl, _Tp, true>;
    using const_iterator = _RbTreeIterator<_NodeImpl, _Tp const, false>;
    using const_reverse_iterator = _RbTreeIterator<_NodeImpl, _Tp const, true>;

protected:
    template<class _Tv>
    const_iterator _M_find(_Tv &&__value) const noexcept {
        auto node = this->_M_find_node<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    template<class _Tv>
    iterator _M_find(_Tv &&__value) noexcept {
        auto node = this->_M_find_node<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    template<class... _Ts>
    iterator _M_multi_emplace(_Ts &&... __value) {
        _NodeImpl *__node = _RbTreeBase::_M_allocate<_NodeImpl>(_M_alloc);
        __node->_M_construct(std::forward<_Ts>(__value)...);
        this->_M_multi_insert_node<_NodeImpl>(__node, _M_comp);
        return __node;
    }

    template<class... _Ts>
    std::pair<iterator, bool> _M_single_emplace(_Ts &&... __value) {
        _RbTreeNode *__node = _RbTreeBase::_M_allocate<_NodeImpl>(_M_alloc);
        static_cast<_NodeImpl *>(__node)->_M_construct(
            std::forward<_Ts>(__value)...);
        _RbTreeNode *__conflict =
                this->_M_single_insert_node<_NodeImpl>(__node, _M_comp);
        if (__conflict) {
            static_cast<_NodeImpl *>(__node)->_M_destruct();
            _RbTreeBase::_M_deallocate<_NodeImpl>(_M_alloc, __node);
            return {__conflict, false};
        } else {
            return {__node, true};
        }
    }

public:
    void clear() noexcept {
        iterator __it = this->begin();
        while (__it != this->end()) {
            __it = this->erase(__it);
        }
    }

    iterator erase(const_iterator __it) noexcept {
        assert(__it != this->end());
        iterator __tmp(__it);
        ++__tmp;
        _RbTreeNode *__node = __it._M_node;
        _RbTreeImpl::_M_erase_node(__node);
        static_cast<_NodeImpl *>(__node)->_M_destruct();
        if(__node->_M_parent == nullptr && __node->_M_left == nullptr && __node->_M_right==nullptr)[[unlikely]] {
            __tmp._M_node = nullptr;
        }
        _RbTreeBase::_M_deallocate<_NodeImpl>(_M_alloc, __node);
        return __tmp;
    }

    using node_type = _RbTreeNodeHandle<_Tp, _Compare, _Alloc, _NodeImpl>;

    template<class... _Ts>
    std::pair<iterator, bool> insert(node_type __nh) {
        _NodeImpl *__node = __nh._M_node;
        _RbTreeNode *__conflict =
                this->_M_single_insert_node<_NodeImpl>(__node, _M_comp);
        if (__conflict) {
            static_cast<_NodeImpl *>(__node)->_M_destruct();
            return {__conflict, false};
        } else {
            return {__node, true};
        }
    }

protected:
    node_type _M_extract(iterator __it) noexcept {
        _RbTreeNode *__node = __it._M_node;
        _RbTreeImpl::_M_erase_node(__node);
        return {static_cast<_NodeImpl*>(__node), _M_alloc};
    }
    template<class _Tv>
    size_t _M_single_erase(_Tv &&__value) noexcept {
        _RbTreeNode *__node = this->_M_find_node<_NodeImpl>(__value, _M_comp);
        if (__node != nullptr) {
            this->_M_erase_node(__node);
            static_cast<_NodeImpl *>(__node)->_M_destruct();
            _RbTreeBase::_M_deallocate<_NodeImpl>(_M_alloc, __node);
            return 1;
        } else {
            return 0;
        }
    }

    std::pair<iterator, size_t> _M_erase_range(const_iterator __first,
                                               const_iterator __last) noexcept {
        size_t __num = 0;
        iterator __it(__first);
        while (__it != __last) {
            __it = this->erase(__it);
            ++__num;
        }
        return {__it, __num};
    }

    template<class _Tv>
    size_t _M_multi_erase(_Tv &&__value) noexcept {
        std::pair<iterator, iterator> __range = this->equal_range(__value);
        return this->_M_erase_range(__range.first, __range.second).second;
    }

public:
    iterator erase(const_iterator __first, const_iterator __last) noexcept {
        return _RbTreeImpl::_M_erase_range(__first, __last).first;
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    iterator lower_bound(_Tv &&__value) noexcept {
        return this->_M_lower_bound<_NodeImpl>(__value, _M_comp);
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    const_iterator lower_bound(_Tv &&__value) const noexcept {
        return this->_M_lower_bound<_NodeImpl>(__value, _M_comp);
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    iterator upper_bound(_Tv &&__value) noexcept {
        return this->_M_upper_bound<_NodeImpl>(__value, _M_comp);
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    const_iterator upper_bound(_Tv &&__value) const noexcept {
        return this->_M_upper_bound<_NodeImpl>(__value, _M_comp);
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    std::pair<iterator, iterator> equal_range(_Tv &&__value) noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    template<class _Tv,
        _LIBPENGCXX_REQUIRES_TRANSPARENT_COMPARE(_Compare, _Tv, _Tp)>
    std::pair<const_iterator, const_iterator>
    equal_range(_Tv &&__value) const noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    iterator lower_bound(_Tp const &__value) noexcept {
        auto node = this->_M_lower_bound<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    const_iterator lower_bound(_Tp const &__value) const noexcept {
        auto node = this->_M_lower_bound<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    iterator upper_bound(_Tp const &__value) noexcept {
        auto node = this->_M_upper_bound<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    const_iterator upper_bound(_Tp const &__value) const noexcept {
        auto node = this->_M_upper_bound<_NodeImpl>(__value, _M_comp);
        return node == nullptr ? end() : node;
    }

    std::pair<iterator, iterator> equal_range(_Tp const &__value) noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

    std::pair<const_iterator, const_iterator>
    equal_range(_Tp const &__value) const noexcept {
        return {this->lower_bound(__value), this->upper_bound(__value)};
    }

protected:
    template<class _Tv>
    size_t _M_multi_count(_Tv &&__value) const noexcept {
        const_iterator __it = this->lower_bound(__value);
        return __it != end() ? std::distance(__it, this->upper_bound(__value)) : 0;
    }

    template<class _Tv>
    bool _M_contains(_Tv &&__value) const noexcept {
        return this->template _M_find_node<_NodeImpl>(__value, _M_comp) !=
               nullptr;
    }

    iterator _M_prevent_end(_RbTreeNode *__node) noexcept {
        return __node == nullptr ? end() : __node;
    }

    reverse_iterator _M_prevent_rend(_RbTreeNode *__node) noexcept {
        return __node == nullptr ? rend() : __node;
    }

    const_iterator _M_prevent_end(_RbTreeNode *__node) const noexcept {
        return __node == nullptr ? end() : __node;
    }

    const_reverse_iterator _M_prevent_rend(_RbTreeNode *__node) const noexcept {
        return __node == nullptr ? rend() : __node;
    }

public:
    iterator begin() noexcept {
        auto min_temp = this->_M_min_node();
        return min_temp==nullptr?end():min_temp;
    }

    reverse_iterator rbegin() noexcept {
        auto max_temp = this->_M_max_node();
        return reverse_iterator(max_temp,iterator::NORMAL);
    }

    iterator end() noexcept {
        auto max_temp = this->_M_max_node();
        return iterator(max_temp, iterator::ENDOFF);
    }

    reverse_iterator rend() noexcept {
        auto min_temp = this->_M_min_node();
        return reverse_iterator(min_temp,reverse_iterator::RENDOFF);
    }

    const_iterator begin() const noexcept {
        auto min_temp = this->_M_min_node();
        return min_temp==nullptr?end():min_temp;
    }

    const_reverse_iterator rbegin() const noexcept {
        auto max_temp = this->_M_max_node();
        return const_reverse_iterator(max_temp, iterator::NORMAL);
    }

    const_iterator end() const noexcept {
        auto max_temp = this->_M_max_node();
        return iterator(max_temp, iterator::ENDOFF);
    }

    const_reverse_iterator rend() const noexcept {
        auto min_temp = this->_M_min_node();
        return reverse_iterator(min_temp,reverse_iterator::RENDOFF);
    }

// 提供用于调试目的的红黑树打印功能
#ifndef NDEBUG
    template<class _Ostream>
    void _M_print(_Ostream &__os, _RbTreeNode *__node) {
        if (__node) {
            _Tp &__value = static_cast<_NodeImpl *>(__node)->_M_value;
            __os << '(';
# if __cpp_concepts && __cpp_if_constexpr
            if constexpr (requires(_Tp __t)
            {
                __t.first;
                __t.second;
            }) {
                __os << __value.first << ':' << __value.second;
            } else {
                __os << __value;
            }
            __os << ' ';
# endif
            __os << (__node->_M_color == _S_black ? 'B' : 'R');
            __os << ' ';
            if (__node->_M_left) {
                if (__node->_M_left->_M_parent != __node ||
                    __node->_M_left->_M_pparent != &__node->_M_left) {
                    __os << '*';
                }
            }
            _M_print(__os, __node->_M_left);
            __os << ' ';
            if (__node->_M_right) {
                if (__node->_M_right->_M_parent != __node ||
                    __node->_M_right->_M_pparent != &__node->_M_right) {
                    __os << '*';
                }
            }
            _M_print(__os, __node->_M_right);
            __os << ')';
        } else {
            __os << '.';
        }
    }

    template<class _Ostream>
    void _M_print(_Ostream &__os) {
        _M_print(__os, this->_M_block->_M_root);
        __os << '\n';
    }
#endif

// 检查红黑树是否为空
bool empty() const noexcept {
    return this->_M_block->_M_root == nullptr;
}

// 计算红黑树中的元素数量
size_t size() const noexcept {
    return std::distance(this->begin(), this->end());
}
};

#endif //RBTREE_HPP
