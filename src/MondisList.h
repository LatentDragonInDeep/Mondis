//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISLIST_H
#define MONDIS_MONDISLIST_H

#include <unordered_map>
#include <vector>

#include "MondisObject.h"
#include "Command.h"

using namespace std;

class MondisListNode {
public:
    MondisListNode * pre = nullptr;
    MondisListNode* next = nullptr;
    MondisObject* data = nullptr;
};

class MondisList:public MondisData
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
    class ListIterator {
    private:
        MondisListNode* cur;
    public:
        ListIterator(MondisList* list) {
            cur = list->head;
        }
        bool next() {
            if(cur->next!= nullptr) {
                cur = cur->next;
                return true;
            }
            return false;
        };

        MondisListNode* operator->() {
            return cur;
        }
    };

public:

    MondisList();
    ~MondisList();
    int pushBack (MondisObject *object);

    MondisObject *popBack();
    int pushFront (MondisObject *object);

    MondisObject *popFront();

    int getRange (int from, int to, vector<MondisObject *> *res);

    MondisObject* get (int index);

    int size();

    int set (int index, MondisObject *object);

    int add (vector<MondisObject *> *data);

    ListIterator iterator();

    void toJson();

    ExecutionResult execute(Command *command);

    MondisObject *locate(Command *command);

private:
    void trim();
    MondisListNode* locate (int index);

};


#endif //MONDIS_MONDISLIST_H
