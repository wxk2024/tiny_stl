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
#include <cassert>
#include <regex>

enum TreeColor {
    BLACK,
    RED,
};

enum TreeChildDir {
    LEFT,
    RIGHT,
};

struct TreeNode {
    TreeNode *left;
    TreeNode *right;
    TreeNode *parent;
    TreeNode **pparent;  // 父节点中指向本节点指针的指针: 比如说 指向 &(parent->right)
    TreeColor color;
    int value;
};

// 一个双向迭代器： 该有的 前置++,前置--
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

// 迭代器适配器
template <bool kReverse>
struct TreeIterator : TreeIteratorBase<kReverse> {
    using TreeIteratorBase<kReverse>::TreeIteratorBase;

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
    int *operator->() const noexcept {
        return std::addressof((this->node)->value);
    }

    int &operator*() const noexcept {
        return (this->node)->value;
    }
    using value_type = int;
    using reference = int &;
    using pointer = int *;
};

struct Set {
    TreeNode *root = nullptr;
    using iterator = TreeIterator<false>;
    using reverse_iterator = TreeIterator<true>;
    //using const_iterator = TreeIterator<T const, false>;
    //using const_reverse_iterator = TreeIterator<T const, true>;
    TreeNode *min_node()const noexcept {
        TreeNode *current = root;
        if(current!=nullptr) {
            while(current->left!=nullptr) {
                current = current->left;
            }
        }
        return current;
    }
    TreeNode *max_node()const noexcept {
        TreeNode *current = root;
        if(current!=nullptr) {
            while(current->right!=nullptr) {
                current = current->right;
            }
        }
        return current;
    }
    iterator begin()const noexcept {
        auto min_temp = min_node();
        return min_temp==nullptr?end():min_temp;
    }
    iterator end()const noexcept {
        auto max_temp = max_node();
        return iterator(max_temp,iterator::ENDOFF);
    }
    reverse_iterator rbegin() {
        auto max_temp = max_node();
        return reverse_iterator(max_temp);
    }
    reverse_iterator rend() {
        auto min_temp = min_node();
        return reverse_iterator(min_temp,reverse_iterator::RENDOFF);
    }
    iterator find(int value) const{
        TreeNode *current = root;
        while (current != nullptr) {
            if (current->value > value) {
                current = current->left;
                continue;
            }
            if (value > current->value) {
                current = current->right;
                continue;
            }
            break; // value == current->value
        }
        return current;
    }
    size_t count(int value) const noexcept {
         return find(value) == end() ? 0 : 1;
    }
    size_t contains(int value) const noexcept {
        return find(value) != end();
    }
    TreeNode *equal_range(int value) {
        TreeNode *node = new TreeNode();
        node->value = value;
    }

    void rotate_left(TreeNode *node) {
        TreeNode *right = node->right;
        node->right = right->left;
        if (right->left != nullptr) {
            right->left->parent = node;
        }
        right->parent = node->parent;
        if (node->parent == nullptr) {
            root = right;
        } else if (node == node->parent->left) {
            node->parent->left = right;
        } else {
            node->parent->right = right;
        }
        right->left = node;
        node->parent = right;
    }

    void rotate_right(TreeNode *node) {
        TreeNode *left = node->left;
        node->left = left->right;
        if (left->right != nullptr) {
            left->right->parent = node;
        }
        left->parent = node->parent;
        if (node->parent == nullptr) {
            root = left;
        } else if (node == node->parent->left) {
            node->parent->left = left;
        } else {
            node->parent->right = left;
        }
        left->right = node;
        node->parent = left;
    }

    void fix_violation(TreeNode *node) {
        while (true) {
            TreeNode *parent = node->parent;
            if (parent == nullptr) {
                // 情况 0：node == root
                node->color = BLACK;
                return;
            }
            if (!(node->color == RED && parent->color == RED)) {
                return; // 无需修复
            }
            TreeNode *uncle;
            TreeNode *grandpa = parent->parent;
            TreeChildDir parent_dir = parent == grandpa->left ? LEFT : RIGHT;
            if (parent_dir == LEFT) {
                uncle = grandpa->right;
            } else {
                uncle = grandpa->left;
            }
            TreeChildDir node_dir = node == parent->left ? LEFT : RIGHT;
            if (uncle != nullptr && uncle->color == RED) {
                // 情况1：叔叔是红色
                uncle->color = BLACK;
                parent->color = BLACK;
                grandpa->color = RED;
                fix_violation(grandpa);
            } else if (node_dir == parent_dir) {
                if (node_dir == RIGHT) {
                    // 情况2：叔叔是黑色(和父亲同侧 RR)
                    rotate_left(grandpa);
                } else {
                    // 情况3：叔叔是黑色(和父亲同侧 LL)
                    rotate_right(grandpa);
                }
                std::swap(parent->color, grandpa->color);
                node = grandpa;
            } else {
                if (node_dir == RIGHT) {
                    // 情况4：叔叔是黑色(和父亲同侧 LR)
                    rotate_left(parent);
                } else {
                    // 情况5：叔叔是黑色(和父亲同侧 RL)
                    rotate_right(parent);
                }
                node = parent;
            }
        }
    }

    std::pair<TreeNode *, bool> insert(int value) {
        // linux 当中的二级指针写法，可以减少使用一个变量
        TreeNode **ppnext = &root;
        TreeNode *parent = nullptr;
        while (*ppnext != nullptr) {
            parent = *ppnext;
            if (parent->value > value) {
                ppnext = &parent->left;
                continue;
            }
            if (value > parent->value) {
                ppnext = &parent->right;
                continue;
            }
            return {parent, false}; // value == current->value
        }
        TreeNode *node = new TreeNode();
        node->value = value;
        node->left = nullptr;
        node->right = nullptr;
        node->color = RED;

        node->parent = parent;
        *ppnext = node;
        fix_violation(node);
        return {node, true};
    }
};
#endif //MAP_HPP
