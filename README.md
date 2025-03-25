# K-means con OpenMP

## Proyecto de Cómputo Paralelo y en la Nube. Primavera 2025. ITAM

###### Pablo Alazraki [@Palazrak](https://github.com/Palazrak) y Frida Márquez [@fridamarquezg](https://github.com/fridamarquezg)

### Introducción

El presente trabajo muestra la implementación y evaluación de una versión paralelizada del algoritmo K-means utilizando OpenMP. Se desarrolló tanto una versión serial como una versión paralela y se llevaron a cabo experimentos para comparar el desempeño en función del número de puntos y la cantidad de hilos (o *cores*) de ejecución. El algoritmo K-means, ampliamente utilizado en análisis de datos y aprendizaje no supervisado, tiene como objetivo agrupar un conjunto de puntos en `k` clusters minimizando la varianza entre ellos. El proyecto inicia con la lectura de un archivo `.csv` que contiene puntos generados aleatoriamente en un espacio bidimensional (coordenadas `x` y `y` distribuidas aleatoriamente entre 0 y 1) generado a partir del notebook `synthetic_clusters.ipynb`, disponible en el repositorio oficial del curso [computoparalelo2025a](https://github.com/octavio-gutierrez/computoparalelo2025a). Una vez hechas las versiones serial y paralela, se realizaron experimentos para observar la potencial mejora de usar más cores (variando en 1, 8, 16 y 32 cores) en distintos escenarios, descritos más a detalle en las siguientes secciones. Los resultados experimentales indican que, mientras para conjuntos de datos pequeños la diferencia entre configuraciones de 8, 16 y 32 hilos es marginal, para volúmenes superiores a 600,000 puntos el uso de 32 hilos ofrece mejoras significativas en el tiempo de ejecución.

En este repositorio se encuentran los archivos que contienen las versiones serial y paralela de los experimentos, el notebook con el que se generaron los puntos, los resultados tabulares obtenidos y un archivo con el código paralelizado listo para recibir un archivo `.csv` y clusterizar.

### Metodología

#### Descripción de la implementación serial

La implementación serial del algoritmo se basa en un esquema iterativo clásico, cuya estructura en pseudocódigo se puede resumir en los siguientes pasos:

1. Inicialización de `k` centroides, distribuyéndolos de forma aleatoria sobre el conjunto de datos.

2. Asignación de cada punto al centroide más cercano, utilizando la distancia euclidiana como métrica.

3. Actualización de la posición de los centroides mediante el cálculo del promedio de las coordenadas `x` y `y` de los puntos asignados a cada cluster.

4. Repetición de los pasos 2 y 3 hasta alcanzar un criterio de convergencia, que definimos como el estado en el que ningún punto cambia de cluster.

#### Estrategia de paralelización

La estrategia de paralelización fue usar OpenMP para implementar la ejecución paralela en dos secciones específicas del algoritmo. En primer lugar, durante la etapa de asignación, es necesario calcular para cada punto la distancia a todos los centroides y determinar el más cercano. Para ello, se distribuyó el conjunto de puntos entre múltiples hilos, de manera que cada hilo se encargara de calcular las distancias de sus puntos asignados a cada centroide y realizar la correspondiente asignación al cluster más cercano. En segundo lugar, durante la etapa de actualización, se paralelizó de tal forma que cada hilo se encargara de realizar los cálculos para las nuevas coordenadas `x` y `y` de cada centroide.

### Configuración experimental

Los experimentos se realizaron ejecutando el algoritmo 10 veces con cada configuración, para posteriormente obtener promedios del tiempo de ejecución. En dichos experimentos, se variaron dos parámetros fundamentales:

1. El número de puntos, considerando conjuntos de datos de 100,000, 200,000, 300,000, 400,000, 600,000, 800,000 y 1,000,000 puntos.

2. El número de hilos, utilizando configuraciones de 1, 8 (la mitad de los núcleos virtuales), 16 (la cantidad total de núcleos virtuales) y 32 (el doble del número de núcleos virtuales).

Es importante considerar que la ejecución se llevó a cabo en un equipo con las siguientes características:

- **Equipo**: MSI Thin GF63 12VF
- **Procesador**: Intel® Core™ i7-12650H de 12ª generación (2.30 GHz)
- **Núcleos**: 16 núcleos virtuales
- **Memoria**: 32 GB de RAM
- **Tarjeta gráfica**: NVIDIA GeForce RTX 4060 Laptop GPU (16 GB)
- **Sistema operativo**: Subsistema de Linux (WSL2) con Ubuntu 22.04.5 LTS
- **Entorno de desarrollo**: Visual Studio Code, utilizando la extensión oficial de C/C++ de Microsoft

### Resultados

En la **Tabla 1** se presentan los tiempos promedio de 10 ejecuciones (en milisegundos) para las diferentes configuraciones de número de puntos y número de hilos. Se incluye también una visualización (**Figura 1**) para representar gráficamente la información de la tabla.

| Número de puntos | Serial (ms) | Paralelo (1 core) (ms) | Paralelo (8 cores) (ms) | Paralelo (16 cores) (ms) | Paralelo (32 cores) (ms) |
|------------------|-------------|------------------------|-------------------------|--------------------------|--------------------------|
| 100000           | 164.5       | 118.308                | 43.4041                 | 60.8701                  | 63.9812                  |
| 200000           | 221.5       | 276.516                | 131.622                 | 114.514                  | 97.2848                  |
| 300000           | 392.2       | 341.541                | 132.083                 | 214.489                  | 150.26                   |
| 400000           | 533.4       | 419.895                | 186.391                 | 179.459                  | 222.485                  |
| 600000           | 1036.1      | 788.043                | 269.145                 | 305.14                   | 231.537                  |
| 800000           | 1768.5      | 1013.31                | 401.796                 | 306.238                  | 273.631                  |
| 1000000          | 1335.8      | 1830.43                | 510.225                 | 436.382                  | 382.63                   |

> **Tabla 1.** Tiempos promedio de ejecución (en milisegundos) para distintas configuraciones de número de cores y número de puntos.

![grafica_tiempos](./imagenes/tiempos_ejecucion.jpg)
> **Figura 1**. Tiempo promedio de ejecución (10 iteraciones) vs número de puntos.

En la gráfica se observa que, antes de los 600,000 puntos, los escenarios con 8, 16 y 32 cores presentan un comportamiento similar, lo cual se justifica por la tendencia y cercanía de las rectas. Sin embargo, a partir de 600,000 datos, el escenario con 32 cores registra de forma consistente los menores tiempos promedio de ejecución. Suponiendo casos de uso con una cantidad de puntos superior a 600,000, consideramos que la opción con 32 cores representa la mejor alternativa entre las configuraciones exploradas. Por esta razón, en la Tabla 2 se presentan los speed-ups obtenidos por la versión paralela con 32 cores en comparación con la versión serial.

| Número de puntos | Versión serial (ms) | Versión 32 cores (ms) | Speed-up |
|------------------|---------------------|----------------------|-----------|
| 100000           | 164.5               | 63.9812              | 2.571     |
| 200000           | 221.5               | 97.2848              | 2.277     |
| 300000           | 392.2               | 150.26               | 2.610     |
| 400000           | 533.4               | 222.485              | 2.397     |
| 600000           | 1036.1              | 231.537              | 4.475     |
| 800000           | 1768.5              | 273.631              | 6.463     |
| 1000000          | 1335.8              | 382.63               | 3.491     |

> **Tabla 2**. Speed-up alcanzado por la versión con 32 hilos respecto a la versión serial.

Podemos observar que los speed-ups se mueven en un rango de 2.397 hasta 6.463. Consideramos que estos speed-ups son razonables, partiendo de que el requerimiento para la entrega del proyecto es de mínimo 1.5 de forma consistente.

### Conclusiones

El estudio presentado demuestra que la implementación paralela del algoritmo k-means utilizando OpenMP permite obtener mejoras significativas en el tiempo de ejecución, especialmente en el procesamiento de grandes volúmenes de datos. Los experimentos realizados evidencian que, para conjuntos de datos menores a 600,000 puntos, la utilización de 8 hilos resulta suficiente para alcanzar un rendimiento competitivo. Sin embargo, para escenarios con una mayor carga computacional, la configuración de 32 hilos resulta la opción más adecuada, logrando reducir drásticamente el tiempo de cómputo en comparación con la versión serial. Estos hallazgos subrayan la importancia de una estrategia de paralelización bien diseñada y la adecuada asignación de recursos en aplicaciones de cómputo intensivo.