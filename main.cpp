#include "Pipeline.h"
#include "CompressorStation.h"
#include "Pipeline.cpp"
#include "Logger.h"
#include "Logger.cpp"
#include "CompressorStation.cpp"
#include <iostream>
#include <vector>
#include <sstream>
#include "FileOperations.h"
#include "FileOperations.cpp"
#include <stack>
#include <unordered_map>
#include <queue>

template <typename T>
void displayFilteredByName(const std::vector<T>& items, const std::string& filterName) {
    bool found = false;
    for (const auto& item : items) {
        if (item.getName().find(filterName) != std::string::npos) {
            item.output();
            found = true;
        }
    }
    if (!found) {
        std::cout << "No items found with name containing: " << filterName << std::endl;
    }
}
void addConnection(std::vector<Pipeline>& pipelines, std::vector<CompressorStation>& stations) {
    int in_id, out_id, diameter;

    // Запрашиваем ID компрессорных станций и диаметр трубы
    do {
        std::cout << "Enter the ID of the input compressor station: ";
        std::cin >> in_id;
        if (std::cin.fail() || in_id <= 0 || in_id > stations.size()) {
            std::cin.clear();
            std::cin.ignore();
            std::cout << "Invalid input. Please enter a valid ID.\n";
        }
    } while (in_id <= 0 || in_id > stations.size());

    do {
        std::cout << "Enter the ID of the output compressor station: ";
        std::cin >> out_id;
        if (std::cin.fail() || out_id <= 0 || out_id > stations.size() || out_id == in_id) {
            std::cin.clear();
            std::cin.ignore();
            std::cout << "Invalid input. Please enter a valid ID.\n";
        }
    } while (out_id <= 0 || out_id > stations.size() || out_id == in_id);

    do {
        std::cout << "Enter the diameter of the connecting pipeline: ";
        std::cin >> diameter;
        if (std::cin.fail() || diameter <= 0) {
            std::cin.clear();
            std::cin.ignore();
            std::cout << "Invalid input. The diameter must be a positive number.\n";
        }
    } while (diameter <= 0);

    // Поиск существующего трубопровода с нужным диаметром
    bool foundExistingPipe = false;
    Pipeline* selectedPipeline = nullptr;

    for (auto& pipe : pipelines) {
        if (pipe.hasDiameter(diameter) && pipe.getIid() == -1 && pipe.getOid() == -1) {
            selectedPipeline = &pipe;
            foundExistingPipe = true;
            break;
        }
    }

    if (foundExistingPipe) {
        // Используем существующий трубопровод
        selectedPipeline->setStations(in_id, out_id, diameter);
        std::cout << "Connected the stations using the existing pipeline with diameter " << diameter << ".\n";
        system("sleep");
    } else {
        // Создаем новый трубопровод
        Pipeline p("", 0, diameter, false, in_id, out_id);
        p.input();
        pipelines.push_back(p);
    }
}


// Вспомогательная функция для DFS
void topologicalSortUtil(int station_id, std::unordered_map<int, bool>& visited, std::stack<int>& stack, const std::unordered_map<int, std::vector<int>>& graph) {
    visited[station_id] = true;

    // Идем по всем соседям этой вершины (связанным станциям)
    if (graph.find(station_id) != graph.end()) {
        for (int neighbor : graph.at(station_id)) {
            if (!visited[neighbor]) {
                topologicalSortUtil(neighbor, visited, stack, graph);
            }
        }
    }

    // Добавляем станцию в стек после того как все её соседи обработаны
    stack.push(station_id);
}

// Функция для вывода топологической сортировки
void displayTopologicalSort(const std::vector<CompressorStation>& stations, const std::vector<Pipeline>& pipelines) {
    std::unordered_map<int, std::vector<int>> graph;
    std::unordered_map<int, bool> visited;
    std::stack<int> stack;

    // Строим граф на основе трубопроводов (рёбер)
    for (const auto& pipeline : pipelines) {
        int station_in_id = pipeline.getIid();
        int station_out_id = pipeline.getOid();

        // Добавляем рёбра (связи между станциями)
        graph[station_in_id].push_back(station_out_id);
    }

    // Для каждой станции, если она не была посещена, вызываем DFS
    for (const auto& station : stations) {
        if (!visited[station.getId()]) {
            topologicalSortUtil(station.getId(), visited, stack, graph);
        }
    }

    // Выводим станции в порядке топологической сортировки
    std::cout << "Topological Sort of Compressor Stations (in order of connection):\n";
    while (!stack.empty()) {
        int station_id = stack.top();
        stack.pop();
        
        // Находим станцию по её ID и выводим информацию о ней
        for (const auto& station : stations) {
            if (station.getId() == station_id) {
                std::cout << "Station ID: " << station.getId() << ", Name: " << station.getName() << "\n";
                break;
            }
        }
    }
}
#include <iostream>
#include <vector>
#include <unordered_map>
#include <limits>
#include <set>
#include <algorithm>

