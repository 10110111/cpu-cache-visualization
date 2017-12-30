#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <QImage>

class Cache
{
public:
    using size_t=std::size_t;
    static constexpr quint32 absent=0xfffafafa;
    static constexpr quint32 present=0xffbbffbb;
    static constexpr quint32 justRead=0xffff7700;
    static constexpr quint32 hit=0xff00ff00;
    static constexpr quint32 miss=0xffff0000;
private:
    struct Line
    {
        int useTime=0;
        bool used=false;
        size_t startAddress; // start address of the current block of RAM mapped to this line
    };

    int hitCount=0, missCount=0;
    int time=0;
    quint32* data;
    size_t dataSize;
    std::vector<Line> lines;
    size_t lineSize;
    size_t setCount;
    size_t wayCount;

    void initStateBeforeRead()
    {
        for(size_t address=0;address<dataSize;++address)
        {
            if(data[address]==justRead || data[address]==hit || data[address]==miss)
                data[address]=present;
        }
    }
    void markLineInData(Line const& line, quint32 state)
    {
        for(size_t i=0; i<lineSize && line.startAddress+i<dataSize; ++i)
            data[line.startAddress+i]=state;
    }
    void readLine(Line& line, size_t address, size_t readSize)
    {
        ++missCount;
        const auto startAddress=address/lineSize*lineSize;
        line.used=true;
        line.useTime=++time;
        line.startAddress=startAddress;
        markLineInData(line, justRead);
        std::fill(data+address,data+address+readSize,quint32(miss));
    }
    void evictLine(Line& line)
    {
        markLineInData(line, absent);
        line.used=false;
        line.useTime=0;
        line.startAddress=0;
    }
    Line& chooseLineToEvict(size_t set)
    {
        Line* lineToEvict=&lines[set];
        for(size_t way=1;way<wayCount;++way)
        {
            auto& line=lines[set+way*setCount];
            if(line.useTime<lineToEvict->useTime)
                lineToEvict=&line;
        }
        return *lineToEvict;
    }
public:
    Cache(quint32* data, size_t dataSize, size_t lineSize, size_t setCount, size_t wayCount)
        : data(data)
        , dataSize(dataSize)
        , lines(setCount*wayCount)
        , lineSize(lineSize)
        , setCount(setCount)
        , wayCount(wayCount)
    {
        std::fill(data,data+dataSize,quint32(absent));
    }
    void memRead(const size_t address, const size_t readSize)
    {
        initStateBeforeRead();

        const auto set=address/lineSize%setCount;
        for(size_t way=0;way<wayCount;++way)
        {
            auto& line=lines[set+way*setCount];
            if(line.used && line.startAddress==address/lineSize*lineSize)
            {
                ++hitCount;
                std::fill(data+address,data+address+readSize,quint32(hit));
                return;
            }
            if(!line.used)
            {
                readLine(line, address, readSize);
                return;
            }
        }

        auto& lineToEvict=chooseLineToEvict(set);
        evictLine(lineToEvict);
        readLine(lineToEvict, address, readSize);
    }
    void memWrite(size_t address, size_t writeSize)
    {
        // FIXME: do we need to do anything here?
    }

    int getHitCount() const { return hitCount; }
    int getMissCount() const { return missCount; }
};

int usage(const char* argv0, int ret)
{
    std::cerr << "Usage: " << argv0 << " [options]...\n";
    std::cerr << "Options:\n"
                 " --msize matrixSideSize   size of square matrix side (default 64)\n"
                 " --scale imageScale       scale image by a factor (default 4)\n"
                 " --files NAME_TEMPLATE    file name template. Must have \"%1\" placeholder for\n"
                 "                          matrix side size and \"%2\" for frame index]\n"
                 " --sets N                 number of sets in the simulated cache\n"
                 " --ways N                 associativity of the simulated cache\n"
                 " --line N                 line size of the simulated cache\n"
                 " --stat-only              only calculate hit/miss statistics\n"
                 ;
    std::cerr << "\n";
    return ret;
}

