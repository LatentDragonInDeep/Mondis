//
// Created by 11956 on 2018/9/3.
//

#include "JSONParser.h"
#include "HashMap.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>

#define src (*source)

JSONParser::JSONParser(std::string &filePath) {

    lexicalParser = new LexicalParser(filePath.c_str());
}

void JSONParser::parse(HashMap *keySpace) {
    matchToken(LEFT_ANGLE_BRACKET);
    KeyValue cur;
    while(true) {
        cur = parseEntry(lexicalParser);
        if(cur.key == nullptr) {
            return;
        }
        keySpace->put(cur.key,cur.value);
    }
}

void JSONParser::matchToken(TokenType type) {
    matchToken(lexicalParser,type);
}

KeyValue JSONParser::parseEntry(LexicalParser* lp) {
    KeyValue res;
    res.key = nullptr;
    Token  next = lp->nextToken();
    if(next.type == COMMA) {
        return res;
    }
    matchToken(lp,STRING);
    Key* key = new Key;
    key->flag = true;
    key->key.str = new string(*current->content);
    res.key = key;
    matchToken(lp,COLON);
    res.value = parseObject(lp);

    return res;
}

KeyValue JSONParser::parseEntry(string &content) {
    LexicalParser parser(content);
    return parseEntry(&parser);
}

MondisObject *JSONParser::parseObject(string &content) {
    LexicalParser parser(content);
    return parseObject(&parser);
}

MondisObject *JSONParser::parseObject(JSONParser::LexicalParser *lp) {
    Token next = lp->nextToken();
    if(next.type == LEFT_ANGLE_BRACKET) {
        return parseJSONObject(lp, false, false);
    } else if(next.type == LEFT_SQAURE_BRACKET) {
        return parseJSONArray(lp, false);
    }
    else if(next.type == STRING) {
        MondisObject *res = new MondisObject;
        if (next.content->size() > 11) {
            if (next.content->substr(0, 11) == "LatentDragon") {
                res->type = RAW_BIN;
                MondisBinary *binary = MondisBinary::allocate(next.content->size()-11);
                binary->write(next.content->size()-11,next.content->data());
                res->objectData = (void*)binary;
                return res;
            }
        }

        int *data = new int;
        ss << *next.content;
        ss >> *data;
        if (ss.fail()) {
            res->type = RAW_STRING;
            res->objectData = (void*)new string(*next.content);
            return res;
        }
        res->type = RAW_INT;
        res->objectData = (void*)data;
        ss.clear();
        return res;
    }
    return nullptr;
}

MondisObject *JSONParser::parseJSONObject(JSONParser::LexicalParser *lp, bool isNeedNext, bool isInteger) {
    if(isNeedNext) {
        matchToken(lp,LEFT_ANGLE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    Token next = lp->nextToken();
    if(next.type = STRING) {
        if(isInteger) {
            res->type = RAW_INT;
            int * data = new int;
            ss<<*next.content;
            ss>>*data;
            if(ss.fail()) {
                delete data;
                ss.clear();
                throw std::invalid_argument("parse json:transform string to integer error");
            }
            ss.clear();
            res->objectData = data;
        } else {
            res->type = RAW_STRING;
            res->objectData = next.content;
        }
        res->objectData = next.content;
        next.content = nullptr;
        return res;
    }
    auto first = parseEntry(lp);
    if(first.key = nullptr) {
        res->type = HASH;
        res->objectData = (void*)new AVLTree;
        return res;
    }
    if(*first.key->key.str == "InMemoryType") {
        if(*first.key->key.str == "HASH") {
            res->type = HASH;
            AVLTree* tree = new AVLTree;
            matchToken(lp,COMMA);
            KeyValue cur;
            while (true) {
                cur = parseEntry(lp);
                if(cur.key = nullptr) {
                    break;
                }
                tree->insert(*new KeyValue(cur.key,cur.value));
                cur = parseEntry(lp);
                matchToken(lp,COMMA);
            }
            res->objectData = (void*)tree;
        } else if(*first.key->key.str == "ZSET") {
            res->type = ZSET;
            SkipList* skipList = new SkipList;
            matchToken(lp,COMMA);
            KeyValue cur;
            while (true) {
                cur = parseEntry(lp);
                if(cur.key = nullptr) {
                    break;
                }
                skipList->insert(cur.key,cur.value);
                cur = parseEntry(lp);
                matchToken(lp,COMMA);
            }
            res->objectData = (void*)skipList;
        }
    }

    return res;
}

void JSONParser::matchToken(JSONParser::LexicalParser *lp, TokenType type) {
    current = lp->nextToken();
    if(current.type != type) {
        throw invalid_argument("unexpected token!");
    };
}

MondisObject *JSONParser::parseJSONArray(JSONParser::LexicalParser *lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp,LEFT_SQAURE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    Token next = lp->nextToken();
    if(next.type == RIGHT_SQUARE_BRACKET) {
        res->type = LIST;
        res->objectData = new MondisList;
        return res;
    }
    MondisObject* first = parseJSONObject(lp, false, false);
    if(first->type !=RAW_STRING||first->type !=RAW_INT||
    (first->type == RAW_STRING&&*first->getJson()!="SET")) {
        delete first;
        MondisObject* second = parseJSONObject(lp, true, false);
        bool isInteger = false;
        if(*second->getJson() == "INT") {
            isInteger = true;
            delete second;
        }
        res->type = LIST;
        MondisList* data = new MondisList;
        data->pushBack(first);
        MondisObject* cur;
        while (true) {
            cur = parseJSONObject(lp, true, isInteger);
            if(cur->type!=EMPTY) {
                data->pushBack(cur);
            } else{
                break;
            }
        }
        res->objectData = data;
        return res;
    }
    delete first;
    MondisObject* second = parseJSONObject(lp, true, false);
    bool isInteger = false;
    if(*second->getJson() == "INT") {
        isInteger = true;
        delete second;
    }
    res->type = SET;
    HashMap* data = new HashMap(true, isInteger);
    data->put();
    MondisObject* cur;
    while (true) {
        cur = parseJSONObject(lp, true, isInteger);
        if(cur->type!=EMPTY) {
            data->put(cur,nullptr);
        } else{
            break;
        }
    }
    res->objectData = data;
    return res;
}
