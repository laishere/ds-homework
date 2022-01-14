#if !defined(BTREE_H)
#define BTREE_H

#define BTREE_M 3
#define FOUND 1
#define NOT_FOUND 0

typedef int KeyType;
typedef int ValueType;

typedef struct Record
{
    KeyType key;
    ValueType value;
} Record;

typedef struct BNode
{
    int keyNum;
    struct BNode *parent;
    struct BNode *children[BTREE_M + 1];
    Record records[BTREE_M + 1]; // 0位置不使用
} BNode, *BTree;

typedef struct Result
{
    BTree node;
    int index;
    int found;
} Result;


void btree_insert(BTree &root, Record record);
Result btree_search(BTree root, KeyType key);
void btree_remove(BTree root, KeyType key);

#endif // BTREE_H
