#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>
#include <omp.h>

using namespace std;

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
Función experimental paralela de k-means que:
- Lee el archivo de entrada.
- Inicializa centroides aleatorios (dentro del bounding box de los datos).
- Ejecuta el algoritmo de clustering hasta convergencia, paralelizando:
   1. La asignación de cada punto a su centroide más cercano.
   2. El reajuste de los centros (suma y actualización) usando reducción manual.
- Si algún cluster queda vacío, se re-inicializa su centroide con un punto aleatorio.
- Mide el tiempo exclusivo de la etapa de clustering usando omp_get_wtime.
- Escribe el resultado en el archivo de salida "<n>_results.csv".
Retorna el tiempo (en ms) empleado en la etapa de clustering.
El parámetro 'cores' indica el número de hilos a utilizar.
*/
double kmeans_experiment_parallel(string input, int n, int k, int cores) {
    omp_set_num_threads(cores);
    
    // 1. Lectura de puntos (se realiza de forma secuencial)
    std::vector<Punto> puntos = lectura(input);
    if (puntos.empty()){
        cerr << "No hay puntos para procesar." << "\n";
        return 0.0;
    }
    
    // Inicializar centroides aleatorios dentro del rango [min, max] de los datos
    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    
    for (const auto &p : puntos) {
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;
    }
    
    vector<Centroid> centroides(k);
    // Puedes inicializar la semilla una vez fuera del bucle para mayor consistencia.
    srand(time(NULL));
    for (int i = 0; i < k; i++){
        centroides[i].x = min_x + ((double)rand() / RAND_MAX) * (max_x - min_x);
        centroides[i].y = min_y + ((double)rand() / RAND_MAX) * (max_y - min_y);
    }
    
    // 3. Clustering: medir el tiempo exclusivo de esta etapa con omp_get_wtime
    double start_cluster = omp_get_wtime();
    
    bool changed;
    int iterations = 0;
    do {
        iterations++;
        int num_changed = 0;
        
        // Paralelizar la asignación de cada punto a su centroide más cercano
        #pragma omp parallel for reduction(+:num_changed)
        for (size_t i = 0; i < puntos.size(); i++){
            double min_dist = std::numeric_limits<double>::max();
            int best_cluster = -1;
            // Cada hilo usa su semilla local para el desempate
            unsigned int seed = (unsigned int)(omp_get_thread_num() + i);
            
            for (int j = 0; j < k; j++){
                double dx = puntos[i].x - centroides[j].x;
                double dy = puntos[i].y - centroides[j].y;
                double dist = dx * dx + dy * dy;  // Distancia al cuadrado
                
                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = j;
                }
                // En caso de empate (distancias prácticamente iguales), desempata aleatoriamente
                else if (fabs(dist - min_dist) < 1e-9) {
                    if (rand_r(&seed) % 2 == 1)
                        best_cluster = j;
                }
            }
            
            if (puntos[i].cluster != best_cluster) {
                puntos[i].cluster = best_cluster;
                puntos[i].distancia = sqrt(min_dist); // Distancia real
                num_changed++;
            }
        }
        
        // Paralelizar el reajuste de los centros: sumar coordenadas y contar puntos
        // Usamos el número de hilos que definimos (cores) para crear los arreglos locales
        int numThreads = cores;
        std::vector<std::vector<double>> localSumX(numThreads, std::vector<double>(k, 0.0));
        std::vector<std::vector<double>> localSumY(numThreads, std::vector<double>(k, 0.0));
        std::vector<std::vector<int>> localCount(numThreads, std::vector<int>(k, 0));
        
        for (size_t i = 0; i < puntos.size(); i++){
            int tid = omp_get_thread_num();
            int cl = puntos[i].cluster;
            localSumX[tid][cl] += puntos[i].x;
            localSumY[tid][cl] += puntos[i].y;
            localCount[tid][cl] += 1;
        }
        
        std::vector<double> sumX(k, 0.0), sumY(k, 0.0);
        std::vector<int> count(k, 0);
        for (int t = 0; t < numThreads; t++){
            for (int j = 0; j < k; j++){
                sumX[j] += localSumX[t][j];
                sumY[j] += localSumY[t][j];
                count[j] += localCount[t][j];
            }
        }
        
        // Actualizar los centros en paralelo, re-inicializando si el cluster está vacío
        for (int j = 0; j < k; j++){
            if (count[j] > 0) {
                centroides[j].x = sumX[j] / count[j];
                centroides[j].y = sumY[j] / count[j];
            } else {
                // Si el cluster está vacío, re-inicializar el centroide con un punto aleatorio del conjunto
                int randomIndex = rand() % puntos.size();
                centroides[j].x = puntos[randomIndex].x;
                centroides[j].y = puntos[randomIndex].y;
            }
        }
        
        changed = (num_changed > 0);
        
    } while (changed);
    
    double end_cluster = omp_get_wtime();
    double duration_cluster = (end_cluster - start_cluster) * 1000.0; // Convertir a milisegundos
    
    // 4. Escribir el resultado en el archivo de salida
    string outputFile = to_string(n) + "_results.csv";
    ofstream outfile(outputFile);
    if (!outfile.is_open()){
        cerr << "Error al abrir el archivo de salida." << "\n";
        return duration_cluster;
    }
    
    outfile << "x,y,cluster\n";
    for (size_t i = 0; i < puntos.size(); i++){
        outfile << puntos[i].x << "," << puntos[i].y << "," << puntos[i].cluster << "\n";
    }
    outfile.close();
    
    return duration_cluster;
}

int main() { 
    double cluster_time = kmeans_experiment_parallel("980000_data.csv", 980000, 8, omp_get_num_procs());
    
    cout << "\nPara 980000 puntos, el tiempo de clustering (paralelo) es: "
         << cluster_time << " ms" << "\n";
    return 0;
}
