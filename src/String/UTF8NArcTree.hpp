#pragma once

#include <String/UTF8Handler.hpp>


namespace String::UnicodeProperty
{
  using TreeLeaf = uint64_t;
  union Node
  {
    constexpr Node() noexcept;
    constexpr Node(int) noexcept;
    constexpr Node(long long unsigned int leaf) noexcept;
    constexpr Node(const Node* node) noexcept;

    TreeLeaf leaf;
    const Node* node;
  };
  using TreeStartNode = const Node[256];
  using TreeNode = const Node[64];
  extern const Node TREE_STOP;

  /**
   * function compute value exist or not into tree
   * @param tree Tree for function evaluation
   * @param str input data with checking character UTF-8 encoded.
   */
  bool belong(const TreeStartNode& tree, const char* str) noexcept;
}

//
// INLINES
//

namespace String::UnicodeProperty
{
  //
  // Node union
  //

  inline constexpr Node::Node() noexcept
    : node(0)
  {
  }

  inline constexpr Node::Node(int) noexcept
    : node(0)
  {
  }

  inline constexpr Node::Node(long long unsigned int leaf) noexcept
    : leaf(leaf)
  {
  }

  inline constexpr Node::Node(const Node* node) noexcept
    : node(node)
  {
  }


  inline bool belong(const TreeStartNode& tree, const char* str) noexcept
  {
    const Node* current_tree = &tree[static_cast<uint8_t>(*str)];
    for (unsigned long depth = UTF8Handler::get_octet_count(*str); depth != 2; depth--)
    {
      if (!current_tree->node)
      {
        return false;
      }

      if (current_tree->node == &TREE_STOP)
      {
        return true;
      }
      current_tree = &current_tree->node[
          static_cast<uint8_t>(*++str) & 0x3F];
    }
    return (current_tree->leaf & (static_cast<TreeLeaf>(1) <<
      (static_cast<uint8_t>(*(++str)) & 0x3F))) != 0;
  }
}
