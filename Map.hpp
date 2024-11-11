//
// Created by wxk on 2024/11/9.
//
/*
 * 1. 节点可以是黑色或者红色的
 * 2. 根节点总是黑色的
 * 3. 所有叶子节点都是黑色 (叶子节点就是 NULL)
 * 4. 红色节点的两个子节点必须都是黑色的
 * 5. 从任一节点到其叶子节点的路径都包含相同数量的黑色节点
 */
#ifndef MAP_HPP
#define MAP_HPP
#include <iostream>
#include <cassert>
#include <regex>
#if __cpp_concepts && __cpp_lib_concepts
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(category, T) category T
#else
#define _LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(category, T) class T, \
std::enable_if_t<std::is_convertible_v< \
typename std::iterator_traits<T>::iterator_category, \
category##_tag>>
#endif
#define _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare) \
class Compare##Tp = Compare, \
class = typename Compare##Tp::is_transparent
// 这里的分层设计：是因为 类型 T 没有必要都知道，很多逻辑和 T 无关

template<bool kReverse>
struct TreeIteratorBase;
template <class T,bool kReverse>
struct TreeIterator;


// -------------------------------------树相关的高层抽象-------------
enum TreeColor {
    BLACK,
    RED,
};

enum TreeChildDir {
    LEFT,
    RIGHT,
};

// 节点的高层抽象
struct TreeNode {
    TreeNode *left;
    TreeNode *right;
    TreeNode *parent;
    TreeNode **pparent;  // 父节点中指向本节点指针的指针: 比如说 指向 &(parent->right)
    TreeColor color;
};
template <class T>
struct TreeNodeImpl;

// "树根"的高层抽象
struct TreeRoot {
    TreeNode *root;
    TreeRoot() noexcept : root(nullptr) {
    }
};

