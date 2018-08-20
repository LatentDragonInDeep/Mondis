//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_AVLTREE_H
#define MONDIS_AVLTREE_H

#include "MondisObject.h"
#include "HashMap.h"

class AVLTreeNode {
public:
    AVLTreeNode* left;
    AVLTreeNode* right;
    int balanceFactor;
    Entry* data;
};
class AVLTree
{
private:
    AVLTreeNode* root;
public:
    bool insert(MondisObject)
};


#endif //MONDIS_AVLTREE_H
