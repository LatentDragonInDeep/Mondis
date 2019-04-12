//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_HASHMAP_H
#define MONDIS_HASHMAP_H

#include <stack>
#include <mutex>
#include <shared_mutex>

#include "MondisObject.h"
#include "Command.h"

using namespace std;

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

class Entry:public MondisData{
public:
    string key;
    MondisObject* object = nullptr;
    Entry* pre = nullptr;
    Entry* next = nullptr;

    Entry(string &key, MondisObject *data) : key(key), object(data) {};
    Entry(){};
    Entry(Entry&other) {
        key = other.key;
        object = other.object;
        other.key = "";
        other.object = nullptr;
    }
    ~Entry (){
        delete object;
    }

    void toJson() {
        json = "";
        json+="\"";
        json += key;
        json+="\"";
        if (object != nullptr) {
            json += " : ";
            json += object->getJson();
        }
    }
    KeyValue* toKeyValue() {
        KeyValue* res = new KeyValue(key,object);
        key = "";
        object = nullptr;
        return res;
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
        AVLTreeNode* operator->() {
            return cur;
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
    };
public:

    void insert(KeyValue *kv);

    void insert(string &key, MondisObject *value);

    void remove(string &key);
    KeyValue* get(string& key);
    MondisObject* getValue(string& key);
    bool containsKey(string& key);
    AVLIterator iterator();
    ~AVLTree();

    ExecRes execute(Command *command);

    MondisObject *locate(Command *command);
    unsigned size();

    bool isModified();
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

class HashMap:public MondisData
{
private:
    float loadFactor = 0.75f;
    unsigned int capacity = 16;
    unsigned int _size = 0;
    const int treeThreshold = 8;
    const bool isValueNull = false;

    struct Content {
        Entry *head = new Entry;
        Entry *tail = new Entry;
        AVLTree *tree = nullptr;
        int listLen = 0;
        bool isList = true;

        Content() {
            head->next = tail;
            tail->pre = head;
        }

        void reset() {
            if (isList) {
                for(Entry *cur = head->next;cur!=tail;cur = cur->next) {
                    Entry* next = cur->next;
                    delete cur;
                    cur = next;
                }
                head->next = tail;
                tail->pre = head;
            } else {
                delete tree;
            }
            isList = true;
            listLen = 0;
        }

        ~Content() {
            delete head;
            delete tail;
            delete tree;
        }
    };
    Content * arrayFrom = nullptr;
    Content * arrayTo = nullptr;
    mutex* mutexes = nullptr;
    shared_mutex globalMutex;
    class MapIterator{
    private:
        Entry* cur = nullptr;
        Content * array;
        unsigned slotIndex = 0;
        AVLTree::AVLIterator avlIterator;
        bool isTree = false;
        HashMap *map = nullptr;
        Content *pc = nullptr;
        bool isInit = true;
        bool lookForNext() {
            slotIndex++;
            while (true) {
                if (slotIndex >= map->capacity) {
                    return false;
                }
                Content &current = array[slotIndex];
                if(current.isList) {
                    if (current.head->next == current.tail) {
                        slotIndex++;
                        continue;
                    }
                    pc = &current;
                    cur = current.head->next;
                    isTree = false;
                    break;
                }
                avlIterator = current.tree->iterator();
                isTree = true;
                break;
            }
            return true;
        };
    public:
        MapIterator(HashMap *map) : array(map->arrayFrom), map(map) {
            isInit = lookForNext();
        };
        bool next() {
            if (map->hasModified()) {
                return false;
            }
            if (isInit) {
                isInit = false;
                return true;
            }
            if(isTree) {
                if(!avlIterator.next()) {
                    return lookForNext();
                }
            } else if (pc == nullptr || cur->next == pc->tail) {
                return lookForNext();
            }
            cur = cur->next;
            return true;
        };

        Entry* operator->() {
            if(isTree) {
                KeyValue *kv = avlIterator->data;
                Entry temp(kv->key,kv->value);
                return &temp;
            }
            return cur;
        };
    };
public:
    HashMap();
    HashMap(unsigned int capacity, float loadFactor,bool = false);
    ~HashMap();

    bool put(string& key, MondisObject *value);
    MondisObject* get (string &key);
    bool containsKey (string &key);
    bool remove (string &key);
    unsigned size();

    void clear();
private:
    unsigned int hash(string &str);
    void rehash();
    void toTree (int index);
    int getCapacity(int);
    unsigned getIndex(unsigned hash);
    void addToSlot(int index, Entry *entry);
    void toJson();
    HashMap::MapIterator iterator();

    void checkType(string *key);

public:
    virtual ExecRes execute(Command *command);

    virtual MondisObject *locate(Command *command);
};


#endif //MONDIS_HASHMAP_H
