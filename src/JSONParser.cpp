//
// Created by 11956 on 2018/9/3.
//

#include "JSONParser.h"
#include "HashMap.h"
#include "MondisBinary.h"
#include "MondisList.h"


JSONParser::JSONParser(std::string &filePath) {

    lexicalParser = LexicalParser(filePath.c_str());
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

KeyValue JSONParser::parseEntry(LexicalParser &lp) {
    KeyValue res;
    res.key = nullptr;
    Token next = lp.nextToken();
    if (next.type != STRING) {
        return res;
    }
    Key *key = new Key(current.content);
    res.key = key;
    matchToken(lp,COLON);
    res.value = parseObject(lp);

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
    } else if(next.type == LEFT_SQAURE_BRACKET) {
        return parseJSONArray(lp, false);
    }
    else if(next.type == STRING) {
        MondisObject *res = new MondisObject;
        if (next.content.size() > 11) {
            if (next.content.substr(0, 11) == "LatentDragon") {
                res->type = RAW_BIN;
                MondisBinary *binary = MondisBinary::allocate(next.content.size() - 11);
                binary->write(next.content.size() - 11, next.content.data());
                res->objectData = (void*)binary;
                return res;
            }
        }

        int *data = new int;
        bool to = util::toInteger(next.content, *data);
        if (!to) {
            res->type = RAW_STRING;
            res->objectData = (void *) new string(next.content);
            return res;
        }
        res->type = RAW_INT;
        res->objectData = (void*)data;
        return res;
    }
    return nullptr;
}

MondisObject *JSONParser::parseJSONObject(LexicalParser &lp, bool isNeedNext, bool isInteger) {
    if(isNeedNext) {
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
        matchToken(lp,COMMA);
    }
    res->objectData = (void*)tree;
    return res;
}

void JSONParser::matchToken(LexicalParser &lp, TokenType type) {
    current = lp.nextToken();
    if(current.type != type) {
        throw invalid_argument("unexpected token!");
    };
}

MondisObject *JSONParser::parseJSONArray(LexicalParser &lp, bool isNeedNext) {
    if(isNeedNext) {
        matchToken(lp,LEFT_SQAURE_BRACKET);
    }
    MondisObject* res = new MondisObject;
    Token next = lp.nextToken();
    if(next.type == RIGHT_SQUARE_BRACKET) {
        res->type = LIST;
        res->objectData = new MondisList;
        return res;
    }
    MondisObject* first = parseJSONObject(lp, false, false);
    if(first->type !=RAW_STRING ||first->type !=RAW_INT ||
       (first->type == RAW_STRING && first->getJson() != "SET")) {
        delete first;
        MondisObject* second = parseJSONObject(lp, true, false);
        bool isInteger = false;
        if (second->getJson() == "INT") {
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
    if (second->getJson() == "INT") {
        isInteger = true;
        delete second;
    }
    res->type = SET;
    HashMap* data = new HashMap(true, isInteger);
    MondisObject* cur;
    while (true) {
        cur = parseJSONObject(lp, true, isInteger);
        if(cur->type!=EMPTY) {
            Key *key = new Key(*(int *) cur->objectData);
            data->put(key, nullptr);
        } else{
            break;
        }
    }
    res->objectData = data;
    return res;
}

JSONParser::JSONParser() {}

Token *Token::leftSquareBracket = new Token(LEFT_SQAURE_BRACKET);
Token *Token::rightSquareBracket = new Token(RIGHT_SQUARE_BRACKET);
Token *Token::leftAngleBracket = new Token(LEFT_ANGLE_BRACKET);
Token *Token::rightAngleBracket = new Token(RIGHT_ANGLE_BRACKET);
Token *Token::comma = new Token(COMMA);
Token *Token::terminator = new Token(TERMINATOR);
Token *Token::colon = new Token(COLON);

std::unordered_map<char, Token *> JSONParser::LexicalParser::directRecognize;

void JSONParser::LexicalParser::init() {
    directRecognize['['] = Token::getToken(LEFT_SQAURE_BRACKET);
    directRecognize[']'] = Token::getToken(RIGHT_SQUARE_BRACKET);
    directRecognize['{'] = Token::getToken(LEFT_ANGLE_BRACKET);
    directRecognize['}'] = Token::getToken(RIGHT_ANGLE_BRACKET);
    directRecognize[','] = Token::getToken(COMMA);
    directRecognize[':'] = Token::getToken(COLON);
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
