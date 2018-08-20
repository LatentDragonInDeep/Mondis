//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISLIST_H
#define MONDIS_MONDISLIST_H

#include <unordered_map>
#include <vector>

#include "MondisObject.h"

using namespace std;

class MondisListNode {
public:
    MondisListNode * pre;
    MondisListNode* next;
    MondisObject* data;

    MondisListNode():pre(nullptr),next(nullptr),data(nullptr) {}
};

class MondisList
{
private:
    unordered_map<int,MondisListNode*> indexToNode;
    unordered_map<MondisListNode*,int> nodeToIndex;

    MondisListNode* head = new MondisListNode;
    MondisListNode* mid = new MondisListNode;
    MondisListNode* tail = new MondisListNode;

    int headModifyNum = 0;
    int preSize = 0;
    int nextSize = 0;

public:

    MondisList();
    virtual ~MondisList();
    int PushBack(MondisObject* object);
    MondisObject* PopBack();
    int PushFront(MondisObject* object);
    MondisObject* PopFront();

    int GetRange(int from, int to,vector<MondisObject*>* res);

    MondisObject* Get(int index);

    int count();

    int Set(int index,MondisObject* object);

    int Add(vector<MondisObject*>* data);

private:
    void Trim();
    MondisListNode* Locate(int index);

};


#endif //MONDIS_MONDISLIST_H