// Функция для поиска кратчайшего пути с помощью алгоритма Дейкстры
std::vector<int> dijkstraShortestPath(const std::vector<Pipeline>& pipelines, int start, int end) {
    // Построение графа
    std::unordered_map<int, std::vector<std::pair<int, double>>> graph;
    for (const auto& pipeline : pipelines) {
        int station_in_id = pipeline.getIid();
        int station_out_id = pipeline.getOid();
        double length = pipeline.getLength() * !pipeline.isInRepair();

        graph[station_in_id].emplace_back(station_out_id, length); 
    }

    // Инициализация
    std::unordered_map<int, double> distances;
    std::unordered_map<int, int> parent;
    std::set<std::pair<double, int>> active_nodes; // Набор пар {расстояние, вершина}, упорядоченных по расстоянию

    for (const auto& entry : graph) {
        distances[entry.first] = std::numeric_limits<double>::infinity();
    }
    distances[start] = 0;
    active_nodes.insert({0, start});
    parent[start] = -1;

    // Алгоритм Дейкстры
    while (!active_nodes.empty()) {
        auto [current_distance, current_station] = *active_nodes.begin();
        active_nodes.erase(active_nodes.begin());

        if (current_station == end) break;

        for (const auto& neighbor : graph[current_station]) {
            int neighbor_id = neighbor.first;
            double weight = neighbor.second;

            if (current_distance + weight < distances[neighbor_id]) {
                active_nodes.erase({distances[neighbor_id], neighbor_id});
                distances[neighbor_id] = current_distance + weight;
                parent[neighbor_id] = current_station;
                active_nodes.insert({distances[neighbor_id], neighbor_id});
            }
        }
    }

    // Восстановление пути
    std::vector<int> path;
    if (distances[end] == std::numeric_limits<double>::infinity()) {
        std::cout << "Path not found.\n";
        return path;
    }

    for (int at = end; at != -1; at = parent[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// Меню для поиска кратчайшего пути
void shortestPathMenu(const std::vector<Pipeline>& pipelines) {
    int start, end;
    std::cout << "Enter the ID of the starting station: ";
    std::cin >> start;
    std::cout << "Enter the ID of the destination station: ";
    std::cin >> end;

    std::vector<int> path = dijkstraShortestPath(pipelines, start, end);

    if (!path.empty()) {
        std::cout << "Shortest path (weighted): ";
        for (int station : path) {
            std::cout << station << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "No path found.\n";
    }
}



int main() {
    std::vector<Pipeline> pipelines;
    std::vector<CompressorStation> stations;

    std::string choice;
    while (true) {
        system("cls");
        std::cout << "1. Add Pipeline\n2. Add Compressor Station\n3. View All\n4. Edit Pipeline\n5. Edit Compressor Station\n6. Save Data\n7. Load Data\n8. Filter by name\n9. Graph menu\n0. Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        char ch = choice[0];

        if (ch == '1') {
            Pipeline p("", 0, 0, false, -1, -1 );
            p.input();
            pipelines.push_back(p);
        } else if (ch == '2') {
            CompressorStation s("", 0, 0, 0);
            s.input();
            stations.push_back(s);
        } else if (ch == '3') {
            std::cout << "Pipelines:\n";
            for (const auto& p : pipelines) p.output();
            std::cout << "\nCompressor Stations:\n";
            for (const auto& s : stations) s.output();
            
            system("pause");
        } else if (ch == '4') {
            std::string id;
            std::cout << "Enter Pipeline IDs to edit (separate with commas): ";
            std::cin >> id;
            std::stringstream ss(id);
            std::string num;
            for (auto& p : pipelines) {
                while (std::getline(ss, num, ',')) {
                    int num_id = std::stoi(num);
                    if (p.getId() == num_id) {
                        p.edit();
                        break;
                    }
                }
            }
        } else if (ch == '5') {
            std::string id;
            std::cout << "Enter Compressor Station IDs to edit (separate with commas): ";
            std::cin >> id;
            std::stringstream ss(id);
            std::string num;
            for (auto& s : stations) {
                while (std::getline(ss, num, ',')) {
                    int num_id = std::stoi(num);
                    if (s.getId() == num_id) {
                        s.edit();
                        break;
                    }
                }
            }
        } else if (ch == '6') {
            savePipelines(pipelines, "pipelines.txt");
            saveCompressorStations(stations, "stations.txt");
            Logger::log("Data saved.");
            system("pause");
        } else if (ch == '7') {
            pipelines.clear();
            stations.clear();
            loadPipelines(pipelines, "pipelines.txt");
            loadCompressorStations(stations, "stations.txt");
            Logger::log("Data loaded.");
            system("pause");
        } else if (ch == '8') {
            std::string filterName;
            std::cout << "Enter a name to filter by: ";
            std::cin >> filterName;

            std::cout << "\nFiltered Pipelines:\n";
            displayFilteredByName(pipelines, filterName);

            std::cout << "\nFiltered Compressor Stations:\n";
            displayFilteredByName(stations, filterName);
            system("pause");
        } else if (ch == '0') {
            break;
        } else if (ch == '9') {
            std::string choice;
            while (true) {
                system("cls");
                std::cout << "1. Create connection\n2. Display topological sort\n3. Search max flow\n4. Shortest way\n0. Exit Graph menu\n";
                std::cout << "Enter your choice: \n";
                std::cin >> choice;
                char ch = choice[0];

                if (ch == '1') {
                    addConnection(pipelines, stations);
                } else if (ch == '2') {
                    std::cout << "Pipelines:\n";
                    for (const auto& p : pipelines) p.output();
                    std::cout << "\nCompressor Stations:\n";
                    for (const auto& s : stations) s.output();
                    displayTopologicalSort(stations, pipelines);
                    system("pause");
                } else if (ch == '3') {
                    std::cout << " tut budet kod"; 
                } else if (ch == '4') {
                    shortestPathMenu(pipelines);
                    system("pause");
                } else if (ch == '0') {
                    break;
                }
            }
            
        } else {
            std::cout << "Invalid input. Try again.\n";
        }
    }

    return 0;
}
