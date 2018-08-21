//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_HASHMAP_H
#define MONDIS_HASHMAP_H

#include <stack>

#include "MondisObject.h"

using namespace std;


class HashKey {
public:
    union {
        string* str;
        int intValue;
    } key;
    bool flag;
    bool compare(HashKey& other) {
        if(flag) {
            return key.str->compare(*other.key.str);
        }
        return key.intValue>other.key.intValue;
    }

    bool equals(HashKey& other) {
        if(flag) {
            return *key.str == *other.key.str;
        }
        return key.intValue == other.key.intValue;
    }

    unsigned int hashCode() {
        if(flag) {
            return strHash(*key.str);
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
        /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
        //m和r这两个值用于计算哈希值，只是因为效果好。
        int len = str.size();
        const char * key = str.c_str();
        uint32_t seed = 2017;
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        /* Initialize the hash to a 'random' value */
        uint32_t h = seed ^ len;    //初始化

        /* Mix 4 bytes at a time into the hash */
        const unsigned char *data = (const unsigned char *)key;

        //将字符串key每四个一组看成uint32_t类型，进行运算的到h
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

        /* Handle the last few bytes of the input array  */
        switch(len) {
            case 3: h ^= data[2] << 16;
            case 2: h ^= data[1] << 8;
            case 1: h ^= data[0]; h *= m;
        };

        /* Do a few final mixes of the hash to ensure the last few
         * bytes are well-incorporated. */
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return (unsigned int)h;
    }
};

class Entry{
public:
    HashKey* key;
    MondisObject* object;
    Entry* pre;
    Entry* next;
    bool compare(Entry& other) {
        return key->compare(*other.key);
    }

    bool equals(Entry& other) {
        return key->equals(*other.key);
    }
    ~Entry (){
        delete key;
        delete object;
    }
};

class AVLTreeNode {
public:
    AVLTreeNode* left = nullptr;
    AVLTreeNode* right = nullptr;
    AVLTreeNode* parent = nullptr;
    Entry* data = nullptr;
    int height = 1;
};

class AVLTree
{
private:
    AVLTreeNode* root;
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
    bool remove(HashKey& key);
    Entry* search(HashKey& key);
    AVLIterator iterator();
private:
    AVLTreeNode* realInsert(Entry& entry,AVLTreeNode* root);
    AVLTreeNode* realRemove(Entry& entry,AVLTreeNode* root);
    AVLTreeNode* getSuccessor(AVLTreeNode* root);
    AVLTreeNode* leftRotate(AVLTreeNode* root);
    AVLTreeNode* rightRotate(AVLTreeNode* root);
    AVLTreeNode* leftRightRotate(AVLTreeNode* root);
    AVLTreeNode* rightLeftRotate(AVLTreeNode* root);
    int getHeight(AVLTreeNode* root);
};

class HashMap
{
private:
    float loadFactor = 0.75f;
    unsigned int capacity = 16;
    unsigned int size = 0;
    const int treeThreshold = 8;
    typedef struct {
        Entry * first;
        AVLTree* tree;
        Entry* end;
        int listLen = 0;
        bool isList = true;
    } Content;
    Content * arrayFrom;
    Content * arrayTo;
public:
    HashMap();
    HashMap(unsigned int capacity, float loadFactor);
    ~HashMap();
    bool put (HashKey &key, MondisObject *value);
    MondisObject* get (HashKey &key);
    bool containsKey (HashKey &key);
    bool remove (HashKey &key);

private:
    void rehash();
    void toTree (int index);
    int getIndex (int hash);
    void add(int index,Entry& entry);

    int getCapacity (int capa);
};


#endif //MONDIS_HASHMAP_H