// "树"的高层抽象
struct TreeBase {
    TreeRoot *block;
    explicit TreeBase(TreeRoot *block):block(block){}
    static void rotate_left(TreeNode *node)noexcept {
        TreeNode *right = node->right;
        node->right = right->left;
        if (right->left != nullptr) {
            right->left->parent = node;
            right->left->pparent = &node->right;
        }
        right->parent = node->parent;
        right->pparent = node->pparent;
        *node->pparent = right;
        right->left = node;
        node->parent = right;
        node->pparent = &right->left;
    }
    static void rotate_right(TreeNode *node) noexcept {
        TreeNode *left = node->left;
        node->left = left->right;
        if (left->right != nullptr) {
            left->right->parent = node;
            left->right->pparent = &node->left;
        }
        left->parent = node->parent;
        left->pparent = node->pparent;
        *node->pparent = left;
        left->right = node;
        node->parent = left;
        node->pparent = &left->right;
    }
    static void fix_violation(TreeNode *node)noexcept {
        while(true) {
            TreeNode *parent = node->parent;
            if(parent==nullptr) {
                // 情况0：等价于node == root
                node->color = BLACK;//根节点必须是黑色
                return;
            }
            if(node->color == BLACK || parent->color == BLACK)
                return; //无需调整
            TreeNode *uncle;
            TreeNode *grandpa = parent->parent;
            assert(grandpa!=nullptr);
            TreeChildDir parent_dir = parent->pparent == &grandpa->left ? LEFT:RIGHT;
            if(parent_dir == LEFT) {
                uncle = grandpa->right;
            }else {
                assert(parent->pparent == &grandpa->right);
                uncle = grandpa->left;
            }
            TreeChildDir node_dir = node->pparent == &parent->left ? LEFT : RIGHT;
            if(uncle!=nullptr && uncle->color == RED) {
                // 情况一：叔叔是红色人士
                parent->color = BLACK;
                uncle->color = BLACK;
                grandpa->color = RED;
                node = grandpa;
            }else if(node_dir == parent_dir) {
                if(node_dir == RIGHT) {
                    assert(node->pparent == &parent->right);
                    // 情况 2: 叔叔是黑色人士（RR）
                    TreeBase::rotate_left(grandpa);
                }else {
                    TreeBase::rotate_right(grandpa);
                }
                std::swap(parent->color, grandpa->color); //位置换了，颜色也换
                node = grandpa;
            }else {
                if(node_dir == RIGHT) {
                    assert(node->pparent == &parent->right);
                    // 情况 4: 叔叔是黑色人士（LR）
                    TreeBase::rotate_left(parent);
                }else {
                    // 情况 5: 叔叔是黑色人士（RL）
                    TreeBase::rotate_right(parent);
                }
                node = parent;
            }
        }
    }
    static void transplant(TreeNode *node,TreeNode *replace)noexcept {
        *node->pparent = replace;
        if(replace != nullptr) {
            replace->parent = node->parent;
            replace->pparent = node->pparent;
        }
    }
    static void delete_fixup(TreeNode *node)noexcept {
        if (node == nullptr)
            return;
        while (node->parent != nullptr && node->color == BLACK) {
            TreeChildDir dir = node->pparent == &node->parent->left ? LEFT : RIGHT;
            TreeNode *sibling = dir == LEFT ? node->parent->right : node->parent->left;
            if (sibling->color == RED) {
                sibling->color = BLACK;
                node->parent->color = RED;
                if (dir == LEFT) {
                    TreeBase::rotate_left(node->parent);
                } else {
                    TreeBase::rotate_right(node->parent);
                }
                sibling = dir == LEFT ? node->parent->right : node->parent->left;
            }
            if (sibling->left->color == BLACK && sibling->right->color == BLACK) {
                sibling->color = RED;
                node = node->parent;
            } else {
                if (dir == LEFT && sibling->right->color == BLACK) {
                    sibling->left->color = BLACK;
                    sibling->color = RED;
                    TreeBase::rotate_right(sibling);
                    sibling = node->parent->right;
                } else if (dir == RIGHT && sibling->left->color == BLACK) {
                    sibling->right->color = BLACK;
                    sibling->color = RED;
                    TreeBase::rotate_left(sibling);
                    sibling = node->parent->left;
                }
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                if (dir == LEFT) {
                    sibling->right->color = BLACK;
                    TreeBase::rotate_left(node->parent);
                } else {
                    sibling->left->color = BLACK;
                    TreeBase::rotate_right(node->parent);
                }
                while (node->parent != nullptr) {
                    node = node->parent;
                }
                // break;
            }
        }
        node->color = BLACK;
    }
    static void erase_node(TreeNode *node)noexcept {
        if(node->left == nullptr) {
            TreeNode *right = node->right;
            TreeColor color = node->color;
            TreeBase::transplant(node, right);
            if(color == BLACK) {
                TreeBase::delete_fixup(right);
            }
        }else if(node->right == nullptr) {
            TreeNode *left = node->left;
            TreeColor color = node->color;
            TreeBase::transplant(node, left);
            if(color == BLACK) {
                TreeBase::delete_fixup(left);
            }
        }else {
            // node 的两个节点都在的情况下
            TreeNode *replace = node->right;
            while(replace->left != nullptr) {
                replace = replace->left; //找到第一个大于 node->value 值的节点
            }
            TreeNode *right = replace->right;
            TreeColor color = node->color;
            if(replace->parent == node) {
                //TODO 检测是否多余
                right->parent = replace;
                right->pparent = &replace->right;
            }else {
                TreeBase::transplant(replace, right);
                replace->right = node->right;
                replace->right->parent = replace;
                replace->right->pparent = &replace->right;
            }
            TreeBase::transplant(node, replace);
            replace->left = node->left;
            replace->left->parent = replace;
            replace->left->pparent = &replace->left;
            if (color == BLACK) {
                TreeBase::delete_fixup(right);
            }
        }
    }
    template <class T, class Compare>
    TreeNode *single_insert_node(TreeNode *node, Compare comp) {
        TreeNode **pparent = &block->root;
        TreeNode *parent = nullptr;
        while (*pparent != nullptr) {
            parent = *pparent;
            if (comp(static_cast<TreeNodeImpl<T> *>(node)->value, static_cast<TreeNodeImpl<T> *>(parent)->value)) {
                pparent = &parent->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(parent)->value, static_cast<TreeNodeImpl<T> *>(node)->value)) {
                pparent = &parent->right;
                continue;
            }
            return parent; // 说明已经有相同的值了
        }

        node->left = nullptr;
        node->right = nullptr;
        node->color = RED;

