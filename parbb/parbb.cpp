#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>
#include <numeric>
#include <omp.h>
#include <sstream>
#include <string>
#include <queue>
#include <utility>
#include <vector>


// COORDS
std::vector<std::pair<double, double>> COORDS;

class BBTask {
public:
    BBTask(int root_level, std::vector<int>& values, std::vector<int>& max_values)
        : root_level(root_level), values(values), max_values(max_values), level(root_level) {}

    int backtracking() {
        while (level >= root_level) {
            if (values[level] < max_values[level]) {
                values[level]++;
                return level;
            }
            values[level] = 0;
            level--;
        }
        return -1;
    }

    BBTask* split_task(size_t min_depth = 0) {
        size_t split_level = root_level;
        while (split_level < (values.size() - min_depth)) {
            if (values[split_level] < max_values[split_level]) {
                std::vector<int> new_values = values;
                std::vector<int> new_max_values = max_values;
                int new_value = std::max((values[split_level] + max_values[split_level]) / 2, values[split_level] + 1);
                new_values[split_level] = new_value;
                max_values[split_level] = new_value - 1;
                return new BBTask(split_level, new_values, new_max_values);
            }
            split_level++;
        }
        return nullptr;
    }

    bool next_node(bool is_feasible) {
        if (is_feasible && level < values.size() - 1) {
            level++;
            values[level] = 0;
            return true;
        }
        level = backtracking();
        return level > 0;
    }

    int workload() const {
        size_t work_level = root_level;
        while (work_level < values.size()) {
            if (values[work_level] < max_values[work_level]) {
                break;
            }
            work_level++;
        }
        return work_level - root_level;
    }

    size_t root_level;
    std::vector<int> values;
    std::vector<int> max_values;
    size_t level;
};

bool constraints(int level, const std::vector<int>& values) {
    // each node is visited exactly once
    std::vector<bool> visited(values.size(), false);
    for (int i = 0; i <= level; i++) {
        if (visited[values[i]]) {
            return false;
        }
        visited[values[i]] = true;
    }

    return true;        
}

double distanceXY(std::pair<double, double> a, std::pair<double, double> b) {
    double dx = a.first - b.first;
    double dy = a.second - b.second;
    return std::sqrt(dx * dx + dy * dy);
}

double cost_function(size_t level, const std::vector<int>& values) {
    double cost = 0;
    for (size_t i = 0; i < level; i++) {
        cost += distanceXY(COORDS[values[i]], COORDS[values[i + 1]]);
    }

    // add the cost of returning to the starting node
    if(level == values.size() - 1) {
        cost += distanceXY(COORDS[values[level]], COORDS[values[0]]);
    }
    return cost;
}

class BBManager {
public:
    BBManager(int num_workers, BBTask* task, double upper_bound = std::numeric_limits<double>::max())
        : upper_bound(upper_bound), num_workers(num_workers), num_active_tasks(0), task(task) {
        // Set the number of threads to use
        omp_set_num_threads(num_workers);
    }

    void run() {
        #pragma omp parallel
        {
            #pragma omp single
            {
                num_active_tasks++;
                #pragma omp task
                {
                    process_task(task);
                }
            }
        }
    }

    void process_task(BBTask *task, int min_depth = 2, int sync_niters = 1000)
    {
        int niters = 0;
        bool is_feasible = constraints(task->level, task->values);
        while (task->next_node(is_feasible)) {
            niters++;

            if ((niters % sync_niters) == 0 && (num_active_tasks < num_workers))
            {
                BBTask* sub_task = task->split_task(min_depth);
                if (sub_task != nullptr) {
                    #pragma omp atomic
                        num_active_tasks++;

                    #pragma omp task
                    {
                        process_task(sub_task);
                    }
                }
            }

            is_feasible = constraints(task->level, task->values);
            if (!is_feasible) continue;

            double cost = cost_function(task->level, task->values);
            if (cost > upper_bound) continue;

            if (task->level == (task->values.size() - 1))
            {
                append_solution(cost, task->values);
            }
        }

        #pragma omp atomic
            num_active_tasks--;
    }

    void append_solution(double cost, const std::vector<int>& values) {
        #pragma omp critical
        {
            if (cost <= upper_bound)
            {
                if (cost < upper_bound)
                {
                    upper_bound = cost;
                    solutions.clear();
                    printf("New upper bound: %f\n", upper_bound);
                }
                solutions.push_back(values);
            }
        }
    }

    std::vector<std::vector<int>> get_solutions() const {
        return solutions;
    }

    double get_upper_bound() const {
        return upper_bound;
    }

private:
    double upper_bound;
    int num_workers;
    int num_active_tasks;
    BBTask* task;
    std::vector<std::vector<int>> solutions;
};


void read_COORDS(std::string filePath) {
    std::ifstream file(filePath);
    std::string line;


    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        exit(EXIT_FAILURE);
    }

    while (getline(file, line)) {
        // Skip non-coordinate lines
        if (line.empty() || isalpha(line[0])) {
            continue;
        }

        std::istringstream iss(line);
        int id;
        double x, y;
        if (!(iss >> id >> x >> y)) {
            break; // Error or end of coordinate section
        }

        COORDS.push_back({x, y});
    }

    file.close();
}


int main() {
    read_COORDS("data/dj38.tsp");

    printf("Number of coordinates: %lu\n", COORDS.size());
    for(size_t i = 0; i < COORDS.size(); i++) {
        printf("Node %ld: X: %f, Y: %f\n", i, COORDS[i].first, COORDS[i].second);
    }

    std::vector<int> values(COORDS.size(), 0);
    std::vector<int> max_values(COORDS.size(), COORDS.size() - 1); 

    BBTask task(0, values, max_values);
    BBManager manager(8, &task, std::numeric_limits<double>::max());
    manager.run();

    std::vector<std::vector<int>> solutions = manager.get_solutions();
    printf("Number of solutions: %lu\n", solutions.size());
    printf("Best cost: %f\n", manager.get_upper_bound());

    return EXIT_SUCCESS;    
}