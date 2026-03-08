#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <set>
#include <climits>
#include <cmath>
#include <random>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <numeric>

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

struct ReadyItem {
    double priority;
    long long p;
    int size;
    int idx;
    int id;
};

struct ReadyCmp {
    bool operator()(ReadyItem const &a, ReadyItem const &b) const {
        if (a.priority < b.priority) return true;
        if (a.priority > b.priority) return false;
        return a.idx < b.idx;
    }
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

long long simulate(const vector<Task>& tasks_input, int MaxProcs, const vector<double>& priorities, vector<Scheduled>* out_schedule) {
    int M = max(1, MaxProcs);
    int T = (int)tasks_input.size();
   
    vector<int> free_procs;
    free_procs.reserve(M);
    for (int i = 0; i < M; ++i) free_procs.push_back(M - 1 - i);

    priority_queue<RunningJob, vector<RunningJob>, greater<RunningJob>> running;

    vector<int> order(T);
    for (int i = 0; i < T; ++i) order[i] = i;
    sort(order.begin(), order.end(), [&](int a, int b){
        if (tasks_input[a].r != tasks_input[b].r) return tasks_input[a].r < tasks_input[b].r;
        return tasks_input[a].p < tasks_input[b].p;
    });

    size_t idx = 0;
    multiset<ReadyItem, ReadyCmp> ready;

    if (out_schedule) {
        out_schedule->clear();
        out_schedule->reserve(T);
    }

    long long current_time = 0;
    if (T > 0) current_time = tasks_input[order[0]].r;
    long long max_finish_time = 0;
    int remaining = T;

    const long long INF = LLONG_MAX / 4;

    while (remaining > 0) {
        // Free up procs from completed tasks
        while (!running.empty() && running.top().finish <= current_time) {
            auto rj = running.top(); running.pop();
            for (int p : rj.procs) free_procs.push_back(p);
        }

        // Add tasks to ready queue
        while (idx < order.size() && tasks_input[order[idx]].r <= current_time) {
            int i = order[idx++];
            int sz = tasks_input[i].size;
            ready.insert(ReadyItem{priorities[i], tasks_input[i].p, sz, i, tasks_input[i].id});
        }

        if (ready.empty()) {
            long long next_t = INF;
            if (!running.empty()) next_t = min(next_t, running.top().finish);
            if (idx < order.size()) next_t = min(next_t, tasks_input[order[idx]].r);
            if (next_t == INF) break;
            current_time = next_t;
            continue;
        }

        // Find the first task that fits within the available processors
        auto it = ready.begin();
        while (it != ready.end() && it->size > (int)free_procs.size()) ++it;

        if (it == ready.end()) {
            long long next_t = INF;
            if (!running.empty()) next_t = min(next_t, running.top().finish);
            if (idx < order.size()) next_t = min(next_t, tasks_input[order[idx]].r);
            if (next_t == INF) break;
            current_time = next_t;
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

        long long finish_time = current_time + ri.p;
        if (finish_time > max_finish_time) max_finish_time = finish_time;

        if (out_schedule) {
            Scheduled s;
            s.id = ri.id;
            s.start = current_time;
            s.finish = finish_time;
            s.procs = assigned;
            out_schedule->push_back(s);
        }

        remaining--;

        RunningJob rj;
        rj.finish = finish_time;
        rj.task_index = task_index;
        rj.procs = assigned;
        running.push(rj);

        if (free_procs.empty() && !running.empty()) {
            current_time = running.top().finish;
        }
    }

    return max_finish_time;
}

vector<double> greedy_priorities_baseline(const vector<Task>& tasks) {
    int n = tasks.size();
    vector<int> order(n);
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b){
        if (tasks[a].p != tasks[b].p) return tasks[a].p < tasks[b].p;
        if (tasks[a].r != tasks[b].r) return tasks[a].r < tasks[b].r;
        return a < b;
    });
    vector<double> pr(n);
    for (int i = 0; i < n; ++i) {
        pr[order[i]] = (n==1?0.0:double(i) / double(n-1));
    }
    return pr;
}

