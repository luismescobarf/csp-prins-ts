#Librerías
import pprint as pp
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt
import cargadorInstancias as ci
import importlib
bf = importlib.import_module("bellmanFord-SP")
#import bellmanFord-SP as bf
import random

#Función para determinar la función objetivo de la secuencia de servicios
def evaluacionPrinsCSP(instancia, ordenAtencion):   
    
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
    
    #Generar los posibles itinerarios    
    posiblesItinerarios = []
    
    #Para cada tamanio de itinerario generar todos los posibles
    for i in range(1,len(ordenAtencion)+1): 
        
        #Variar los puntos de arranque de los itinerarios de tamaño 'i'
        for k in range(len(ordenAtencion)): 
            
            #Recorrer el orden de atención para extraer los posibles itinerarios desde k hasta k+i
            itinerario = []#Inicializar el contenedor del posible itinerario    
            j=k#j variará entre k y k+1           
            
            #Validar que no se haya llegado al final del orden de atención 
            #para evitar desborde al generar los itinerarios de tanaño i desde k
            if k+i <= len(ordenAtencion):
                
                #Factibilidad en duración de itinerario (minutos)
                duracionItinerario = 0                                
                
                #Construir el itinerario revisando factibilidad:
                #En transición y en duración del turno o itinerario
                for j in range(k,k+i):
                    if not(duracionItinerario): 
                        duracionItinerario += instancia['matrizTiemposTransicion'][0][ordenAtencion[j]]
                        duracionItinerario += instancia['bloques'][ ordenAtencion[j] - 1]['duracion']
                        itinerario.append(ordenAtencion[j])
                    else:
                        #Revisar factibilidad de transición
                        if not(instancia['matrizCostos'][ itinerario[-1] , ordenAtencion[j] ] == M):
                            
                            #Revisar factibilidad en duración de itinerario
                            duracionAuxiliar = duracionItinerario + instancia['matrizTiemposTransicion'][ itinerario[-1] ][ ordenAtencion[j] ]
                            duracionAuxiliar += instancia['bloques'][ ordenAtencion[j] - 1]['duracion']
                            if duracionAuxiliar <= instancia['minutosJornada']:
                                duracionItinerario = duracionAuxiliar
                                itinerario.append(ordenAtencion[j])
                            else:
                                break#Detener el crecimiento del itinerario por duración de turno
                            
                        else:                            
                            break#Detener el crecimiento por infactibilidad de transición         
                               
                #Validación para adicionar itinerarios con servicios
                #Evitar repeticiones por el truncado
                if  not(itinerario==[]) and not(itinerario in posiblesItinerarios):                     
                    posiblesItinerarios.append(itinerario)
                    
    #Salida de diagnóstico
    print('Itinerarios Factibles')
    print(posiblesItinerarios)    
    
    #####Obtener de los posibles itinerarios el digrafo (listas de productores, receptores y pesos)       
    
    #Retornar la función objetivo del orden de atención de servicios
    #return bf.spBellmanFord(instancia, [], [], []) 
       
    
#Llamados de diagnóstico
#-----------------------

#Carga de instancia
instancia = ci.beasley('./instances/csp50.txt')

#Generar orden de atención aleatorio
ordenAtencion = list(range(1,instancia['numBloques']+1))
random.shuffle(ordenAtencion)

#Evaluar con adaptación de Prins
evaluacionPrinsCSP(instancia, ordenAtencion)

#Ejemplo alternativo (Prins, 04)
#productores = [0 ,0 ,1 ,1 ,1  ,2 ,2 ,3 ,3 , 4]
#receptores  = [1 ,2 ,2 ,3 ,4  ,3 ,4 ,4 ,5 , 5]
#pesos       = [40,55,50,85,120,60,95,80,90,70]

#spBellmanFord(instancia, productores, receptores, pesos)