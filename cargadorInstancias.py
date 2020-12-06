#Librerías
import pprint as pp
import numpy as np

#Función para cargar instancias csp propuestas por (Beasley, 96)
def beasley(ruta):   
    
    #Preparación del contenedor tipo diccionario que recibirá la lectura del archivo instancia
    instancia = {}
    instancia['nombreInstancia'] = str(ruta)#Nombre del caso de la librería
    instancia['numBloques'] = int()#Total de bloques o servicios
    instancia['minutosJornada'] = int()#Duración total de la jornada en minutos
    instancia['bloques'] = [] #Listado de diccionarios de bloques de trabajo o servicios
    instancia['transiciones'] = []#Listado de diccionarios de transiciones entre bloques o servicios
    instancia['matrizCostos'] = []#Matriz con los costos de transición
    instancia['mayorCostoReactivo'] = 0#Costo reactivo para generar el costo de salida de cada nuevo operador en los turnos
    instancia['matrizTiemposTransicion'] = []#Matriz para facilitar la consulta de factibilidad
    
    #Valor grande para costos prohibitivos
    M = 100000000      
    
    #Intentar abrir el archivo 
    try:
        
        #Ciclo general del recorrido del archivo mientras está abierto
        with open(ruta) as manejadorArchivo: 
            
            #Contador de líneas
            contadorLineas = 0           
         
            #Recorrer cada línea
            for linea_n in manejadorArchivo:
                
                #Cargar línea, limpiar caracteres especiales y volverlo un arreglo
                linea_n = linea_n.strip()
                linea_n = linea_n.split(' ')
                
                #Si es la primera línea realizar capturas correpsondientes
                if not(contadorLineas):
                   instancia['numBloques'], instancia['minutosJornada'] = int(linea_n[0]), int(linea_n[1])  
                else:
                    #Diferenciar si se están cargando los bloques
                    if contadorLineas <= instancia['numBloques']:                        
                        instancia['bloques'].append( { 't0':int(linea_n[0]), 'tf':int(linea_n[1]), 'duracion':int(linea_n[1]) - int(linea_n[0]) } )
                    #Si se están cargando las transiciones
                    elif contadorLineas > instancia['numBloques']:
                        instancia['transiciones'].append({
                                
                             'i': int(linea_n[0]),
                             'j': int(linea_n[1]),
                             'costo': int(linea_n[2])
                                
                            })
                        
                        #Burbuja para calcular el mayor costo reactivo
                        if instancia['transiciones'][-1]['costo'] > instancia['mayorCostoReactivo']:
                            instancia['mayorCostoReactivo'] = instancia['transiciones'][-1]['costo']
                        
                                      
                #Diferenciar la primera línea
                contadorLineas += 1              
            
    except:
         return 'Error en la carga del archivo CSP!!'  
    
    #Incrementar el costo reactivo para diferenciarlo de las transiciones internas
    instancia['mayorCostoReactivo'] = instancia['mayorCostoReactivo'] * 2
    
    #Generar la matriz de costos y de tiempos de transición
    #La matriz de tiempo facilita la consulta de factibilidad
    
    #Inicializar matriz de costos
    dimension = instancia['numBloques']+1
    instancia['matrizCostos'] = np.array([ [M] * dimension ] * dimension )
    instancia['matrizCostos'][0] = instancia['mayorCostoReactivo']    
    instancia['matrizCostos'][:,0] = 0
    instancia['matrizCostos'][0,0] = M
    
    #Inicializar matriz de transiciones
    instancia['matrizTiemposTransicion'] = np.array([ [M] * dimension ] * dimension )
    instancia['matrizTiemposTransicion'][0] = 0    
    instancia['matrizTiemposTransicion'][:,0] = 0
    instancia['matrizTiemposTransicion'][0,0] = M
    
    #Reflejar en las matrices las transiciones (costo y tiempo transcurrido)
    for transicion in instancia['transiciones']:
        
        #Bloques de la transición
        desde = transicion['i']
        hasta = transicion['j']
        
        #Reflejar en la matriz de costos
        instancia['matrizCostos'][desde][hasta] = transicion['costo']
        
        #Reflejar tiempo de transición o espacio entre tareas 
        #*Consumen tiempo del trabajador y agotan el recurso que en este caso es la jornada
        instancia['matrizTiemposTransicion'][desde][hasta] = instancia['bloques'][hasta-1]['t0'] - instancia['bloques'][desde-1]['tf']  
    	       
    
    #Retornar la instancia que se ha cargado
    return instancia 
       
    
#Llamados de diagnóstico
#-----------------------
"""
instancia = beasley('./instances/csp50.txt')	

#Organizar salida en pantalla
print('Bloques:')
for bloque in instancia['bloques']:
    print(bloque)
del bloque
    
print('Transiciones:')
for transicion in instancia['transiciones']:
    print(transicion)
del transicion
""" 

#Matrices cargador versión C++
"""
//Redimensionar matriz de costos y asignar un costo prohibitivo a todas las transiciones
matrizCostosBeasley.resize(num+2);
matrizTiempoTransiciones.resize(num+2);//Reflejar dimensionalidad en la matriz de tiempos de transición
for(size_t i=0;i<num+2;++i){
	matrizCostosBeasley[i].resize(num+2);
	matrizTiempoTransiciones[i].resize(num+2);
	for(size_t j=0;j<num+2;++j){
		//Prohibir loops y conexiones de servicios ilegales
		if(i==j){							
			matrizCostosBeasley[i][j] = 100000000;
			matrizTiempoTransiciones[i][j] = 100000000;
			//matrizTiempoTransiciones[i][j] = this->ht;

		//Costo saliendo (o regresando) del depósito virtual 0
		}else if(i==0 || j==num+1){
			matrizCostosBeasley[i][j] = mayorCostoReactivo * 10;
			matrizTiempoTransiciones[i][j] = 0;																
		
		//Infactibilidad para los demás casos
		}else{								
			matrizCostosBeasley[i][j] = 100000000;
			matrizTiempoTransiciones[i][j] = 100000000;
			//matrizTiempoTransiciones[i][j] = this->ht;
		}
	}			
}

//Prohibir la comunicación entre los depósitos virtuales
matrizCostosBeasley[0][num+1] = 100000000;
matrizTiempoTransiciones[0][num+1] = 0;//No hay consumo de tiempo, es el mismo sitio


//Habilitar las transiciones permitidas en la instancia
for(size_t i=0;i<listadoTransicionesPermitidasBeasley.size();++i){
	
	//Carga en variables temporales para asignación clara en la matriz de costos
	int desde = listadoTransicionesPermitidasBeasley[i].i;	
	int hasta = listadoTransicionesPermitidasBeasley[i].j;
	int costo = listadoTransicionesPermitidasBeasley[i].costoTransicion;

	//Habilitar estableciendo el costo de transición legal
	matrizCostosBeasley[desde][hasta] = costo;

	//Reflejar la transición permitida en el tiempo que consume cambiar de tarea
	matrizTiempoTransiciones[desde][hasta] = this->listadoServiciosBeasley[hasta].t0- this->listadoServiciosBeasley[desde].tf;

}
"""								


