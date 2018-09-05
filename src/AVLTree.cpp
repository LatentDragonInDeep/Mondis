//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"

#include <algorithm>

using namespace std;
bool AVLTree::insert (Entry &entry)
{
    return realInsert(*entry.toKeyValue(),root) == nullptr;
}

AVLTreeNode *AVLTree::realInsert (KeyValue &kv, AVLTreeNode *root)
{
    if(root == nullptr) {
        root = new AVLTreeNode;
        root->data = &kv;
    }
    else if(kv.compare(*root->data)) {
        root->right = realInsert(kv,root->right);
        if(getHeight(root->right)-getHeight(root->left) == 2) {
            if(kv.compare(*root->right->data)) {
                root=leftRotate(root);
            }
            else{
                root = rightLeftRotate(root);
            }
        }
    }
    else{
        root->left = realInsert(kv,root->left);
        if(getHeight(root->left)-getHeight(root->right) == 2) {
            if(kv.compare(*root->left->data)) {
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
    KeyValue* temp = new KeyValue;
    temp->key = &key;
    bool res = realRemove(*temp,root)==nullptr;
    delete temp;
    return res;
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

AVLTreeNode *AVLTree::realRemove (KeyValue &kv, AVLTreeNode *root)
{
    if(root == nullptr) {
        return nullptr;
    }
    if(kv.equals(*root->data)) {
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
            kv.key = successor->data->key;
            root->right = realRemove(kv,root->right);
            if(getHeight(root->left)-getHeight(root->right) == 2) {
                root = rightRotate(root);
            }

            return root;
        }
    }
    else if(kv.compare(*root->data)){
        root->right = realRemove(kv,root->right);
        if(getHeight(root->left)-getHeight(root->right) == 2) {
            root = rightRotate(root);
        }
        root->height = max(getHeight(root->left),getHeight(root->right))+1;

        return root;
    }
    else{
        root->right = realRemove(kv,root->right);
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
        *json+=*iterator->data->getJson();
        *json+=",";
        *json+="\n";
    }
    *json+="}\n";
}

AVLTree::~AVLTree() {
    delete root;
}

bool AVLTree::insert(KeyValue &kv) {
    return realInsert(kv,root) == nullptr;
}

KeyValue *AVLTree::get(Key &key) {
    AVLTreeNode* cur = root;
    while (true) {
        if(cur == nullptr) {
            return nullptr;
        }
        if(key.equals(*cur->data->key)) {
            return cur->data;
        } else if(key.compare(*cur->data->key)) {
            cur = cur->right;
            continue;
        } else{
            cur = cur->left;
            continue;
        }
    }
}

MondisObject *AVLTree::getValue(Key &key) {
    KeyValue* kv = get(key);
    if(kv == nullptr) {
        return nullptr;
    }
    return kv->value;
}

bool AVLTree::containsKey(Key &key) {
    return get(key) == nullptr;
}

