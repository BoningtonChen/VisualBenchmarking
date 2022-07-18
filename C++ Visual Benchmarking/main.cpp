#include <iostream>
#include <utility>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>

// TODO struct def
struct ProfileResult
{
    std::string name;
    long long Start, End;
};

struct InstrumentationSession
{
    std::string Name;

    explicit InstrumentationSession(std::string  name)
            : Name(std::move(name))
    {}
};

// TODO class def
class Instrumentor
{
private:
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;
    int m_ProfileCount;

public:
    Instrumentor()
            :m_CurrentSession(nullptr), m_ProfileCount(0)
    {}

    void BeginSession(const std::string& name, const std::string& filepath = "results.json")
    {
        m_OutputStream.open(filepath);
        WriteHeader();
        m_CurrentSession = new InstrumentationSession( name );
    }

    void EndSession()
    {
        WriteFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
        m_ProfileCount = 0;
    }

    void WriteProfile(const ProfileResult& result)
    {
        if(m_ProfileCount++ > 0)
        {
            m_OutputStream << ",";
        }

        std::string name = result.name;
        std::replace(name.begin(), name.end(), '"', '\'');

        m_OutputStream << "{";
        m_OutputStream << R"("cat":"function",)";
        m_OutputStream << R"("dur":)" << (result.End - result.Start) << ',';
        m_OutputStream << R"("name":")" << name << "\",";
        m_OutputStream << R"("ph":"x",)";
        m_OutputStream << R"("pid":0,)";
        m_OutputStream << R"("tid":0,)";
        m_OutputStream << R"("ts":)" << result.Start;
        m_OutputStream <<"}";
    }

    void WriteHeader()
    {
        m_OutputStream << R"({"otherData":{},"traceEvents":[)";
        m_OutputStream.flush();
    }

    void WriteFooter()
    {
        m_OutputStream << "]}";
        m_OutputStream.flush();
    }

    static Instrumentor& Get()
    {
        static auto* instance = new Instrumentor();
        return *instance;
    }

};

class InstrumentationTimer
{
private:
    const char* m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimePoint;
    bool m_Stopped;

public:
    explicit InstrumentationTimer(const char* name)
        : m_Name(name), m_Stopped(false)
    {
        m_StartTimePoint = std::chrono::high_resolution_clock::now();
    }

    void Stop()
    {
        auto EndTimePoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(EndTimePoint).time_since_epoch().count();

        Instrumentor::Get().WriteProfile({m_Name, start,end});
        m_Stopped = true;
    }

    ~InstrumentationTimer()
    {
        if (!m_Stopped)
        {
            Stop();
        }
    }
};

// TODO func def
void OutPut1(const std::vector<int>& array)
{
    InstrumentationTimer timer("Output1");

    for (int value : array)
    {
        std::cout << "Hello for " << value+1 << " times. :D" << std::endl;
    }
}
void OutPut2(const std::vector<int>& array)
{
    InstrumentationTimer timer("Output2");

    for (int value : array)
    {
        std::cout << "Hello for " << sqrt(value+1) << " times. :D" << std::endl;
    }
}

// TODO main func
int main()
{
    std::vector<int> array;
    for (int i=0; i<1000; i++)
    {
        array.emplace_back(i);
    }

    Instrumentor::Get().BeginSession("Profile");
    OutPut1(array);
    OutPut2(array);
    Instrumentor::Get().EndSession();

    std::cin.get();
    return 0;
}
