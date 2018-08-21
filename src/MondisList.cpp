//
// Created by caesarschen on 2018/8/20.
//
#include "MondisList.h"

MondisList::MondisList ()
{
    head->next = mid;
    mid->pre = head;
    mid->next = tail;
    tail->pre = mid;
}

int MondisList::pushBack (MondisObject *object)
{
    MondisListNode * newNode = new MondisListNode;
    newNode->data = object;
    MondisListNode * pre = tail->pre;
    pre->next = newNode;
    newNode->pre = pre;
    newNode->next = tail;
    tail->pre = newNode;
    nextSize++;
    nodeToIndex[newNode] = nextSize+headModifyNum;
    indexToNode[nextSize+headModifyNum] = newNode;
}

MondisObject *MondisList::popBack ()
{
    if(nextSize == 0) {
        trim();
    }
    if(nextSize == 0) {
        return MondisObject::getNullObject();
    }
    nextSize--;
    MondisListNode* last = tail->pre;
    last->pre->next = tail;
    tail->pre = last->pre;

    MondisObject* obj = last->data;
    delete last;

    return obj;
}

void MondisList::trim ()
{
    MondisListNode* last = mid->pre;
    last->next = tail;
    tail->pre = last;
    MondisListNode* first = head->next;
    head->next = mid;
    mid->pre = head;
    mid->next = first;
    first->pre = mid;
    int index = 1;
    for (MondisListNode* node = first;node!= tail;node = node->next)
    {
        indexToNode[index] = node;
        nodeToIndex[node] = index;
        index++;
    }

    preSize = 0;
    nextSize = index--;
}

int MondisList::pushFront (MondisObject *object)
{
    MondisListNode * newNode = new MondisListNode;
    newNode->data = object;
    if(headModifyNum>0)
    {
        headModifyNum --;
        MondisListNode *next = mid->next;
        mid->next = newNode;
        newNode->pre = mid;
        newNode->next = next;
        next->pre = newNode;
        nextSize ++;
    }
    else{
        MondisListNode* next = head->next;
        head->next = newNode;
        newNode->pre = head;
        newNode->next = next;
        next->pre = newNode;
        preSize++;
        indexToNode[-preSize] = newNode;
        nodeToIndex[newNode] = -preSize;
    }

}

MondisObject *MondisList::popFront ()
{
    if(preSize>0) {
        preSize--;
        MondisListNode* res = head->next;
        MondisListNode* temp = res->next;
        head->next = temp;
        temp->pre = head;
        MondisObject* obj = res->data;
        delete res;
        return obj;
    }
    else{
        nextSize--;
        MondisListNode* res = mid->next;
        MondisListNode* temp = res->next;
        mid->next = temp;
        temp->pre = mid;
        MondisObject* obj = res->data;
        headModifyNum++;
        delete res;
        return obj;
    }
}

MondisObject *MondisList::get (int index)
{
    return locate(index)->data;
}

int MondisList::getRange (int from, int to, vector<MondisObject *> *res)
{
    MondisListNode* start = locate(from);
    MondisListNode* end = locate(to);
    for (MondisListNode* cur = start;cur!=end;cur = cur->next)
    {
        if(cur!=mid) {
            res->push_back(cur->data);
        }
    }
}

MondisListNode *MondisList::locate (int index)
{
    if(index<preSize) {
        return indexToNode[-preSize+index-1];
    }
    return indexToNode[index-preSize+headModifyNum];
}

int MondisList::count ()
{
    return preSize+nextSize;
}

int MondisList::set (int index, MondisObject *object)
{
    locate(index)->data = object;
}

int MondisList::add (vector<MondisObject *> *data)
{
    for(MondisObject* pObj:*data) {
        pushBack(pObj);
    }
}

MondisList::~MondisList ()
{
    for (MondisListNode * cur = head;cur!= nullptr;)
    {
        MondisListNode* next = cur->next;
        delete cur;
        cur = next;
    }
}




