#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "btree.h"
#include <assert.h>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

static void skip_whitespace(const char *&s)
{
    while (*s == ' ') s++;
}

static BTree next_node(const char *&s)
{
    BTree t = (BTree)malloc(sizeof(BNode));
    t->keyNum = 0;
    t->parent = NULL;
    t->children[0] = NULL;
    skip_whitespace(s);
    assert(*s == '[');
    s++;
    while (1) {
        skip_whitespace(s);
        if (*s >= '0' && *s <= '9') {
            int d = 0;
            while (*s >= '0' && *s <= '9') {
                d *= 10;
                d += *s - '0';
                s++;
            }
            assert(++t->keyNum < BTREE_M);
            t->records[t->keyNum] = {d, d};
            t->children[t->keyNum] = NULL;
            skip_whitespace(s);
            assert(*s == ',' || *s == ']');
            if (*s == ',') s++;
        }
        else break;
    }
    assert(*s == ']');
    s++;
    return t;
}

static BTree next_tree(const char *&s)
{
    BTree t = next_node(s);
    skip_whitespace(s);
    assert(*s == '(' || *s == ',' || *s == '\0' || *s == ')');
    if (*s == ',' || *s == '\0' || *s == ')') return t;
    int i = 0;
    s++;
    while (*s) {
        assert(i <= t->keyNum);
        t->children[i] = next_tree(s);
        t->children[i]->parent = t;
        i++;
        skip_whitespace(s);
        assert(*s == ',' || *s == ')');
        if (*s == ',') s++;
        else break;
    }
    assert(*s == ')');
    s++;
    return t;
}

/**
 * @brief ���ַ���ת��ΪB�����ַ���������Ҫ��ʾһ�źϷ���B����
 *
 * ���磺[2]([0, 1], [3, 4])��
 * [����, ...] ��ʾһ���ڵ㡣
 * A(B, ...) ��ʾB��A��һ������
 *
 * @param s �ַ���
 * @return BTree
 */
BTree load_tree(const char *s)
{
    return next_tree(s);
}

static void dump_node(BTree t) {
    if (!t) return;
    printf("[");
    for (int i = 1; i <= t->keyNum; i++) {
        if (i > 1) printf(", ");
        printf("%d", t->records[i].key);
    }
    printf("]");
}

static void dump(BTree t) {
    dump_node(t);
    if (!t->children[0]) return;
    printf("(");
    for (int i = 0; i <= t->keyNum; i++) {
        if (i > 0) printf(", ");
        dump(t->children[i]);
    }
    printf(")");
}

/**
 * @brief ��B�����л����ַ���
 *
 * @param t B��
 */
void dump_tree(BTree t)
{
    dump(t);
    printf("\n");
}

static void check_error(BTree t, const char *msg) {
    printf("[���Ϸ�] ");
    dump_node(t);
    printf(" %s\n", msg);
}

static int check(BTree t, int &mink, int &maxk) {
    if (t->keyNum >= BTREE_M) {
        check_error(t, "keyNumӦС��M");
        return false;
    }
    if (t->keyNum < BTREE_M / 2 - 1 && t->parent) {
        check_error(t, "�Ǹ��ڵ��keyNumӦ���ڵ���M / 2 - 1");
        return false;
    }
    if (!t->keyNum) return true;
    mink = maxk = t->records[1].key;
    for (int i = 2; i <= t->keyNum; i++) {
        if (t->records[i - 1].key >= t->records[i].key) {
            check_error(t, "�ؼ���Ӧ���ϸ����");
            return false;
        }
        mink = min(mink, t->records[i].key);
        maxk = max(maxk, t->records[i].key);
    }
    if (!t->children[0]) return true; // �ն˽ڵ�
    for (int i = 0; i <= t->keyNum; i++) {
        auto ch = t->children[i];
        if (!ch) {
            check_error(t, "�����������������keyNum + 1");
            return false;
        }
        if (ch->parent != t) {
            check_error(t, "�����ĸ�ָ����Ϣ����ȷ");
            return false;
        }
        int ch_mink, ch_maxk;
        if (!check(ch, ch_mink, ch_maxk)) return false;
        if ((i >= 1 && ch_mink <= t->records[i].key) || (i + 1 <= t->keyNum && ch_maxk >= t->records[i + 1].key)) {
            check_error(ch, "������ֵ��parent�Ĵ�С��ϵ���Ϸ�");
            return false;
        }
        mink = min(mink, ch_mink);
        maxk = max(maxk, ch_maxk);
    }
    return true;
}

