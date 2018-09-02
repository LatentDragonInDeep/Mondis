//
// Created by caesarschen on 2018/8/21.
//

#ifndef MONDIS_SKIPLIST_H
#define MONDIS_SKIPLIST_H

#include <random>

#include "MondisObject.h"
#include "HashMap.h"

class SkipListNode {
public:
    static const int MAX_LEVEL = 32;
    MondisObject* data = nullptr;
    Key* key = nullptr;
    SkipListNode* backward = nullptr;
    struct SkipListLevel {
        SkipListNode * next = nullptr;
        int span = 0;//跨度
    } *forwards;

    SkipListNode(int levelNum) {
        forwards = new SkipListLevel[levelNum];
    }

    static SkipListNode* getDummyNode() {
        return new SkipListNode(MAX_LEVEL);
    }
};

class SkipList
{
private:
    SkipListNode* head;
    SkipListNode* tail = nullptr;
    unsigned long length = 0;
    int level = 1;
    static uniform_int_distribution<int> distribution;
    static default_random_engine engine;
    static const float constexpr SKIP_LIST_P = 0.25f;
    static const int SKIP_LIST_RANDOM_UPPER_BOUND = 65535;
public:
    SkipList();
    ~SkipList ();
    SkipListNode *insert(Key& key, MondisObject* obj);
    int remove(Key& key);
    void getRange(Key& from, Key& to,vector<MondisObject*>* res);

private:
    int getRandomLevel();

};

class ZSet{
private:
    HashMap* dict;
    SkipList* list;
};


#endif //MONDIS_SKIPLIST_H
