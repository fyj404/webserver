#pragma once
#include <string>
#include "Macros.h"

class Buffer{
public:
    Buffer()=default;
    ~Buffer()=default;
    DISALLOW_COPY_AND_MOVE(Buffer);
    void Append(const char *_str,const int size);
    ssize_t Size()const;
    const char* ToStr()const;
    void Clear();
    void Getline();
    void SetBuf(const char *buf);
private:
    std::string buf_;
    
};