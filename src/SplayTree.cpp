//
// Created by caesarschen on 2018/8/21.
//

#include <limits>
#include "SplayTree.h"


void SplayTree::toJson() {
    SplayIterator iterator = this->iterator();
    json += "{";
    json += "\n";
    while (iterator.next()) {
        json += iterator->data->getJson();
        json += ",\n";
    }
    json += "}\n";
}

SplayTree::SplayIterator SplayTree::iterator() {
    return SplayIterator(this);
}

SplayTree::SplayTree() {
    root = head;
    head->right = tail;
    tail->parent = head;
}

SplayTree::~SplayTree() {
    deleteTree(root);
}

bool SplayTree::insert(int score, MondisObject *obj) {
    bool leftOrRight;
    SplayTreeNode* cur =root;
    SplayTreeNode* parent = nullptr;
    while (true) {
        if(cur == nullptr) {
            cur = new SplayTreeNode(score,obj);
            cur->parent = parent;
            splay(cur);
            sizeUpdate(cur,1);
            if(parent!= nullptr) {
                if(leftOrRight) {
                    parent->left = cur;
                } else{
                    parent->right = cur;
                }
            }
            return true;
        }
        if(cur->score == score) {
            delete cur->data;
            cur->data = obj;
            splay(cur);
            return false;
        } else if(cur->score>score) {
            parent = cur;
            cur = cur->right;
            leftOrRight = false;
        } else{
            parent = cur;
            cur = cur->left;
            leftOrRight = true;
        }
    }
}

MondisObject *SplayTree::getByScore(int score) {
    SplayTreeNode* cur = getNodeByScore(score);
    if(cur == nullptr) {
        return nullptr;
    }

    return cur->data;
}

MondisObject *SplayTree::getByRank(SplayTreeNode* root,int rank) {
    SplayTreeNode* node = getNodeByRank(root,rank);
    if(node == nullptr) {
        return nullptr;
    }
    return node->data;
}

MondisObject *SplayTree::getByRank(int rank) {
    return getByRank(root,rank);
}

unsigned SplayTree::getSize(SplayTreeNode *root) {
    if(root == nullptr||root == head||root == tail) {
        return 0;
    }
    return root->treeSize;
}

SplayTreeNode *SplayTree::getNodeByRank(SplayTreeNode *root, int rank) {
    if(root == nullptr) {
        return nullptr;
    }
    if(root == this->root&&rank == 0) {
        return head;
    }
    if(root == this->root&&rank == root->treeSize+1) {
        return tail;
    }
    unsigned leftSize = getSize(root->left);
    if(rank<leftSize) {
        return getNodeByRank(root->left,rank);
    }
    if(rank == leftSize+1) {
        return root;
    }
    return getNodeByRank(root->right,rank-(leftSize+1));
}

SplayTreeNode *SplayTree::getNodeByScore(int score) {
    if(root == this->root&&score == numeric_limits<int>::min()) {
        return head;
    }
    if(root == this->root&&score == numeric_limits<int>::max()) {
        return tail;
    }
    SplayTreeNode* cur = root;
    while (true) {
        if(cur == nullptr) {
            return nullptr;
        }
        if(cur->score == score) {
            splay(cur);
            return cur;
        } else if(cur->score>score) {
            cur = cur->right;
        } else{
            cur = cur->left;
        }
    }
}

bool SplayTree::contains(int score) {
    return getByScore(score) == nullptr;
}

unsigned SplayTree::count(int startScore, int endScore) {
    if(startScore == endScore) {
        return 0;
    }
    SplayTreeNode* from = getLowerBound(startScore);
    SplayTreeNode* to = getUpperBound(endScore);
    splay(from);
    splay(to);
    return getSize(root->right->left);
}

unsigned SplayTree::size() {
    return getSize(root);
}

void SplayTree::sizeUpdate(SplayTreeNode *cur,int delta) {
    while (cur!= nullptr) {
        cur->treeSize+=delta;
        cur = cur->parent;
    }
}

