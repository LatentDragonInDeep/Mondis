//
// Created by 11956 on 2018/9/3.
//

#ifndef MONDIS_JSONPARSER_H
#define MONDIS_JSONPARSER_H

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <exception>
#include <sstream>

#include "MondisObject.h"
#include "Command.h"
#include "HashMap.h"

#define src (*source)

class MondisObject;
class HashMap;
class KeyValue;

enum TokenType {
    LEFT_SQAURE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    LEFT_ANGLE_BRACKET,
    RIGHT_ANGLE_BRACKET,
    STRING,
    COMMA,
    COLON,
    TERMINATOR
};

class Token {
public:
    TokenType type;
    std::string* content;
    Token(TokenType type):type(type) {

    }
    Token(Token&& other) = default;
    Token(Token& other):content(other.content),type(other.type) {
        other.content = nullptr;
    }
    Token(std::string& data):type(STRING),content(&data){

    }
    Token& operator=(Token& other) {
        type = other.type;
        content = other.content;
        other.content = nullptr;

        return *this;
    };
    Token():Token(TERMINATOR){};
    static Token* leftSquareBracket;
    static Token* rightSquareBracket;
    static Token* leftAngleBracket;
    static Token* rightAngleBracket;
    static Token* comma;
    static Token* terminator;
    static Token* colon;

    static Token* getToken(TokenType type) {
        switch (type) {
            case LEFT_SQAURE_BRACKET:
                return leftSquareBracket;
            case RIGHT_SQUARE_BRACKET:
                return rightSquareBracket;
            case LEFT_ANGLE_BRACKET:
                return leftAngleBracket;
            case RIGHT_ANGLE_BRACKET:
                return rightAngleBracket;
            case COMMA:
                return comma;
            case COLON:
                return colon;
            case TERMINATOR:
                return terminator;
        }
    }

    ~Token() {
        delete content;
    }
};

Token* Token::leftSquareBracket = new Token(LEFT_SQAURE_BRACKET);
Token* Token::rightSquareBracket = new Token(RIGHT_SQUARE_BRACKET);
Token* Token::leftAngleBracket = new Token(LEFT_ANGLE_BRACKET);
Token* Token::rightAngleBracket = new Token(RIGHT_ANGLE_BRACKET);
Token* Token::comma = new Token(COMMA);
Token* Token::terminator = new Token(TERMINATOR);
Token* Token::colon = new Token(COLON);

class JSONParser {
public:
    class LexicalParser{
    private:
        std::string* source;
        unsigned curIndex = 0;
        bool isEnd = false;
        static bool hasInit;
        static std::unordered_map<char,Token*> directRecognize;

        static void initDirect() {
            directRecognize['['] = Token::getToken(LEFT_SQAURE_BRACKET);
            directRecognize[']'] = Token::getToken(RIGHT_SQUARE_BRACKET);
            directRecognize['{'] = Token::getToken(LEFT_ANGLE_BRACKET);
            directRecognize['}'] = Token::getToken(RIGHT_ANGLE_BRACKET);
            directRecognize[','] = Token::getToken(COMMA);
            directRecognize[':'] = Token::getToken(COLON);
        }
        void skip() {
            while (src[curIndex] ==' '||src[curIndex]=='\n'||src[curIndex] == '\r'||src[curIndex] == '\t') {
                curIndex++;
                if(curIndex>source->size()) {
                    isEnd = true;
                    break;
                }
            }
        };
    public:
        LexicalParser(string& source):source(&source) {
            if(!hasInit) {
                initDirect();
                hasInit = true;
            }
        };
        LexicalParser(const char* filePath){
            struct stat info;
            int fd = open(filePath, O_RDONLY);
            fstat(fd, &info);

            source = new string("");
            source->reserve(info.st_size);

            ifstream in;
            in.open(filePath);
            string line;
            while (getline(in,line)) {
                *source+=line;
                *source+="\n";
            }
            close(fd);
            in.close();
        };
        ~LexicalParser(){
            delete source;
        };
        Token nextToken() {
            Token res;
            if(isEnd) {
                return res;
            }
            skip();
            int start;
            int end;
            if(directRecognize[src[curIndex]] != nullptr) {
                res = *directRecognize[src[curIndex]];
                curIndex++;
                return res;
            }
            if(src[curIndex] == '"') {
                start = curIndex;
                while (true) {
                    curIndex++;
                    if(src[curIndex] == '"') {
                        if(src[curIndex-1]!='\\') {
                            end = curIndex;
                            res.type = STRING;
                            res.content = new string(src,start,end-start);
                            return res;
                        }
                    }
                }
            }

        }

        void reset() {
            source = nullptr;
            curIndex = 0;
            isEnd = false;
        }

        void setSource(std::string* newSrc) {
            source = newSrc;
        }
    };
    Token current;
    JSONParser();
    JSONParser(std::string& filePath);
    void parse(HashMap* keySpace);
    MondisObject* parseObject(string& content);
    KeyValue parseEntry(string& content);
private:
    LexicalParser* lexicalParser;
    MondisObject* parseObject(LexicalParser* lp);
    MondisObject *parseJSONObject(JSONParser::LexicalParser *lp, bool isNeedNext, bool isInteger);
    MondisObject* parseJSONArray(LexicalParser* lp, bool isNeedNext);

    KeyValue parseEntry(LexicalParser* lp);
    void matchToken(LexicalParser* lp,TokenType type);
    void matchToken(TokenType type);
};

bool JSONParser::LexicalParser::hasInit = false;


#endif //MONDIS_JSONPARSER_H
