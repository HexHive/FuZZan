#include "asan_rbtree.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#if defined(PRINT_STATICS)
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef ENABLERBTREE
#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_symbolizer.h"

extern "C" void *__libc_malloc(uptr size);
extern "C" void __libc_free(void *ptr);

#ifdef ENABLECACHE
#ifdef USEMMAPCACHE
__attribute__((visibility("default"))) extern CacheEntry *HashMapCache;
#else
__attribute__((visibility("default")))
extern CacheEntry HashMapCache[MAXCACHESIZE];
#endif

#ifdef ENABLEPERRBTREE
__attribute__((visibility("default"))) extern rbtree_type **TopRBTree;
#endif
#endif

namespace __asan {
  typedef enum rbtree_node_color color;

  static node grandparent(node n);
  static node sibling(node n);
  static node uncle(node n);
  static color node_color(node n);

  static node new_node(atreekey key, color node_color,
                       node left, node right);
  static void rotate_left(node n);
  static void rotate_right(node n);

  static void replace_node(node oldn, node newn);
  static void insert_case1(node n);
  static void insert_case2(node n);
  static void insert_case3(node n);
  static void insert_case4(node n);
  static void insert_case5(node n);
  static node maximum_node(node root);
  static void delete_case1(node n);
  static void delete_case2(node n);
  static void delete_case3(node n);
  static void delete_case4(node n);
  static void delete_case5(node n);
  static void delete_case6(node n);

  node grandparent(node n) {
    assert(n != NULL);
    assert(n->parent != NULL); /* Not the root node */
    assert(n->parent->parent != NULL); /* Not child of root */
    return n->parent->parent;
  }

  node sibling(node n) {
    assert(n != NULL);
    assert(n->parent != NULL); /* Root node has no sibling */
    if (n == n->parent->left)
      return n->parent->right;
    else
      return n->parent->left;
  }

  node uncle(node n) {
    assert(n != NULL);
    assert(n->parent != NULL); /* Root node has no uncle */
    assert(n->parent->parent != NULL); /* Children of root have no uncle */
    return sibling(n->parent);
  }

  color node_color(node n) {
    return n == NULL ? BLACK : n->color;
  }

#ifdef ENABLETHREETREE
  rbtree getTargetRBTree(void *ptr) {
    if ((uptr)ptr < 0x10007fff8000) {
      return ASanTree_global;
    } else if ((uptr)ptr >= 0x600000000000 && (uptr)ptr < 0x700000000000) {
      return ASanTree_heap;
    } else {
      return ASanTree_stack;
    }
  }
#endif

  extern "C" NOINLINE INTERFACE_ATTRIBUTE rbtree rbtree_create() {
    rbtree t = (rbtree)__libc_malloc(sizeof(struct rbtree_t));
    t->root = NULL;
    return t;
  }

  node new_node(atreekey key, color node_color, node left,
                node right) {
    node result = (node)__libc_malloc(sizeof(struct rbtree_node_t));

    if (result == NULL) {
      return NULL;
    } else {
      result->key = key;
#ifdef ENABLECACHE
      result->key.saveEntryCnt = 0;
#endif
      result->color = node_color;
      result->left = left;
      result->right = right;
      if (left  != NULL)  left->parent = result;
      if (right != NULL) right->parent = result;
      result->parent = NULL;
      return result;
    }
  }

  int compare(atreekey key, atreekey target) {
    if ((uptr) target.end < (uptr) key.start)
      return 1;
    else if ((uptr) target.start > (uptr) key.end)
      return -1;
    return 0;
  }

  inline int get_min(int a, int b) {
    if (a < b) return a;
    else
      return b;
  }

