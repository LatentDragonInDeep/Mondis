//
// Created by 11956 on 2018/9/3.
//

#include <unistd.h>
#include "JSONParser.h"
#include "HashMap.h"
#include "MondisBinary.h"
#include "MondisList.h"
#include "SplayTree.h"


JSONParser::JSONParser(std::string &source) {

    lexicalParser = LexicalParser(source);
}


void JSONParser::parse(HashMap *keySpace) {
    Token next = lexicalParser.nextToken();
    if (next.type != LEFT_ANGLE_BRACKET) {
        return;
    }
    KeyValue cur;
    while(true) {
        cur = parseEntry(lexicalParser);
        if(cur.key == nullptr) {
            break;
        }
        keySpace->put(cur.key,cur.value);
    }
    Token end = lexicalParser.nextToken();
    if (end.type != RIGHT_ANGLE_BRACKET) {
        keySpace->clear();
        throw new std::invalid_argument("unexpected token!");
    }
    Token ter = lexicalParser.nextToken();
    if (ter.type != TERMINATOR) {
        keySpace->clear();
        throw new std::invalid_argument("unexpected token!");
    }
}

KeyValue JSONParser::parseEntry(LexicalParser &lp) {
    Token next = lp.nextToken();
    KeyValue res;
    res.key = nullptr;
    if (next.type != STRING) {
        lp.back();
        return res;
    }
    Key *key = new Key(current.content);
    res.key = key;
    matchToken(lp,COLON);
    res.value = parseObject(lp);
    Token n = lp.nextToken();
    if (n.type == RIGHT_ANGLE_BRACKET) {
        lp.back();
    } else if (n.type != COMMA) {
        throw std::invalid_argument("unexpected token!");
    }

    return res;
}

KeyValue JSONParser::parseEntry(string &content) {
    lexicalParser.reset();
    lexicalParser.setSource(content);
    return parseEntry(lexicalParser);
}

MondisObject *JSONParser::parseObject(string &content) {
    lexicalParser.reset();
    lexicalParser.setSource(content);
    return parseObject(lexicalParser);
}

MondisObject *JSONParser::parseObject(LexicalParser &lp) {
    Token next = lp.nextToken();
    if(next.type == LEFT_ANGLE_BRACKET) {
        return parseJSONObject(lp, false, false);
    } else if (next.type == LEFT_SQUARE_BRACKET) {
        return parseJSONArray(lp, false);
    }
    else if(next.type == STRING) {
        MondisObject *res = new MondisObject;
        if (next.content.size() > 12) {
            if (next.content.substr(0, 12) == "LatentDragon") {
                res->type = RAW_BIN;
                MondisBinary *binary = MondisBinary::allocate(next.content.size() - 12);
                binary->write(next.content.size() - 12, next.content.data() + 12);
                binary->setPosition(0);
                res->objectData = (void*)binary;
                return res;
            }
        }

        int *data = new int;
        bool to = util::toInteger(next.content, *data);
        if (!to) {
            res->type = RAW_STRING;
            res->objectData = (void *) new string(next.content);
            delete data;
            return res;
        }
        res->type = RAW_INT;
        res->objectData = (void*)data;
        return res;
    } else {
        lp.back();
        return nullptr;
    }
}

