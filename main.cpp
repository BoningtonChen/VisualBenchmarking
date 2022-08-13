#include <iostream>
#include <utility>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <thread>

// struct def
struct ProfileResult
{
    std::string name;
    long long Start;
    long long End;
    uint32_t ThreadID;
};

struct InstrumentationSession
{
    std::string Name;

    explicit InstrumentationSession(std::string  name)
            : Name(std::move(name))
    {}
};

// class def
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
        m_OutputStream << R"("name":")" << name << R"(",)";
        m_OutputStream << R"("ph":"X",)";
        m_OutputStream << R"("pid":0,)";
        m_OutputStream << R"("tid":)" << result.ThreadID << R"(,)";
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

        uint32_t ThreadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
        Instrumentor::Get().WriteProfile({m_Name, start,end, ThreadID });
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

// Macro mark plugin
#define PROFILING 1
#if PROFILING
#define PROFILE_SCOPE(name) InstrumentationTimer timer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#define PROFILE_SCOPE(name)
#endif

namespace Benchmark{
    // func def
    void OutPut1(const std::vector<int>& vector)
    {
        PROFILE_FUNCTION();

        for (int value : vector)
        {
            std::cout << "Hello for " << value + 1 << " times. :D" << std::endl;
        }
    }

    void OutPut2(const std::vector<int>& vector)
    {
        PROFILE_FUNCTION();

        for (int value : vector)
        {
            std::cout << "Hello for " << sqrt(value + 1 ) << " times. :D" << std::endl;
        }
    }

    void RunBenchmark()
    {
        PROFILE_FUNCTION();
        std::cout << "Running Benchmarking..." << std::endl;

        // function emplace back zone!!! The following is just an example!!!
        std::thread a([]() {
            std::vector<int> l_array;
            for (int i=0; i<=1000; i++)
            {
                l_array.emplace_back(i);
            }
            OutPut1(l_array);
        });

        std::thread b([]() {
            std::vector<int> l_array;
            for (int i=0; i<=1000; i++)
            {
                l_array.emplace_back(i);
            }
            OutPut2(l_array);
        });

        a.join();
        b.join();

    }
}


// main func
int main()
{
    // variable def
    std::vector<int> array;

    for (int i=0; i<1000; i++)
    {
        array.emplace_back(i);
    }

    Instrumentor::Get().BeginSession("Profile");

    // main function to use benchmark namespace to do benchmarking as an example is as follows:
    Benchmark::OutPut1(array);
    Benchmark::OutPut2(array);
    Benchmark::RunBenchmark();

    Instrumentor::Get().EndSession();

    std::cin.get();
    return 0;
}
