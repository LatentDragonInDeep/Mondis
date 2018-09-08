//
// Created by caesarschen on 2018/8/21.
//

#include "SplayTree.h"


SplayTreeNode *SplayTree::insert (Key &key, MondisObject *obj)
{


}

void SplayTree::toJson() {
    SkipIterator iterator = this->iterator();
    *json += "{";
    *json += "\n";
    while (iterator.next()) {
        *json += iterator->data->getJson();
        *json += ",\n";
    }
    *json += "}\n";
}

SplayTree::SkipIterator SplayTree::iterator() {
    return SplayTree::SkipIterator(this);
}