MondisObject *JSONParser::parseJSONObject(LexicalParser &lp, bool isNeedNext, bool isInteger) {
    if(isNeedNext) {
        Token next = lp.nextToken();
        matchToken(lp,LEFT_ANGLE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    res->type = HASH;
    AVLTree* tree = new AVLTree;
    KeyValue cur;
    while (true) {
        cur = parseEntry(lp);
        if (cur.key == nullptr) {
            break;
        }
        tree->insert(new KeyValue(cur.key, cur.value));
    }
    res->objectData = (void*)tree;
    matchToken(lp, RIGHT_ANGLE_BRACKET);
    return res;
}

MondisObject *JSONParser::parseJSONArray(LexicalParser &lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp, LEFT_SQUARE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    MondisObject *first = parseObject(lp);
    if (first != nullptr) {
        if (first->type == RAW_STRING) {
            if (first->getJson() == "\"LIST\"") {
                list:
                res->type = LIST;
                MondisList *data = new MondisList;
                res->objectData = data;
                MondisObject *cur = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    if (cur != nullptr) {
                        data->pushBack(cur);
                    } else {
                        break;
                    }
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            } else if (first->getJson() == "\"SET\"") {
                res->type = SET;
                HashMap *data = new HashMap(true, true);
                res->objectData = data;
                MondisObject *cur = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    if (cur != nullptr) {
                        if (cur->type != RAW_STRING || cur->type != RAW_INT) {
                            throw "unexpected element";
                        }
                        string json = cur->getJson();
                        Key *key = new Key(json);
                        data->put(key, nullptr);
                    } else {
                        break;
                    }
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            } else if (first->getJson() == "\"ZSET\"") {
                res->type = ZSET;
                SplayTree *data = new SplayTree;
                res->objectData = data;
                MondisObject *cur = nullptr;
                MondisObject *next = nullptr;
                while (true) {
                    cur = parseObject(lp);
                    next = parseObject(lp);
                    if (cur == nullptr) {
                        break;
                    }
                    if (cur->type != RAW_INT) {
                        throw "unexpected element";
                    }
                    string curScore = cur->getJson();
                    int score = *reinterpret_cast<int *>(curScore.data());
                    delete cur;
                    data->insert(score, next);
                }
                delete first;
                matchToken(lp, RIGHT_SQUARE_BRACKET);
                return res;
            }
        } else {
            goto list;
        }

    } else {
        res->type = LIST;
        res->objectData = new MondisList;
        return res;
    }
}

JSONParser::JSONParser() {}

void JSONParser::matchToken(JSONParser::LexicalParser &lp, ParserTokenType type) {
    Token next = lp.nextToken();
    if (next.type != type) {
        throw new std::invalid_argument("unexpected token!");
    }
}

JSONParser::JSONParser(const char *filePath) {
    lexicalParser = LexicalParser(filePath);
}

JSONParser::JSONParser(std::string &&s) {
    lexicalParser = LexicalParser(s);
}

void JSONParser::parseAll(std::vector<HashMap *> &dbs) {
    Token next = lexicalParser.nextToken();
    if (next.type != LEFT_ANGLE_BRACKET) {
        return;
    }
    KeyValue cur;
    while (true) {
        cur = parseEntry(lexicalParser);
        if (cur.key == nullptr) {
            break;
        }
        int dbIndex = cur.key->intValue;
        AVLTree *tree = (AVLTree *) cur.value->objectData;
        AVLTree::AVLIterator iter = tree->iterator();
        while (iter.next()) {
            dbs[dbIndex]->put(iter->data->key, iter->data->value);
            tree->remove(*iter->data->key);
        }
    }
    Token end = lexicalParser.nextToken();
    if (end.type != RIGHT_ANGLE_BRACKET) {
        throw new std::invalid_argument("unexpected token!");
    }
    Token ter = lexicalParser.nextToken();
    if (ter.type != TERMINATOR) {
        throw new std::invalid_argument("unexpected token!");
    }
}

Token *Token::leftSquareBracket = new Token(LEFT_SQUARE_BRACKET);
Token *Token::rightSquareBracket = new Token(RIGHT_SQUARE_BRACKET);
Token *Token::leftAngleBracket = new Token(LEFT_ANGLE_BRACKET);
Token *Token::rightAngleBracket = new Token(RIGHT_ANGLE_BRACKET);
Token *Token::comma = new Token(COMMA);
Token *Token::terminator = new Token(TERMINATOR);
Token *Token::colon = new Token(COLON);

std::unordered_map<char, Token *> JSONParser::LexicalParser::directRecognize;

void JSONParser::LexicalParser::init() {
    directRecognize['['] = Token::getToken(LEFT_SQUARE_BRACKET);
    directRecognize[']'] = Token::getToken(RIGHT_SQUARE_BRACKET);
    directRecognize['{'] = Token::getToken(LEFT_ANGLE_BRACKET);
    directRecognize['}'] = Token::getToken(RIGHT_ANGLE_BRACKET);
    directRecognize[','] = Token::getToken(COMMA);
    directRecognize[':'] = Token::getToken(COLON);
}

void JSONParser::LexicalParser::skip() {
    if (curIndex >= source.size()) {
        isEnd = true;
        return;
    }
    while (source[curIndex] == ' ' || source[curIndex] == '\n' || source[curIndex] == '\r' ||
           source[curIndex] == '\t') {
        curIndex++;
        if (curIndex >= source.size()) {
            isEnd = true;
            break;
        }
    }
}

JSONParser::LexicalParser::LexicalParser(std::string &s) : source(s) {}

JSONParser::LexicalParser::LexicalParser() {}

JSONParser::LexicalParser::LexicalParser(const char *filePath) {
    struct stat info;
    int fd = open(filePath, O_RDONLY);
    fstat(fd, &info);

    source.reserve(info.st_size);

    std::ifstream in;
    in.open(filePath);
    std::string line;
    while (std::getline(in, line)) {
        source += line;
        source += "\n";
    }
    close(fd);
    in.close();
}

Token JSONParser::LexicalParser::nextToken() {
    preIndex = curIndex;
    hasBacked = false;
    Token res;
    skip();
    if (isEnd) {
        return res;
    }
    int start;
    int end;
    if (directRecognize.find(source[curIndex]) != directRecognize.end()) {
        res = *directRecognize[source[curIndex]];
        curIndex++;
        return res;
    }
    if (source[curIndex] == '"') {
        start = curIndex;
        while (true) {
            curIndex++;
            if (source[curIndex] == '"') {
                if (source[curIndex - 1] != '\\') {
                    end = curIndex;
                    res.type = STRING;
                    std::string raw(source, start + 1, end - start - 1);
                    util::eraseBackSlash(raw);
                    curIndex++;
                    res.content = raw;
                    return res;
                }
            }
        }
    }

}

bool JSONParser::LexicalParser::back() {
    if (hasBacked) {
        return false;
    }
    curIndex = preIndex;
    hasBacked = true;
    return true;
}

void JSONParser::LexicalParser::reset() {
    source = "";
    curIndex = 0;
    isEnd = false;
}

void JSONParser::LexicalParser::setSource(std::string newSrc) {
    source = newSrc;
}

JSONParser::LexicalParser::LexicalParser(std::string &&s) {
    source = s;
}

void util::eraseBackSlash(std::string &data) {
    auto iter = data.begin();
    for (; iter != data.end(); ++iter) {
        if (*iter == '\\') {
            if (*(iter + 1) == '"') {
                iter = data.erase(iter);
            }
        }
    }
}
