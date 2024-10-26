#include "httprequest.h"
using namespace std;

//DEFAULT_HTML: 定义一个常量集合，包含了一些默认的 HTML 页面路径，用于处理 HTTP 请求。
//DEFAULT_HTML_TAG: 定义一个映射，将特定的 HTML 文件路径与整数标识符关联，可能用于快速查找或处理请求。
const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

//method_, path_, version_, body_ 被设置为空字符串。
//state_ 被设置为 REQUEST_LINE，表示当前解析状态为请求行。
//清空 header_ 和 post_，准备接收新的请求数据。
void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

//IsKeepAlive(): 检查 HTTP 请求是否应该保持连接。
//首先检查请求头中是否包含 "Connection" 字段。
//如果包含且其值为 "keep-alive"，并且版本为 "1.1"，则返回 true，表示连接应保持活跃。
//否则返回 false。
bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.ReadableBytes() <= 0) {
        return false;
    }
    while(buff.ReadableBytes() && state_ != FINISH) {
        //当缓冲区有可读字节且状态不是 FINISH 时持续解析。
        //查找当前缓冲区中行结束符的位置。
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.Peek(), lineEnd);
        //创建当前行的字符串
        switch(state_)
        {
        case REQUEST_LINE:
            //REQUEST_LINE: 调用 ParseRequestLine_() 解析请求行，如果失败返回 false。然后调用 ParsePath_() 解析请求路径。
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();
            break;    
        case HEADERS:
            //HEADERS: 调用 ParseHeader_() 解析请求头。如果缓冲区可读字节小于或等于2（表示请求头结束），则状态设置为 FINISH。
            ParseHeader_(line);
            if(buff.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            //BODY: 调用 ParseBody_() 解析请求体。
            ParseBody_(line);
            break;
        default:
            break;
        }
        //if(lineEnd == buff.BeginWrite()) { break; }: 如果找不到行结束符，退出循环
        if(lineEnd == buff.BeginWrite()) { break; }
        //从缓冲区中移除已解析的行及行结束符
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}