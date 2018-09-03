//
// Created by 11956 on 2018/9/3.
//

#ifndef MONDIS_MONDISDATA_H
#define MONDIS_MONDISDATA_H

#include <string>

class MondisData {
protected:
    std::string *json=new std::string("");
    bool hasSerialized = false;
    virtual void toJson() = 0;
public:
     std::string* getJson() {
        if(hasSerialized) {
            return json;
        }
        toJson();
        hasSerialized  = true;
        return json;
    }
    virtual ~MondisData() {
         delete json;
     };

};


#endif //MONDIS_MONDISDATA_H
