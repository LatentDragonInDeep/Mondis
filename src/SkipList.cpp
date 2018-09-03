//
// Created by caesarschen on 2018/8/21.
//

#include "SkipList.h"

SkipList::SkipList ():head(SkipListNode::getDummyNode()) {}

int SkipList::getRandomLevel ()
{
    int level = 1;
    while (distribution(engine)%SKIP_LIST_RANDOM_UPPER_BOUND<SKIP_LIST_P*SKIP_LIST_RANDOM_UPPER_BOUND) {
        level++;
    }

    return level>SkipListNode::MAX_LEVEL?SkipListNode::MAX_LEVEL:level;
}

SkipListNode *SkipList::insert (Key &key, MondisObject *obj)
{

    SkipListNode * cur = head;
    SkipListNode** last = new SkipListNode*[level];
    for (int i = level; i>=0; --i)
    {
        while (true) {
            if(key.equals(*cur->forwards[i].next->key)) {
                cur->forwards[i].next->data = obj;
                return cur;
            }
            else if(key.compare(*cur->forwards[i].next->key)) {
                cur = cur->forwards[i].next;
            }
            else{
                break;
            }
        }
        last[i] = cur;
    }
    int ranLevel = getRandomLevel();
    if(ranLevel>level) {
        level++;
        level = min(level,SkipListNode::MAX_LEVEL);
    }
    SkipListNode* newNode = new SkipListNode(level);
    newNode->backward = cur;
    newNode->data = obj;
    for (int j = 0; j <level; ++j)
    {
        newNode->forwards[j] = last[j]->forwards[j];
        last[j]->forwards[j].next = newNode;
    }

}

void SkipList::toJson() {
    if(isKeyInteger) {
        SkipIterator iterator = this->iterator();
        *json += "[";
        *json += "\n";
        while (iterator.next()) {
            *json += iterator->data->getJson();
            *json += "\n";
        }
        *json += "]\n";
    } else{
        SkipIterator iterator = this->iterator();
        *json += "{";
        *json += "\n";
        *json+="\"InMemoryType\":\"ZSET\"";
        while (iterator.next()) {
            *json+=iterator->key->getJson();
            *json+=" : ";
            *json += iterator->data->getJson();
            *json += ",\n";
        }
        *json += "}\n";
    }
}

SkipList::SkipIterator SkipList::iterator() {
    return SkipList::SkipIterator(this);
}




