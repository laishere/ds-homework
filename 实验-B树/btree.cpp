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
    t->children[0] = t->children[1] = NULL; // keyNum֮ǰ����ΪNULL��ȷ���������Ұָ��
    return t;
}

static int search(BTree t, KeyType key)
{
    int i = 1;
    while (i <= t->keyNum && t->records[i].key < key) i++; // �ҵ�һ��i��ʹ��keys[i] >= key
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
    int mid = (BTREE_M + 1) >> 1; // m / 2 ��ȡ��
    record = t->records[mid];
    
    // ����һ������
    ch = (BTree)malloc(sizeof(BNode));
    ch->parent = NULL;
    ch->keyNum = t->keyNum - mid;
    memcpy(ch->records + 1, t->records + mid + 1, sizeof(Record) * ch->keyNum);
    for (int i = 0; i <= ch->keyNum; i++)
        set_child(ch, i, t->children[mid + i]); // ��Ҫֱ�Ӹ����ڴ棬��Ϊ��Ҫ�޸�parent����

    // ԭ�ڵ�ؼ�����������
    t->keyNum = mid - 1;
}

static void remove(BTree t, int i);

static void restore(BTree cur, int i)
{
    BTree p = cur->parent;
    if (!p) {
        // tΪ���ڵ㣬����Ҫ�����Ļ��϶����ն˽ڵ㣬�ն˸��ڵ㲻��Ҫ����
        return;
    }
    int j = 0;
    while (p->children[j] != cur) j++;
    if (j - 1 >= 0 && p->children[j - 1]->keyNum > (BTREE_M - 1) / 2) {
        // ���ֵ��ж���ļ�¼
        BTree left = p->children[j - 1];
        insert(cur, 1, cur->children[0], p->records[j]); // ����ʱ��ch[0] Ӧ���Ƶ� ch[1]λ��
        set_child(cur, 0, left->children[left->keyNum]); // ���ֵܵ����һ�������Ƶ�ch[0]λ��
        p->records[j] = left->records[left->keyNum]; // ���ֵܵ����һ����¼�Ƶ�parent��¼�ĵ�jλ��
        remove_record(left, left->keyNum);
        return;
    }
    if (j + 1 <= p->keyNum && p->children[j + 1]->keyNum > (BTREE_M - 1) / 2) {
        // ���ֵ��ж���ļ�¼
        BTree right = p->children[j + 1];
        insert(cur, cur->keyNum + 1, right->children[0], p->records[j + 1]);
        p->records[j + 1] = right->records[1];
        set_child(right, 0, right->children[1]); // ���ֵܵ�ch[1]��Ҫ�Ƶ�ch[0]����Ϊɾ��ʱ���ch[1]ɾ����������Ҫ��ɾ��ǰ����
        remove_record(right, 1);
        return;
    }
    BTree left, right;
    int mid;
    if (j - 1 >= 0) {
        // ���ֵܴ���
        left = p->children[j - 1];
        right = cur; // ��ǰ�ڵ����ұ�
        mid = j; // ����֮��parent��¼�� j
    }
    else {
        // ���ֵܱض�����
        left = cur; // ��ǰ�ڵ�����
        right = p->children[j + 1];
        mid = j + 1;
    }
    // left��parent��mid��¼��right�ϲ���left�ڵ�
    int &len = left->keyNum;
    len++;
    left->records[len] = p->records[mid]; // parent��mid��¼�ƶ���leftĩβ
    set_child(left, len, right->children[0]); // right�ĵ�һ�������ƶ���leftĩβ
    for (int k = 1; k <= right->keyNum; k++) {
        len++;
        left->records[len] = right->records[k]; // ����right�ļ�¼��leftĩβ
        set_child(left, len, right->children[k]); // ����right��children��leftĩβ
    }
    free(right); // right�е���Ϣ�Ѿ����ϲ���left�У���Ҫ����right����Ҫע����ǣ���rightΪ���ڵ�ʱ��Ҫ�Ƴ�
    p->children[mid] = NULL; // ��һ��ɾ��p��mid����Ҫ��ch[mid]��ΪNULL����ֹ���ӽ�Ԫ��
    int isRoot = !p->parent; // �ȱ��棬��Ϊ�����remove���ܻᵼ��p���ͷ�
    remove(p, mid); // �ݹ��Ƴ�parent�������ӽڵ�ϲ���K
    if (isRoot && p->keyNum == 0) { // ���isRoot����ôp���ᱻ�ͷţ����ǿ��԰�ȫ��������ڴ�
        // pΪ���ڵ���p�ļ�¼�ѱ��ϲ���left�У���p����ϢΪ�գ���ô����left����Ϣ��p������������ڵ㣩
        *p = *left;
        for (int i = 0; i <= p->keyNum; i++) {
            BTree ch = p->children[i];
            if (ch)
                ch->parent = p;
        }
        p->parent = NULL;
        free(left); // ����left
    }
}

static void remove(BTree t, int i)
{
    if (t->children[i]) {
        // �����²���С�ؼ����滻Ҫɾ����¼��Ȼ��ת��Ϊɾ���滻�ڵ����С��¼λ��
        BTree ch = t->children[i];
        while (ch->children[0]) ch = ch->children[0];
        t->records[i] = ch->records[1];
        remove(ch, 1);
        return;
    }

    // ɾ��i��¼
    remove_record(t, i);

    if (t->keyNum < (BTREE_M - 1) / 2)
        restore(t, i);
}

void btree_insert(BTree &root, Record record)
{
    if (!root) {
        root = new_node(NULL, record); // ���ڵ�Ϊ�գ�����һ������
        return;
    }
    Result ret = btree_search(root, record.key);
    if (ret.found) return; // �Ѿ�����
    BTree p = ret.node;
    BTree ch = NULL;
    int i = ret.index;
    while (1) {
        insert(p, i, ch, record);
        if (p->keyNum < BTREE_M) break; // ����Ҫ����
        split(p, ch, record); // ����
        if (p == root) {
            root = new_node(NULL, record);
            set_child(root, 0, p);
            set_child(root, 1, ch);
            break; // �������µĸ�
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
            return {t, i, FOUND}; // �ҵ�
        if (!t->children[i - 1])
            return {t, i, NOT_FOUND};
        t = t->children[i - 1];
    }
    return {NULL, -1, NOT_FOUND}; // ����ֱ�ӷ���NULL
}

void btree_remove(BTree root, KeyType key)
{
    Result ret = btree_search(root, key);
    if (!ret.found) return;
    remove(ret.node, ret.index);
}
