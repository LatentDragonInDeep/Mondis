//
// Created by Yyvv on 2019/4/12.
//

#ifndef MONDIS_AVLTREE_H
#define MONDIS_AVLTREE_H

#include "MondisObject.h"
#include "Command.h"
#include <string>
#include <stack>

class KeyValue :public MondisData{
public:
    string key = "";
    MondisObject* value = nullptr;

    KeyValue(string &k, MondisObject *v) : key(k), value(v) {}
    KeyValue(){};
    KeyValue(KeyValue&& other) = default;
    KeyValue(KeyValue& other):key(other.key),value(other.value) {
        other.key = "";
        other.value = nullptr;
    }
    bool compare(KeyValue& other) {
        return key.compare(other.key);
    }

    bool equals(KeyValue& other) {
        return key == other.key;
    }
    ~KeyValue() {
        delete value;
    }

    void toJson() {
        json = "";
        json+="\"";
        json += key;
        json+="\"";
        if (value == nullptr) {
            json += ":";
            json += value->getJson();
        }
    }

    KeyValue &operator=(KeyValue &other) {
        key = other.key;
        value = other.value;
        other.key = "";
        other.value = nullptr;
        return *this;
    };

    KeyValue &operator=(KeyValue &&other) {
        operator=(other);
        return *this;
    }
};

class AVLTreeNode {
public:
    AVLTreeNode* left = nullptr;
    AVLTreeNode* right = nullptr;
    AVLTreeNode* parent = nullptr;
    KeyValue* data = nullptr;
    int height = 1;
    ~AVLTreeNode() {
        delete data;
    }
};

class AVLTree:public MondisData
{
private:
    AVLTreeNode *root = nullptr;
    unsigned _size = 0;
public:
    class AVLIterator {
    private:
        AVLTreeNode *cur = nullptr;
        stack<AVLTreeNode*> s;
        void dfs(AVLTreeNode* cur) {
            while (cur!= nullptr) {
                s.push(cur);
                cur=cur->left;
            }
        }
    public:
        AVLIterator() {};

        AVLIterator(AVLTree *avlTree) : cur(avlTree->root)
        {
            dfs(cur);
        }
        KeyValue* operator->() {
            return cur->data;
        };

        bool next() {
            if(s.empty())
            {
                return false;
            }
            cur = s.top();
            s.pop();
            dfs(cur->right);
            return true;
        }
        KeyValue* getData() {
            return cur->data;
        }
    };
public:

    void insert(KeyValue *kv);
    void insert(string &key, MondisObject *value);
    void remove(string &key);
    MondisObject* get(string &key);
    bool containsKey(string& key);
    AVLIterator iterator();
    ~AVLTree();

    ExecRes execute(Command *command);

    MondisObject *locate(Command *command);
    unsigned size();

    bool hasModified();
private:
    void realInsert(KeyValue *kv);

    void realRemove(KeyValue &kv, AVLTreeNode *root);
    AVLTreeNode* getSuccessor(AVLTreeNode* root);

    void leftRotate(AVLTreeNode *root);

    void rightRotate(AVLTreeNode *root);

    void leftRightRotate(AVLTreeNode *root);

    void rightLeftRotate(AVLTreeNode *root);
    int getHeight(AVLTreeNode* root);
    void deleteTree(AVLTreeNode* root);

    void rebalance(AVLTreeNode *root);

    void toJson();
};

#endif //MONDIS_AVLTREE_H
