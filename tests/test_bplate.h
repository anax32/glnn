#ifndef TEST_BPLATE_H
#define TEST_BPLATE_H

class test_set_t
{
public:
    typedef std::function<bool(void)>       test_t;     // test function and result
    typedef std::map<std::string, test_t>   tests_t;    // test batch
    typedef std::map<std::string, bool>     results_t;

    tests_t         tests;
    results_t       results;

    void run ()
    {
        results.clear();

        for (auto& t : tests)
        {
            results[t.first] = t.second();
        }
    }

    void report () const
    {
        for (auto& a : results)
        {
            std::cout << a.first.c_str()
                << " : "
                << ((a.second) ? "pass" : "fail")
                << std::endl;
        }
    }

    auto fail_count () const -> int
    {
        return std::count_if(
            std::begin(results),
            std::end(results),
            [](const std::pair<std::string, bool>& p)
        {
            return p.second != true;
        });
    }
};
#endif