#Librerías
import pprint as pp
import numpy as np
import networkx as nx
import bellmanford as bf
import matplotlib.pyplot as plt
import cargadorInstancias as ci

#Función para encontrar el camino más corto del digrafo de itinerarios
def spBellmanFord(instancia, productores, receptores, pesos):   
    
    # Campos de la instancia recibida
    # instancia = {}
    # instancia['nombreInstancia'] = str(ruta)#Nombre del caso de la librería
    # instancia['numBloques'] = int()#Total de bloques o servicios
    # instancia['minutosJornada'] = int()#Duración total de la jornada en minutos
    # instancia['bloques'] = [] #Listado de diccionarios de bloques de trabajo o servicios
    # instancia['transiciones'] = []#Listado de diccionarios de transiciones entre bloques o servicios
    # instancia['matrizCostos'] = []#Matriz con los costos de transición
    # instancia['mayorCostoReactivo'] = 0#Costo reactivo para generar el costo de salida de cada nuevo operador en los turnos
    # instancia['matrizTiemposTransicion'] = []#Matriz para facilitar la consulta de factibilidad
    
    #Valor grande para costos prohibitivos
    M = 100000000      
    
    #Generar el grafo a partir de los vectores de entrada    
    #instancia['numBloques'] = 5 #Forzar solo para el ejemplo de Prins
    pesos = list(map(lambda x:{'weight':x},pesos))
    arcos = list(zip(productores,receptores,pesos))   
    G = nx.DiGraph()
    G.add_edges_from(arcos)
    #Dibujado de diagnóstico
    #nx.draw(G, with_labels=True, arrowsize=30) 
    #plt.show()    
    shortest_path = bf.bellman_ford(G, source=0, target=instancia['numBloques'])
    funcionObjetivo = shortest_path[0]   
    
    #Retornar el costo del camino más corto
    return funcionObjetivo 
       
    
#Llamados de diagnóstico
#-----------------------

#Carga de instancia
#instancia = ci.beasley('./instances/csp50.txt')

#Ejemplo alternativo (Prins, 04)
#productores = [0 ,0 ,1 ,1 ,1  ,2 ,2 ,3 ,3 , 4]
#receptores  = [1 ,2 ,2 ,3 ,4  ,3 ,4 ,4 ,5 , 5]
#pesos       = [40,55,50,85,120,60,95,80,90,70]

#spBellmanFord(instancia, productores, receptores, pesos)