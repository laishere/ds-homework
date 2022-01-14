#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static BTree new_node(BTree p, Record record)
{
    BTree t = (BTree)malloc(sizeof(BNode));
    t->keyNum = 1;
    t->records[1] = record;
    t->parent = p;
    t->children[0] = t->children[1] = NULL; // keyNum之前都置为NULL，确保不会出现野指针
    return t;
}

static int search(BTree t, KeyType key)
{
    int i = 1;
    while (i <= t->keyNum && t->records[i].key < key) i++; // 找到一个i，使得keys[i] >= key
    return i;
}

static void remove_record(BTree t, int i) {
    t->keyNum--;
    for (int j = i; j <= t->keyNum; j++) {
        t->records[j] = t->records[j + 1];
        t->children[j] = t->children[j + 1];
    }
}

static void set_child(BTree p, int i, BTree ch) {
    p->children[i] = ch;
    if (ch) ch->parent = p;
}

static void insert(BTree t, int i, BTree ch, Record record)
{
    t->keyNum++;
    for (int j = t->keyNum; j > i; j--) {
        t->records[j] = t->records[j - 1];
        t->children[j] = t->children[j - 1];
    }
    t->records[i] = record;
    set_child(t, i, ch);
}

static void split(BTree t, BTree &ch, Record &record) 
{
    int mid = (BTREE_M + 1) >> 1; // m / 2 上取整
    record = t->records[mid];
    
    // 分裂一颗新树
    ch = (BTree)malloc(sizeof(BNode));
    ch->parent = NULL;
    ch->keyNum = t->keyNum - mid;
    memcpy(ch->records + 1, t->records + mid + 1, sizeof(Record) * ch->keyNum);
    for (int i = 0; i <= ch->keyNum; i++)
        set_child(ch, i, t->children[mid + i]); // 不要直接复制内存，因为需要修改parent属性

    // 原节点关键字数量减少
    t->keyNum = mid - 1;
}

static void remove(BTree t, int i);

static void restore(BTree cur, int i)
{
    BTree p = cur->parent;
    if (!p) {
        // t为根节点，且需要调整的话肯定是终端节点，终端根节点不需要调整
        return;
    }
    int j = 0;
    while (p->children[j] != cur) j++;
    if (j - 1 >= 0 && p->children[j - 1]->keyNum > (BTREE_M - 1) / 2) {
        // 左兄弟有多余的记录
        BTree left = p->children[j - 1];
        insert(cur, 1, cur->children[0], p->records[j]); // 插入时，ch[0] 应该移到 ch[1]位置
        set_child(cur, 0, left->children[left->keyNum]); // 左兄弟的最后一个孩子移到ch[0]位置
        p->records[j] = left->records[left->keyNum]; // 左兄弟的最后一个记录移到parent记录的第j位置
        remove_record(left, left->keyNum);
        return;
    }
    if (j + 1 <= p->keyNum && p->children[j + 1]->keyNum > (BTREE_M - 1) / 2) {
        // 右兄弟有多余的记录
        BTree right = p->children[j + 1];
        insert(cur, cur->keyNum + 1, right->children[0], p->records[j + 1]);
        p->records[j + 1] = right->records[1];
        set_child(right, 0, right->children[1]); // 右兄弟的ch[1]需要移到ch[0]，因为删除时会把ch[1]删除，所以需要在删除前操作
        remove_record(right, 1);
        return;
    }
    BTree left, right;
    int mid;
    if (j - 1 >= 0) {
        // 左兄弟存在
        left = p->children[j - 1];
        right = cur; // 当前节点在右边
        mid = j; // 两者之间parent记录是 j
    }
    else {
        // 右兄弟必定存在
        left = cur; // 当前节点在左
        right = p->children[j + 1];
        mid = j + 1;
    }
    // left、parent的mid记录、right合并到left节点
    int &len = left->keyNum;
    len++;
    left->records[len] = p->records[mid]; // parent的mid记录移动到left末尾
    set_child(left, len, right->children[0]); // right的第一个孩子移动到left末尾
    for (int k = 1; k <= right->keyNum; k++) {
        len++;
        left->records[len] = right->records[k]; // 复制right的记录到left末尾
        set_child(left, len, right->children[k]); // 复制right的children到left末尾
    }
    free(right); // right中的信息已经被合并到left中，需要回收right，需要注意的是，当right为根节点时不要移除
    p->children[mid] = NULL; // 下一步删除p的mid，需要把ch[mid]置为NULL，禁止从子借元素
    int isRoot = !p->parent; // 先保存，因为下面的remove可能会导致p被释放
    remove(p, mid); // 递归移除parent中已与子节点合并的K
    if (isRoot && p->keyNum == 0) { // 如果isRoot，那么p不会被释放，我们可以安全访问这个内存
        // p为根节点且p的记录已被合并到left中，而p的信息为空，那么复制left的信息到p（避免更换根节点）
        *p = *left;
        for (int i = 0; i <= p->keyNum; i++) {
            BTree ch = p->children[i];
            if (ch)
                ch->parent = p;
        }
        p->parent = NULL;
        free(left); // 回收left
    }
}

static void remove(BTree t, int i)
{
    if (t->children[i]) {
        // 用最下层最小关键字替换要删除记录，然后转换为删除替换节点的最小记录位置
        BTree ch = t->children[i];
        while (ch->children[0]) ch = ch->children[0];
        t->records[i] = ch->records[1];
        remove(ch, 1);
        return;
    }

    // 删除i记录
    remove_record(t, i);

    if (t->keyNum < (BTREE_M - 1) / 2)
        restore(t, i);
}

void btree_insert(BTree &root, Record record)
{
    if (!root) {
        root = new_node(NULL, record); // 根节点为空，构造一颗新树
        return;
    }
    Result ret = btree_search(root, record.key);
    if (ret.found) return; // 已经存在
    BTree p = ret.node;
    BTree ch = NULL;
    int i = ret.index;
    while (1) {
        insert(p, i, ch, record);
        if (p->keyNum < BTREE_M) break; // 不需要分裂
        split(p, ch, record); // 分裂
        if (p == root) {
            root = new_node(NULL, record);
            set_child(root, 0, p);
            set_child(root, 1, ch);
            break; // 产生了新的根
        }
        p = p->parent;
        i = search(p, record.key);
    }
}

Result btree_search(BTree root, KeyType key)
{
    BTree t = root;
    while (t) {
        int i = search(t, key);
        if (i <= t->keyNum && t->records[i].key == key)
            return {t, i, FOUND}; // 找到
        if (!t->children[i - 1])
            return {t, i, NOT_FOUND};
        t = t->children[i - 1];
    }
    return {NULL, -1, NOT_FOUND}; // 空树直接返回NULL
}

void btree_remove(BTree root, KeyType key)
{
    Result ret = btree_search(root, key);
    if (!ret.found) return;
    remove(ret.node, ret.index);
}
