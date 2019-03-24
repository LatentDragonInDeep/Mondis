//
// Created by caesarschen on 2018/8/21.
//

#ifndef MONDIS_SKIPLIST_H
#define MONDIS_SKIPLIST_H

#include "MondisObject.h"
#include "HashMap.h"

class SplayTreeNode {
public:
    const int score;
    MondisObject* data = nullptr;
    SplayTreeNode* left = nullptr;
    SplayTreeNode* right = nullptr;
    SplayTreeNode* parent = nullptr;
    unsigned treeSize = 0;
    SplayTreeNode(int sc,MondisObject* d):score(sc),data(d){};
    ~SplayTreeNode() {
        delete data;
    }
};

class SplayTree:public MondisData
{
private:
    SplayTreeNode* root;
    SplayTreeNode* head = new SplayTreeNode(numeric_limits<int>::min(), nullptr);
    SplayTreeNode* tail = new SplayTreeNode(numeric_limits<int>::max(), nullptr);
public:
    class SplayIterator {
    private:
        SplayTree* tree;
        SplayTreeNode* cur;
        stack<SplayTreeNode*> s;

        void dfs(SplayTreeNode *cur);
    public:
        SplayIterator(SplayTree *avlTree);

        SplayTreeNode *operator->();;

        bool next();
    };
    SplayTree();
    ~SplayTree();
    bool insert(int score, MondisObject* obj);
    bool removeByScore(int score);
    bool removeByRank(int rank);
    MondisObject* getByScore(int score);
    MondisObject* getByRank(int rank);
    MondisObject* getByRank(SplayTreeNode* root,int rank);
    SplayTreeNode* getNodeByRank(SplayTreeNode* root,int rank);

    SplayTreeNode *getNodeByRank(int rank);
    SplayTreeNode* getNodeByScore(int score);
    bool remove(SplayTreeNode* target);
public:
    bool contains(int score);
    unsigned count(int startScore,int endScore);
    void removeRangeByRank(int start,int end);
    void removeRangeByScore(int startScore,int endScore);
    void getRangeByRank(int start,int end,vector<MondisObject*>* res);
    void getRangeByScore(int startScore, int endScore,vector<MondisObject*>* res);
    unsigned size();
    void toJson();

    SplayIterator iterator();

    ExecutionResult execute(Command *command);

    MondisObject *locate(Command *command);

    SplayTreeNode *getLowerBound(int score, bool canEqual);

    SplayTreeNode *getUpperBound(int score, bool canEqual);

private:
    void splay(SplayTreeNode *cur, SplayTreeNode *target);
    void sizeUpdate(SplayTreeNode* cur, int delta);
    unsigned getSize(SplayTreeNode* root);
    void leftRotate(SplayTreeNode* cur);
    void rightRotate(SplayTreeNode* cur);
    void leftRightRotate(SplayTreeNode* cur);
    void rightLeftRotate(SplayTreeNode* cur);
    void deleteTree(SplayTreeNode* root);
    SplayTreeNode* getSuccessor(SplayTreeNode* root);
    SplayTreeNode* getPredecessor(SplayTreeNode* root);
    void inOrderTraversal(SplayTreeNode* root,vector<MondisObject*>* res);
};


#endif //MONDIS_SKIPLIST_H
