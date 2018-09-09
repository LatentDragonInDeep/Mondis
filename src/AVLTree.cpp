//
// Created by caesarschen on 2018/8/20.
//

#include "HashMap.h"
#include "MondisServer.h"
#include "Command.h"

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
        _size++;
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
    } else if(kv.equals(*root->data)) {
        delete root->data;
        root->data = &kv;
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
            _size--;
            return nullptr;
        }
        else if(root->left!= nullptr&&root->right== nullptr) {
            AVLTreeNode* res = root->left;
            delete root;
            _size--;
            return res;
        }
        else if(root->left== nullptr&&root->right!= nullptr) {
            AVLTreeNode* res = root->right;
            delete root;
            _size--;
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
            _size--;
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
    deleteTree(root);
}

bool AVLTree::insert(KeyValue &kv) {
    return realInsert(kv,root) == nullptr;
}

KeyValue *AVLTree::get(Key &key) {
    if(key.isInteger()) {
        key.toString();
    }
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

MondisObject *AVLTree::locate(Command &command) {
    return getValue(KEY(0));
}

ExecutionResult AVLTree::execute(Command &command) {
    ExecutionResult res;
    switch (command.type) {
        case SET:
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0,PLAIN)
            CHECK_PARAM_TYPE(1,STRING)
            insert(*new KEY(0),MondisServer::getJSONParser()->parseObject(command[1].content));
            OK_AND_RETURN;
        case GET:
            CHECK_PARAM_NUM(1);
            CHECK_PARAM_TYPE(0,PLAIN)
            auto * obj = getValue(KEY(0));
            if(obj!= nullptr) {
                res.res = *obj->getJson();
            }
            OK_AND_RETURN;
        case DEL:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            remove(KEY(0));
            OK_AND_RETURN
        case EXISTS:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            res.res = to_string(containsKey(KEY(0)));
            OK_AND_RETURN
        case SIZE:
            CHECK_PARAM_NUM(0)
            res.res = to_string(size());
            OK_AND_RETURN
    }
    INVALID_AND_RETURN
}

bool AVLTree::insert(Key &key, MondisObject *value) {
    return insert(*new KeyValue(&key,value));
}

unsigned AVLTree::size() {
    return _size;
}

void AVLTree::deleteTree(AVLTreeNode *root) {
    if(root == nullptr) {
        return;
    }
    AVLTreeNode* left = root->left;
    AVLTreeNode* right = root->right;
    delete root;
    deleteTree(left);
    deleteTree(right);
}

