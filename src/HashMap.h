//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_HASHMAP_H
#define MONDIS_HASHMAP_H

#include <stack>

#include "MondisObject.h"
#include "MondisData.h"
#include "Command.h"

using namespace std;


class Key :public MondisData{
public:
    union {
        string str;
        int intValue;
    } key;
    bool flag = true;
    bool compare(Key& other) {
        if(flag!=other.flag) {
            return false;
        }
        if(flag) {
            return key.str.compare(other.key.str);
        }
        return key.intValue>other.key.intValue;
    }
    Key(string& k) {
        if(toInteger(k,key.intValue)) {
            flag = false;
        } else{
            key.str = k;
        }
    }
    Key(int k):key.intValue(k),flag(false){};

    bool isInteger() {
        return !flag;
    }
    void toString() {
        if(flag) {
            return;
        }
        key.str = to_string(key.intValue);
        flag = true;
    }

    bool toInteger() {
        bool res = toInteger(key.str,key.intValue);
        if(res) {
            flag = false;
            return true;
        }
        return false;
    }
    bool equals(Key& other) {
        if(flag!=other.flag) {
            return false;
        }
        if(flag) {
            return key.str == other.key.str;
        }
        return key.intValue == other.key.intValue;
    }

    unsigned int hashCode() {
        if(flag) {
            return strHash(key.str);
        }
        else  {
            return intHash(key.intValue);
        }
    }

private:
    unsigned int intHash(int key) {
        key += ~(key << 15);
        key ^=  (key >> 10);
        key +=  (key << 3);
        key ^=  (key >> 6);
        key += ~(key << 11);
        key ^=  (key >> 16);
        return key;
    }

    unsigned int strHash(string& str) {
        int len = str.size();
        const char * key = str.c_str();
        uint32_t seed = 2017;
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        uint32_t h = seed ^ len;    //初始化

        const unsigned char *data = (const unsigned char *)key;

        while(len >= 4) {
            uint32_t k = *(uint32_t*)data;

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            data += 4;
            len -= 4;
        }

        switch(len) {
            case 3: h ^= data[2] << 16;
            case 2: h ^= data[1] << 8;
            case 1: h ^= data[0]; h *= m;
        };


        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return (unsigned int)h;
    }

    void toJson() {
        if(flag) {
            *json+="\"";
            *json+=key.str;
            *json+="\"";
        } else{
            *json+="\"";
            *json+=to_string(key.intValue);
            *json+="\"";
        }
    }
};

class KeyValue :public MondisData{
public:
    Key* key = nullptr;
    MondisObject* value = nullptr;
    KeyValue(Key* k,MondisObject* v):key(k),value(v) {

    }
    KeyValue(){};
    KeyValue(KeyValue&& other) = default;
    KeyValue(KeyValue& other):key(other.key),value(other.value) {
        other.key = nullptr;
        other.value = nullptr;
    }
    bool compare(KeyValue& other) {
        return key->compare(other.key);
    }

    bool equals(KeyValue& other) {
        return key->equals(other.key);
    }
    ~KeyValue() {
        delete key;
        delete value;
    }

    void toJson() {
        *json+=*key->getJson();
        *json+=",";
        *json+=*value->getJson();
        *json+="\n";
    }
};

class Entry:public MondisData{
public:
    Key* key = nullptr;
    MondisObject* object = nullptr;
    Entry* pre = nullptr;
    Entry* next = nullptr;
    bool compare(Entry& other) {
        return key->compare(*other.key);
    }

    bool equals(Entry& other) {
        return key->equals(*other.key);
    }
    Entry(Key* key,MondisObject* data):key(key),object(data) {

    };
    Entry(KeyValue* kv):key(kv->key),object(kv->value){
        kv->key= nullptr;
        kv->value = nullptr;
    };
    Entry(){};
    ~Entry (){
        delete key;
        delete object;
    }

