#Función para cargar instancias csp
def beasley(ruta):
    
    
    #Preparación del contenedor tipo diccionario de la instancia
    instancia = {}
    instancia['bloques'] = [] #Listado de diccionarios de bloques de trabajo o servicios
    
    
    
    nombreCaso = str()
    arcos = tuple()
    
    
    
    
    
    #Intentar abrir el archivo 
    try:
        
        #Ciclo general del recorrido del archivo mientras está abierto
        with open(ruta) as manejadorArchivo: 
            
            #Contador de líneas
            contadorLineas = 0
            
         
            coordenadasPuntos = []
            for linea_n in manejadorArchivo: 
                if not(contadorLineas):
                    pass
                
                
                
                linea_n = linea_n.strip()
                linea_n = linea_n.split(' ')     
                coordenadasPuntos.append( [ int(linea_n[0]), float(linea_n[1]), float(linea_n[2]) ] )
            
    except:
        return 'Error en la carga del archivo CSP!!'

    
    
    
    return instancia
               
                
    
       
    
    
    
    
    


#Llamados de diagnóstico



				 

/////////////////////
// Lectura de datos//
/////////////////////


//Mostrar en pantalla la variante que se está trabajando
cout<<endl<< "-- Repositorio Beasley 96 -- " <<endl;
cout<< "----------------------------- " <<endl;
					
//Nombre de la instancia
nombreArchivo = testname;
cout<<endl<<"Nombre del archivo -> "<<testname;//getchar();					

//Total de servicios o eventos
data_input >> num;
cout<<endl<<"Total de servicios -> "<<num;//getchar();

//Duración total de los turnos en minutos
data_input >> ht;
cout<<endl<<"Duración máxima en minutos de un turno -> "<<ht;//getchar();

//Servicio sin duración (asociado al depósito)						
servicio servicioDeposito;										
servicioDeposito.t0 = 0;					
servicioDeposito.tf = 0;
servicioDeposito.duracion = 0;					
listadoServiciosBeasley.push_back(servicioDeposito);

//Realizar la carga de cada uno de los servicios de la instancia
for(size_t i=0;i<num;++i){
	//Contenedor auxiliar	
	servicio servicioAuxiliar;
	//Hora de inicio en minutos						
	data_input >> servicioAuxiliar.t0;
	//Hora de finalización en minutos
	data_input >> servicioAuxiliar.tf;
	servicioAuxiliar.duracion = servicioAuxiliar.tf - servicioAuxiliar.t0;
	
	//Descargar el servicio en el contenedor
	listadoServiciosBeasley.push_back(servicioAuxiliar);

}//Terminación de la carga de servicios

//Almacenar las transiciones posibles (número indeterminado, hasta finalizar el archivo)
while (!data_input.eof()) {				

	//Contenedor auxiliar de transición
	transicion transicionAuxiliar;
	//Desde qué servicio
	data_input >> transicionAuxiliar.i;
	//Ajuste para que coincida con los subíndeces de los contenedores
	//--transicionAuxiliar.i;
	//Hacia cuál servicio
	data_input >> transicionAuxiliar.j;
	//Ajuste para que coincida con los subíndeces de los contenedores
	//--transicionAuxiliar.j;
	//Costo de la transición
	data_input >> transicionAuxiliar.costoTransicion;
	//Actualizar burbuja reactiva de costos
	if(transicionAuxiliar.costoTransicion > mayorCostoReactivo){
		mayorCostoReactivo = transicionAuxiliar.costoTransicion;						
	}

	//Parar si se ha alcanzado el final del archivo después de la última carga
	if(data_input.eof() ){
		break;						
	}

	//Adicionar al atributo multivalor correspondiente en la instancia
	listadoTransicionesPermitidasBeasley.push_back(transicionAuxiliar);
							
}//Terminación de la carga de transiciones posibles

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

//Cerrar el archivo desde donde se está cargando la instancia
data_input.close();					

//Salida en pantalla de validación
cout<<endl;
cout<<"Cantidad de Servicios: "<<listadoServiciosBeasley.size()<<endl;	
cout<<endl;

cout<<endl;
cout<<"Cantidad de Transiciones Permitidas: "<<listadoTransicionesPermitidasBeasley.size()<<endl;	
cout<<endl;										

////Mostrar los servicios almacenados
//cout<<"Servicios Capturados: "<<endl;
//for(size_t i = 0;i<listadoServiciosBeasley.size();++i){
//	cout<<i<<":"<<" t0 = "<<listadoServiciosBeasley[i].t0<<" tf = "<<listadoServiciosBeasley[i].tf<<" duracion = "<<listadoServiciosBeasley[i].duracion<<endl;										
//}

////Mostrar las transiciones permitidas
//cout<<"Transiciones Permitidas: "<<endl;
//for(size_t i = 0;i<listadoTransicionesPermitidasBeasley.size();++i){
//	cout<<i<<":"<<" i = "<<listadoTransicionesPermitidasBeasley[i].i<<" tf = "<<listadoTransicionesPermitidasBeasley[i].j<<" costo = "<<listadoTransicionesPermitidasBeasley[i].costoTransicion<<endl;										
//}

////Mostrar matriz de costos generada
//cout<<endl;
//for(size_t i=0;i<num+2;++i){						
//	for(size_t j=0;j<num+2;++j){							
//		cout<<matrizCostosBeasley[i][j]<<" ";
//	}
//	cout<<endl;
//}

//Parar ejecución para visualzar salida en terminal	
//getchar();
