//
// Created by caesarschen on 2018/8/20.
//
#include <vector>

#include "MondisList.h"
#include "MondisServer.h"

MondisList::MondisList ()
{
    head->next = mid;
    mid->pre = head;
    mid->next = tail;
    tail->pre = mid;
}

int MondisList::pushBack (MondisObject *object)
{
    auto * newNode = new MondisListNode;
    newNode->data = object;
    MondisListNode * pre = tail->pre;
    pre->next = newNode;
    newNode->pre = pre;
    newNode->next = tail;
    tail->pre = newNode;
    nextSize++;
    nodeToIndex[newNode] = nextSize+headModifyNum;
    indexToNode[nextSize+headModifyNum] = newNode;
    modified();
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
    modified();

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
    modified();

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
        modified();
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
        modified();
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

int MondisList::size()
{
    return preSize+nextSize;
}

int MondisList::set (int index, MondisObject *object)
{
    locate(index)->data = object;
    modified();
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

void MondisList::toJson() {
    json = "";
    json += "[\n";
    ListIterator iterator = this->iterator();
    while (iterator.next()) {
        json += iterator->data->getJson();
        json += "\n";
    }
    json += "]\n";
}

MondisList::ListIterator MondisList::iterator() {
    return MondisList::ListIterator(this);
}

MondisObject *MondisList::locate(Command *command) {
    if (command->params.size() != 1) {
        return nullptr;
    }
    if ((*command)[0].type != Command::ParamType::PLAIN) {
        return nullptr;
    }
    int index;
    if (!util::toInteger((*command)[0].content, index)) {
        return nullptr;
    }

    return locate(index)->data;
}

ExecutionResult MondisList::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            CHECK_PARAM_TYPE(1, STRING)
            if (index < 0 || index > size()) {
                res.res = "index out of range";
                return res;
            }
            set(index, MondisServer::getJSONParser()->parseObject((*command)[1].content));
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, index)
            if (index < 0 || index > size()) {
                res.res = "index out of range";
                return res;
            }
            res.res = get(index)->getJson();
            OK_AND_RETURN
        }
        case PUSH_FRONT:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,STRING)
            pushFront(MondisServer::getJSONParser()->parseObject((*command)[0].content));
            OK_AND_RETURN
        case PUSH_BACK:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,STRING)
            pushBack(MondisServer::getJSONParser()->parseObject((*command)[0].content));
            OK_AND_RETURN
        case POP_FRONT:
            CHECK_PARAM_NUM(0)
            CHECK_PARAM_TYPE(0,STRING)
            res.res = popFront()->getJson();
            OK_AND_RETURN
        case POP_BACK:
            CHECK_PARAM_NUM(0)
            CHECK_PARAM_TYPE(0,STRING)
            res.res = popBack()->getJson();
            OK_AND_RETURN
        case GET_RANGE:
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0,PLAIN)
            CHECK_PARAM_TYPE(1,PLAIN)
            CHECK_INT_START_LEGAL(0)
            CHECK_START
            CHECK_INT_END_LEGAL(1)
            CHECK_END(size())
            std::vector<MondisObject*> data;
            getRange(start, end, &data);
            res.res+="{\n";
            for (auto obj:data) {
                res.res += obj->getJson();
            }
            res.res+="}\n";
            OK_AND_RETURN
    }
    INVALID_AND_RETURN
}