  void rotate_left(node n) {
    node r = n->right;
    replace_node(n, r);
    n->right = r->left;
    if (r->left != NULL) {
      r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
  }

  void rotate_right(node n) {
    node L = n->left;
    replace_node(n, L);
    n->left = L->right;
    if (L->right != NULL) {
      L->right->parent = n;
    }
    L->right = n;
    n->parent = L;
  }

  void replace_node(node oldn, node newn) {
    if (oldn->parent == NULL) {
#ifdef ENABLEPERRBTREE
      unsigned long idx1 =
        (unsigned long)oldn->key.start >> selectedshift & selectedmask;
      rbtree ASanTree = TopRBTree[idx1];
#endif
#ifdef ENABLETHREETREE
      rbtree ASanTree = getTargetRBTree(oldn->key.start);
#endif
      ASanTree->root = newn;
    } else {
      if (oldn == oldn->parent->left)
        oldn->parent->left = newn;
      else
        oldn->parent->right = newn;
    }
    if (newn != NULL) {
      newn->parent = oldn->parent;
    }
  }

  extern "C" NOINLINE INTERFACE_ATTRIBUTE
    void hexasan_insert(atreekey key) {
#ifdef PRINT_STATICS
  if (getpid() == InitPid) {
    totalAccess = 0, safeAccess = 0, badAccess = 0;
    rbtreeGlobalInsert = 0, rbtreeGlobalDelete = 0;
    rbtreeHeapInsert = 0, rbtreeHeapDelete = 0;
    rbtreeStackInsert = 0, rbtreeStackDelete = 0;
    Inline = 0, Callfun = 0, InlineError = 0;
    CacheHit = 0, CacheMiss = 0, CacheInit = 0, EntryUse = 0;
    OverAccess = 0, TotalCheck = 0;
    rbtreeInsert = 0, rbtreeDelete = 0, rbtreeLookup = 0;
  }
#endif

#ifdef DISABLE_META
      return;
#endif
#ifdef ENABLEPERRBTREE
      unsigned long idx1 =
        (unsigned long)key.start >> selectedshift & selectedmask;
      if (!TopRBTree[idx1])
        TopRBTree[idx1] = (rbtree_type *)rbtree_create();
      rbtree ASanTree = TopRBTree[idx1];
#endif
#ifdef ENABLETHREETREE
      rbtree ASanTree = getTargetRBTree(key.start);
#endif
      node inserted_node = new_node(key, RED, NULL, NULL);
      if (inserted_node == NULL) {
        return;
      }

      if (ASanTree->root == NULL) {
        ASanTree->root = inserted_node;
      } else {
        node n = ASanTree->root;
        while (1) {
          if ((uptr) n->key.start <= (uptr) key.start
              && (uptr) key.end <= (uptr) n->key.end) {
            __libc_free(inserted_node);
            return;
          } else if ((uptr) key.end < (uptr) n->key.start) {
            if (n->left == NULL) {
              n->left = inserted_node;
              break;
            } else {
              n = n ->left;
            }
          } else if ((uptr) n->key.end < (uptr) key.start) {
            if (n->right == NULL) {
              n->right = inserted_node;
              break;
            } else {
              n = n->right;
            }
          } else {
            atreekey input;
            if (key.start < n->key.start)
              input.start = key.start;
            else
              input.start = n->key.start;

            if (key.end > n->key.end)
              input.end = key.end;
            else
              input.end = n->key.end;
            __libc_free(inserted_node);
            hexasan_delete(n->key);
            hexasan_insert(input);
            return;
          }
        }
        inserted_node->parent = n;
      }
#ifdef ENABLECACHE
      ASanTree->versionNum++;
#endif
      insert_case1(inserted_node);
    }

  void insert_case1(node n) {
    if (n->parent == NULL)
      n->color = BLACK;
    else
      insert_case2(n);
  }

  void insert_case2(node n) {
    if (node_color(n->parent) == BLACK)
      return; /* Tree is still valid */
    else
      insert_case3(n);
  }

  void insert_case3(node n) {
    if (node_color(uncle(n)) == RED) {
      n->parent->color = BLACK;
      uncle(n)->color = BLACK;
      grandparent(n)->color = RED;
      insert_case1(grandparent(n));
    } else {
      insert_case4(n);
    }
  }

  void insert_case4(node n) {
    if (n == n->parent->right && n->parent == grandparent(n)->left) {
      rotate_left(n->parent);
      n = n->left;
    } else if (n == n->parent->left && n->parent == grandparent(n)->right) {
      rotate_right(n->parent);
      n = n->right;
    }
    insert_case5(n);
  }

  void insert_case5(node n) {
    n->parent->color = BLACK;
    grandparent(n)->color = RED;
    if (n == n->parent->left && n->parent == grandparent(n)->left) {
      rotate_right(grandparent(n));
    } else {
      assert(n == n->parent->right && n->parent == grandparent(n)->right);
      rotate_left(grandparent(n));
    }
  }

  extern "C" NOINLINE INTERFACE_ATTRIBUTE
    int hexasan_delete(atreekey key) {
      int iscollison = 0;
#ifdef PRINT_STATICS
      if (getpid() == InitPid) {
        totalAccess = 0, safeAccess = 0, badAccess = 0;
        rbtreeGlobalInsert = 0, rbtreeGlobalDelete = 0;
        rbtreeHeapInsert = 0, rbtreeHeapDelete = 0;
        rbtreeStackInsert = 0, rbtreeStackDelete = 0;
        Inline = 0, Callfun = 0, InlineError = 0;
        CacheHit = 0, CacheMiss = 0, CacheInit = 0, EntryUse = 0;
        OverAccess = 0, TotalCheck = 0;
        rbtreeInsert = 0, rbtreeDelete = 0, rbtreeLookup = 0;
      }
#endif
      if (!key.start) return false;
#ifdef DISABLE_META
      return true;
#endif

#ifdef ENABLEPERRBTREE
      unsigned long idx1 =
        (unsigned long)key.start >> selectedshift & selectedmask;
      rbtree ASanTree = TopRBTree[idx1];
#endif
#ifdef ENABLETHREETREE
      rbtree ASanTree = getTargetRBTree(key.start);
#endif

      node n = ASanTree->root;
      node child;
      atreekey input;
      atreekey input2;

      while (n != NULL) {
        // same
        if ((uptr) key.start == (uptr) n->key.start
            && (uptr) n->key.end == (uptr) key.end) {
          break;
        }
        // overleap
        if ((uptr) key.start <= (uptr) n->key.start
            && (uptr) n->key.end <= (uptr) key.end) {
          iscollison = 3;
          break;
        } else if ((uptr) key.end < (uptr) n->key.start) {
            n = n ->left;
        //right
        } else if ((uptr) n->key.end < (uptr) key.start) {
            n = n->right;
        // cross
        } else {
          if ((uptr) n->key.start < (uptr) key.start
              && (uptr) key.end < (uptr) n->key.end) {
            iscollison = 2;
            input.start = n->key.start;
            input.end = reinterpret_cast<void*>((uptr)key.start - 1);

            input2.start = reinterpret_cast<void*>((uptr)key.end + 1);
            input2.end = n->key.end;
            break;
          } else if ((uptr) n->key.start <= (uptr) key.end
               && (uptr) key.end <= (uptr) n->key.end) {
            iscollison = 1;
            input.start = key.start;
            input.end = reinterpret_cast<void*>((uptr)n->key.start - 1);
            break;
          } else if ((uptr) n->key.start <= (uptr) key.start
                  && (uptr) key.start <= (uptr) n->key.end) {
            iscollison = 1;
            input.start = n->key.start;
            input.end = reinterpret_cast<void*>((uptr)key.start - 1);
            break;
          }
        }
      }

      if (n == NULL) {
        return 0;  /* Key not found, do nothing */
      }

#ifdef ENABLECACHE
      ASanTree->versionNum++;
#endif

      if (n->left != NULL && n->right != NULL) {
        /* Copy key/value from predecessor and then delete it instead */
        node pred = maximum_node(n->left);
        n->key = pred->key;
        n = pred;
      }

      assert(n->left == NULL || n->right == NULL);
      child = n->right == NULL ? n->left : n->right;
      if (node_color(n) == BLACK) {
        n->color = node_color(child);
        delete_case1(n);
      }

      if (n->parent == NULL) {
        ASanTree->root = child;
      } else {
        if (n == n->parent->left)
          n->parent->left = child;
        else
          n->parent->right = child;
      }
      if (child != NULL) {
        child->parent = n->parent;
      }

      if (n->parent == NULL && child != NULL) // root should be black
        child->color = BLACK;

      __libc_free(n);
      if (iscollison == 1)
        hexasan_insert(input);

      if (iscollison == 2) {
        hexasan_insert(input);
        hexasan_insert(input2);
      }

      if (iscollison > 0) {
        hexasan_delete(key);
      }

      return 1;
    }

  static node maximum_node(node n) {
    assert(n != NULL);
    while (n->right != NULL) {
      n = n->right;
    }
    return n;
  }

  void delete_case1(node n) {
    if (n->parent == NULL)
      return;
    else
      delete_case2(n);
  }

  void delete_case2(node n) {
    if (node_color(sibling(n)) == RED) {
      n->parent->color = RED;
      sibling(n)->color = BLACK;
      if (n == n->parent->left)
        rotate_left(n->parent);
      else
        rotate_right(n->parent);
    }
    delete_case3(n);
  }

  void delete_case3(node n) {
    if (node_color(n->parent) == BLACK &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK) {
      sibling(n)->color = RED;
      delete_case1(n->parent);
    } else
      delete_case4(n);
  }

  void delete_case4(node n) {
    if (node_color(n->parent) == RED &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK) {
      sibling(n)->color = RED;
      n->parent->color = BLACK;
    } else
      delete_case5(n);
  }

  void delete_case5(node n) {
    if (n == n->parent->left &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == RED &&
        node_color(sibling(n)->right) == BLACK) {
      sibling(n)->color = RED;
      sibling(n)->left->color = BLACK;
      rotate_right(sibling(n));
    } else if (n == n->parent->right &&
               node_color(sibling(n)) == BLACK &&
               node_color(sibling(n)->right) == RED &&
               node_color(sibling(n)->left) == BLACK) {
      sibling(n)->color = RED;
      sibling(n)->right->color = BLACK;
      rotate_left(sibling(n));
    }
    delete_case6(n);
  }

  void delete_case6(node n) {
    sibling(n)->color = node_color(n->parent);
    n->parent->color = BLACK;
    if (n == n->parent->left) {
      assert(node_color(sibling(n)->right) == RED);
      sibling(n)->right->color = BLACK;
      rotate_left(n->parent);
    } else {
      assert(node_color(sibling(n)->left) == RED);
      sibling(n)->left->color = BLACK;
      rotate_right(n->parent);
    }
  }
} // namespace __asan

using namespace __asan;

inline void *calc_address(const void *add, int changeVal) {
  uptr p = reinterpret_cast<uptr>(add);
  uptr realAddressP = p + changeVal;
  return reinterpret_cast<void *>(realAddressP);
}

int hexasan_range_check(const void *ptr, int size) {
  if (!checkStart || size <= 0) return 0;
#ifdef PRINT_STATICS
  totalAccess++;
  rbtreeLookup++;
  if (getpid() == InitPid) {
    totalAccess = 0, safeAccess = 0, badAccess = 0;
    rbtreeGlobalInsert = 0, rbtreeGlobalDelete = 0;
    rbtreeHeapInsert = 0, rbtreeHeapDelete = 0;
    rbtreeStackInsert = 0, rbtreeStackDelete = 0;
    Inline = 0, Callfun = 0, InlineError = 0;
    CacheHit = 0, CacheMiss = 0, CacheInit = 0, EntryUse = 0;
    OverAccess = 0, TotalCheck = 0;
    rbtreeInsert = 0, rbtreeDelete = 0, rbtreeLookup = 0;
  }
#endif

#ifdef ENABLEPERRBTREE
  rbtree ASanTree = nullptr;
  unsigned long idx1 =
    (unsigned long) ptr >> selectedshift & selectedmask;
  ASanTree = TopRBTree[idx1];
#endif
#ifdef ENABLETHREETREE
  ASanTree = getTargetRBTree((void *)ptr);
#endif

  if (!ASanTree || !ASanTree->root)
    return 0;

  uptr start = (uptr) ptr;
  uptr end = (uptr) calc_address(ptr, size-1);

#ifdef ENABLECACHE
  int MapIndex = 0;
  MapIndex = (((start >> 32) ^ (start)) & 0xffff);
  // search cachemap first
  if (HashMapCache[MapIndex].availability &&
      (uptr) HashMapCache[MapIndex].address == start &&
      (uptr) HashMapCache[MapIndex].end == end &&
      HashMapCache[MapIndex].version == ASanTree->versionNum) {
    return;
  }
#endif

  node nTemp = ASanTree->root;
  while (nTemp) {
    if ((uptr) nTemp->key.end < (uptr) ptr) {
      nTemp = nTemp->right;
    } else if ((uptr) nTemp->key.start > end) {
      nTemp = nTemp->left;
    } else if (((start <= (uptr) nTemp->key.start) &&
                (((uptr) nTemp->key.end) <= end)) ||
               (((uptr) nTemp->key.start <= start) &&
                (start <= (uptr) nTemp->key.end)) ||
               (((uptr) nTemp->key.start <= end) &&
                (end <= (uptr) nTemp->key.end))) {
#ifdef PRINT_CHECK_RESULT
      Printf("\tMemory safety violation: %p %p %d\n", ptr, end, size);
#endif
#ifdef TERMINATE_PROGRAM
      Die();
#endif
      return 1;
    }
  }

#ifdef ENABLECACHE
  HashMapCache[MapIndex].availability = true;
  HashMapCache[MapIndex].address = (void *) start;
  HashMapCache[MapIndex].end = (void *) end;
  HashMapCache[MapIndex].version = ASanTree->versionNum;
#endif

  return 0;
}
#endif