/**
 * @brief ���һ��B���Ƿ�Ϸ�
 *
 * @param t B��
 * @return int
 */
int check_is_legal(BTree t)
{
    if (!t) return true;
    int mink, maxk;
    if (!check(t, mink, maxk)) return false;
    // ���ÿ���ն˽ڵ��Ƿ���ͬһ��
    vector<BTree> q;
    q.push_back(t);
    int i = 0;
    while (i < q.size() && q[i]->children[0]) {
        int len = q.size();
        for (; i < len; i++) {
            BTree t = q[i];
            for (int j = 0; j <= t->keyNum; j++)
                q.push_back(t->children[j]);
        }
    }
    for (; i < q.size(); i++) 
        if (q[i]->children[0]) {
            check_error(t, "�����ն˽ڵ�Ӧ��ͬһ��");
            return false;
        }
    return true;
}

void print_option(const char *cmd, const char *desc)
{
    printf("%-20s\t%s\n", cmd, desc);
}

void print_help()
{
    print_option("help", "����˰���");
    print_option("load\t�ַ���", "���ַ�������һ��B��");
    print_option("dump", "���л���ǰB�������Ϊ�ַ���");
    print_option("check", "��鵱ǰB���Ƿ�Ϸ�");
    print_option("insert\t����", "����һ����¼����ǰB��");
    print_option("search\t����", "�ڵ�ǰB����������Ӧ��¼");
    print_option("remove\t����", "ɾ����ǰB���еĶ�Ӧ��¼");
    print_option("auto", "�����Զ�����");
    print_option("exit", "�˳��˳���");
}

void command_begin()
{
    printf("\n> ");
}

void error(const char *msg)
{
    printf("[����] %s\n", msg);
}

void free_tree(BTree &t) {
    if (t) {
        free(t);
        t = NULL;
    }
}

int main()
{
    printf("��ӭʹ��B�����Գ��� by lgz��\n");
    printf("֧�����������ʽ:\n\n");
    print_help();
    char line[200];
    char cmd[100];
    char tail[100];
    BTree tree = NULL;
    while (1) {
        command_begin();
        gets_s(line);
        sscanf(line, "%s", cmd);
        sprintf(tail, "%s", line + strlen(cmd));
        if (strcmp("help", cmd) == 0) {
            print_help();
        }
        else if (strcmp("load", cmd) == 0) {
            free_tree(tree);
            tree = load_tree(tail);
        }
        else if (strcmp("dump", cmd) == 0) {
            if (!tree) 
                printf("��ǰB��Ϊ��\n");
            else dump_tree(tree);
        }
        else if (strcmp("check", cmd) == 0) {
            if (!tree)
                printf("��ǰB��Ϊ��\n");
            else if (check_is_legal(tree))
                printf("B���Ϸ�!\n");
        }
        else if (strcmp("insert", cmd) == 0) {
            int d;
            sscanf(tail, "%d", &d);
            btree_insert(tree, {d, d});
        }
        else if (strcmp("search", cmd) == 0) {
            if (!tree)
                printf("��ǰB��Ϊ��\n");
            else {
                int d;
                sscanf(tail, "%d", &d);
                Result ret = btree_search(tree, d);
                if (ret.found) 
                    printf("[�ҵ�] ");
                else
                    printf("[����λ��] ");
                dump_node(ret.node);
                printf(" �±�: %d\n" , ret.index);
            }
        }
        else if (strcmp("remove", cmd) == 0) {
            if (!tree)
                printf("��ǰB��Ϊ��\n");
            else {
                int d;
                sscanf(tail, "%d", &d);
                btree_remove(tree, d);
            }
        }
        else if (strcmp("auto", cmd) == 0) {
            int legal = 1;
            vector<int> insert;
            vector<pair<int, int>> remove;
            for (int i = 0; i < 10000; i++) {
                int k = rand();
                insert.push_back(k);
                int order = rand();
                remove.push_back({order, k});
            }
            for (auto k : insert) {
                btree_insert(tree, {k, k});
                legal = check_is_legal(tree);
                if (!legal) break;
            }
            sort(remove.begin(), remove.end()); // �������order����
            for (auto p : remove) {
                btree_remove(tree, p.second);
                legal = check_is_legal(tree);
                if (!legal) break;
            }
            if (legal)
                printf("\n[�ɹ�] �Զ������ɾ���Լ�ÿ�β����ĺϷ��Բ������! \n");
        }
        else if (strcmp("exit", cmd) == 0) {
            printf("Bye~");
            return 0;
        }
        else error("��֧�ֵ��������help�������");
    }
    return 0;
}