        node->parent = parent;
        node->pparent = pparent;
        *pparent = node;
        TreeBase::fix_violation(node);
        return nullptr;
    }
    template <class T, class Compare>
    void multi_insert_node(TreeNode *node, Compare comp) {
        TreeNode **pparent = &block->root;
        TreeNode *parent = nullptr;
        while (*pparent != nullptr) {
            parent = *pparent;
            if (comp(static_cast<TreeNodeImpl<T> *>(node)->value, static_cast<TreeNodeImpl<T> *>(parent)->value)) {
                pparent = &parent->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(parent)->value, static_cast<TreeNodeImpl<T> *>(node)->value)) {
                pparent = &parent->right;
                continue;
            }
            pparent = &parent->right;
        }

        node->left = nullptr;
        node->right = nullptr;
        node->color = RED;

        node->parent = parent;
        node->pparent = pparent;
        *pparent = node;
        TreeBase::fix_violation(node);
    }
    TreeNode *min_node() const noexcept {
        TreeNode *current = block->root;
        if (current != nullptr) {
            while (current->left != nullptr) {
                current = current->left;
            }
        }
        return current;
    }
    TreeNode *max_node() const noexcept {
        TreeNode *current = block->root;
        if (current != nullptr) {
            while (current->right != nullptr) {
                current = current->right;
            }
        }
        return current;
    }
    template <class T, class Tv, class Compare>
    TreeNode *find_node(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        while (current != nullptr) {
            if (comp(value, static_cast<TreeNodeImpl<T> *>(current)->value)) {
                current = current->left;
                continue;
            }
            if (comp(static_cast<TreeNodeImpl<T> *>(current)->value, value)) {
                current = current->right;
                continue;
            }
            // value == curent->value
            return current;
        }
        return nullptr;
    }
    // 找到第一个 >= value
    template <class T, class Tv, class Compare>
    TreeNode *lower_bound(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        TreeNode *result = nullptr;
        while (current != nullptr) {
            if (!(comp(static_cast<TreeNodeImpl<T> *>(current)->value, value))) { // current->value >= value
                result = current;
                current = current->left;
            } else {
                current = current->right;
            }
        }
        return result;
    }
    // 找到第一个 > value
    template <class T, class Tv, class Compare>
    TreeNode *upper_bound(Tv &&value, Compare comp) const noexcept {
        TreeNode *current = block->root;
        TreeNode *result = nullptr;
        while (current != nullptr) {
            if (comp(value, static_cast<TreeNodeImpl<T> *>(current)->value)) { // current->value > value
                result = current;
                current = current->left;
            } else {                        // current->value <= value
                current = current->right;
            }
        }
        return result;
    }
    template <class T, class Tv, class Compare>
    std::pair<TreeNode *, TreeNode *> equal_range(Tv &&value, Compare comp) const noexcept {
        return {this->lower_bound<T>(value, comp), this->upper_bound<T>(value, comp)};
    }
};

//-----------------------------------树的底层抽象------------------
template <class T>
struct TreeNodeImpl : TreeNode {
    union {
        T value;
    }; // union 可以阻止里面成员的自动初始化，方便不支持 T() 默认构造的类型
    TreeNodeImpl(const T&value) noexcept : value(value) {}
    TreeNodeImpl(T&& value) noexcept : value(std::move(value)) {}
    TreeNodeImpl() noexcept {}
    ~TreeNodeImpl() noexcept {}
};

