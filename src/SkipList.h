//
// Created by caesarschen on 2018/8/21.
//

#ifndef MONDIS_SKIPLIST_H
#define MONDIS_SKIPLIST_H

#include <random>

#include "MondisObject.h"
#include "HashMap.h"
#include "MondisData.h"

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

class SkipList:public MondisData
{
private:
    SkipListNode* head = new SkipListNode;
    SkipListNode* tail = new SkipListNode;
    unsigned long length = 0;
    int level = 1;
    static uniform_int_distribution<int> distribution;
    static default_random_engine engine;
    static const float constexpr SKIP_LIST_P = 0.25f;
    static const int SKIP_LIST_RANDOM_UPPER_BOUND = 65535;
    bool isKeyInteger = true;
public:
    class SkipIterator {
    private:
        SkipListNode* cur;
    public:
        SkipIterator(SkipList* list) {
            cur = list->head;
        }
        bool next() {
            if(cur->forwards[0].next!= nullptr) {
                cur = cur->forwards[0].next;
                return true;
            }
            return false;
        }

        SkipListNode*operator->() {
            return cur;
        }
    };
    SkipList();
    ~SkipList();
    SkipListNode *insert(int score, MondisObject* obj);
    //TODO
    bool removeByScore(int score);
    bool removeByRank(int rank)
    MondisObject* getByScore(int score);
    MondisObject* getByRank(int rank);
    bool containsKey(int score);
    unsigned count(int startScore,int endScore);
    void removeRangeByRank(int start,int end);
    void removeRangeByScore(int startScore,int endScore);
    void getRangeByRank(int start,int end,vector<MondisObject*>* res);
    void getRangeByScore(int startScore, int endScore,vector<MondisObject*>* res);
    unsigned size();
    void toJson();
    SkipIterator iterator();
    ExecutionResult execute(Command& command);
    MondisObject* locate(Command& command);
private:
    static int getRandomLevel();
};


#endif //MONDIS_SKIPLIST_H
