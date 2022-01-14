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
 * @brief 把字符串转换为B树（字符串本身需要表示一颗合法的B树）
 *
 * 形如：[2]([0, 1], [3, 4])。
 * [数字, ...] 表示一个节点。
 * A(B, ...) 表示B是A的一个孩子
 *
 * @param s 字符串
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
 * @brief 把B树序列化成字符串
 *
 * @param t B树
 */
void dump_tree(BTree t)
{
    dump(t);
    printf("\n");
}

static void check_error(BTree t, const char *msg) {
    printf("[不合法] ");
    dump_node(t);
    printf(" %s\n", msg);
}

static int check(BTree t, int &mink, int &maxk) {
    if (t->keyNum >= BTREE_M) {
        check_error(t, "keyNum应小于M");
        return false;
    }
    if (t->keyNum < BTREE_M / 2 - 1 && t->parent) {
        check_error(t, "非根节点的keyNum应大于等于M / 2 - 1");
        return false;
    }
    if (!t->keyNum) return true;
    mink = maxk = t->records[1].key;
    for (int i = 2; i <= t->keyNum; i++) {
        if (t->records[i - 1].key >= t->records[i].key) {
            check_error(t, "关键字应该严格递增");
            return false;
        }
        mink = min(mink, t->records[i].key);
        maxk = max(maxk, t->records[i].key);
    }
    if (!t->children[0]) return true; // 终端节点
    for (int i = 0; i <= t->keyNum; i++) {
        auto ch = t->children[i];
        if (!ch) {
            check_error(t, "子树的数量必须等于keyNum + 1");
            return false;
        }
        if (ch->parent != t) {
            check_error(t, "子树的父指针信息不正确");
            return false;
        }
        int ch_mink, ch_maxk;
        if (!check(ch, ch_mink, ch_maxk)) return false;
        if ((i >= 1 && ch_mink <= t->records[i].key) || (i + 1 <= t->keyNum && ch_maxk >= t->records[i + 1].key)) {
            check_error(ch, "子树键值与parent的大小关系不合法");
            return false;
        }
        mink = min(mink, ch_mink);
        maxk = max(maxk, ch_maxk);
    }
    return true;
}

/**
 * @brief 检查一颗B树是否合法
 *
 * @param t B树
 * @return int
 */
int check_is_legal(BTree t)
{
    if (!t) return true;
    int mink, maxk;
    if (!check(t, mink, maxk)) return false;
    // 检查每个终端节点是否在同一层
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
            check_error(t, "所有终端节点应在同一层");
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
    print_option("help", "输出此帮助");
    print_option("load\t字符串", "从字符串构造一颗B树");
    print_option("dump", "序列化当前B树并输出为字符串");
    print_option("check", "检查当前B树是否合法");
    print_option("insert\t数字", "插入一个记录到当前B树");
    print_option("search\t数字", "在当前B树中搜索对应记录");
    print_option("remove\t数字", "删除当前B树中的对应记录");
    print_option("auto", "运行自动测试");
    print_option("exit", "退出此程序");
}

void command_begin()
{
    printf("\n> ");
}

void error(const char *msg)
{
    printf("[错误] %s\n", msg);
}

void free_tree(BTree &t) {
    if (t) {
        free(t);
        t = NULL;
    }
}

int main()
{
    printf("欢迎使用B树测试程序 by lgz。\n");
    printf("支持以下命令格式:\n\n");
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
                printf("当前B树为空\n");
            else dump_tree(tree);
        }
        else if (strcmp("check", cmd) == 0) {
            if (!tree)
                printf("当前B树为空\n");
            else if (check_is_legal(tree))
                printf("B树合法!\n");
        }
        else if (strcmp("insert", cmd) == 0) {
            int d;
            sscanf(tail, "%d", &d);
            btree_insert(tree, {d, d});
        }
        else if (strcmp("search", cmd) == 0) {
            if (!tree)
                printf("当前B树为空\n");
            else {
                int d;
                sscanf(tail, "%d", &d);
                Result ret = btree_search(tree, d);
                if (ret.found) 
                    printf("[找到] ");
                else
                    printf("[插入位置] ");
                dump_node(ret.node);
                printf(" 下标: %d\n" , ret.index);
            }
        }
        else if (strcmp("remove", cmd) == 0) {
            if (!tree)
                printf("当前B树为空\n");
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
            sort(remove.begin(), remove.end()); // 根据随机order排序
            for (auto p : remove) {
                btree_remove(tree, p.second);
                legal = check_is_legal(tree);
                if (!legal) break;
            }
            if (legal)
                printf("\n[成功] 自动插入和删除以及每次操作的合法性测试完成! \n");
        }
        else if (strcmp("exit", cmd) == 0) {
            printf("Bye~");
            return 0;
        }
        else error("不支持的命令，输入help输出帮助");
    }
    return 0;
}