template<class T,class Compare>
struct TreeImpl:TreeBase {
    [[no_unique_address]] Compare comp;
    TreeImpl()noexcept:TreeBase(new TreeRoot) {}
    explicit TreeImpl(Compare comp)noexcept:TreeBase(new TreeRoot),comp(comp){}
    TreeImpl(TreeImpl &&that) noexcept : TreeBase(that.block) {
        that.block = nullptr;
    }
    TreeImpl &operator=(TreeImpl &&that) noexcept {
        std::swap(block, that.block);
        return *this;
    }
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void single_insert(InputIt first, InputIt last) {
        while (first != last) {
            this->single_insert(first);
            ++first;
        }
    }
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void single_insert(InputIt input_iter) {
        auto node = new TreeNodeImpl<T>(*input_iter); //TODO 依赖了底层具体 TreeNodeImpl
        TreeBase::single_insert_node<T>(node,comp);
    }
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void multi_insert(InputIt first, InputIt last) {
        while (first != last) {
            this->multi_insert(*first);
            ++first;
        }
    }
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void multi_insert(InputIt input) {
        auto node = new TreeNodeImpl<T>(*input); //TODO 依赖了底层具体 TreeNodeImpl
        TreeBase::multi_insert_node<T>(node,comp);
    }
    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void assign(InputIt first, InputIt last) {
        this->clear();
        this->multi_insert(first, last);
    }
    using iterator = TreeIterator<T, false>;
    using reverse_iterator = TreeIterator<T, true>;
    using const_iterator = TreeIterator<T const, false>;
    using const_reverse_iterator = TreeIterator<T const, true>;
    template <class Tv>
    const_iterator find(Tv &&value) const noexcept {
        auto node = this->find_node<T>(value, comp);
        return node == nullptr ? end() : node;
    }
    template <class Tv>
    iterator find(Tv &&value) noexcept {
        auto node = this->find_node<T>(value, comp);
        return node == nullptr ? end() : node;
    }
    template <class ...Ts>
    iterator multi_emplace(Ts &&...value) {
        TreeNodeImpl<T> *node = new TreeNodeImpl<T>;
        new (std::addressof(node->value)) T(std::forward<Ts>(value)...);
        this->multi_insert_node<T>(node, comp);
        return node;
    }
    template <class ...Ts>
    std::pair<iterator, bool> single_emplace(Ts &&...value) {
        TreeNode *node = new TreeNodeImpl<T>;
        new (std::addressof(static_cast<TreeNodeImpl<T> *>(node)->value)) T(std::forward<Ts>(value)...);
        TreeNode *conflict = this->single_insert_node<T>(node, comp);
        if (conflict) {
            static_cast<TreeNodeImpl<T> *>(node)->value.~T();
            delete node;
            return {conflict, false};
        } else {
            return {node, true};
        }
    }
    void clear() noexcept {
        for (auto it = this->begin(); it != this->end(); ) {
            it = this->erase(it);
        }
    }
    static iterator erase(const_iterator it) noexcept {
        const_iterator tmp = it;
        ++tmp;
        TreeNode *node = it.node;           // node 是迭代器的一个成员
        // 假如删除的是最后一个成员,应当对 将下一个指针 tmp 更改为 end() 的值 ： 即 node = nullptr , status = ENDOFF
        TreeImpl::erase_node(node);
        static_cast<TreeNodeImpl<T> *>(node)->value.~T();
        if(node->parent == nullptr && node->left == nullptr && node->right==nullptr)[[unlikely]] {
            tmp.node = nullptr;
        }
        delete node;
        return iterator(tmp);       // 强制转换
    }
    template <class Tv>
    size_t single_erase(Tv &&value) noexcept {
        TreeNode *node = this->find_node<T>(value, comp);
        if (node != nullptr) {
            this->erase_node(node);
            static_cast<TreeNodeImpl<T> *>(node)->value.~T();
            delete node;
            return 1;
        } else {
            return 0;
        }
    }
    static std::pair<iterator, size_t> erase_range(const_iterator first, const_iterator last) noexcept {
        size_t num = 0;
        while (first != last) {
            first = TreeImpl::erase(first);
            ++num;
        }
        return {iterator(first), num};
    }
    template <class Tv>
    size_t multi_erase(Tv &&value) noexcept {
        std::pair<iterator, iterator> range = this->equal_range(value);
        return this->erase_range(range.first, range.second).second;
    }
    static iterator erase(const_iterator first, const_iterator last) noexcept {
        return TreeImpl::erase_range(first, last).first;
    }
    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator lower_bound(Tv &&value) noexcept {
        return TreeBase::lower_bound<T>(value, comp);
    }
    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator lower_bound(Tv &&value) const noexcept {
        return TreeBase::lower_bound<T>(value, comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    iterator upper_bound(Tv &&value) noexcept {
        return TreeBase::upper_bound<T>(value, comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator upper_bound(Tv &&value) const noexcept {
        return TreeBase::upper_bound<T,Compare>(value, comp);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    std::pair<iterator, iterator> equal_range(Tv &&value) noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    std::pair<const_iterator, const_iterator> equal_range(Tv &&value) const noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }


    iterator lower_bound(T const &value) noexcept {
        auto node = TreeBase::lower_bound<T>(value, comp);
        return node == nullptr ? end() : node;
    }

    const_iterator lower_bound(T const &value) const noexcept {
        auto node = TreeBase::lower_bound<T>(value, comp);
        return node == nullptr ? end() : node;
    }

    iterator upper_bound(T const &value) noexcept {
        auto node = TreeBase::upper_bound<T>(value, comp);
        return node == nullptr ? end() : node;
    }

    const_iterator upper_bound(T const &value) const noexcept {
        auto node = TreeBase::upper_bound<T>(value, comp);
        return node == nullptr ? end() : node;
    }

    std::pair<iterator, iterator> equal_range(T const &value) noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }

    std::pair<const_iterator, const_iterator> equal_range(T const &value) const noexcept {
        return {this->lower_bound(value), this->upper_bound(value)};
    }
    template <class Tv>
    size_t multi_count(Tv &&value) const noexcept {
        auto it = this->lower_bound(value);
        return it != end() ? std::distance(it, this->upper_bound()) : 0;
    }

    template <class Tv>
    bool contains(Tv &&value) const noexcept {
        return this->template find_node<T>(value, comp) != nullptr;
    }

    iterator begin() noexcept {
        auto min_temp = this->min_node();
        return min_temp==nullptr?end():min_temp;
    }

    reverse_iterator rbegin() noexcept {
        auto max_temp = this->max_node();
        return reverse_iterator(max_temp);
    }

    iterator end() noexcept {
        auto max_temp = this->max_node();
        return iterator(max_temp, iterator::ENDOFF);
    }

    reverse_iterator rend() noexcept {
        auto min_temp = this->min_node();
        return reverse_iterator(min_temp,reverse_iterator::RENDOFF);
    }

    const_iterator begin() const noexcept {
        auto min_temp = this->min_node();
        return min_temp==nullptr?end():min_temp;
    }

    const_reverse_iterator rbegin() const noexcept {
        auto node = this->max_node();
        return node == nullptr ? rend() : node;
    }

    const_iterator end() const noexcept {
        auto max_temp = this->max_node();
        return iterator(max_temp, iterator::ENDOFF);
    }

    const_reverse_iterator rend() const noexcept {
        return &block->root;
    }

    void print(TreeNode *node) {
        if (node) {
            T &value = static_cast<TreeNodeImpl<T> *>(node)->value;
            if constexpr (requires (T t) { t.first; t.second; }) {
                std::cout << value.first << ':' << value.second;
            } else {
                std::cout << value;
            }
            std::cout << '(' << (node->color == BLACK ? 'B' : 'R') << ' ';
            if (node->left) {
                if (node->left->parent != node || node->left->pparent != &node->left) {
                    std::cout << '*';
                }
            }
            print(node->left);
            std::cout << ' ';
            if (node->right) {
                if (node->right->parent != node || node->right->pparent != &node->right) {
                    std::cout << '*';
                }
            }
            print(node->right);
            std::cout << ')';
        } else {
            std::cout << '.';
        }
    }

    void print() {
        print(this->block->root);
        std::cout << '\n';
    }
};


//------------------------------------迭代器的高层抽象----------------------
// 一个双向迭代器： 该有的 前置++,前置--。仅仅依赖于 TreeNode
template<bool kReverse>
struct TreeIteratorBase {};
template<>
struct TreeIteratorBase<false> {
    TreeNode *node;
    enum TraverseStatus {
        BEGINOFF=0,    // node 指向最小的节点
        RENDOFF=0,
        ENDOFF=1,      // node 指向最大的节点
        RBEGINOFF=1,
        NORMAL,
    };
    TraverseStatus status;

    TreeIteratorBase(TreeNode *node)noexcept:node(node),status(NORMAL) {
        if(node==nullptr) {
            status=ENDOFF;
        }
    }
    TreeIteratorBase(TreeNode *node, enum TraverseStatus status):node(node),status(status){}

    bool operator==(TreeIteratorBase const &that)const noexcept {
        return (status == NORMAL && that.status == NORMAL && node == that.node) ||
                (status == BEGINOFF && that.status == BEGINOFF && node == that.node) ||
                (status == ENDOFF && that.status == ENDOFF && node == that.node) ;
    }
    bool operator!=(TreeIteratorBase const &that)const noexcept {
        return !(*this == that);
    }
    void operator++() noexcept {
        if(status == ENDOFF) {
            return;   // 已经指向了最大的节点
        }
        if(status == BEGINOFF) {
            status = NORMAL;
            return;
        }
        // normal
        assert(node);
        if(node->right != nullptr) {
            node = node->right;
            while(node->left != nullptr) {
                node = node->left;
            }
        }else {
            auto temp = node;
            while(node->parent != nullptr && node == node->parent->right) {
                node = node->parent;
            }
            if(node->parent==nullptr) {
                status = ENDOFF; // 此时 node 指向的是根节点
                node = temp;     // 此时 node 指向的是最大的节点
                return;
            }
            node = node->parent;
        }
    }
    void operator--() noexcept {
        if(status == BEGINOFF) {
            return;
        }
        if(status == ENDOFF) {
            status = NORMAL;
            return;
        }
        // 为了支持 --end()
        if(node->left!=nullptr) {
            node = node->left;
            while(node->right!=nullptr) {
                node = node->right;
            }
        }else {
            auto temp = node;
            while(node->parent != nullptr && node == node->parent->left) {
                node = node->parent;
            }
            if(node->parent==nullptr) {
                status = BEGINOFF;    // 此时 node 指向根节点
                node = temp;          // 此时 node 指向最小的节点
                return;
            }
            node = node->parent;
        }
    }

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
};
template<>
struct TreeIteratorBase<true>:TreeIteratorBase<false> {
    using TreeIteratorBase<false>::TreeIteratorBase; // 构造函数
    void operator++() noexcept {
        TreeIteratorBase<false>::operator--();
    }
    void operator--() noexcept {
        TreeIteratorBase<false>::operator++();
    }
};

//--------------------------------开始和 T 类型相关了---------------
// 迭代器适配器
template <class T,bool kReverse>
struct TreeIterator : TreeIteratorBase<kReverse> {
    using TreeIteratorBase<kReverse>::TreeIteratorBase;
    // const -> non const
    template <class T0 = T>
    explicit operator std::enable_if_t<std::is_const_v<T0>,
        TreeIterator<std::remove_const_t<T0>, kReverse>>() const noexcept {
        return TreeIterator<std::remove_const_t<T0>, kReverse>(this->node,this->status);
    }
    // non const -> const
    template <class T0 = T>
    operator std::enable_if_t<!std::is_const_v<T0>,
        TreeIterator<std::add_const_t<T0>, kReverse>>() const noexcept {
        return TreeIterator<std::add_const_t<T0>, kReverse>(this->node,this->status);
    }

    TreeIterator &operator++() noexcept {   // ++it
        TreeIteratorBase<kReverse>::operator++();
        return *this;
    }

    TreeIterator &operator--() noexcept {   // --it
        TreeIteratorBase<kReverse>::operator--();
        return *this;
    }

    TreeIterator operator++(int) noexcept { // it++
        TreeIterator tmp = *this;
        ++*this;
        return tmp;
    }
    TreeIterator operator--(int) noexcept { // it--
        TreeIterator tmp = *this;
        --*this;
        return tmp;
    }
    T *operator->() const noexcept {
        return std::addressof(static_cast<TreeNodeImpl<T> *>(this->node)->value);
    }

    T &operator*() const noexcept {
        return static_cast<TreeNodeImpl<T> *>(this->node)->value;
    }

    using value_type = std::remove_const_t<T>;
    using reference = T &;
    using pointer = T *;
};

template <class T, class Compare = std::less<T>>
struct Set : TreeImpl<T, Compare> {
    using typename TreeImpl<T, Compare>::const_iterator;
    using iterator = const_iterator;
    using value_type = T;

    Set() = default;
    explicit Set(Compare comp) : TreeImpl<T, Compare>(comp) {}

    Set(Set &&) = default;
    Set &operator=(Set &&) = default;

    Set(Set const &that) : TreeImpl<T, Compare>() {
        this->single_insert(that.begin(), that.end());
    }

    Set &operator=(Set const &that) {
        if (&that != this) {
            this->assign(that.begin(), that.end());
        }
        return *this;
    }

    Set &operator=(std::initializer_list<T> ilist) {
        this->assign(ilist);
        return *this;
    }

    void assign(std::initializer_list<T> ilist) {
        this->clear();
        this->single_insert(ilist.begin(), ilist.end());
    }

    Compare value_comp() const noexcept {
        return this->comp;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    const_iterator find(T &&value) const noexcept {
        return TreeImpl<T,Compare>::find(value);
    }

    const_iterator find(T const &value) const noexcept {
        return TreeImpl<T,Compare>::find(value);
    }

    std::pair<iterator, bool> insert(T &&value) {
        return this->single_emplace(std::move(value));
    }

    std::pair<iterator, bool> insert(T const &value) {
        return this->single_emplace(value);
    }

    template <class ...Ts>
    std::pair<iterator, bool> emplace(Ts &&...value) {
        return this->single_emplace(std::forward<Ts>(value)...);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void insert(InputIt first, InputIt last) {
        return this->single_insert(first, last);
    }

    template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
    void assign(InputIt first, InputIt last) {
        this->clear();
        return this->single_insert(first, last);
    }

    using TreeImpl<T, Compare>::erase;

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t erase(Tv &&value) {
        return this->single_erase(value);
    }

    size_t erase(T const &value) {
        return this->single_erase(value);
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    size_t count(Tv &&value) const noexcept {
        return TreeImpl<T,Compare>::contains(value) ? 1 : 0;
    }

    size_t count(T const &value) const noexcept {
        return TreeImpl<T,Compare>::contains(value) ? 1 : 0;
    }

    template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
    bool contains(Tv &&value) const noexcept {
        return TreeImpl<T,Compare>::contains(value);
    }

    bool contains(T const &value) const noexcept {
        return TreeImpl<T,Compare>::contains(value);;
    }
};

// template <class T, class Compare = std::less<T>>
// struct MultiSet : TreeImpl<T, Compare> {
//     using typename TreeImpl<T, Compare>::const_iterator;
//     using iterator = const_iterator;
//
//     MultiSet() = default;
//     explicit MultiSet(Compare comp) : TreeImpl<T, Compare>(comp) {}
//
//     MultiSet(MultiSet &&) = default;
//     MultiSet &operator=(MultiSet &&) = default;
//
//     MultiSet(MultiSet const &that) : TreeImpl<T, Compare>() {
//         this->multi_insert(that.begin(), that.end());
//     }
//
//     MultiSet &operator=(MultiSet const &that) {
//         if (&that != this) {
//             this->assign(that.begin(), that.end());
//         }
//         return *this;
//     }
//
//     MultiSet &operator=(std::initializer_list<T> ilist) {
//         this->assign(ilist);
//         return *this;
//     }
//
//     void assign(std::initializer_list<T> ilist) {
//         this->clear();
//         this->multi_insert(ilist.begin(), ilist.end());
//     }
//
//     Compare value_comp() const noexcept {
//         return this->comp;
//     }
//
//     template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
//     const_iterator find(T &&value) const noexcept {
//         return this->find(value);
//     }
//
//     const_iterator find(T const &value) const noexcept {
//         return this->find(value);
//     }
//
//     iterator insert(T &&value) {
//         return this->multi_emplace(std::move(value));
//     }
//
//     iterator insert(T const &value) {
//         return this->multi_emplace(value);
//     }
//
//     template <class ...Ts>
//     iterator emplace(Ts &&...value) {
//         return this->multi_emplace(std::forward<Ts>(value)...);
//     }
//
//     template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
//     void insert(InputIt first, InputIt last) {
//         return this->multi_insert(first, last);
//     }
//
//     using TreeImpl<T, Compare>::assign;
//
//     template <_LIBPENGCXX_REQUIRES_ITERATOR_CATEGORY(std::input_iterator, InputIt)>
//     void assign(InputIt first, InputIt last) {
//         this->clear();
//         return this->multi_insert(first, last);
//     }
//
//     using TreeImpl<T, Compare>::erase;
//
//     template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
//     size_t erase(Tv &&value) {
//         return this->multi_erase(value);
//     }
//
//     size_t erase(T const &value) {
//         return this->multi_erase(value);
//     }
//
//     template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
//     size_t count(Tv &&value) const noexcept {
//         return this->multi_count(value);
//     }
//
//     size_t count(T const &value) const noexcept {
//         return this->multi_count(value);
//     }
//
//     template <class Tv, _LIBPENGCXX_REQUIRES_TRANSPARENT(Compare)>
//     bool contains(Tv &&value) const noexcept {
//         return this->contains(value);
//     }
//
//     bool contains(T const &value) const noexcept {
//         return this->contains(value);
//     }
// };
#endif //MAP_HPP
