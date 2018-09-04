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
#define src (*source)


enum TokenType {
    LEFT_SQAURE_BRACKET,
    RIGHT_SQUARE_BRACKET,
    LEFT_ANGLE_BRACKET,
    RIGHT_ANGLE_BRACKET,
    STRING,
    COMMA,
    COLON,
    TERMINATOR,
};

class Token {
public:
    TokenType type;
    std::string* content;
    Token(TokenType type):type(type) {

    }

    Token(std::string& data):type(STRING),content(&data){

    }
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
        string * source;
        Token* current = Token::getToken(TERMINATOR);
        unsigned curIndex = 0;
        bool isEnd = false;
        static bool hasInit;
        static unordered_map<char,Token*> directRecognize;

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
        }
    public:
        LexicalParser(string& source):source(&source) {
            if(!hasInit) {
                initDirect();
                hasInit = true;
            }
        }
        LexicalParser(const char* filePath){
            struct stat info;
            int fd = open(filePath.c_str(), O_RDONLY);
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
        }
        ~LexicalParser(){
            delete source;
        }
        Token* nextToken() {
            if(current->type == STRING) {
                delete current;
            }
            current = Token::getToken(TERMINATOR);
            if(isEnd) {
                return current;
            }
            int start;
            int end;
            if(directRecognize[src[curIndex]] != nullptr) {
                current = directRecognize[src[curIndex]];
                curIndex++;
                return current;
            }
            if(src[curIndex] == '"') {
                start = curIndex;
                while (true) {
                    curIndex++;
                    if(src[curIndex] == '"') {
                        if(src[curIndex-1]!='\\') {
                            end = curIndex;
                            current->type = STRING;
                            current->content = new string(src,start,end-start);
                            return current;
                        }
                    }
                }
            }

        }
    };
    Token* current;
    stringstream ss;
    JSONParser(std::string& filePath);
    void parse(HashMap* keySpace);
    MondisObject* parseObject(string& content);
    pair<Key*,MondisObject*> parseEntry(string& content);
private:
    LexicalParser* lexicalParser;
    MondisObject* parseObject(LexicalParser* lp);
    MondisObject* parseJSONObject(LexicalParser* lp,bool isNeedNext);
    MondisObject* parseJSONArray(LexicalParser* lp, bool isNeedNext);

    pair<Key*,MondisObject*> parseEntry(LexicalParser* lp);
    void matchToken(LexicalParser* lp,TokenType type);
    void matchToken(TokenType type);
};

bool JSONParser::LexicalParser::hasInit = false;


#endif //MONDIS_JSONPARSER_H
