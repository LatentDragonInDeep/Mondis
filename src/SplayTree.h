//
// Created by caesarschen on 2018/8/21.
//

#ifndef MONDIS_SKIPLIST_H
#define MONDIS_SKIPLIST_H

#include <random>

#include "MondisObject.h"
#include "HashMap.h"
#include "MondisData.h"

class SplayTreeNode {
public:
    int score;
    MondisObject* data = nullptr;
    SplayTreeNode* left = nullptr;
    SplayTreeNode* right = nullptr;
    unsigned height = 0;
    unsigned treeSize = 0;
};

class SplayTree:public MondisData
{
private:
    SplayTreeNode* root;
public:
    class SplayIterator {
    private:
        SplayTreeNode* cur;
    public:
        SkipIterator(SplayTree* list) {
            cur = list->head;
        }
        bool next() {
            if(cur->forwards[0].next!= nullptr) {
                cur = cur->forwards[0].next;
                return true;
            }
            return false;
        }

        SplayTreeNode*operator->() {
            return cur;
        }
    };
    SplayTree();
    ~SplayTree();
    bool insert(int score, MondisObject* obj);
    //TODO
    bool removeByScore(int score);
    bool removeByRank(int rank)
    MondisObject* getByScore(int score);
    MondisObject* getByRank(int rank);
    bool contains(int score);
    unsigned count(int startScore,int endScore);
    void removeRangeByRank(int start,int end);
    void removeRangeByScore(int startScore,int endScore);
    void getRangeByRank(int start,int end,vector<MondisObject*>* res);
    void getRangeByScore(int startScore, int endScore,vector<MondisObject*>* res);
    unsigned size();
    void toJson();
    SplayIterator iterator();
    ExecutionResult execute(Command& command);
    MondisObject* locate(Command& command);
private:
    static int getRandomLevel();
};


#endif //MONDIS_SKIPLIST_H