void SplayTree::leftRotate(SplayTreeNode *root) {
    SplayTreeNode* parent = root->parent;
    int state;
    if(parent == nullptr) {
        state = 0;
    } else if(root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    SplayTreeNode* right = root->right;
    SplayTreeNode* rightLeft = right->left;
    right->left = root;
    root->parent = right;
    right->treeSize = root->treeSize;
    root->right = rightLeft;
    if(rightLeft!= nullptr) {
        rightLeft->parent = root;
    }
    root->treeSize = getSize(root->left)+getSize(root->right)+1;
    switch (state) {
        case 0:
            right->parent = nullptr;
            break;
        case 1:
            right->parent = parent;
            parent->left = right;
            break;
        case 2:
            right->parent = parent;
            parent->right = right;
            break;
    }
}

void SplayTree::rightRotate(SplayTreeNode *root) {
    SplayTreeNode* parent = root->parent;
    int state;
    if(parent == nullptr) {
        state = 0;
    } else if(root == root->parent->left) {
        state = 1;
    } else {
        state = 2;
    }
    SplayTreeNode* left = root->left;
    SplayTreeNode* leftRight = left->right;
    left->right = root;
    root->parent = left;
    left->treeSize = root->treeSize;
    root->left = leftRight;
    if(leftRight!= nullptr) {
        leftRight->parent = root;
    }
    root->treeSize = getSize(root->left)+getSize(root->right)+1;
    switch (state) {
        case 0:
            left->parent = nullptr;
            break;
        case 1:
            left->parent = parent;
            parent->left = left;
            break;
        case 2:
            left->parent = parent;
            parent->right = left;
            break;
    }
}

void SplayTree::leftRightRotate(SplayTreeNode *root) {
    leftRotate(root->left);
    rightRotate(root);
}

void SplayTree::rightLeftRotate(SplayTreeNode *root) {
    rightRotate(root->right);
    leftRotate(root);
}

void SplayTree::splay(SplayTreeNode *cur) {
    SplayTreeNode* parent = cur->parent;
    while (parent!= nullptr) {
        SplayTreeNode* grandParent = parent->parent;
        if(cur == parent->left) {
            if(grandParent == nullptr) {
                rightRotate(parent);
            } else if(parent == grandParent->left) {
                rightRotate(parent);
                rightRotate(grandParent);
            } else {
                rightLeftRotate(grandParent);
            }
        } else{
            if(grandParent == nullptr) {
                leftRotate(parent);
            } else if(parent ==  grandParent->left) {
                leftRightRotate(grandParent);
            } else {
                leftRotate(parent);
                leftRotate(grandParent);
            }
        }
        parent = cur->parent;
    }
    root = cur;
}

bool SplayTree::removeByScore(int score) {
    SplayTreeNode* target = getNodeByScore(score);
    return remove(target);
}

void SplayTree::deleteTree(SplayTreeNode *root) {
    if(root == nullptr) {
        return;
    }
    SplayTreeNode* left = root->left;
    SplayTreeNode* right = root->right;
    delete root;
    deleteTree(left);
    deleteTree(right);
}

SplayTreeNode *SplayTree::getSuccessor(SplayTreeNode *root) {
    SplayTreeNode* cur = root->right;
    if(cur== nullptr) {
        SplayTreeNode *parent = root->parent;
        SplayTreeNode* ch = root;
        while (parent!= nullptr&&ch == parent->right) {
            ch = parent;
            parent = parent->parent;
        }
        return parent;
    }
    while (cur->left!= nullptr) {
        cur = cur->left;
    }

    return cur;
}

bool SplayTree::removeByRank(int rank) {
    SplayTreeNode* target = getNodeByRank(root,rank);
    return remove(target);
}

bool SplayTree::remove(SplayTreeNode *target) {
    if(target == nullptr||target == head||target == tail) {
        return false;
    }
    if(target->left == nullptr&&target->right == nullptr) {
        sizeUpdate(target,-1);
        delete target;
        return true;
    }
    else if(target->left == nullptr&&target->right!= nullptr) {
        sizeUpdate(target,-1);
        SplayTreeNode* parent = target->parent;
        if(parent!= nullptr) {
            SplayTreeNode* right = target->right;
            delete target;
            parent->right = right;
            right->parent = parent;
        }
        return true;
    }
    else if(target->left != nullptr&&target->right== nullptr) {
        sizeUpdate(target,-1);
        SplayTreeNode* parent = target->parent;
        if(parent!= nullptr) {
            SplayTreeNode* left = target->left;
            delete target;
            parent->left = left;
            left->parent = parent;
        }
        return true;
    } else{
        SplayTreeNode* successor = getSuccessor(target);
        if(successor == tail) {
            SplayTreeNode* predecessor = getPredecessor(target);
            if(predecessor = head) {
                root = head;
                head->right = tail;
                tail->parent = head;
                return true;
            }
            delete target->data;
            target->data = predecessor->data;
            remove(predecessor);
            return true;
        }
        delete target->data;
        target->data = successor->data;
        remove(successor);
        return true;
    }
}

SplayTreeNode *SplayTree::getPredecessor(SplayTreeNode *root) {
    SplayTreeNode* cur = root->left;
    if(cur== nullptr) {
        SplayTreeNode *parent = root->parent;
        SplayTreeNode* ch = root;
        while (parent != nullptr&&ch == parent->left) {
            ch = parent;
            parent = parent->parent;
        }
        return parent;
    }
    while (cur->right!= nullptr) {
        cur = cur->right;
    }

    return cur;
}

void SplayTree::removeRangeByRank(int start, int end) {
    if(start == end||start+1 == end) {
        return;
    }
    SplayTreeNode* from = getNodeByRank(root,start);
    SplayTreeNode* to = getNodeByRank(root,end);
    splay(from);
    splay(to);
    sizeUpdate(root->right,-getSize(root->right->left));
    deleteTree(root->right->left);
}

void SplayTree::getRangeByRank(int start, int end, vector<MondisObject*> *res) {
    if(start == end||start+1 == end) {
        return;
    }
    SplayTreeNode* from = getNodeByRank(root,start);
    SplayTreeNode* to = getNodeByRank(root,end);
    splay(from);
    splay(to);
    inOrderTraversal(root->right->left,res);
}

void SplayTree::inOrderTraversal(SplayTreeNode *root, vector<MondisObject *> *res) {
    if(root == nullptr||root==head||root == tail) {
        return;
    }
    inOrderTraversal(root->left,res);
    res->push_back(root->data);
    inOrderTraversal(root->right,res);
}

void SplayTree::removeRangeByScore(int startScore, int endScore) {
    if(startScore == endScore) {
        return;
    }
    SplayTreeNode* from = getLowerBound(startScore);
    SplayTreeNode* to = getUpperBound(endScore);
    if(from == tail||to == head) {
        return;
    }
    splay(from);
    splay(to);
    sizeUpdate(root->right,-getSize(root->right->left));
    deleteTree(root->right->left);
}

void SplayTree::getRangeByScore(int startScore, int endScore, vector<MondisObject *> *res) {
    if(startScore == endScore) {
        return;
    }
    SplayTreeNode* from = getLowerBound(startScore);
    SplayTreeNode* to = getUpperBound(endScore);
    if(from == tail||to == head) {
        return;
    }
    splay(from);
    splay(to);
    inOrderTraversal(root->right->left,res);

}

SplayTreeNode *SplayTree::getLowerBound(int score) {
    SplayTreeNode* cur = root;
    while (true) {
        if(cur->score == score) {
            return getSuccessor(root);
        } else if(cur->score>score) {
            if(cur->left == nullptr) {
                return cur;
            }
            cur = cur->left;
        } else{
            if(cur->right == nullptr) {
                return getSuccessor(cur);
            }
            cur = cur->right;
        }
    }
}

SplayTreeNode *SplayTree::getUpperBound(int score) {
    SplayTreeNode* cur = root;
    while (true) {
        if(cur->score == score) {
            return getPredecessor(root);
        } else if(cur->score<score) {
            if(cur->right == nullptr) {
                return cur;
            }
            cur = cur->right;
        } else{
            if(cur->left == nullptr) {
                return getPredecessor(cur);
            }
            cur = cur->left;
        }
    }
}

ExecutionResult SplayTree::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case ADD: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, socre)
            CHECK_PARAM_TYPE(1, STRING)
            MondisObject *data = MondisServer::getJSONParser()->parseObject((*command)[1].content);
            insert(socre, data);
            OK_AND_RETURN
        }
        case REMOVE_BY_RANK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, rank)
            if (rank < 1) {
                res.res = "can not remove under rank 1";
                return res;
            }
            if (rank > size()) {
                res.res = "can not remove over rank ";
                res.res += size();
                return res;
            }
            removeByRank(rank);
            OK_AND_RETURN
        }
        case REMOVE_BY_SCORE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            removeByScore(score);
            OK_AND_RETURN
        }
        case REMOVE_RANGE_BY_RANK: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_START_AND_DEFINE(0)
            CHECK_END_AND_DEFINE(1, size())
            removeRangeByRank(start, end);
            OK_AND_RETURN
        }
        case REMOVE_RANGE_BY_SCORE: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            removeRangeByScore(start, end);
            OK_AND_RETURN
        }
        case GET_BY_RANK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, rank)
            if (rank < 1) {
                res.res = "can not remove under rank 1";
                return res;
            }
            if (rank > size()) {
                res.res = "can not remove over rank ";
                res.res += size();
                return res;
            }
            res.res = getByRank(rank)->getJson();
            OK_AND_RETURN
        }
        case GET_BY_SCORE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            MondisObject *data = getByScore(score);
            if(data== nullptr) {
                res.res = "null object";
                return res;
            }
            res.res = data->getJson();
            OK_AND_RETURN
        }
        case GET_RANGE_BY_RANK: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_START_AND_DEFINE(0)
            CHECK_END_AND_DEFINE(1, size())
            vector<MondisObject *> v;
            getRangeByRank(start, end, &v);
            res.res += "[\n";
            for (MondisObject *every:v) {
                res.res += every->getJson();
                res.res += "\n";
            }
            res.res += "]\n";
            OK_AND_RETURN
        }
        case GET_RANGE_BY_SCORE:{
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            vector<MondisObject *> v;
            getRangeByScore(start, end, &v);
            res.res += "[\n";
            for (MondisObject *every:v) {
                res.res += every->getJson();
                res.res += "\n";
            }
            res.res += "]\n";
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, score)
            res.res = to_string(contains(score));
            OK_AND_RETURN
        }
        case SIZE:
            CHECK_PARAM_NUM(0)
            res.res = to_string(size());
            OK_AND_RETURN
        case COUNT:
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            res.res = to_string(count(start,end));
            OK_AND_RETURN
    }
    INVALID_AND_RETURN
}

MondisObject *SplayTree::locate(Command *command) {
    MondisObject* res;
    if (command->params.size() != 2) {
        return nullptr;
    }
    if(command[0].type!=Command::ParamType::PLAIN) {
        return nullptr;
    }
    if ((*command)[1].content == "RANK") {
        int rank;
        if (!util::toInteger((*command)[1].content, rank)) {
            return nullptr;
        }
        if(rank<1||rank>size()) {
            return nullptr;
        }
        return getByRank(rank);
    }
    if ((*command)[1].content == "SCORE") {
        int score;
        if (!util::toInteger((*command)[1].content, score)) {
            return nullptr;
        }
        return getByScore(score);
    }

    return nullptr;
}