vector<Scheduled> solve_sa(const vector<Task>& tasks, int MaxProcs) {
    int n = (int)tasks.size();
    if (n == 0) return {};

    const double TIME_LIMIT = 299.0;
    const int MAX_ITER = 100000;    
    const double T_END = 1e-9;      

    auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
    mt19937_64 rng(seed);
    uniform_real_distribution<double> ur(0.0, 1.0);
    uniform_int_distribution<int> idx_dist(0, max(0, n-1));

    auto time_start = chrono::high_resolution_clock::now();

    //Greedy start
    vector<double> priorities = greedy_priorities_baseline(tasks);
    long long current_cost = simulate(tasks, MaxProcs, priorities, nullptr);
    vector<double> best_prior = priorities;
    long long best_cost = current_cost;

    double T = 100000;
    double T_start = T;
    double alpha = 0.995;
   
    int no_improve_count = 0;
    long long last_improve_cost = best_cost;
    int iter = 0;
    // SA main loop
    while (T >= T_END) {
        iter++;
        double elapsed = chrono::duration<double>(chrono::high_resolution_clock::now() - time_start).count();
        if (elapsed > TIME_LIMIT) {
        
            break;
        }

        double swap_prob = 0.625;
        if (no_improve_count > 500) swap_prob += 0.1;

        // propose neighbor
        vector<double> neigh = priorities;
        double r = ur(rng);
        if (n >= 2 && r < swap_prob) {
            // Strong move: swap two indices
            int i = idx_dist(rng);
            int j = idx_dist(rng);
            while (i == j && n > 1) j = idx_dist(rng);
            swap(neigh[i], neigh[j]);
        } else if (r < 0.95) {
            //scale-based perturbation
            int i = idx_dist(rng);
            double progress = T / T_start;
            double scale = 0.01 + progress;
            double d = (ur(rng) - 0.5) * 2.0 * scale;
            neigh[i] += d;
            if (neigh[i] < 0.0) neigh[i] = 0.0;
            if (neigh[i] > 1.0) neigh[i] = 1.0;
        } else {
            //small constant perturbation
            int i = idx_dist(rng);
            double d = (ur(rng) - 0.5) * 0.1;
            neigh[i] += d;
            if (neigh[i] < 0.0) neigh[i] = 0.0;
            if (neigh[i] > 1.0) neigh[i] = 1.0;
        }

        // Evaluate neighbor
        long long neigh_cost = simulate(tasks, MaxProcs, neigh, nullptr);

        long long delta = neigh_cost - current_cost;
        bool accept = false;
        if (delta < 0) {
            accept = true;
        } else {
            // Metropolis criterion with adaptive threshold
            double prob = exp(-double(delta) / max(1e-12, T));
            if (ur(rng) < prob) accept = true;
        }

        if (accept) {
            priorities = neigh;
            current_cost = neigh_cost;
            if (neigh_cost < best_cost) {
                best_cost = neigh_cost;
                best_prior = neigh;
                no_improve_count = 0;
                
            } else {
                no_improve_count++;
            }
        } else {
            no_improve_count++;
        }

        T *= alpha;
        
    }

    vector<Scheduled> final_schedule;
    simulate(tasks, MaxProcs, best_prior, &final_schedule);
    return final_schedule;
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

    if (MaxProcs <= 0) MaxProcs = 1;

    auto schedule = solve_sa(tasks_all, MaxProcs);

    ofstream fout("output_sa.swf");
    if (!fout) {
        cerr << "Cannot create file: output_sa.swf\n";
        return 1;
    }
    for(auto &s: schedule){
        fout<<s.id<<" "<<s.start<<" "<<s.finish;
        for(int p: s.procs) fout<<" "<<p;
        fout<<"\n";
    }
    fout.close();

    return 0;
}