#Función para cargar instancias csp
def beasley(ruta):
    
    #Preparación de atributos de la instancia o caso de estudio
    
    
    
    #Ciclo general del recorrido del archivo mientras está abierto
    with open(ruta) as manejadorArchivo:
        coordenadasPuntos = []
        for linea_n in manejadorArchivo:    
            linea_n = linea_n.strip()
            linea_n = linea_n.split(' ')     
            coordenadasPuntos.append( [ int(linea_n[0]), float(linea_n[1]), float(linea_n[2]) ] )  
    
    numeroNodos = len(coordenadasPuntos)
    
    #Construir matriz de adyacencia
    matrizAdyacencia = []
    for i in range(numeroNodos):
        matrizAdyacencia.append( [0] *  numeroNodos)
        
    for i in range(numeroNodos):
        for j in range(numeroNodos):
            if i!=j:
                p_1 = [coordenadasPuntos[i][1], coordenadasPuntos[i][2]]
                p_2 = [coordenadasPuntos[j][1], coordenadasPuntos[j][2]]
                distanciaEuclidiana_p1_p2 = int((((p_1[0] - p_2[0])**2 + (p_1[1] - p_2[1])**2)**(1/2))+0.5)
                matrizAdyacencia[i][j] = distanciaEuclidiana_p1_p2            
                
    
    #print(matrizAdyacencia) 
    
    #Distancia euclidiana (consultar)
    #p_1 = [34,56]
    #p_2 = [33,88]
    #distanciaEuclidiana_p1_p2 = int(((p_1[0] - p_2[0])**2 + (p_1[1] - p_2[1])**2)**(1/2))
    
    #Parámetros de entrada (representación del problema)
        
    #Forzar grafo para los ejemplos
    """
    matrizAdyacencia=[]    
    matrizAdyacencia = [	[0,5,8,4,9,12,5,3,13,22],
    			[5,0,6,7,6,8,6,9,12,13],
    			[8,6,0,9,10,12,14,11,9,21],
    			[4,7,9,0,5,9,14,21,16,17],
    			[9,6,10,5,0,8,12,10,9,16],
    			[12,8,12,9,8,0,13,16,12,17],
    			[5,6,14,14,12,13,0,9,11,13],
    			[3,9,11,21,10,16,9,0,14,21],
    			[13,12,9,16,9,12,11,14,0,8],
    			[22,13,21,17,16,17,13,21,8,0]	]
    numeroNodos = len(matrizAdyacencia)
    """
    
    
    
    
    
    
    
    
    pass


#Llamados de diagnóstico


"""
//Establecer parámetros
hb = 300;//Cantidad de minutos máximo por bloque

//Este parámetro se obtiene desde las instancias de Beasley
//ht = 600;//Cantidad de minutos máximo por turno

extrab = 12;//Minutos extra permitidos al máximo de un bloque (5 horas)
extrat = 20;//Minutos extra permitidos al máximo de un turno (10 horas)
balance = 6;//Eventos o servicios mínimos en un bloque
PosiblesBloques = 4;//Cantidad de bloques máxima en los que se puede partir una tabla (esclavo)						
c = 1; //Costo asociado a la función objetivo de las columnas iniciales
descanso = 30; //Descanso entre bloques
descanson = 480; //Descanso entre dias

////Conjunto de puntos de intercambio (parámetro establecido por código)
//vector<string> puntosIntercambio;
//puntosIntercambio.push_back("Viajero");
//puntosIntercambio.push_back("Intercambiador_D/das");
//puntosIntercambio.push_back("Intercambiador_Cuba");
//puntosIntercambio.push_back("Estacion_Milan");					 
//puntosIntercambio.push_back("Estacion_Cam");					 
//puntosIntercambio.push_back("Integra_S.A.");					 
//puntosIntercambio.push_back("Integra");					 

/////////////////////
// Lectura de datos//
/////////////////////

//Abrir el archivo con la matriz de costos e información de los depósitos
ifstream data_input((testname).c_str());				
if (! data_input.is_open() ) {
	cout << "     Can not read the CSP IntegraUniAndes file " << (testname).c_str() << endl;
	exit(1);
	//return EXIT_FAILURE;
	//return 1;
}

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
"""