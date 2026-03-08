#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <set>
#include <climits>

using namespace std;

struct Task {
    int id;
    long long r;
    long long p;
    int size;
    bool valid;
};

struct Scheduled {
    int id;
    long long start;
    long long finish;
    vector<int> procs;
};

struct RunningJob {
    long long finish;
    int task_index;
    vector<int> procs;
    bool operator>(const RunningJob& o) const { return finish > o.finish; }
};

bool parse_swf_tokens(const vector<string>& tokens, Task &t) {
    if (tokens.size() < 5) return false;
    try {
        int id = stoi(tokens[0]);
        long long r = stoll(tokens[1]);
        long long p = stoll(tokens[3]);
        int size = stoi(tokens[4]);
        if (p <= 0 || size <= 0) return false;
        if (tokens[0] == "-1" || tokens[1] == "-1" ||
            tokens[3] == "-1" || tokens[4] == "-1") return false;

        t.id = id;
        t.r = r;
        t.p = p;
        t.size = size;
        t.valid = true;
        return true;
    } catch (...) {
        return false;
    }
}

vector<string> split_ws(const string &line) {
    vector<string> out;
    stringstream ss(line);
    string tok;
    while (ss >> tok) out.push_back(tok);
    return out;
}

vector<Scheduled> construct_solution_greedy(const vector<Task>& tasks_input, int MaxProcs) {
    int M = max(1, MaxProcs);
    int T = (int)tasks_input.size();
    vector<bool> scheduled(T, false);
    int remaining = T;

    // free processors as vector
    vector<int> free_procs;
    free_procs.reserve(M);
    for (int i = 0; i < M; ++i) free_procs.push_back(M - 1 - i);

    priority_queue<RunningJob, vector<RunningJob>, greater<RunningJob>> running;

    // prepare arrivals sorted by r
    vector<int> order(T);
    for (int i = 0; i < T; ++i) order[i] = i;
    sort(order.begin(), order.end(), [&](int a, int b){
        if (tasks_input[a].r != tasks_input[b].r) return tasks_input[a].r < tasks_input[b].r;
        return tasks_input[a].p < tasks_input[b].p;
    });

    size_t idx = 0; // next arrival index

    // ready multiset ordered by p (SPT) then by size then index
    struct ReadyItem { long long p; int size; int idx; int id; };
    struct ReadyCmp { bool operator()(ReadyItem const &a, ReadyItem const &b) const {
        if (a.p != b.p) return a.p < b.p;
        if (a.size != b.size) return a.size < b.size;
        return a.idx < b.idx;
    }};
    multiset<ReadyItem, ReadyCmp> ready;

    vector<Scheduled> schedule;
    schedule.reserve(T);

    long long t = 0;
    if (T > 0) t = tasks_input[order[0]].r;

    const long long INF = LLONG_MAX / 4;

    while (remaining > 0) {
        // release finished jobs
        while (!running.empty() && running.top().finish <= t) {
            auto rj = running.top(); running.pop();
            for (int p : rj.procs) free_procs.push_back(p);
        }

        // add arrivals to ready
        while (idx < order.size() && tasks_input[order[idx]].r <= t) {
            int i = order[idx++];
            int sz = tasks_input[i].size;
            ready.insert(ReadyItem{tasks_input[i].p, sz, i, tasks_input[i].id});
        }

        if (ready.empty()) {
            // advance to next event
            long long next_t = INF;
            if (!running.empty()) next_t = min(next_t, running.top().finish);
            if (idx < order.size()) next_t = min(next_t, tasks_input[order[idx]].r);
            if (next_t == INF) break;
            t = next_t;
            continue;
        }

        // find first ready item that fits available procs
        auto it = ready.begin();
        while (it != ready.end() && it->size > (int)free_procs.size()) ++it;
        if (it == ready.end()) {
            long long next_t = INF;
            if (!running.empty()) next_t = min(next_t, running.top().finish);
            if (idx < order.size()) next_t = min(next_t, tasks_input[order[idx]].r);
            if (next_t == INF) break;
            t = next_t;
            continue;
        }

        ReadyItem ri = *it;
        ready.erase(it);
        int task_index = ri.idx;

        vector<int> assigned;
        assigned.reserve(ri.size);
        for (int k = 0; k < ri.size; ++k) {
            assigned.push_back(free_procs.back());
            free_procs.pop_back();
        }

        Scheduled s;
        s.id = ri.id;
        s.start = t;
        s.finish = t + ri.p;
        s.procs = assigned;
        schedule.push_back(s);

        scheduled[task_index] = true;
        remaining--;

        RunningJob rj;
        rj.finish = s.finish;
        rj.task_index = task_index;
        rj.procs = s.procs;
        running.push(rj);

        if (free_procs.empty() && !running.empty()) {
            t = running.top().finish;
        }
    }

    return schedule;
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc<3){
        cerr<<"Usage: "<<argv[0]<<" file.swf N\n";
        return 1;
    }

    string filename = argv[1];
    int N = stoi(argv[2]);

    ifstream fin(filename);
    if(!fin){
        cerr<<"Cannot open file: "<<filename<<"\n";
        return 2;
    }

    int MaxProcs=-1;
    string line;
    vector<Task> tasks_all;
    int read_records=0;

    while(getline(fin,line) && read_records<N){
        size_t pos = line.find_first_not_of(" \t\r\n");
        if(pos==string::npos) continue;

        if (line[pos] == ';') {
            size_t i = pos + 1;
            while (i < line.size() && isspace(line[i])) {
                i++;
            }
            const string tag = "MaxProcs:";
            if (line.compare(i, tag.size(), tag) == 0) {
                string tmp = line.substr(i + tag.size());
                stringstream ss(tmp);
                ss >> MaxProcs;
            }
            continue;
        }
        
        auto toks = split_ws(line);
        if(toks.empty()) continue;

        Task t;
        if(parse_swf_tokens(toks,t)) {
            tasks_all.push_back(t);
            read_records++;
        }
    }
    fin.close();
    if(tasks_all.empty()) return 0;

    auto schedule = construct_solution_greedy(tasks_all, MaxProcs);

    ofstream fout("output_greedy.swf");
    if (!fout) {
        cerr << "Cannot create file: output_greedy.swf\n";
        return 1;
    }
    for(auto &s:schedule){
        fout<<s.id<<" "<<s.start<<" "<<s.finish;
        for(int p:s.procs) fout<<" "<<p;
        fout<<"\n";
    }
    fout.close();
    return 0;
}