    void toJson() {
        *json+=key->getJson();
        if(!isValueNull) {
            *json += " : ";
            *json += object->getJson();
        }
    }
    KeyValue* toKeyValue() {
        KeyValue* res = new KeyValue(key,object);
        key = nullptr;
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
    AVLTreeNode* root;
    unsigned _size;
public:
    class AVLIterator {
    private:
        AVLTree* tree;
        AVLTreeNode* cur;
        stack<AVLTreeNode*> s;
        void dfs(AVLTreeNode* cur) {
            while (cur!= nullptr) {
                s.push(cur);
                cur=cur->left;
            }
        }
    public:
        AVLIterator(AVLTree* avlTree):tree(avlTree),cur(avlTree->root)
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
    bool insert(Entry& entry);
    bool insert(KeyValue& kv);
    bool insert(Key& key,MondisObject* value);
    bool remove(Key& key);
    KeyValue* get(Key& key);
    MondisObject* getValue(Key& key);
    bool containsKey(Key& key);
    AVLIterator iterator();
    ~AVLTree();
    ExecutionResult execute(Command& command);
    MondisObject* locate(Command& command);
    unsigned size();
private:
    AVLTreeNode* realInsert(KeyValue& kv,AVLTreeNode* root);
    AVLTreeNode* realRemove(KeyValue& kv,AVLTreeNode* root);
    AVLTreeNode* getSuccessor(AVLTreeNode* root);
    AVLTreeNode* leftRotate(AVLTreeNode* root);
    AVLTreeNode* rightRotate(AVLTreeNode* root);
    AVLTreeNode* leftRightRotate(AVLTreeNode* root);
    AVLTreeNode* rightLeftRotate(AVLTreeNode* root);
    int getHeight(AVLTreeNode* root);
    void deleteTree(AVLTreeNode* root);

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
    bool isIntset = true;
    typedef struct {
        Entry * first;
        AVLTree* tree;
        Entry* end;
        int listLen = 0;
        bool isList = true;
    } Content;
    Content * arrayFrom;
    Content * arrayTo;
    class MapIterator{
    private:
        Entry* cur = nullptr;
        Content * array;
        unsigned slotIndex = 0;
        AVLTree::AVLIterator avlIterator;
        bool isTree = false;

        bool lookForNext() {
            while (true) {
                if(slotIndex>capacity) {
                    return false;
                }
                Content current = array[slotIndex];
                if(current.isList) {
                    if(current.first == nullptr) {
                        slotIndex++;
                        continue;
                    }
                    cur = current.first;
                    isTree = false;
                    break;
                }
                avlIterator = current.tree->iterator();
                cur = avlIterator.next();
                isTree = true;
                break;
            }
            return true;

        }
    public:
        MapIterator(HashMap* map)array(map->arrayFrom) {
            lookForNext();
        };
        bool next() {
            if(isTree) {
                if(!avlIterator.next()) {
                    return lookForNext();
                }
            }
            else if(cur->next == nullptr) {
                return lookForNext();
            }
            cur = cur->next;
            return true;
        };
        Entry* operator->() {
            if(isTree) {
                return avlIterator->();
            }
            return cur;
        };
    };
public:
    HashMap();
    HashMap(unsigned int capacity, float loadFactor);

    HashMap(float loadFactor, unsigned int capacity, const bool isIntset);

    HashMap(const bool isIntset);

    ~HashMap();
    bool put (Key &key, MondisObject *value);
    MondisObject* get (Key &key);
    bool containsKey (Key &key);
    bool remove (Key &key);
    unsigned size();
private:
    void rehash();
    void toStringSet();
    void toTree (int index);
    int getIndex (int hash);
    void add(int index,Entry* entry);

    int getCapacity (int capa);
    void toJson();
    HashMap::MapIterator iterator();
    void checkType(Key& key);

    virtual ExecutionResult execute(Command& command);
    virtual MondisObject* locate(Command& command);
};


#endif //MONDIS_HASHMAP_H
