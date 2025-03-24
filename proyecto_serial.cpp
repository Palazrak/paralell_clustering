#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>
#include <chrono>

using namespace std;
using namespace std::chrono;

struct Punto {
    double x;
    double y;
    int cluster;
    double distancia;
};

struct Centroid {
    double x;
    double y;
};

std::vector<Punto> lectura(const std::string& archivo) {
    std::vector<Punto> puntos;
    std::ifstream file(archivo);
    
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo: " << archivo << "\n";
        return puntos;  // Retorna vector vacío en caso de error
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        std::stringstream lineStream(line);
        std::string cell;
        Punto p;
        
        // Se asume que el CSV tiene dos columnas: x,y
        if (std::getline(lineStream, cell, ',')) {
            p.x = std::stod(cell);
        }
        if (std::getline(lineStream, cell, ',')) {
            p.y = std::stod(cell);
        }
        
        p.cluster = -1;
        p.distancia = -1;
        
        puntos.push_back(p);
    }
    
    file.close();
    return puntos;
}

/*
Función experimental de k-means que:
- Lee el archivo de entrada.
- Inicializa centroides aleatoriamente (dentro del bounding box de los datos).
- Ejecuta el algoritmo de clustering hasta convergencia, midiendo el tiempo exclusivo de esta etapa.
- Escribe el archivo de salida con los clusters asignados.
Retorna el tiempo (en ms) empleado en la etapa de clustering.
*/
double kmeans_experiment(string input, int n, int k) {
    // Tiempo total
    auto start_total = high_resolution_clock::now();
    
    // 1. Lectura de puntos desde el archivo
    // auto start_read = high_resolution_clock::now();
    std::vector<Punto> puntos = lectura(input);
    // auto end_read = high_resolution_clock::now();
    // auto duration_read = duration_cast<milliseconds>(end_read - start_read);
    
    if (puntos.empty()) {
         cerr << "No hay puntos para procesar." << "\n";
         return 0.0;
    }
    
    // cout << "Se leyeron " << puntos.size() << " puntos." << "\n";
    
    // 2. Inicializar centroides aleatorios fuera de los datos
    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    
    for (const auto& p : puntos) {
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;
    }
    
    vector<Centroid> centroides(k);
    
    for (int i = 0; i < k; i++) {
        centroides[i].x = min_x + ((double)rand() / RAND_MAX) * (max_x - min_x);
        centroides[i].y = min_y + ((double)rand() / RAND_MAX) * (max_y - min_y);
    }
    
    // 3. Clustering: medir el tiempo desde que inicia esta etapa hasta su finalización.
    auto start_cluster = high_resolution_clock::now();
    
    bool changed;
    int iterations = 0;
    
    do {
         iterations++;
         int num_changed = 0;
         
         // Para cada punto se busca el centroide más cercano
         for (size_t i = 0; i < puntos.size(); i++) {
              double min_dist = std::numeric_limits<double>::max();
              int best_cluster = -1;
              
              for (int j = 0; j < k; j++) {
                   double dx = puntos[i].x - centroides[j].x;
                   double dy = puntos[i].y - centroides[j].y;
                   double dist = dx * dx + dy * dy;  // Distancia al cuadrado
                   
                   if (dist < min_dist) {
                        min_dist = dist;
                        best_cluster = j;
                   }
                   // En caso de empate (distancias prácticamente iguales), desempata aleatoriamente
                   else if (fabs(dist - min_dist) < 1e-9) {
                        if (rand() % 2 == 1) {
                            best_cluster = j;
                        }
                   }
              }
              
              if (puntos[i].cluster != best_cluster) {
                   puntos[i].cluster = best_cluster;
                   puntos[i].distancia = sqrt(min_dist); // Distancia real
                   num_changed++;
              }
         }
         
         // Recalcular los centros de los clusters (promedio de coordenadas)
         vector<double> sumX(k, 0.0), sumY(k, 0.0);
         vector<int> count(k, 0);
         
         for (size_t i = 0; i < puntos.size(); i++) {
              int cl = puntos[i].cluster;
              sumX[cl] += puntos[i].x;
              sumY[cl] += puntos[i].y;
              count[cl]++;
         }
         
         for (int j = 0; j < k; j++) {
              if (count[j] > 0) {
                   centroides[j].x = sumX[j] / count[j];
                   centroides[j].y = sumY[j] / count[j];
              }
         }
         
         // cout << "Iteración " << iterations << ": " << num_changed << " puntos cambiaron de cluster." << "\n";
         changed = (num_changed > 0);
         
    } while (changed);
    
    auto end_cluster = high_resolution_clock::now();
    auto duration_cluster = duration_cast<milliseconds>(end_cluster - start_cluster);
    // cout << "Tiempo de clustering: " << duration_cluster.count() << " ms" << "\n";
    // cout << "Convergencia alcanzada en " << iterations << " iteraciones." << "\n";
    
    // 4. Escribir el resultado en el archivo de salida
    // auto start_write = high_resolution_clock::now();
    string outputFile = to_string(n) + "_results.csv";
    ofstream outfile(outputFile);
    if (!outfile.is_open()){
         cerr << "Error al abrir el archivo de salida." << "\n";
         return duration_cluster.count();
    }
    
    outfile << "x,y,cluster\n";
    for (size_t i = 0; i < puntos.size(); i++) {
         outfile << puntos[i].x << "," << puntos[i].y << "," << puntos[i].cluster << "\n";
    }
    outfile.close();
    // auto end_write = high_resolution_clock::now();
    // auto duration_write = duration_cast<milliseconds>(end_write - start_write);
    // cout << "Tiempo de escritura: " << duration_write.count() << " ms" << "\n";
    
    // auto end_total = high_resolution_clock::now();
    // auto duration_total = duration_cast<milliseconds>(end_total - start_total);
    // cout << "Tiempo total del proceso: " << duration_total.count() << " ms" << "\n";
    
    // Retornamos únicamente el tiempo de clustering
    return duration_cluster.count();
}

int main() {
    // Definir los distintos tamaños de datos
    vector<int> sizes = {100000, 200000, 300000, 400000, 600000, 800000, 1000000};
    int k = 5;      // Número de clusters (puedes ajustar según convenga)
    int trials = 10; // Número de repeticiones para cada tamaño
    
    // Se ejecuta 10 veces para cada tamaño y se promedian los tiempos de clustering.
    for (int size : sizes) {
        string inputFile = to_string(size) + "_data.csv";
        double sum_cluster_time = 0.0;
        // cout << "\n--- Ejecutando para " << size << " puntos ---\n";
        
        for (int i = 0; i < trials; i++) {
            // cout << "\nEjecución " << (i+1) << ":\n";
            double cluster_time = kmeans_experiment(inputFile, size, k);
            sum_cluster_time += cluster_time;
        }
        double avg_cluster_time = sum_cluster_time / trials;
        cout << "\nPara " << size << " puntos, el tiempo promedio de clustering es: "
             << avg_cluster_time << " ms" << "\n";
    }
    
    return 0;
}
