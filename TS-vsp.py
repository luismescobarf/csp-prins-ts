# -*- coding: utf-8 -*-
"""
Created on Sun Dec  6 16:47:53 2020

@author: usuario
"""
import numpy as np
from itertools import combinations
from random import shuffle

def swapTS(vector, listaTaboo):
    saltos= []
    vectorcambios = np.array(vector)
    for i in combinations(vector,2):
        saltos.append(i)
    shuffle(saltos)
    j=0
    Vecinos=[]
    while j<25:
         P1,P2= saltos[j][0],saltos[j][1]
         vectorcambios[P1], vectorcambios[P2] = vectorcambios[P2], vectorcambios[P1]
         encontro = False
         for k in range(length(listaTaboo)-1):
            encontro = (vectorcambios == listaTaboo[k,:]
                        if encontro == True:
                            break 
                        
        if encontro == False:
            Vecinos.append(vectorcambios)
            vectorcambios = np.array(vector)
            j+=1
            
    
    
    
    
    
    ###### diccionario con mejor movimiento y su F.O
    diccionarioswap = {"vector": mejorvector, "FO": FuncionObj}
    return diccionarioswap

n = 50 #Numero de bloques
nlistaTabu = 7
ListaTabu = np.zeros([nlistaTabu, n])
############## Generar vector aleatorio ############################
SI  = np.random.permutation(n) #
SI +=1
################# Evaluar la FO

############################### Guadar Incumbente
Inc = SI
FInc = 10000
#### Exploraciones 
for i in range(0,itermax):
    
    for j in range(0,nlistaTabu):
        
        
