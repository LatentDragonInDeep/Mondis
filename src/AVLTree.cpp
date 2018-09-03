//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"

#include <algorithm>

using namespace std;
bool AVLTree::insert (Entry &entry)
{
    realInsert(entry,root);
    return true;
}

AVLTreeNode *AVLTree::realInsert (Entry &entry, AVLTreeNode *root)
{
    if(root == nullptr) {
        root = new AVLTreeNode;
        root->data = &entry;
    }
    else if(entry.compare(*root->data)) {
        root->right = realInsert(entry,root->right);
        if(getHeight(root->right)-getHeight(root->left) == 2) {
            if(entry.compare(*root->right->data)) {
                root=leftRotate(root);
            }
            else{
                root = rightLeftRotate(root);
            }
        }
    }
    else{
        root->left = realInsert(entry,root->left);
        if(getHeight(root->left)-getHeight(root->right) == 2) {
            if(entry.compare(*root->left->data)) {
                root=leftRightRotate(root)
            }
            else{
                root = rightRotate(root);
            }
        }
    }
    root->height = max(getHeight(root->left),getHeight(root->right))+1;

    return root;
}

bool AVLTree::remove (Key &key)
{
    Entry* temp = new Entry;
    temp->key = &key;
    bool res = realRemove(*temp,root)==nullptr;
    delete temp;
    return res;
}

Entry *AVLTree::search (Key &key)
{
    return nullptr;
}

AVLTree::AVLIterator AVLTree::iterator ()
{
    return AVLTree::AVLIterator(this);
}

AVLTreeNode *AVLTree::leftRotate (AVLTreeNode *root)
{
    AVLTreeNode* right = root->right;
    AVLTreeNode* rightLeft = right->left;
    right->left = root;
    root->right = rightLeft;

    return right;
}

AVLTreeNode *AVLTree::rightRotate (AVLTreeNode *root)
{
    AVLTreeNode* left = root->left;
    AVLTreeNode* leftRight = left->right;
    left->right = root;
    root->left = leftRight;

    return left;
}

AVLTreeNode *AVLTree::leftRightRotate (AVLTreeNode *root)
{
    root->left = leftRotate(root->left);
    return rightRotate(root);
}

AVLTreeNode *AVLTree::rightLeftRotate (AVLTreeNode *root)
{
    root->right = rightRotate(root->right);

    return leftRotate(root);
}

int AVLTree::getHeight (AVLTreeNode *root)
{
    if(root == nullptr) {
        return 0;
    }
    return root->height;
}

AVLTreeNode *AVLTree::realRemove (Entry &entry, AVLTreeNode *root)
{
    if(root == nullptr) {
        return nullptr;
    }
    if(entry.equals(*root->data)) {
        if(root->left == nullptr&&root->right == nullptr) {
            delete root;
            return nullptr;
        }
        else if(root->left!= nullptr&&root->right== nullptr) {
            AVLTreeNode* res = root->left;
            delete root;
            return res;
        }
        else if(root->left== nullptr&&root->right!= nullptr) {
            AVLTreeNode* res = root->right;
            delete root;
            return res;
        }
        else{
            AVLTreeNode* successor = getSuccessor(root);
            root->data = successor->data;
            entry.key = successor->data->key;
            root->right = realRemove(entry,root->right);
            if(getHeight(root->left)-getHeight(root->right) == 2) {
                root = rightRotate(root);
            }

            return root;
        }
    }
    else if(entry.compare(*root->data)){
        root->right = realRemove(entry,root->right);
        if(getHeight(root->left)-getHeight(root->right) == 2) {
            root = rightRotate(root);
        }
        root->height = max(getHeight(root->left),getHeight(root->right))+1;

        return root;
    }
    else{
        root->right = realRemove(entry,root->right);
        if(getHeight(root->left)-getHeight(root->right) == 2) {
            root = rightRotate(root);
        }
        root->height = max(getHeight(root->left),getHeight(root->right))+1;

        return root;
    }
}

AVLTreeNode *AVLTree::getSuccessor (AVLTreeNode *root)
{
    AVLTreeNode* cur = root->right;
    while (cur->left!= nullptr) {
        cur = cur->left;
    }

    return cur;
}

void AVLTree::toJson() {
    AVLIterator iterator = this->iterator();
    *json+="{\n";
    *json+="\"InMemoryType\":\"HASH\"";
    while (iterator.next()) {
        *json+=iterator->data->getJson();
        *json+=",";
        *json+="\n";
    }
    *json+="}\n";
}

AVLTree::~AVLTree() {
    delete root;
}