int needsParams(std::string const& arg, int ret)
{
    std::cerr << "Option " << arg << " requires parameter\n";
    return ret;
}

int main(int argc, char** argv)
{
    unsigned long matrixSide=64;
    unsigned long imageScale=4;
    size_t lineSize=64;
    size_t setCount=32;
    size_t wayCount=4;
    bool printStatsAndQuit=false;
    QString fileNameTemplate="/tmp/mat%1-%2.png";
    try
    {
        for(int i=1;i<argc;++i)
        {
            const std::string arg(argv[i]);
            if(arg=="--msize")
            {
                if(i+1==argc) return needsParams(arg,1);
                matrixSide=std::stoul(argv[++i]);
            }
            else if(arg=="--scale")
            {
                if(i+1==argc) return needsParams(arg,1);
                imageScale=std::stoul(argv[++i]);
            }
            else if(arg=="--files")
            {
                if(i+1==argc) return needsParams(arg,1);
                fileNameTemplate=argv[++i];
                if(!fileNameTemplate.contains("%1") || !fileNameTemplate.contains("%2"))
                {
                    std::cerr << "Filename template must contain \"%1\" and \"%2\" placeholders\n";
                    return 1;
                }
            }
            else if(arg=="--line")
            {
                if(i+1==argc) return needsParams(arg,1);
                lineSize=std::stoul(argv[++i]);
            }
            else if(arg=="--sets")
            {
                if(i+1==argc) return needsParams(arg,1);
                setCount=std::stoul(argv[++i]);
            }
            else if(arg=="--ways")
            {
                if(i+1==argc) return needsParams(arg,1);
                wayCount=std::stoul(argv[++i]);
            }
            else if(arg=="--stats-only")
            {
                printStatsAndQuit=true;
            }
            else if(arg=="--help")
            {
                return usage(argv[0], 0);
            }
            else
            {
                std::cerr << "Unknown option " << arg << "\n";
                return 1;
            }
        }
    }
    catch(std::exception& ex)
    {
        return usage(argv[0], 2);
    }
    constexpr auto matrixElementSize=4;

    std::vector<quint32> data(matrixSide*matrixSide*matrixElementSize);
    QImage result(reinterpret_cast<uchar*>(data.data()), matrixSide*matrixElementSize, matrixSide, QImage::Format_RGB32);
    const QVector<QRgb> colorTable={Cache::absent,Cache::present,Cache::justRead,Cache::hit,Cache::miss};

    Cache cache(data.data(), matrixSide*matrixSide*matrixElementSize, lineSize, setCount, wayCount);

    int frame=0;
    auto saveFrame=[printStatsAndQuit,matrixSide,imageScale,&fileNameTemplate,&frame,&result,&colorTable]
    {
        if(printStatsAndQuit) return;
        result.scaled(matrixSide*imageScale, matrixSide*imageScale).convertToFormat(QImage::Format_Indexed8,colorTable).save(QString(fileNameTemplate).arg(matrixSide).arg(frame++,6,10,QChar('0')));
        std::cerr << "\rSaved frame " << frame;
    };
    saveFrame();
    for(int i=0; i<matrixSide; i++)
    {
        for(int j=0; j<matrixSide; j++)
        {
            cache.memRead ((i*matrixSide+j)*matrixElementSize, 4);
            saveFrame();
            cache.memRead ((j*matrixSide+i)*matrixElementSize, 4);
            saveFrame();
            /*cache.memWrite((i*matrixSide+j)*matrixElementSize, 4);
            cache.memWrite((j*matrixSide+i)*matrixElementSize, 4);*/
        }
    }
    if(!printStatsAndQuit)
        std::cerr << "\n";
    else
    {
        const auto hits=cache.getHitCount();
        const auto misses=cache.getMissCount();
        std::cout << "Hits  : " << hits << "\n"
                     "Misses: " << misses << " (" << 100.*misses/(misses+hits) << "% of all reads)\n";
    }
}
