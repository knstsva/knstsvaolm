#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

using namespace std;

struct TestCase
{
    vector<pair<int, int>> wallet;
    int amount;
    string strategy;
};

class Parser
{
private:
    string s;
    size_t pos = 0;

    void skip()
    {
        while (pos < s.size() && isspace((unsigned char)s[pos]))
            ++pos;
    }

    void expect(char c)
    {
        skip();

        if (pos >= s.size() || s[pos] != c)
            throw runtime_error("parse error");

        ++pos;
    }

    string parseString()
    {
        skip();
        expect('"');

        string res;

        while (pos < s.size() && s[pos] != '"')
            res += s[pos++];

        expect('"');

        return res;
    }

    int parseInt()
    {
        skip();

        if (pos >= s.size() || !isdigit((unsigned char)s[pos]))
            throw runtime_error("parse error");

        int value = 0;

        while (pos < s.size() && isdigit((unsigned char)s[pos]))
        {
            value = value * 10 + (s[pos] - '0');
            ++pos;
        }

        return value;
    }

public:
    Parser(const string& text)
        : s(text)
    {
    }

    vector<TestCase> parse()
    {
        vector<TestCase> tests;

        skip();
        expect('[');

        skip();

        while (pos < s.size() && s[pos] != ']')
        {
            TestCase tc;

            expect('{');

            while (true)
            {
                string key = parseString();

                expect(':');

                if (key == "wallet")
                {
                    expect('[');

                    skip();

                    while (pos < s.size() && s[pos] != ']')
                    {
                        expect('[');

                        int nominal = parseInt();

                        expect(',');

                        int count = parseInt();

                        expect(']');

                        tc.wallet.push_back({nominal, count});

                        skip();

                        if (pos < s.size() && s[pos] == ',')
                        {
                            ++pos;
                            skip();
                        }
                    }

                    expect(']');
                }
                else if (key == "amount")
                {
                    tc.amount = parseInt();
                }
                else if (key == "strategy")
                {
                    tc.strategy = parseString();
                }
                else
                {
                    throw runtime_error("unknown key");
                }

                skip();

                if (pos < s.size() && s[pos] == ',')
                {
                    ++pos;
                    continue;
                }

                break;
            }

            expect('}');

            tests.push_back(tc);

            skip();

            if (pos < s.size() && s[pos] == ',')
            {
                ++pos;
                skip();
            }
        }

        expect(']');

        return tests;
    }
};

struct Item
{
    int weight;
    int nominal;
    int count;
};

struct State
{
    bool reachable = false;
    int priority = -1;
    int prevSum = -1;
    int itemIndex = -1;
};

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ifstream fin("input.json");

    if (!fin.is_open())
        return 1;

    string jsonText(
        (istreambuf_iterator<char>(fin)),
        istreambuf_iterator<char>());

    vector<TestCase> tests;

    try
    {
        Parser parser(jsonText);
        tests = parser.parse();
    }
    catch (...)
    {
        return 1;
    }

    ofstream fout("output.json");

    if (!fout.is_open())
        return 1;

    fout << "[\n";

    for (size_t tcIndex = 0; tcIndex < tests.size(); ++tcIndex)
    {
        const TestCase& tc = tests[tcIndex];

        int targetNominal;

        if (tc.strategy == "MAX")
        {
            targetNominal = max_element(
                tc.wallet.begin(),
                tc.wallet.end(),
                [](const auto& a, const auto& b)
                {
                    return a.first < b.first;
                })->first;
        }
        else
        {
            targetNominal = min_element(
                tc.wallet.begin(),
                tc.wallet.end(),
                [](const auto& a, const auto& b)
                {
                    return a.first < b.first;
                })->first;
        }

        vector<Item> items;

        for (const auto& p : tc.wallet)
        {
            int nominal = p.first;
            int count = p.second;

            int k = 1;

            while (count > 0)
            {
                int take = min(k, count);

                items.push_back(
                {
                    nominal * take,
                    nominal,
                    take
                });

                count -= take;
                k <<= 1;
            }
        }

        vector<State> dp(tc.amount + 1);

        dp[0].reachable = true;
        dp[0].priority = 0;

        for (int i = 0; i < (int)items.size(); ++i)
        {
            int w = items[i].weight;

            int bonus = (items[i].nominal == targetNominal)
                ? items[i].count
                : 0;

            for (int sum = tc.amount; sum >= w; --sum)
            {
                if (!dp[sum - w].reachable)
                    continue;

                int candidate = dp[sum - w].priority + bonus;

                if (!dp[sum].reachable || candidate > dp[sum].priority)
                {
                    dp[sum].reachable = true;
                    dp[sum].priority = candidate;
                    dp[sum].prevSum = sum - w;
                    dp[sum].itemIndex = i;
                }
            }
        }

        fout << "  {\n";
        fout << "    \"dispense\": ";

        if (!dp[tc.amount].reachable)
        {
            fout << "[]\n";
        }
        else
        {
            unordered_map<int, int> used;

            int cur = tc.amount;

            while (cur > 0)
            {
                const State& st = dp[cur];
                const Item& it = items[st.itemIndex];

                used[it.nominal] += it.count;

                cur = st.prevSum;
            }

            vector<pair<int, int>> answer(
                used.begin(),
                used.end());

            sort(
                answer.begin(),
                answer.end(),
                [](const auto& a, const auto& b)
                {
                    return a.first < b.first;
                });

            fout << "[";

            for (size_t i = 0; i < answer.size(); ++i)
            {
                if (i)
                    fout << ",";

                fout << "[" << answer[i].first
                     << "," << answer[i].second
                     << "]";
            }

            fout << "]\n";
        }

        fout << "  }";

        if (tcIndex + 1 != tests.size())
            fout << ",";

        fout << "\n";
    }

    fout << "]\n";

    return 0;
}