#include "log.h"
Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    //如果有正在写入的线程，等待所有日志写入完成。
    if(writeThread_ && writeThread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        };
        deque_->Close();
        writeThread_->join();
    }
    //关闭队列和文件指针。
    if(fp_) { 
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}
//接受日志级别、文件路径、后缀和最大队列大小。
void Log::init(int level = 1, const char* path, const char* suffix,
    int maxQueueSize) {
    isOpen_ = true;
    level_ = level;
    //根据 maxQueueSize 判断是否启用异步写入
    if(maxQueueSize > 0) {
        isAsync_ = true;
        if(!deque_) {
            //如果启用异步写入，则创建一个 BlockDeque 队列和一个线程，用于写日志。
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = move(newDeque);
            
            std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    //创建文件名，根据当前日期生成。
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        //使用 std::lock_guard 确保对文件指针的访问是线程安全的，并处理文件的打开和关闭。
        std::lock_guard<std::mutex> locker(mtx_);
        ////RetrieveAll(): 重置缓冲区，将读写位置归零。
        buff_.RetrieveAll();
        if(fp_) { 
            flush();
            fclose(fp_); 
        }

        fp_ = fopen(fileName, "a");
        if(fp_ == nullptr) {
            //0777表示新创建目录的权限设置
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        } 
        assert(fp_ != nullptr);
    }
}


void Log::write(int level, const char *format, ...) {
    
    struct timeval now = {0, 0};
    //gettimeofday(&now, nullptr); 获取当前时间，精确到微秒。now 包含两个值：秒数（tv_sec）和微秒数（tv_usec）。
    gettimeofday(&now, nullptr);
    //localtime(&tSec) 将秒数转换为可读的时间格式，并保存在 struct tm 中，包含年月日时分秒等信息。
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    //如果当前日志的日期与当前系统日期不符（toDay_ != t.tm_mday），则意味着进入了新的一天，需要创建一个新日志文件。
    //如果日志行数达到上限 MAX_LINES（lineCount_ % MAX_LINES == 0），则同样需要新建一个文件。
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_  %  MAX_LINES == 0)))
    {
        //使用 unique_lock<mutex> 保证线程安全，但在之后解锁（locker.unlock();）来执行不需要锁保护的操作。
        //这段可能可以给移动到下面
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday)
        {
            //如果是新的一天（toDay_ != t.tm_mday），生成文件名为 path/YYYY_MM_DD。
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            //如果日志行数达到上限，则文件名后面会加上一个递增的数字（例如 YYYY_MM_DD-1）
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }
        
        //重新加锁，确保线程安全。
        //刷新缓冲区并关闭当前日志文件，然后打开一个新文件用于后续日志写入。
        locker.lock();
        flush();
        //关闭旧的
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {

        std::unique_lock<std::mutex> locker(mtx_);
        lineCount_++;
        //生成时间戳并格式化为 YYYY-MM-DD HH:MM:SS.microsecond，写入到缓冲区 buff_ 中。
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        //增加日志行数，并调用 AppendLogLevelTitle_ 追加日志级别的标识。
        buff_.HasWritten(n);

        AppendLogLevelTitle_(level);

        //使用 va_list 和 vsnprintf 实现可变参数的日志写入（即支持类似 printf 的格式化输入）
        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);
        //将日志内容追加到缓冲区，并在最后追加换行符和字符串终止符。
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        //异步写入：如果启用了异步写入（isAsync_ 为真），且队列不满，则将日志信息推送到日志队列中，供后台线程处理。
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {
            //同步写入：否则，直接将日志内容写入文件。
            fputs(buff_.Peek(), fp_);
        }
        //最后，清空缓冲区 buff_，以便下一条日志的写入。
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if(isAsync_) { 
        //如果使用异步模式，日志数据会首先存储在一个队列（deque_）中，然后由后台线程写入文件。
        //调用 deque_->flush() 会强制将队列中的所有日志立即写入文件。
        //这是为了确保在异步情况下，后台线程没有漏掉日志信息，所有排队的日志数据都能被写入。
        deque_->flush(); 
    }
    //fflush(fp_)：fflush 是标准库函数，它用于强制将文件流 
    //fp_ 的缓冲区中的内容写入到日志文件中。这一步适用于同步和异步两种模式，
    //保证即使在缓存满之前，也可以通过手动调用 flush() 确保数据立即被写入文件。
    fflush(fp_);
}

//这是一个私有函数，用于从日志队列中异步地提取日志字符串并将其写入文件。
void Log::AsyncWrite_() {
    std::string str = "";
    while(deque_->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}
//Instance(): 这是一个静态方法，返回日志类的单例实例。
//使用单例模式可以确保程序中只有一个 Log 实例，从而避免多个实例导致的资源竞争和冲突。
Log* Log::Instance() {
    static Log inst;
    return &inst;
}
//FlushLogThread(): 这是一个静态方法，通常用于在单独的线程中调用，以执行异步写入操作。
//Log::Instance()->AsyncWrite_();: 通过 Instance() 方法获取日志的单例实例，
//并调用 AsyncWrite_() 方法。这使得后台线程可以从日志队列中读取并写入日志数据。
void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}