//Crew Scheduling Generación de Columnas Daniel Esteban UniAndes
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <time.h>
#include <iomanip>
#include <sys/time.h>
#include <ilcplex/ilocplex.h>
#include <chrono>       // std::chrono::system_clock

using namespace std;

///////Definición de tipos para arreglos de CPLEX de 1,2 y 3 dimensiones
//typedef IloArray <IloIntVarArray> IloIntVarArray2;

//typedef IloArray<IloNumVarArray> NumVarMatrix;
//typedef IloArray<NumVarMatrix> NumVar3Matrix;

typedef IloArray <IloBoolVarArray> IloBoolVarArray2;

typedef IloArray <IloBoolArray> IloBoolArray2;

typedef IloArray<IloBoolVarArray> BoolVarMatrix;
typedef IloArray<BoolVarMatrix> BoolVar3Matrix;

typedef IloArray<IloBoolArray> BoolMatrix;
typedef IloArray<BoolMatrix> Bool3Matrix;

typedef IloArray<IloIntVarArray> IntMatrix;

/////////////////////////////
//Funciones complementarias//
/////////////////////////////
double distxy(double x1, double y1, double x2, double y2) {
    return ( sqrt(pow(x1 - x2,2) + pow(y1 - y2,2)) );
}

//Función que retorna "a - b" en segundos 
double timeval_diff(struct timeval *a, struct timeval *b){
  return
    (double)(a->tv_sec + (double)a->tv_usec/1000000) -
    (double)(b->tv_sec + (double)b->tv_usec/1000000);
}

//Generación de números aleatorios
double distribucionUniforme(double a, double b){
	random_device rd;
	mt19937 gen(rd());
	//mt19937 gen(1);
	uniform_real_distribution<> dis(a,b);	
	return dis(gen);
}
//temp.distancia   = distribucionUniforme((1-instancia.deltha)*instancia.matrizDistancias[(*it_cliente).i-1][(*it_deposito).i-1],(1+instancia.deltha)*instancia.matrizDistancias[(*it_cliente).i-1][(*it_deposito).i-1]);

//Función para generar enteros aleatoriamente (orientada a random_shuffle)
//int myrandom (int i) { return std::rand()%i;}
int myrandom (int i) { return int(distribucionUniforme(0,i));}


////////////////////////////////////////////////////////////////////////
//Contenedores de la información de las instancias o casos de estudio///
////////////////////////////////////////////////////////////////////////


//Estructuras Rostering

struct turno {
	
	//Identificador del turno
	int id;
	int hora_entrada;
	int hora_salida;
	int duracion_turno;
	
	string clasificacion;//Campo adicional para tipificación de turnos y ajustar al caso de Integra
	
	//Posibles constructores de los turnos
	turno() : hora_entrada(0), hora_salida(0), duracion_turno(0) {}
	
};

struct dia {
	
	int numeroTurnos;
	vector<turno> turnos;
	
	//Posibles constructores de los días
	dia() : numeroTurnos(0) {}
	dia(int a) : numeroTurnos(a), turnos(a) {}
	
};

//Caracterización de los turnos para instancias Musliu 2006
struct tipoTurnoMusliu {
	
	//Formato de la instancia
	//#ShiftName, Start, Length, Name, MinlengthOfBlocks, MaxLengthOfBlocks
	//D  360 480 2 7
	
	//Atributos
	string ShiftName;
	int Start;
	int End;
	int Length;
	int MinlengthOfBlocks;
	int MaxLengthOfBlocks;	
	
	//Posibles constructores de los turnos Musliu
	tipoTurnoMusliu() : Start(0), Length(0), MinlengthOfBlocks(0), MaxLengthOfBlocks(0)  {}
	
};


//Función para determinar si la ubicación está en el conjunto de puntos de intercambio
bool esta(vector<string> PuntosIntercambio, string inicio){
	
	//Suponer que no está
	bool bandera = false;
	
	//Recorrer conjunto
	for(int i=0;i<PuntosIntercambio.size();++i){
		
		if(PuntosIntercambio[i] == inicio){
			
			//Avisar que sí está
			bandera = true;		
			return bandera;
			
			
		}
		
	}
	
}


//Tipo de dato compuesto que me representa un bloque de trabajo
struct bloque {	
	//Atributos	
	string nombreTabla;//Ruta	
	float t0b;//Hora de inicio del servicio (minutos)
	string iniciob;//Lugar donde inicia el servicio
	float tfb;//Hora de finalización del servicio (minutos)
	string finb;//Lugar donde finaliza el servicio	
	float duracionb;//Minutos de duración del servicio
};

//Solución esclavo
struct solucionEsclavo{
};


//Tipo de dato compuesto que me representa un evento
struct servicio {	
	
	//Atributos
	string nombreTabla;//Ruta
	int evento;//Autonumérico
	float t0;//Hora de inicio del servicio (minutos)
	string inicio;//Lugar donde inicia el servicio
	float tf;//Hora de finalización del servicio (minutos)
	string fin;//Lugar donde finaliza el servicio	
	float duracion;//Minutos de duración del servicio
	int noI;//Si es verdadero, (1), no inicia en intercambiador
	
	//Constructores	
	servicio(){}
	servicio(int a, float b, string c, float d, string e, float f) : evento(a), t0(b), inicio(c), tf(d), fin(e), duracion(f) {}
	
};

//Contenedor de tablas
struct tabla {	
	string nombreTabla;//Ruta		
	vector< servicio > eventos;//servicios que pertenecen a la tabla	
	vector< bloque > bloques;//servicios que pertenecen a la tabla	
	int EventosTabla;//Eventos o servicios pertenecientes a la tabla actual
};

//Transiciones permitidas instancias Beasley 96
struct transicion {
	int i;
	int j;
	int costoTransicion;	
};

//Contenedores Punto inicial Beasley Modificado
struct transicionBeasleyModificado{

	//Atributos de la transición permitida
	int i;
	int j;
	int costo;
	
	//Operadores de ordenamiento para este TAD
	bool operator<(const transicionBeasleyModificado& b) const { return (this->costo < b.costo); }
    bool operator>(const transicionBeasleyModificado& b) const { return (this->costo > b.costo); }    
    
    //Posibles constructores
	transicionBeasleyModificado() : costo(0) {}
	transicionBeasleyModificado(int a, int b, int c) : i(a),j(b),costo(c) {}

};

//Contenedores Punto inicial Beasley Modificado
struct solucionConstructivoPropuesto{

	//Atributos de la transición permitida	
	int costo;
	int numeroTurnos;
	vector < vector < int > > turnos;//Especificación de los turnos en el contenedor
	vector < vector < int > > arcos;//Arcos para activar en el modelo
	vector < vector < int > > y_i;//Carga laboral acumulada hasta cada servicio en los turnos
	
	//Operadores de ordenamiento para este TAD
	bool operator<(const solucionConstructivoPropuesto& b) const { return (this->costo < b.costo); }
    bool operator>(const solucionConstructivoPropuesto& b) const { return (this->costo > b.costo); }
	//bool operator<(const solucionConstructivoPropuesto& b) const { return (this->numeroTurnos < b.numeroTurnos); }
    //bool operator>(const solucionConstructivoPropuesto& b) const { return (this->numeroTurnos > b.numeroTurnos); }    
    
    //Posibles constructores
	//----------------------
	//Sin parámetros
	solucionConstructivoPropuesto() : costo(0), numeroTurnos(0) {}
	//Para ordenamiento rápido
	solucionConstructivoPropuesto(vector < vector < int > > t, int nt, int c) : turnos(t), numeroTurnos(nt), costo(c) {}	
	//Completo
	solucionConstructivoPropuesto(vector < vector < int > > t, int nt, vector < vector < int > > a, int c, vector < vector < int > > y) : turnos(t), numeroTurnos(nt), arcos(a), costo(c), y_i(y) {}	

};

//Instancia o caso completo
struct casoCrewScheduling {
	
	//Atributos Beasley 96 Modificado
	int mayorCostoReactivo = 0;//Mayor costo reactivo para penalizaciones en el modelo
	vector < servicio > listadoServiciosBeasley;//Todos los servicios
	vector < transicion > listadoTransicionesPermitidasBeasley;//Las transiciones permitidas en el modelo
	vector < vector < int > > matrizCostosBeasley;//Costos de las transiciones (unidades genéricas)	
	vector < vector < int > > matrizTiempoTransiciones;//Tiempo de transición (10e-8 para infactibilidades)	
	int numeroConductoresDisponibles = 500;
	vector < vector < int > > arcosBeasleyModificado;//Contenedor de las aristas solución
	vector < vector < int > > turnosConstructivo;//Contenedor de la especificación de los turnos del constructivo
	vector < vector < int > > y_i_Constructivo;//Contenedor de la especificación del tiempo transcurrido en cada paso del turno
	vector < int > duracionTurnosConstructivo;//Tiempo en minutos que corresponde al contenedor donde están especificados los turnos
	int funcionObjetivoConstructivo = 0;//Si se utiliza el constructivo, almacenar la fo sin salidas y retornos
	int numeroConductoresConstructivo = 0;//Si se utiliza el constructivo, almacenar el número de conductores del constructivo
	double tiempoComputoConstructivo = 0;//Tiempo en segundos que tardó el constructivo
	double tiempoComputoModelo = 0;//Tiempo en segundos que tardó el modelo

	//Atributos para las instancias CSP - GC
	string nombreArchivo;
	int CantidadTablas;
	vector < tabla > TablasInstancia;	
	vector< vector< vector< int > > > A;//Matriz Identidad de turnos iniciales
	vector< vector < int > > columnaAuxiliar;//Columna generada por el problema auxiliar para agregar a A
	vector< vector< vector< int > > > Y_sol; //Solucion del esclavo
	vector< vector < float > > DUAL_MP;//Duales de las restricciones del maestro	
	
	//Constantes (Tuning de la metodología)
	float hb;//Cantidad de minutos máximo por bloque
	float ht;//Cantidad de minutos máximo por turno
	float extrab;//Minutos extra permitidos al máximo
	float extrat;//Minutos extra permitidos al máximo del turno
	int balance;//Eventos o servicios mínimos en un bloque
	int PosiblesBloques;//Cantidad de bloques máxima en los que se puede partir una tabla	
	int num;//Cantidad total eventos o servicios del caso	
	int c; //Costo asociado a la función objetivo de las columnas iniciales
	float descanson; //Descanso minimo nocturno
	float descanso;  //Descanso minimo obligatorio entre bloques
	
	//Posibles constructores de la instancia
	casoCrewScheduling() : num(0), CantidadTablas(0) {}
	//casoCrewScheduling(int a, int b) : numeroTurnosMaximo(a), numeroConductores(b) {}	
	
	//Constructor: Seleccionar el tipo de variante de programación de conductores para proceder a realizar la carga
	casoCrewScheduling(string testname, int idVarianteCSP){
		
		//Identificar el tipo de carga especificado por el parámetro de entrada 
		switch (idVarianteCSP){
			
			
			//Variante Datos GC UniAndes
			case 0:
			{
				
					//Establecer parámetros
					hb = 300;//Cantidad de minutos máximo por bloque
					ht = 600;//Cantidad de minutos máximo por turno
					extrab = 12;//Minutos extra permitidos al máximo de un bloque (5 horas)
					extrat = 20;//Minutos extra permitidos al máximo de un turno (10 horas)
					balance = 6;//Eventos o servicios mínimos en un bloque
					PosiblesBloques = 4;//Cantidad de bloques máxima en los que se puede partir una tabla (esclavo)						
					c = 1; //Costo asociado a la función objetivo de las columnas iniciales
					descanso = 30; //Descanso entre bloques
					descanson = 480; //Descanso entre dias
					
					//Conjunto de puntos de intercambio (parámetro establecido por código)
					vector<string> puntosIntercambio;
					puntosIntercambio.push_back("Viajero");
					puntosIntercambio.push_back("Intercambiador_D/das");
					puntosIntercambio.push_back("Intercambiador_Cuba");
					puntosIntercambio.push_back("Estacion_Milan");					 
					puntosIntercambio.push_back("Estacion_Cam");					 
					puntosIntercambio.push_back("Integra_S.A.");					 
					puntosIntercambio.push_back("Integra");					 
			
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
					
					//Nombre de la instancia
					nombreArchivo = testname;
					cout<<endl<<"     Nombre del archivo -> "<<testname;//getchar();					
					
					//Total de servicios o eventos
					data_input >> num;
					cout<<endl<<"     Total de servicios de eventos -> "<<num;//getchar();
					
					//Omitir enunciado de la longitud de la programación
					string foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					
					
					//Tabla auxiliar
					//--------------	
					//string nombreTabla;//Ruta		
					//vector< servicio > eventos;//servicios que pertenecen a la tabla	
					//int CantidadEventosTabla;//Eventos o servicios pertenecientes a la tabla actual
					tabla tablaAuxiliar;					
					
					//Recorrer todos los servicios registrados en el archivo
					for(int i=0;i<num;++i){
						
						//Validar si es tabla vacía
						if(tablaAuxiliar.eventos.empty()){
							
							//Cargar el nombre de la tabla
							data_input >> tablaAuxiliar.nombreTabla;
							
							//Carga del servicio
							servicio servicioAuxiliar;
							servicioAuxiliar.nombreTabla = tablaAuxiliar.nombreTabla;
							data_input >> servicioAuxiliar.evento;
							data_input >> servicioAuxiliar.t0;
							data_input >> servicioAuxiliar.inicio;
							data_input >> servicioAuxiliar.tf;
							data_input >> servicioAuxiliar.fin;
							data_input >> servicioAuxiliar.duracion;							
							if(!esta(puntosIntercambio, servicioAuxiliar.inicio)){
								servicioAuxiliar.noI = 1;
							}else{
								servicioAuxiliar.noI = 0;
							}							
							
							//Carga del servicio en la tabla														
							tablaAuxiliar.eventos.push_back(servicioAuxiliar);														
							
						}else{//Si la tabla ya tiene eventos							
							
							//Carga del servicio
							servicio servicioAuxiliar;
							data_input >> servicioAuxiliar.nombreTabla;
							data_input >> servicioAuxiliar.evento;
							data_input >> servicioAuxiliar.t0;
							data_input >> servicioAuxiliar.inicio;
							data_input >> servicioAuxiliar.tf;
							data_input >> servicioAuxiliar.fin;
							data_input >> servicioAuxiliar.duracion;							
							if(!esta(puntosIntercambio, servicioAuxiliar.inicio)){
								servicioAuxiliar.noI = 1;
							}else{
								servicioAuxiliar.noI = 0;
							}							
							
							//Validar si el servicio pertenece a la misma tabla
							if(tablaAuxiliar.nombreTabla == servicioAuxiliar.nombreTabla){
								
								//Carga del servicio en la tabla
								tablaAuxiliar.eventos.push_back(servicioAuxiliar);
								
							}else{//Tabla diferente
								
								//Carga de la tabla en la instancia
								tablaAuxiliar.EventosTabla = tablaAuxiliar.eventos.size();
								TablasInstancia.push_back(tablaAuxiliar);
								
								//Limpiar los eventos en la tabla auxiliar
								tablaAuxiliar.eventos.clear();	
								
								//Carga del servicio en la tabla
								tablaAuxiliar.nombreTabla = servicioAuxiliar.nombreTabla;								
								tablaAuxiliar.eventos.push_back(servicioAuxiliar);									
								
							}//Fin if tabla diferente						
							
							
						}//Fin del if eventos vacíos					
						
						
					}//Fin recorrido registros
					
					//Descarga de la última tabla si tiene eventos
					if(!tablaAuxiliar.eventos.empty()){
						
						//Carga de la tabla en la instancia
						tablaAuxiliar.EventosTabla = tablaAuxiliar.eventos.size();
						TablasInstancia.push_back(tablaAuxiliar);
						
						//Limpiar los eventos en la tabla auxiliar
						tablaAuxiliar.eventos.clear();	
						
					}
					
					
					//Calcular la cantidad de tablas
					CantidadTablas = TablasInstancia.size();				

					//Cerrar el archivo que contiene la matriz de costos
					data_input.close();
					
					
					//Salida en pantalla de validación
					cout<<endl;
					cout<<"     Cantidad de Tablas: "<<CantidadTablas<<endl;	
					cout<<endl;					
					//Especificación primeras dos tablas										
					//for(int i=0;i<2;++i){
					//	
					//	cout<<endl;
					//	cout<<"Nombre de la tabla: "<<TablasInstancia[i].nombreTabla<<endl;
					//	cout<<"Número de eventos de la tabla: "<<TablasInstancia[i].EventosTabla<<endl;
					//	cout<<"Eventos:"<<endl;
					//	for(int j=0;j<TablasInstancia[i].EventosTabla;++j){
					//		
					//		cout<<TablasInstancia[i].eventos[j].nombreTabla<<" ";
					//		cout<<TablasInstancia[i].eventos[j].evento<<" ";
					//		cout<<TablasInstancia[i].eventos[j].t0<<" ";
					//		cout<<TablasInstancia[i].eventos[j].inicio<<" ";
					//		cout<<TablasInstancia[i].eventos[j].tf<<" ";
					//		cout<<TablasInstancia[i].eventos[j].fin<<" ";
					//		cout<<TablasInstancia[i].eventos[j].duracion<<" ";						
					//		cout<<TablasInstancia[i].eventos[j].noI<<" ";						
					//		
					//		cout<<endl;
					//	}						
					//	
					//}
					
					//Redimensión del contenedor de la solución del esclavo
					Y_sol.resize(CantidadTablas);	
					for(auto t=0 ; t < CantidadTablas; ++t){
						Y_sol[t].resize(TablasInstancia[t].EventosTabla);						
						for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){							
							Y_sol[t][e].resize(PosiblesBloques);
						}
					}
					
// mejorado //		//Redimensión y construcción de la matriz A
					//vector< vector< vector< int > > > A;//Matriz Identidad de turnos iniciales
					//A.resize(CantidadTablas);	
					//for(auto t=0 ; t < CantidadTablas; ++t){
					//	A[t].resize(TablasInstancia[t].EventosTabla);						
					//	for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){							
					//		A[t][e].resize(num);
					//	}
					//}
					//
					//int contadorNum = 0;
					//for(auto t=0 ; t < CantidadTablas; ++t){						
					//	for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){
					//		
					//		A[t][e][contadorNum] = 1;
					//		++contadorNum;							
					//									
					//	}
					//}
					
					
					////Salida diagnóstico de A
					//cout<<endl<<"Dimensiones de A -> "<<A.size()<<" "<<A[0].size()<<" "<<A[0][0].size();				
					//cout<<endl<<"Dimensiones de A -> "<<A.size()<<" "<<A[10][0].size();				
					//
					//int sumatoriaUnosA = 0;
					//for(auto t=0 ; t < CantidadTablas; ++t){						
					//	for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){
					//		for(auto i=0 ; i < num; ++i){
					//			
					//			sumatoriaUnosA+=A[t][e][i];
					//			
					//			
					//			
					//		}						
					//	}
					//}
					//
					////cout<<endl<<"Sum ones A -> "<<sumatoriaUnosA;				
					//
					//
					//
					////Archivo de salida para corridas en bloque
					//ofstream archivoMatriz("matrizA.csv",ios::app | ios::binary);		
					////archivoMatriz<<setprecision(10);//Cifras significativas para el reporte
					//if (!archivoMatriz.is_open()){
					//	cout << "Fallo en la apertura del archivo de salida MATRIZ A.";				
					//}else{
					//	
					//	
					//	int sumatoriaUnosA = 0;
					//	for(auto t=0 ; t < CantidadTablas; ++t){						
					//		for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){
					//			
					//			archivoMatriz<<TablasInstancia[t].nombreTabla<<";"<<e+1<<";";
					//			
					//			
					//			for(auto i=0 ; i < num; ++i){
					//				
					//				
					//				archivoMatriz<<A[t][e][i]<<";";
					//				
					//				
					//			}
					//			
					//			archivoMatriz<<endl;
					//									
					//		}
					//		
					//	}
					//	
					//	
					//	
					//}
					//archivoMatriz.close();	
					//
					//
					//getchar();
					
					
					//Redimensión del contenedor de las duales del maestro
					DUAL_MP.resize(CantidadTablas);	
					for(auto t=0 ; t < CantidadTablas; ++t){
						DUAL_MP[t].resize(TablasInstancia[t].EventosTabla);												
					}					
					
					//Redimensionar contenedor de la columna auxiliar
					columnaAuxiliar.resize(CantidadTablas);					
					for(auto t = 0u; t< CantidadTablas; ++t){
						columnaAuxiliar[t].resize(TablasInstancia[t].EventosTabla);
					}								
					
					
			}break;//Fin de carga de datos Integra UniAndes		
			

			//Variante Beasley
			case 1:
			{
				
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
					
					
			}break;//Fin de carga de datos Beasley 96
			
			//Valor indeterminado en los argumentos enviados por la regla make o la línea de comandos
			default: cout<<endl<< "Variante de carga de datos para los modelos inexistente!!!!!";
			break;
			
		}//Final del selector de carga de datos		
			
		
	}//Fin del constructor para carga de datos desde archivo
	
	
	
//Modelo Auxiliar
float modeloGCAuxiliar(){	
	
		float valorFuncionObjetivo = 0;
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO AUXILIAR
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		// los parametros de entrada son dinamicos para resolverf el problema maestro.
		
		////////////Variable U_tb que indica si se selecciona el bloque b de la tabla t	
		char name_s[200];//Vector para nombrar el conjunto de variables de decisión'U'
		BoolVarMatrix U (env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			U[t]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<PosiblesBloques;++b){					
				sprintf(name_s,"U_%u_%u",t,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					U[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}
		
		/////////////////////////////////////////////////////////////////
		
		//////////// Variable Z_te que indica si se selecciona el evento e de la tabla t 		
		
		BoolVarMatrix Z (env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			Z[t]=IloBoolVarArray(env,TablasInstancia[t].EventosTabla);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int e=0;e<TablasInstancia[t].EventosTabla;++e){					
				sprintf(name_s,"Z_%u_%u",t,e);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					Z[t][e] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}
		
		/////////////////////////////////////////////////////////////////
		
		//////////// Variable first_tb que indica si el bloque b de la tabla t es el primer bloque del turno
		
		BoolVarMatrix first (env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			first[t]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<PosiblesBloques;++b){					
				sprintf(name_s,"first_%u_%u",t,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					first[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}	
		/////////////////////////////////////////////////////////////////
		
		//Variable last_tb que indica que el bloque b de la tabla t es el último bloque de un turno	
		
		BoolVarMatrix last(env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			last[t]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<PosiblesBloques;++b){					
				sprintf(name_s,"last_%u_%u",t,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					last[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}
		
		
		/////////////////////////////////////////////////////////////////	
		
		//Variable middle_tb que indica si el bloque b de la tabla t está en la mitad del turno en construcción 
		
		BoolVarMatrix middle(env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			middle[t]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<PosiblesBloques;++b){					
				sprintf(name_s,"middle_%u_%u",t,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					middle[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}				
		
		
		/////////////////////////////////////////////////////////////////
		
		//Variable para definir la hora de entrada
		IloFloatVar ltime (env,0,1440);
		
		//Variable para definir la hora de salida
		IloFloatVar utime (env,0,1440);
		
		//Variable para definir si el turno tiene un solo bloque
		IloBoolVar bsolo(env);	
		
		
		//******************************************************************
		//* 					RESTRICCION MODELO MAESTRO AUXILIAR
		//******************************************************************
		
		//Relación de variables entre z y u (R23)
		IloRangeArray BloquesEvento(env);
		stringstream nombreBloquesEvento;		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){				
				for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){										
					if(Y_sol[t][e][b]){						
						IloExpr restriccionBloquesEvento(env);						
						restriccionBloquesEvento += Z[t][e] - U[t][b];						
						nombreBloquesEvento << "BloquesEvento_{ Z_"<< t<<","<<e<<"_U_"<<t<<","<<b<<"}";
						BloquesEvento.add (IloRange(env, 0, restriccionBloquesEvento, 0, nombreBloquesEvento.str().c_str()));
						nombreBloquesEvento.str(""); // Limpiar variable para etiquetas de las restricciones
					}
				}				
			}
		}		
		//Adicionar restricciones 
		modelo.add(BloquesEvento);			
		
		//Conjunto de restricciones que evaluan la que la solucion no tenga más de 3 bloques (R25 y R26)
		IloRangeArray CantidadBloques(env);
		stringstream nombreCantidadBloques;
		IloExpr restriccionCantidadBloques(env);
		
		IloRangeArray CantidadBloquesO(env);
		stringstream nombreCantidadBloquesO;
		IloExpr restriccionCantidadBloquesO(env);
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){//Una restricción por cada bloque de cada tabla
				if(TablasInstancia[t].bloques[b].t0b > 0){
					restriccionCantidadBloques += U[t][b];
				}else{
					restriccionCantidadBloquesO += U[t][b];
				}
				
			}
		}	
		nombreCantidadBloques << "CantidadBloques_{}";
		CantidadBloques.add (IloRange(env, -IloInfinity, restriccionCantidadBloques, 3, nombreCantidadBloques.str().c_str()));
		//CantidadBloques.add (IloRange(env, 1, restriccionCantidadBloques, 3, nombreCantidadBloques.str().c_str()));
		nombreCantidadBloques.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(CantidadBloques); 
		
		nombreCantidadBloquesO << "CantidadBloquesO_{}";
		CantidadBloquesO.add (IloRange(env, 0, restriccionCantidadBloquesO, 0, nombreCantidadBloquesO.str().c_str()));
		nombreCantidadBloquesO.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(CantidadBloquesO); 
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
			
			
		//Conjunto de restricciones que asegura que el bloque b de la tabla T es el primer bloque (R27)
			
		IloRangeArray EvaluacionPrimerBloque(env);
		stringstream nombreEvaluacionPrimerBloque;
		IloExpr restriccionEvaluacionPrimerBloque(env);
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){//Una restricción por cada bloque de cada tabla
				restriccionEvaluacionPrimerBloque += first[t][b];
			}
		}	
		nombreEvaluacionPrimerBloque << "EvaluacionPrimerBloque_{}";
		EvaluacionPrimerBloque.add (IloRange(env, 1, restriccionEvaluacionPrimerBloque, 1, nombreEvaluacionPrimerBloque.str().c_str()));
		nombreEvaluacionPrimerBloque.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(EvaluacionPrimerBloque); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					
		//Conjunto de restricciones que asegura que el bloque b de la tabla T es el ultimo bloque (R28)
			
		IloRangeArray EvaluacionUltimoBloque(env);
		stringstream nombreEvaluacionUltimoBloque;
		IloExpr restriccionEvaluacionUltimoBloque(env);
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionEvaluacionUltimoBloque += last[t][b];
			}
		}	
		nombreEvaluacionUltimoBloque << "EvaluacionUltimoBloque_{}";
		EvaluacionUltimoBloque.add (IloRange(env, 1, restriccionEvaluacionUltimoBloque, 1, nombreEvaluacionUltimoBloque.str().c_str()));
		nombreEvaluacionUltimoBloque.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(EvaluacionUltimoBloque); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Conjunto de restricciones que asegura que el bloque b de la tabla T es el bloque de la mitad (R29)
			
		IloRangeArray EvaluacionBloqueMitad(env);
		stringstream nombreEvaluacionBloqueMitad;
		IloExpr restriccionEvaluacionBloqueMitad(env);
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionEvaluacionBloqueMitad += middle[t][b];
			}
		}	
		nombreEvaluacionBloqueMitad << "EvaluacionBloqueMitad_{}";
		EvaluacionBloqueMitad.add (IloRange(env, -IloInfinity, restriccionEvaluacionBloqueMitad, 1, nombreEvaluacionBloqueMitad.str().c_str()));
		nombreEvaluacionBloqueMitad.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(EvaluacionBloqueMitad); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Relación entre selección general y selección como primera (R30)
			
		IloRangeArray BloqueInicial(env);
		stringstream nombreBloqueInicial;
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){//Una restricción por cada bloque de cada tabla
				IloExpr restriccionBloqueInicial(env);
				restriccionBloqueInicial = first[t][b] - U[t][b];
				nombreBloqueInicial << "BloqueInicial_{}";
				BloqueInicial.add (IloRange(env, -IloInfinity, restriccionBloqueInicial, 0, nombreBloqueInicial.str().c_str()));
				nombreBloqueInicial.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		
		//Adicionar restricciones 
		modelo.add(BloqueInicial); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		//Relación entre selección general y selección como último (bloque final) (R31)
			
		IloRangeArray BloqueFinal(env);
		stringstream nombreBloqueFinal;
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				IloExpr restriccionBloqueFinal(env);
				restriccionBloqueFinal = last[t][b] - U[t][b];
				nombreBloqueFinal << "BloqueMitad_{"<< t<<","<<b<<"}";
				BloqueFinal.add (IloRange(env, -IloInfinity, restriccionBloqueFinal, 0, nombreBloqueFinal.str().c_str()));
				nombreBloqueFinal.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		
		//Adicionar restricciones 
		modelo.add(BloqueFinal); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		////Relación entre selección general y selección como bloque de la mitad (R32)
			
		IloRangeArray BloqueMitad(env);
		stringstream nombreBloqueMitad;
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				IloExpr restriccionBloqueMitad(env);
				restriccionBloqueMitad = middle[t][b] - U[t][b];
				nombreBloqueMitad << "BloqueMitad_{"<< t<<","<<b<<"}";
				BloqueMitad.add (IloRange(env, -IloInfinity, restriccionBloqueMitad, 0, nombreBloqueMitad.str().c_str()));
				nombreBloqueMitad.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		
		//Adicionar restricciones 
		modelo.add(BloqueMitad); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		//Ecuación que controla la asignación del bloque al turno (R33)
			
		IloRangeArray ControlBloques(env);
		stringstream nombreControlBloques;
		
		IloExpr	restriccionControlBloques(env); //// expresion que guarda los terminos
		IloExpr SumaBI(env); // Expresion bloques iniciales
		IloExpr SumaBM(env); // Expresion bloques medios
		IloExpr SumaBF(env); // Exprsion bloques finales
		IloExpr SumaUTB(env); // Exprsion turnos en los bloques	
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				SumaBI += first[t][b];
				SumaBM += middle[t][b];
				SumaBF += last[t][b];
				SumaUTB += U[t][b];
			}
		}	
		restriccionControlBloques =  SumaBF + SumaBM + SumaBI - SumaUTB;
		nombreControlBloques << "ControlBloques_{}";
		ControlBloques.add (IloRange(env, 0, restriccionControlBloques, IloInfinity, nombreControlBloques.str().c_str()));
		nombreControlBloques.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(ControlBloques); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		//ecuación que controla la asignación de bloques medios (R34)
			
		IloRangeArray ControlBloquesM(env);
		stringstream nombreControlBloquesM;
		
		IloExpr	restriccionControlBloquesM(env); //// expresion que guarda los términos
		IloExpr SumaBloquesM(env); // Expresion bloques medio divividos entre 2 
		
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionControlBloquesM += middle[t][b];
				SumaBloquesM += U[t][b];
			}
		}	
		SumaBloquesM = (SumaBloquesM - 1)/2;
		restriccionControlBloquesM =  restriccionControlBloquesM - SumaBloquesM;
		nombreControlBloquesM << "ControlBloquesM_{}";
		ControlBloquesM.add (IloRange(env, -IloInfinity, restriccionControlBloquesM, 0, nombreControlBloquesM.str().c_str()));
		nombreControlBloquesM.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(ControlBloquesM); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		//Ecuación que controla la duración del turno creado (R35)
			
		IloRangeArray ControlDuracion(env);
		stringstream nombreControlDuracion;
		
		IloExpr	restriccionControlDuracion(env); 
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionControlDuracion += U[t][b]* TablasInstancia[t].bloques[b].duracionb;
			}
		}	
		restriccionControlDuracion = restriccionControlDuracion - ht - extrat;
		nombreControlDuracion << "ControlBloquesM_{}";
		ControlDuracion.add (IloRange(env, -IloInfinity, restriccionControlDuracion, 0, nombreControlDuracion.str().c_str()));
		nombreControlDuracion.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(ControlDuracion); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		
		//La hora de inicio y de fianl deben ser positivas (R36)
			
		IloRangeArray HoraLTime(env);
		stringstream nombreHoraLTime;
		IloExpr	restriccionHoraLTime(env); 
		
		IloRangeArray HoraUTime(env);
		stringstream nombreHoraUTime;
		IloExpr	restriccionHoraUTime(env);
			
		restriccionHoraLTime=ltime;
		restriccionHoraUTime=utime;
		nombreHoraLTime << "nombreHoraLTime_{}";
		nombreHoraUTime << "nombreHoraUTime_{}";
		
		HoraLTime.add (IloRange(env, 0, restriccionHoraLTime, IloInfinity, nombreHoraLTime.str().c_str()));
		nombreHoraLTime.str(""); // Limpiar variable para etiquetas de las restricciones
		
		HoraUTime.add (IloRange(env, 0, restriccionHoraUTime, IloInfinity, nombreHoraUTime.str().c_str()));
		nombreHoraUTime.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(HoraLTime); 
		modelo.add(HoraUTime);
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
		//Cota superior de la hora de entrada 		(R37)
		
		IloRangeArray MaximoLTime(env);
		stringstream nombreMaximoLTime;
		
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				
				IloExpr	restriccionMaximoLTime(env);
				
				restriccionMaximoLTime=ltime - TablasInstancia[t].bloques[b].t0b*U[t][b]-(1-U[t][b])*1440;
				nombreMaximoLTime << "nombreMaximoLTime_{"<< t<<","<<b<<"}";
				
				MaximoLTime.add (IloRange(env, -IloInfinity, restriccionMaximoLTime, 0, nombreMaximoLTime.str().c_str()));
				nombreMaximoLTime.str(""); // Limpiar variable para etiquetas de las restricciones
				
			}
		}		
		
		
		//Adicionar restricciones 
		modelo.add(MaximoLTime); 

		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Cota inferior de la hora de entrada 		(R38)
		
		IloRangeArray MinimoLTime(env);
		stringstream nombreMinimoLTime;		
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){				
				IloExpr	restriccionMinimoLTime(env);
				restriccionMinimoLTime=ltime-TablasInstancia[t].bloques[b].t0b * first[t][b];
				nombreMinimoLTime << "nombreMinimoLTime_{"<< t<<","<<b<<"}";
				MinimoLTime.add (IloRange(env, 0, restriccionMinimoLTime, IloInfinity, nombreMinimoLTime.str().c_str()));
				nombreMinimoLTime.str(""); // Limpiar variable para etiquetas de las restricciones				
			}
		}		
		//Adicionar restricciones 
		modelo.add(MinimoLTime); 
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
		//Cota superior de la hora de salida 		(R39)
		
		IloRangeArray MaximoUTime(env);
		stringstream nombreMaximoUTime;
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				
				
				IloExpr	restriccionMaximoUTime(env);
				
				restriccionMaximoUTime= utime - TablasInstancia[t].bloques[b].tfb * last[t][b] - (1-last[t][b])*1440;
				nombreMaximoUTime << "nombreMaximoUTime_{"<< t<<","<<b<<"}";
				
				MaximoUTime.add (IloRange(env, -IloInfinity, restriccionMaximoUTime, 0, nombreMaximoUTime.str().c_str()));
				nombreMaximoUTime.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}
		
		
		//Adicionar restricciones 
		modelo.add(MaximoUTime); 

		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Cota inferior de la hora de salida 		(R40)
		
		IloRangeArray MinimoUTime(env);
		stringstream nombreMinimoUTime;
		
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				IloExpr	restriccionMinimoUTime(env);
				restriccionMinimoUTime=utime-TablasInstancia[t].bloques[b].tfb * U[t][b];
				nombreMinimoUTime << "nombreMinimoUTime_{"<< t<<","<<b<<"}";
				MinimoUTime.add (IloRange(env, 0, restriccionMinimoUTime, IloInfinity, nombreMinimoUTime.str().c_str()));
				nombreMinimoUTime.str(""); // Limpiar variable para etiquetas de las restricciones
				
			}
		}
		
		
		//Adicionar restricciones 
		modelo.add(MinimoUTime); 
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Restriccion que asegura que si madruga no trasnocha   (R41)
		
		IloRangeArray MadrugaTrasnocha(env);
		stringstream nombreMadrugaTrasnocha;
		IloExpr	restriccionMadrugaTrasnocha(env);
		
		restriccionMadrugaTrasnocha = 1440 - utime + ltime - descanson;
		
		nombreMadrugaTrasnocha << "nombreMadrugaTrasnocha{}";
		
		MadrugaTrasnocha.add (IloRange(env, 0, restriccionMadrugaTrasnocha, IloInfinity, nombreMadrugaTrasnocha.str().c_str()));
		nombreMadrugaTrasnocha.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(MadrugaTrasnocha); 
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Restriccion que acota a no haber bloque intermedio   (R42)
		
		IloRangeArray ExtrictoMitad(env);
		stringstream nombreExtrictoMitad;
		IloExpr	restriccionExtrictoMitad(env);
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionExtrictoMitad += U[t][b];
			}
		}
		nombreExtrictoMitad << "nombreExtrictoMitad{}";
		
		restriccionExtrictoMitad=bsolo - (3-restriccionExtrictoMitad)/2;
		
		ExtrictoMitad.add (IloRange(env, -IloInfinity, restriccionExtrictoMitad, 0, nombreExtrictoMitad.str().c_str()));
		nombreExtrictoMitad.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(ExtrictoMitad); 
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Restriccion que garantiza un descanso entre bloques   (R43)
		
		IloRangeArray GarantiaDescanso(env);
		stringstream nombreGarantiaDescanso;
		IloExpr	restriccionGarantiaDescanso(env);
		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				restriccionGarantiaDescanso += U[t][b] * TablasInstancia[t].bloques[b].duracionb;
			}
		}
		
		restriccionGarantiaDescanso = utime-ltime - restriccionGarantiaDescanso - (1-bsolo)*descanso;
		
		nombreGarantiaDescanso << "nombreGarantiaDescanso{}";
		
		GarantiaDescanso.add (IloRange(env, 0, restriccionGarantiaDescanso, IloInfinity, nombreGarantiaDescanso.str().c_str()));
		nombreGarantiaDescanso.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(GarantiaDescanso);
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Restriccion genera un descanso obligatorio a un posible bloque medio   (R44)
		
		IloRangeArray DifFirstMiddle(env);
		stringstream nombreDifFirstMiddle;
		IloExpr	restriccionDifFirstMiddle(env);	
		IloExpr	expresionFirst(env);	
		IloExpr	expresionMitad(env);
		IloExpr	expresionRelajacion(env);		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				
				expresionFirst += first[t][b] * TablasInstancia[t].bloques[b].tfb;	
				expresionMitad += middle[t][b] * TablasInstancia[t].bloques[b].t0b;;
				expresionRelajacion += middle[t][b];
				
			}			
		}		
		restriccionDifFirstMiddle = expresionFirst - expresionMitad - descanso - (1-expresionRelajacion)*1440;		
		
		
		nombreDifFirstMiddle << "nombreDifFirstMiddle_{}";
		
		DifFirstMiddle.add (IloRange(env, -IloInfinity, restriccionDifFirstMiddle, 0, nombreDifFirstMiddle.str().c_str()));
		nombreDifFirstMiddle.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(DifFirstMiddle); 
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//Restricción   (R45)
		IloRangeArray DifLastMiddle(env);
		stringstream nombreDifLastMiddle;
		IloExpr	restriccionDifLastMiddle(env);	
		IloExpr	expresionLast(env);	
		IloExpr	expresionMitadB(env);		
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				
				expresionLast += last[t][b] * TablasInstancia[t].bloques[b].t0b;	
				expresionMitadB += middle[t][b] * TablasInstancia[t].bloques[b].tfb;;				
				
			}			
		}		
		restriccionDifLastMiddle = expresionLast - expresionMitadB - descanso;		
		
		
		nombreDifLastMiddle << "nombreDifLastMiddle_{}";
		
		DifLastMiddle.add (IloRange(env, 0, restriccionDifLastMiddle, IloInfinity, nombreDifLastMiddle.str().c_str()));
		nombreDifLastMiddle.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar restricciones 
		modelo.add(DifLastMiddle); 
		

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		
		for(auto t = 0u; t< CantidadTablas; ++t){
			for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){
				obj += Z[t][e] * DUAL_MP[t][e];
			}
		}	
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		//modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloAuxiliar.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 8000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//Establecer variantes de solución
		int optimization = 2;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		cplex.setParam(IloCplex::IntParam::RootAlg, optimization);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//////////Cambiar salida para este modelo  
		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){							
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo Auxiliar: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> ltime =  "<<cplex.getValue(ltime)<<endl;
			cout<<endl<<"-----> utime =  "<<cplex.getValue(utime)<<endl;
			
			cout<<endl<<"Se seleccionan los siguientes bloques: "<<endl;
			for(auto t =0u; t<CantidadTablas; ++t){
				for(auto b=0u; b<PosiblesBloques; ++b){
					if(cplex.getValue(U[t][b])>=0.95){
						cout<<"Tabla "<<TablasInstancia[t].nombreTabla<<" b"<<b<<endl;
					}
				}
			}
			for(auto t =0u; t<CantidadTablas; ++t){
				for(auto b=0u; b<PosiblesBloques; ++b){
					if(cplex.getValue(first[t][b])>=0.95){
						cout<<"first:  Tabla "<< TablasInstancia[t].nombreTabla<<", bloque "<<b;
						cout<<": "<<TablasInstancia[t].bloques[b].t0b<<" - "<<TablasInstancia[t].bloques[b].tfb<<endl;
					}
					if(cplex.getValue(middle[t][b])>=0.95){
						cout<<"middle: Tabla "<< TablasInstancia[t].nombreTabla<<", bloque "<<b;
						cout<<": "<<TablasInstancia[t].bloques[b].t0b<<" - "<<TablasInstancia[t].bloques[b].tfb<<endl;
					}
					if(cplex.getValue(last[t][b])>=0.95){
						cout<<"last:   Tabla "<< TablasInstancia[t].nombreTabla<<", bloque "<<b;
						cout<<": "<<TablasInstancia[t].bloques[b].t0b<<" - "<<TablasInstancia[t].bloques[b].tfb<<endl;
					}
				}					
			}
						
			
			//Guardar columna generada
			for(auto t = 0u; t< CantidadTablas; ++t){
				for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){
					
					columnaAuxiliar[t][e] = cplex.getValue(Z[t][e]);
					
				}
			}					
			
			//Almacenar para retornar la función objetivo
			valorFuncionObjetivo = cplex.getObjValue();				
			
			//Retorno de la función objetivo en formato float
			return valorFuncionObjetivo;	
			
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo AUXILIAR)";
			
		}			
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX	

		
		
		
		
		
	
	}
	
	
	//Modelo maestro tipo bin-packing Integra-UniAndes
	void modeloGCMaestro(){
		
		cout<<endl<<"Dimensiones de A en el maestro --->>>"<<A.size()<<" "<<A[0].size()<<" "<<A[0][0].size(); 
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO MAESTRO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		// los parametros de entrada son dinamicos para resolverf el problema maestro.
		
		char name_s[200];//Vector para nombrar el conjunto de variables de decisión'W'
		IloBoolVarArray W(env, A[0][0].size());//Dimensión b = posibles bloques (de 1 a 4 posibles)
		for(unsigned int i=0;i<A[0][0].size();++i){					
			sprintf(name_s,"W_%u",i);
			//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
			W[i] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
		}	
				
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCION MODELO MAESTRO 
		//******************************************************************	
		
		//Conjunto de restricciones que evaluan la asignación de la columna con la matriz de coeficientes.
		IloRangeArray EvalucionM(env);
		stringstream nombreEvaluacionM;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){//Una restricción por cada evento de cada tabla
				IloExpr restriccionEvluacionM(env);
				for(auto i = 0u; i < A[t][e].size(); ++i) { // la dimension de 
				//for(auto i = num; i < A[t][e].size(); ++i) { // la dimension de 
					restriccionEvluacionM += W[i]*A[t][e][i];//la nueva columna por cada uno de las posiciones en i 
				}
				nombreEvaluacionM << "Servicio Asignado_{"<< t<<","<<e<<"}";
				EvalucionM.add (IloRange(env, 1, restriccionEvluacionM, 1, nombreEvaluacionM.str().c_str()));
				nombreEvaluacionM.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		//Adicionar restricciones 
		modelo.add(EvalucionM); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);		
		
		//for (auto i = 0u; i<num; ++i){
		//		obj += c*W[i];
		//}	
		
		for (auto i = 0; i<A[0][0].size(); ++i){
				obj += W[i];
		}
		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("modeloMaestro.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 8000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 4);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//Establecer variantes de solución
		int optimization = 2;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		cplex.setParam(IloCplex::IntParam::RootAlg, optimization);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//////////Cambiar salida para este modelo  
		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){		
			
					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo Maestro: "<<cplex.getObjValue()<<endl;		
			
			
			//Archivo de salida para corridas en bloque
			ofstream archivoFormatoHorario("GC_Turnos.log",ios::app | ios::binary);	
			archivoFormatoHorario<<setprecision(10);//Cifras significativas para el reporte
			if (!archivoFormatoHorario.is_open()){
				cout << "Fallo en la apertura del archivo de salida de los turnos.";				
			}		
			
			
			//Mostrar solución			
			for(auto i = 0u; i < A[0][0].size(); ++i) {					
				
				if(cplex.getValue(W[i])>0.95){
				//if(cplex.getValue(W[i])>0.5){
				//if(cplex.getValue(W[i])>0.99999 && cplex.getValue(W[i])<1.1){
				//if(cplex.getValue(W[i])>0.95){
				
					//Cambiar turno
					archivoFormatoHorario<<endl<<"-----------------------------------------------------"<<endl;					
					archivoFormatoHorario<<"Turno "<<i;
					archivoFormatoHorario<<endl<<"-----------------------------------------------------"<<endl;
					
					int sumatoriaServicios = 0;
					
					for(auto t = 0u; t < CantidadTablas; ++t){ 
						for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){											
							if(A[t][e][i]){
							//if(A[t][e][i]>0.5){
							//if(A[t][e][i]>0.5){
								sumatoriaServicios+=TablasInstancia[t].eventos[e].duracion;
								
								//Ruta						
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].nombreTabla<<" ";
								//Autonumérico
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].evento<<" ";
								//Hora de inicio del servicio (minutos)
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].t0<<" ";
								//Hora de finalización del servicio (minutos)
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].tf<<" ";
								//Lugar donde inicia el servicio	
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].inicio<<" ";
								//Lugar donde finaliza el servicio	
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].fin<<" ";
								//Minutos de duración del servicio	
								archivoFormatoHorario<<TablasInstancia[t].eventos[e].duracion<<endl;
															
							}
						}
						
						//Línea de Diferenciación de Bloques
						archivoFormatoHorario<<endl;
						
					}	
														
					cout<<"Se seleccionó de servicio turno "<<i<<" con "<<sumatoriaServicios<<" minutos"<<endl;				
			
				}
			}
			
			//Cerrar el controlador del archivo de salida en formato horario
			archivoFormatoHorario.close();			
			
			
			////Archivo de salida para corridas en bloque
			//ofstream archivoFormatoHorario("rosteringFormatoHorario.log",ios::app | ios::binary);		
			//archivoFormatoHorario<<setprecision(10);//Cifras significativas para el reporte
			//if (!archivoFormatoHorario.is_open()){
			//	cout << "Fallo en la apertura del archivo de salida FORMATO HORARIO.";				
			//}		
			////Cerrar el controlador del archivo de salida en formato horario
			//archivoFormatoHorario.close();	
			
		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo maestro)";
			
		}
			
				
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}


	//Modelo maestro tipo bin-packing Integra-UniAndes variables relajadas
	float modeloGCMaestroRelajado(){
		
		//Salida diagnóstico
		//cout<<endl<<"Tercera Dimensión (turnos) Matriz A *****----> "<<A[0][0].size()<<endl;
		
		
		//Función Objetivo del Modelo
		float valorFuncionObjetivo = 0;
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO MAESTRO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		// los parametros de entrada son dinamicos para resolverf el problema maestro.
		
		char name_s[200];//Vector para nombrar el conjunto de variables de decisión'W'
		IloFloatVarArray W(env, A[0][0].size());//Dimensión b = posibles bloques (de 1 a 4 posibles)
		for(unsigned int i=0;i<A[0][0].size();++i){					
			sprintf(name_s,"W_%u",i);
			//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
			//W[i] = IloFloatVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
			W[i] = IloFloatVar(env,0,1,name_s);//Asociación explícita de la variable de decisión del problema			
						
		}	
				
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCION MODELO MAESTRO 
		//******************************************************************	
		
		//Conjunto de restricciones que evaluan la asignación de la columna con la matriz de coeficientes.
		IloRangeArray EvaluacionM(env);
		stringstream nombreEvaluacionM;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){//Una restricción por cada evento de cada tabla
				IloExpr restriccionEvaluacionM(env);
				for(auto i = 0u; i < A[t][e].size(); ++i) { // la dimension de 
					restriccionEvaluacionM += W[i]*A[t][e][i];//la nueva columna por cada uno de las posiciones en i 
				}
				nombreEvaluacionM << "Servicio Asignado_{"<< t<<","<<e<<"}";
				EvaluacionM.add (IloRange(env, 1, restriccionEvaluacionM, 1, nombreEvaluacionM.str().c_str()));
				nombreEvaluacionM.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		//Adicionar restricciones 
		modelo.add(EvaluacionM); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);		
		
		//for (auto i = 0u; i<num; ++i){
		//		obj += c*W[i];
		//}	
		
		for (auto i = 0; i<A[0][0].size(); ++i){
				obj += W[i];
		}
		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("modeloMaestroRelajado.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 8000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//Establecer variantes de solución
		int optimization = 4;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		cplex.setParam(IloCplex::IntParam::RootAlg, optimization);
		
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//////////Cambiar salida para este modelo  
		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){		
			
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo Maestro Relajado: "<<cplex.getObjValue()<<endl;		
			
			//Cargar en variable para retornar el valor
			valorFuncionObjetivo = cplex.getObjValue();
			
			//Mostrar solución			
			for(auto i = 0u; i < A[0][0].size(); ++i) {					
				
				if(cplex.getValue(W[i])>0.95){				
					
										
					//cout<<endl<<"W["<<i<<"]:"<<endl;
					
					int sumatoriaServicios = 0;
					
					for(auto t = 0u; t < CantidadTablas; ++t){ 
						for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){//Una restricción por cada evento de cada tabla												
							sumatoriaServicios+=A[t][e][i];									
							
						}
					}				
					if(sumatoriaServicios > 1){								
						cout<<"Cantidad de eventos en turno "<<i<<" : "<<sumatoriaServicios<<endl;				
						
					}
				}
			}	
			
			
			
			//Obtener las duales del maestro
			int ee=0;
			for(auto t = 0u; t < CantidadTablas; ++t){ 
				for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){
					DUAL_MP[t][e]=cplex.getDual(EvaluacionM[ee]);
					++ee;
				}
			}
		
		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo maestro relajado)";
			
		}	
		
		
		
		//Retornar el valor para controlar mejoras del algoritmo
		return valorFuncionObjetivo;
		
			
				
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}

	
	
	
	void HeuristicaParticion(){ //Heuristica para generar combinaciones de bloques a partir de timetables
		
		//Vector de cortes
		vector < vector < vector < int > > > CORTES; //Vector donde se almacenan las particiones por cada tabla
		//CORTES.resize(CantidadTablas);
		
		//Definición de los vectores a usar
		vector < int > X; //Vector de cortes de los bloques
		X.resize(PosiblesBloques);
		
		vector < int > Cantidades;//Vector de cantidades en los bloques
		Cantidades.resize(PosiblesBloques);
		
		vector < float > SumDuracionBloque;//Vector de duraciones de los bloques
		SumDuracionBloque.resize(PosiblesBloques);
		
		//Variable de aprovación
		bool aprobado = true; // Si llega a inclumplir alguna restricción, tendrá pasará a ser false
		
		int hb, tbalance, extrab;
		
		//Establecer parámetros
		hb = 300;//Cantidad de minutos máximo por bloque
		tbalance = 40;//Cantidad de minutos mínima por bloque
		extrab = 12;//Minutos extra permitidos al máximo de un bloque (5 horas)
		
		//Conjunto de puntos de intercambio (parámetro establecido por código)
					vector<string> puntosIntercambio;
					puntosIntercambio.push_back("Viajero");
					puntosIntercambio.push_back("Intercambiador_D/das");
					puntosIntercambio.push_back("Intercambiador_Cuba");
					puntosIntercambio.push_back("Integra");	
					puntosIntercambio.push_back("Estacion_Milan");					 
					puntosIntercambio.push_back("Estacion_Cam");					 
					puntosIntercambio.push_back("Integra_S.A.");					 
		
		//for(auto t=0 ; t < CantidadTablas; ++t){
		for(auto t=0 ; t < 1; ++t){
			
			int contador = 0; //Inicializar contador de cortes por tabla
			vector <vector <int>> VectorX;
			
				
			for(auto i=1; i < TablasInstancia[t].EventosTabla; ++i){
				for(auto j=i; j < TablasInstancia[t].EventosTabla; ++j){
					for(auto k=j; k < TablasInstancia[t].EventosTabla; ++k){
						
						aprobado = 1; // Iniciar la aprovación
							
						//Valores de cortes
						X[0]=i;
						X[1]=j;
						X[2]=k;
						X[3]=TablasInstancia[t].EventosTabla-1;
						//Valores de cantidades
						Cantidades[0]=i+1;
						Cantidades[1]=j-i;
						Cantidades[2]=k-j;
						Cantidades[3]=TablasInstancia[t].EventosTabla-1-k;
						//Valores de duraciones
						SumDuracionBloque[0]=0;
						for(auto e=0; e <= X[0]; ++e){
							SumDuracionBloque[0]=SumDuracionBloque[0]+TablasInstancia[t].eventos[e].duracion;
						}
						for(auto b=1; b < PosiblesBloques; ++b){
							SumDuracionBloque[b]=0;
							if(Cantidades[b]>0){
								for(auto e=X[b-1]+1; e <=X[b]; ++e){
									SumDuracionBloque[b]=SumDuracionBloque[b]+TablasInstancia[t].eventos[e].duracion;
								}
							}
						}
						
						//Impresión del corte a evaluar
						for(auto b=0; b<PosiblesBloques; ++b){
							cout<<"X"<<b<<":"<<X[b]<<endl;
						}
						for(auto b=0; b<PosiblesBloques; ++b){
							cout<<"Cant_"<<b<<":"<<Cantidades[b]<<endl;
						}
						for(auto b=0; b<PosiblesBloques; ++b){
							cout<<"Duracion_"<<b<<":"<<SumDuracionBloque[b]<<endl;
						}
						
						///////////////////////////
						//Pruebas
						
						// 1. Revisión de que no repetición ni bloques intermedios vacios
						if(Cantidades[1]=0 && Cantidades[2]>0){
							aprobado=0;
						}
						if(Cantidades[1]=0 && Cantidades[3]>0){
							aprobado=0;
						}
						if(Cantidades[2]=0 && Cantidades[3]>0){
							aprobado=0;
						}
						
						// 2. Revisión de duración maxima y mínima de bloques
						for(auto b=0u; b<PosiblesBloques; ++b){
							if(SumDuracionBloque[b]>hb+extrab){
								aprobado=0;
							}
							if(SumDuracionBloque[b]<tbalance && Cantidades[b]>0){
								aprobado=0;
							}
						}
						
						// 3. Revisión de realizar cortes en intercambiadores
						for(auto b=0; b<PosiblesBloques; ++b){
							if(!esta(puntosIntercambio, TablasInstancia[t].eventos[X[b]].fin)){
								aprobado=0;
							}
						}
						
						//**********************************************
						//Inclusión del corte si resultó aprovado
						//**********************************************
						cout<<endl<<aprobado<<endl<<endl;
						if (aprobado=1 && contador<6){
							/*
							for(auto b=0; b < PosiblesBloques; ++b){
								CORTES[t][contador][b].push_back(X[b]);
							}*/
							VectorX.push_back(X);
							contador+=1;
						}
						// Si no fue aprobado, sigue probando otra partición
						
					}
				}
			}// Fin de probar todas las conbinaciones
			CORTES.push_back(VectorX);
								
		}// Fin de cortar todas las tabla
		
		//******************************************************************
		//Agregación de los nuevos bloques al vector de bloques de cada tabla de la instancia
		//******************************************************************
		
		////Exportar cómo quedaron las particiones
		//
		//ofstream archivoMatriz("matrizA.csv",ios::app | ios::binary);		
		//archivoMatriz<<setprecision(10);//Cifras significativas para el reporte
		//if (!archivoMatriz.is_open()){
		//	cout << "Fallo en la apertura del archivo de salida MATRIZ A.";	
		//}else{
		//
		//
		//for(auto t=0; t < CantidadTablas; ++t){
		//	for(auto b=0u; b < CORTES[t].size(); ++b){
		//		
		//		archivoMatriz<<"Tabla ["<<t<<"]bloque["<<b<<"] = ["<<CORTES[t][b][0]<<","<<CORTES[t][b][1]<<","<<CORTES[t][b][2]<<","<<CORTES[t][b][3]<<"]"<<endl;
		//		
		//	}
		//	archivoMatriz<<endl;
		//}
		//archivoMatriz.close();	
		//
		//}
		
		//Mostrar cómo quedaron las particiones
		
		cout<<endl<<"Resumen de cortes "<<endl;
		for(auto t=0; t < CantidadTablas; ++t){
			for(auto c=0u; c < CORTES[t].size(); ++c){
				
				cout<<"Tabla "<< TablasInstancia[t].nombreTabla <<": corte["<<c<<"] = [";
				cout<<CORTES[t][c][0]<<","<<CORTES[t][c][1]<<","<<CORTES[t][c][2]<<","<<CORTES[t][c][3]<<"]";
				cout<< " con cantidades = ";
				cout<<CORTES[t][c][0]+1<<"_"<<CORTES[t][c][1]-CORTES[t][c][0]<<"_"<<CORTES[t][c][2]-CORTES[t][c][1]<<"_"<<TablasInstancia[t].EventosTabla-1-CORTES[t][c][2]<<endl;
				
			}
			cout<<endl;
		}
		//Guardar los bloques generados por el esclavo
		//
		//	for(int t=0;t<CantidadTablas;++t){								
		//		for(int b=0;b<PosiblesBloques;++b){
		//			float minimoAux;
		//			minimoAux=1440;	
		//			string inicioAux;
		//			float maximoAux;	
		//			maximoAux=0;	
		//			string finAux;	
		//			maximoAux=0;	
		//			float duracionAux = 0;
		//			
		//			bloque bloqueAux;
		//											
		//			for(int e=0;e<TablasInstancia[t].EventosTabla;++e){										
		//				if(cplex.getValue(Y[t][e][b])>0.95 && TablasInstancia[t].eventos[e].t0<minimoAux){										
		//					minimoAux=TablasInstancia[t].eventos[e].t0;
		//					inicioAux=TablasInstancia[t].eventos[e].inicio;
		//				}															
		//				if(cplex.getValue(Y[t][e][b])>0.95 && TablasInstancia[t].eventos[e].t0>maximoAux){										
		//					maximoAux=TablasInstancia[t].eventos[e].tf;
		//					finAux=TablasInstancia[t].eventos[e].fin;
		//				}
		//				if(cplex.getValue(Y[t][e][b])>0.95){
		//					duracionAux+=TablasInstancia[t].eventos[e].duracion;
		//					Y_sol[t][e][b]=1;
		//				}
		//						
		//			
		//			}
		//			
		//			bloqueAux.t0b = minimoAux;
		//			bloqueAux.iniciob = inicioAux;
		//			bloqueAux.tfb = maximoAux;
		//			bloqueAux.finb = finAux;
		//			bloqueAux.duracionb = duracionAux;
		//			TablasInstancia[t].bloques.push_back(bloqueAux);
		//			
		//			
		//							
		//		}				
		//	}
		
		
		
		
		
		
	} //Fin de la heurística

	//Generar grafo en formato .dot del constructivo
	void generarGrafoDotConstructivo(){				

		//Salida de las cadenas de costo mínimo CSP en formato DOT
		//string nombreGrafo = "grafoConstructivo.dot";
		string nombreGrafo = "./soluciones/grafoConstructivo_"+this->nombreArchivo.substr(21,this->nombreArchivo.length())+".dot";
		//cout<<endl<<nombreGrafo<<endl;getchar();//Salida de diagnóstico			
		std::ofstream dot_file(nombreGrafo.c_str());
		
		//Cabecera del archivo
		dot_file << "digraph D {\n"
		<<"rankdir=LR\n"
		<<"size=\"4,3\"\n"
		<<"ratio=\"fill\"\n"		
		<<"node[color=\"black\",shape=\"square\",fillcolor=\"darkseagreen3\",style=\"filled\"]"
		<<endl<<"0"
		<<endl<<"N1"
		<<endl<<"node[color=\"black\",shape=\"circle\",style=\"\"]";

		/*
		//Dibujar todas las transiciones factibles en gris		
		dot_file << "edge[style=\"dotted\"]\n";//Estilo de los arcos factibles 
		for(size_t t=0;t<this->listadoTransicionesPermitidasBeasley.size();++t){

			//Precarga de valores valores
			int i = this->listadoTransicionesPermitidasBeasley[t].i;
			int j = this->listadoTransicionesPermitidasBeasley[t].j;
			int costoTransicion = this->listadoTransicionesPermitidasBeasley[t].costoTransicion;
			int f_i = this->listadoServiciosBeasley[i].tf;
			int s_j = this->listadoServiciosBeasley[j].t0;
			int tiempoTransicion = s_j - f_i;

			dot_file<<i<<"->"<<j
			<<"[label=\"c="<<costoTransicion<<",t="
			<<tiempoTransicion
			<<"\",color=\"grey\"]"<<endl;

			//Arista de ejemplo
			//0 -> 3[label="180.568", color="green"]

		}
		*/
		

		//Líneas de separación en el archivo del grafo para inspección visual
		dot_file<<endl<<endl<<endl;

		//Dibujar los arcos generados por la solución
		dot_file << "edge[style=\"solid\"]\n";//Estilo de los arcos solución
		for(size_t t=0;t<this->arcosBeasleyModificado.size();++t){			

			//Precarga de valores valores
			int i = arcosBeasleyModificado[t][0];
			int j = arcosBeasleyModificado[t][1];
			int costoTransicion = this->matrizCostosBeasley[i][j];
			int f_i = this->listadoServiciosBeasley[i].tf;
			int s_j = this->listadoServiciosBeasley[j].t0;
			int tiempoTransicion = s_j - f_i;
			

			dot_file << "edge[style=\"solid\"]\n";//Restaurar estilo

			//dot_file<<i<<"->"<<j
			//<<"[label=\"c="<<costoTransicion<<",t="
			//<<tiempoTransicion<<"\",";
			
			//Definir el color de la arista para diferenciar 
			//aquellas que se relacionan con el depósito
			if(i==0 || j==num+1){
				
				//Arista que llega al depósito
				if(j==num+1){
					dot_file<<i<<"->N1";
				}else{
					//Arista que sale al depósito
					dot_file<<i<<"->"<<j;
				}
				
				//Etiqueta de la arista
				dot_file<<"[label=\"c="<<costoTransicion<<",t="
				<<tiempoTransicion<<"\",";

				//Alternativas de color
				//dot_file<<"color=\"darkorange2\"]"<<endl;//Salidas del depósito
				dot_file<<"color=\"darkseagreen3\"]"<<endl;//Salidas del depósito
			}else{

				//Encabezado de las cadenas
				dot_file<<i<<"->"<<j
				<<"[label=\"c="<<costoTransicion<<",t="
				<<tiempoTransicion<<"\",";

				//Establecer color si es factible o no
				if(costoTransicion != 100000000){
					dot_file<<"color=\"dodgerblue2\"]"<<endl;//Transiciones de cadena o turno				
				}else{
					dot_file<<"color=\"red\",style=\"dotted\"]"<<endl;//Transiciones ilegales					
				}				
			}	

			//Arista de ejemplo
			//0 -> 3[label="180.568", color="green"]		
			
		}		
		dot_file << "}";//Cierre del archivo

	}//Fin del método que genegenera el grafo en formato .dot


	//Generar grafo en formato .dot (Modelo Beasey CSP)
	void generarGrafoDot(){		

		//Salida de las cadenas de costo mínimo CSP en formato DOT
		//string nombreGrafo = "grafoConstructivo.dot";
		string nombreGrafo = "./soluciones/grafoModelo_"+this->nombreArchivo.substr(21,this->nombreArchivo.length())+".dot";
		//cout<<endl<<nombreGrafo<<endl;getchar();			
		std::ofstream dot_file(nombreGrafo.c_str());
		
		//Cabecera del archivo
		dot_file << "digraph D {\n"
		<<"rankdir=LR\n"
		<<"size=\"4,3\"\n"
		<<"ratio=\"fill\"\n"		
		<<"node[color=\"black\",shape=\"square\",fillcolor=\"darkseagreen3\",style=\"filled\"]"
		<<endl<<"0"
		<<endl<<"N1"
		<<endl<<"node[color=\"black\",shape=\"circle\",style=\"\"]";

		/*
		//Dibujar todas las transiciones factibles en gris		
		dot_file << "edge[style=\"dotted\"]\n";//Estilo de los arcos factibles 
		for(size_t t=0;t<this->listadoTransicionesPermitidasBeasley.size();++t){

			//Precarga de valores valores
			int i = this->listadoTransicionesPermitidasBeasley[t].i;
			int j = this->listadoTransicionesPermitidasBeasley[t].j;
			int costoTransicion = this->listadoTransicionesPermitidasBeasley[t].costoTransicion;
			int f_i = this->listadoServiciosBeasley[i].tf;
			int s_j = this->listadoServiciosBeasley[j].t0;
			int tiempoTransicion = s_j - f_i;

			dot_file<<i<<"->"<<j
			<<"[label=\"c="<<costoTransicion<<",t="
			<<tiempoTransicion
			<<"\",color=\"grey\"]"<<endl;

			//Arista de ejemplo
			//0 -> 3[label="180.568", color="green"]

		}
		*/

		//Líneas de separación en el archivo del grafo para inspección visual
		dot_file<<endl<<endl<<endl;

		//Dibujar los arcos generados por la solución
		dot_file << "edge[style=\"solid\"]\n";//Estilo de los arcos solución
		for(size_t t=0;t<this->arcosBeasleyModificado.size();++t){			

			//Precarga de valores valores
			int i = arcosBeasleyModificado[t][0];
			int j = arcosBeasleyModificado[t][1];
			int costoTransicion = this->matrizCostosBeasley[i][j];
			int f_i = this->listadoServiciosBeasley[i].tf;
			int s_j = this->listadoServiciosBeasley[j].t0;
			int tiempoTransicion = s_j - f_i;
			

			dot_file << "edge[style=\"solid\"]\n";//Restaurar estilo

			//dot_file<<i<<"->"<<j
			//<<"[label=\"c="<<costoTransicion<<",t="
			//<<tiempoTransicion<<"\",";
			
			//Definir el color de la arista para diferenciar 
			//aquellas que se relacionan con el depósito
			if(i==0 || j==num+1){
				
				//Arista que llega al depósito
				if(j==num+1){
					dot_file<<i<<"->N1";
				}else{
					//Arista que sale al depósito
					dot_file<<i<<"->"<<j;
				}
				
				//Etiqueta de la arista
				dot_file<<"[label=\"c="<<costoTransicion<<",t="
				<<tiempoTransicion<<"\",";

				//Alternativas de color
				//dot_file<<"color=\"darkorange2\"]"<<endl;//Salidas del depósito
				dot_file<<"color=\"darkseagreen3\"]"<<endl;//Salidas del depósito
			}else{

				//Encabezado de las cadenas
				dot_file<<i<<"->"<<j
				<<"[label=\"c="<<costoTransicion<<",t="
				<<tiempoTransicion<<"\",";

				//Establecer color si es factible o no
				if(costoTransicion != 100000000){
					dot_file<<"color=\"dodgerblue2\"]"<<endl;//Transiciones de cadena o turno				
				}else{
					dot_file<<"color=\"red\",style=\"dotted\"]"<<endl;//Transiciones ilegales					
				}				
			}	

			//Arista de ejemplo
			//0 -> 3[label="180.568", color="green"]		
			
		}		
		dot_file << "}";//Cierre del archivo

	}//Fin del método que genegenera el grafo en formato .dot

	///////////// ////////////

	//Método para generar un punto inicial a partir de la instancia cargada
	//Instancias librería Beasley & Cao 96
	//Barrido o arranque en diferentes tareas
	void puntoInicialBarridoModeloBeasleyModificado(){

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;

		//Tomar tiempo inicial del constructivo de barrido
		gettimeofday(&t_ini, NULL);

		//Contenedores para el control del barrido
		solucionConstructivoPropuesto mejorSolucionNumeroTurnos;
		vector< solucionConstructivoPropuesto > solucionesBarrido;

		//Realizar el barrido de todos los arranques (objetivo: construcción de pool y fortalecer P0)
		for(size_t i_barrido=0;i_barrido<num+1;++i_barrido){
		
			//Contenedor con las cadenas o itinerarios de la heurística
			vector< vector < int > > cadenasPuntoP0_Modelo;

			//Inicializar costo del atributo de la instancia
			this->funcionObjetivoConstructivo = 0;//Inicializar función objetivo del constructivo en la instancia
			this->y_i_Constructivo.clear();
			this->arcosBeasleyModificado.clear();//Contenedor de los arcos, traducidos a solución
			this->duracionTurnosConstructivo.clear();//Contenedor con la duración de cada turno (diagnóstico de minutos totales laborados)

			//Conjuntos de control
			unordered_set<int> serviciosCubiertos;
			unordered_set<int> serviciosPendientes;

			//Inicializar el conjunto de servicios pendientes en cada inicialización
			for(size_t i = 0;i<num;++i){
				serviciosPendientes.insert(i+1);			
			}						

			//Condiciones iniciales para el proceso de construcción de turnos
			//Se asume que el primer servicio, al ser el  que se realiza más temprano,
			//presenta un número considerable de salidas
			//int servicioActual = 1;//Inicializar en el primer servicio
			int servicioActual = i_barrido + 1;//Inicializar en el primer servicio
			//Actualizar los conjuntos de control
			serviciosCubiertos.insert(servicioActual);
			serviciosPendientes.erase(servicioActual);
			vector<int> turnoActual;
			turnoActual.push_back(servicioActual);		
			int duracionTurnoActual = this->listadoServiciosBeasley[servicioActual].duracion;		
			vector<int> tiemposTurnoActual;
			tiemposTurnoActual.push_back(duracionTurnoActual);
			//Contenedor de todas las posibles salidas todas las posibles salidas
			vector< transicionBeasleyModificado > salidasServicio_i;		

			//Recorrer uno a uno los servicios para cubrirlos
			while(!serviciosPendientes.empty()){	

				//Obtener todas las transiciones factibles a partir del servicio actual
				salidasServicio_i.clear();
				for(size_t s=0;s<this->listadoTransicionesPermitidasBeasley.size();++s){

					//Precarga de valores
					int i = this->listadoTransicionesPermitidasBeasley[s].i;
					int j = this->listadoTransicionesPermitidasBeasley[s].j;
					int costo = this->listadoTransicionesPermitidasBeasley[s].costoTransicion;

					//Parar el proceso o continuar hasta alcanzar el grupo de transiciones
					//if(s < servicioActual){
					//	continue;			
					//}else if(s > servicioActual){
					//	break;
					//}else if(s == servicioActual){
					//	//Agregar las posibles salidas
					//	salidasServicio_i.push_back(transicionBeasleyModificado(i,j,costo));
					//}
					//Agregar las posibles salidas
					if(i == servicioActual){
						salidasServicio_i.push_back(transicionBeasleyModificado(i,j,costo));								
					}else if(i>servicioActual){
						break;				
					}				
					

				}

				//Ordenar las salidas del servicio actual por costo (definido en el TAD)
				sort(salidasServicio_i.begin(),salidasServicio_i.end());

				//Bandera de adición
				bool servicioAdicionado = false;//Suponer que no se realiza adición

				//Revisar duración de turno y si está pendiente la posible salida 's'
				for(size_t s=0;s<salidasServicio_i.size();++s){

					//Precarga de valores valores
					int i = servicioActual;
					int j = salidasServicio_i[s].j;			
					int f_i = this->listadoServiciosBeasley[i].tf;
					int s_j = this->listadoServiciosBeasley[j].t0;
					int tiempoTransicion = s_j - f_i;
					int tiempoServicio_j = this->listadoServiciosBeasley[j].duracion;

					//Buscar en pendientes para completar los criterios de adición
					unordered_set<int>::const_iterator itServicioPendiente = serviciosPendientes.find(j);

					//Si es factible y está pendiente, agregar la transición y actualizar 
					//(Está ordenado por menor costo)
					if(	duracionTurnoActual + tiempoTransicion + tiempoServicio_j <= ht &&
						itServicioPendiente != serviciosPendientes.end()){
						//Actualizar duración del turno
						duracionTurnoActual += tiempoTransicion + tiempoServicio_j;

						//Salida de diagnóstico
						//cout<<endl<<"Costo que se va a agregar:"<<salidasServicio_i[s].costo<<endl;
						//getchar();

						//Actualizar la función objetivo del constructivo
						this->funcionObjetivoConstructivo += salidasServicio_i[s].costo;
						//Actualizar el servicio actual
						servicioActual = j;
						//Actualizar grupos de control
						serviciosCubiertos.insert(servicioActual);
						serviciosPendientes.erase(servicioActual);
						//Actualizar el turno en construcción
						turnoActual.push_back(servicioActual);
						//Actualizar los tiempos parciales hasta cada servicio dentro del turno
						tiemposTurnoActual.push_back(duracionTurnoActual);
						//Activar bandera para registrar que se adicionó un servicio
						servicioAdicionado = true;
						//Terminar las iteraciones sobre el conjunto
						break;
					}		

				}//Fin del recorrido de las salidas

				//Si la bandera no fue activada, cerrar itinerario y abrir uno nuevo
				if(!servicioAdicionado){			

					//Almacenar el itinerario en el contenedor de P0
					cadenasPuntoP0_Modelo.push_back(turnoActual);
					//Almacenar el tiempo parcial en el contenedor de los y_i
					this->y_i_Constructivo.push_back(tiemposTurnoActual);
					//Almacenar la duración del turno creado para validaciones posteriores
					this->duracionTurnosConstructivo.push_back(duracionTurnoActual);
					//Limpiar el contenedor de itinerarios
					turnoActual.clear();
					//Limpiar el contenedor de tiempos parciales
					tiemposTurnoActual.clear();
					//Inicializar el itinerario con el primer servicio que se encuentre en los pendientes
					for(size_t i=0;i<num+1;++i){

						//Buscar si el servicio i está en pendientes para iniciar nuevo itinerario
						unordered_set<int>::const_iterator itServicioPendiente = serviciosPendientes.find(i);
						
						//Si fue encontrado, entonces inicializar y romper el recorrido
						if(itServicioPendiente != serviciosPendientes.end()){
							
							//Inicializar el servicio actual para las salidas en la siguiente iteración
							servicioActual = i;

							//Inicializar el turno
							turnoActual.push_back(servicioActual);												

							//Actualizar los grupos de control
							serviciosCubiertos.insert(servicioActual);
							serviciosPendientes.erase(servicioActual);

							//Inicializar el tiempo del turno
							duracionTurnoActual = this->listadoServiciosBeasley[servicioActual].duracion;

							//Almacenar el tiempo parcial
							tiemposTurnoActual.push_back(duracionTurnoActual);

							//Romper la búsqueda
							break;	
						}
						
					}//Terminar recorrido de inicialización

				}//Fin del if que inicializa itinerario

			}//Fin del recorrido de cobertura de todos los servicios

			
			//Cálculo de valores de y_i
			
			//Traducir las aristas para visualizar el grafo y para el punto inicial
			this->arcosBeasleyModificado.clear();
			for(size_t i = 0;i<cadenasPuntoP0_Modelo.size();++i){			

				//Contenedor temporal de los arcos involucrados
				vector<int> turnoConstructivo;

				//Salida del depósito
				turnoConstructivo.push_back(0);
				turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][0]);
				arcosBeasleyModificado.push_back(turnoConstructivo);
				turnoConstructivo.clear();

				//Regreso al depósito			
				turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i].back());
				turnoConstructivo.push_back(num+1);
				arcosBeasleyModificado.push_back(turnoConstructivo);
				turnoConstructivo.clear();

				//Si la longitud es mayor a uno, entonces hay arcos intermedios y se procede a almacenarlos
				if(cadenasPuntoP0_Modelo[i].size()>1){
					for(size_t j = 0;j<cadenasPuntoP0_Modelo[i].size()-1;++j){
						turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][j]);
						turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][j+1]);					
						arcosBeasleyModificado.push_back(turnoConstructivo);
						turnoConstructivo.clear();				
					}
				}
										
			}
			
			////Visualizar traducción de los turnos a aristas para diagnóstico
			//cout<<endl<<"Aristas constructivo: "<<endl;
			//for(size_t i = 0;i<arcosBeasleyModificado.size();++i){
			//	for(size_t j = 0;j<arcosBeasleyModificado[i].size();++j){
			//		cout<<arcosBeasleyModificado[i][j]<<" - ";
			//	}
			//	cout<<endl;			
			//}

			//Actualización de contenedores (burbuja)
			//Validar si es la inicialización, o si se trata de los demás casos
			if(mejorSolucionNumeroTurnos.numeroTurnos == 0){

				//Actualizar todos los atributos
				mejorSolucionNumeroTurnos.arcos = this->arcosBeasleyModificado;
				mejorSolucionNumeroTurnos.costo = this->funcionObjetivoConstructivo;				
				mejorSolucionNumeroTurnos.numeroTurnos = cadenasPuntoP0_Modelo.size();
				mejorSolucionNumeroTurnos.turnos = cadenasPuntoP0_Modelo;
				mejorSolucionNumeroTurnos.y_i = this->y_i_Constructivo;

			}else{

				//Revisar si se trata de una mejora primero en turnos y luego en costo para actualizar la incumbente
				if(mejorSolucionNumeroTurnos.numeroTurnos > cadenasPuntoP0_Modelo.size()){
					
					//Actualizar todos los atributos
					mejorSolucionNumeroTurnos.arcos = this->arcosBeasleyModificado;
					mejorSolucionNumeroTurnos.costo = this->funcionObjetivoConstructivo;				
					mejorSolucionNumeroTurnos.numeroTurnos = cadenasPuntoP0_Modelo.size();
					mejorSolucionNumeroTurnos.turnos = cadenasPuntoP0_Modelo;
					mejorSolucionNumeroTurnos.y_i = this->y_i_Constructivo;					

				//Si no hay mejora, revisar 
				}else{

					if(mejorSolucionNumeroTurnos.numeroTurnos == cadenasPuntoP0_Modelo.size()){			

						//Revisión de costo
						if(mejorSolucionNumeroTurnos.costo > this->funcionObjetivoConstructivo){
							
							//Actualizar todos los atributos
							mejorSolucionNumeroTurnos.arcos = this->arcosBeasleyModificado;
							mejorSolucionNumeroTurnos.costo = this->funcionObjetivoConstructivo;				
							mejorSolucionNumeroTurnos.numeroTurnos = cadenasPuntoP0_Modelo.size();
							mejorSolucionNumeroTurnos.turnos = cadenasPuntoP0_Modelo;
							mejorSolucionNumeroTurnos.y_i = this->y_i_Constructivo;	

						}
					}									

				}//Fin del caso que revisa cuando el número de turnos es igual (no hay mejora)
				
								
			}//Fin del if que valida inicialización o actualización

			//Adicionar solución al pool para análisis o posible set partitioning
			//turnos(t), numeroTurnos(nt), arcos(a), costo(c), y_i(y)
			solucionesBarrido.push_back( solucionConstructivoPropuesto(cadenasPuntoP0_Modelo, cadenasPuntoP0_Modelo.size(), this->arcosBeasleyModificado, this->funcionObjetivoConstructivo, this->y_i_Constructivo ) );		
			
		}//Fin del for que realiza el barrido para las diferentes inicializaciones


		//Tomar tiempo final del constructivo
		gettimeofday(&t_fin, NULL);	

		//Almacenar diferencia
		this->tiempoComputoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Almacenar el número de turnos del constructivo de barrido en el atributo de la instancia
		//this->numeroConductoresConstructivo = cadenasPuntoP0_Modelo.size();
		this->numeroConductoresConstructivo = mejorSolucionNumeroTurnos.numeroTurnos;

		//Copiar la especificación de turnos en el atributo correspondiente de la instancia
		//this->turnosConstructivo = cadenasPuntoP0_Modelo;
		this->turnosConstructivo = mejorSolucionNumeroTurnos.turnos;

		//Carga para envío de los arcos
		this->arcosBeasleyModificado = mejorSolucionNumeroTurnos.arcos;

		//Carga laboral acumulada en cada punto
		this->y_i_Constructivo = mejorSolucionNumeroTurnos.y_i;

		//Costo de la incumbente del constructivo
		this->funcionObjetivoConstructivo = mejorSolucionNumeroTurnos.costo;

		//Mostrar itinerarios construídos en pantalla
		cout<<endl<<"Especificación de turnos ("<<mejorSolucionNumeroTurnos.numeroTurnos<<"): "<<endl;		
		for(size_t i = 0;i<mejorSolucionNumeroTurnos.numeroTurnos;++i){
			for(size_t j = 0;j<mejorSolucionNumeroTurnos.turnos[i].size();++j){
				cout<<mejorSolucionNumeroTurnos.turnos[i][j]<<" ("<< this->y_i_Constructivo[i][j] <<")- ";
			}
			//cout<<"> "<<this->duracionTurnosConstructivo[i]<<" min"<<endl;			
			cout<<endl;			
		}
		//getchar();//Pausar ejecución

		cout<<endl<<endl<<"-----------------------------------------";		
		cout<<endl<<this->nombreArchivo;
		cout<<endl<<"-----------------------------------------";
		//cout<<endl<<"Cantidad de Servicios Cubiertos -> "<<serviciosCubiertos.size();
		//cout<<endl<<"Cantidad de Servicios Pendientes -> "<<serviciosPendientes.size();
		cout<<endl<<"Turnos construídos Barrido -> "<<this->numeroConductoresConstructivo;
		cout<<endl<<"FO Constructivo Barrido -> "<<this->funcionObjetivoConstructivo;
		cout<<endl<<"Tiempo Cómputo Barrido -> "<<this->tiempoComputoConstructivo;
		cout<<endl;
		//getchar();//Parada de diagnóstico en la salida		

	}//Fin de la generación por barrido del punto inicial del modelo (P0)


	//Método para generar un punto inicial a partir de la instancia cargada
	//Instancias librería Beasley & Cao 96
	void puntoInicialModeloBeasleyModificado(){

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
		
		//Contenedor con las cadenas o itinerarios de la heurística
		vector< vector < int > > cadenasPuntoP0_Modelo;

		//Conjuntos de control
		unordered_set<int> serviciosCubiertos;
		unordered_set<int> serviciosPendientes;

		//Inicializar el conjunto de servicios pendientes
		for(size_t i = 0;i<num;++i){
			serviciosPendientes.insert(i+1);			
		}

		//Tomar tiempo inicial del constructivo
		gettimeofday(&t_ini, NULL);			

		//Condiciones iniciales para el proceso de construcción de turnos
		//Se asume que el primer servicio, al ser el  que se realiza más temprano,
		//presenta un número considerable de salidas
		int servicioActual = 1;//Inicializar en el primer servicio
		//Actualizar los conjuntos de control
		serviciosCubiertos.insert(servicioActual);
		serviciosPendientes.erase(servicioActual);
		vector<int> turnoActual;
		turnoActual.push_back(servicioActual);		
		int duracionTurnoActual = this->listadoServiciosBeasley[servicioActual].duracion;		
		vector<int> tiemposTurnoActual;
		tiemposTurnoActual.push_back(duracionTurnoActual);
		//Contenedor de todas las posibles salidas todas las posibles salidas
		vector< transicionBeasleyModificado > salidasServicio_i;		

		//Recorrer uno a uno los servicios para cubrirlos
		while(!serviciosPendientes.empty()){	

			//Obtener todas las transiciones factibles a partir del servicio actual
			salidasServicio_i.clear();
			for(size_t s=0;s<this->listadoTransicionesPermitidasBeasley.size();++s){

				//Precarga de valores
				int i = this->listadoTransicionesPermitidasBeasley[s].i;
				int j = this->listadoTransicionesPermitidasBeasley[s].j;
				int costo = this->listadoTransicionesPermitidasBeasley[s].costoTransicion;

				//Parar el proceso o continuar hasta alcanzar el grupo de transiciones
				//if(s < servicioActual){
				//	continue;			
				//}else if(s > servicioActual){
				//	break;
				//}else if(s == servicioActual){
				//	//Agregar las posibles salidas
				//	salidasServicio_i.push_back(transicionBeasleyModificado(i,j,costo));
				//}
				//Agregar las posibles salidas
				if(i == servicioActual){
					salidasServicio_i.push_back(transicionBeasleyModificado(i,j,costo));								
				}else if(i>servicioActual){
					break;				
				}				
				

			}

			//Ordenar las salidas del servicio actual por costo (definido en el TAD)
			sort(salidasServicio_i.begin(),salidasServicio_i.end());

			//Bandera de adición
			bool servicioAdicionado = false;//Suponer que no se realiza adición

			//Revisar duración de turno y si está pendiente la posible salida 's'
			for(size_t s=0;s<salidasServicio_i.size();++s){

				//Precarga de valores valores
				int i = servicioActual;
				int j = salidasServicio_i[s].j;			
				int f_i = this->listadoServiciosBeasley[i].tf;
				int s_j = this->listadoServiciosBeasley[j].t0;
				int tiempoTransicion = s_j - f_i;
				int tiempoServicio_j = this->listadoServiciosBeasley[j].duracion;

				//Buscar en pendientes para completar los criterios de adición
				unordered_set<int>::const_iterator itServicioPendiente = serviciosPendientes.find(j);

				//Si es factible y está pendiente, agregar la transición y actualizar 
				//(Está ordenado por menor costo)
				if(	duracionTurnoActual + tiempoTransicion + tiempoServicio_j <= ht &&
					itServicioPendiente != serviciosPendientes.end()){
					//Actualizar duración del turno
					duracionTurnoActual += tiempoTransicion + tiempoServicio_j;

					//Salida de diagnóstico
					//cout<<endl<<"Costo que se va a agregar:"<<salidasServicio_i[s].costo<<endl;
					//getchar();

					//Actualizar la función objetivo del constructivo
					this->funcionObjetivoConstructivo += salidasServicio_i[s].costo;
					//Actualizar el servicio actual
					servicioActual = j;
					//Actualizar grupos de control
					serviciosCubiertos.insert(servicioActual);
					serviciosPendientes.erase(servicioActual);
					//Actualizar el turno en construcción
					turnoActual.push_back(servicioActual);
					//Actualizar los tiempos parciales hasta cada servicio dentro del turno
					tiemposTurnoActual.push_back(duracionTurnoActual);
					//Activar bandera para registrar que se adicionó un servicio
					servicioAdicionado = true;
					//Terminar las iteraciones sobre el conjunto
					break;
				}		

			}//Fin del recorrido de las salidas

			//Si la bandera no fue activada, cerrar itinerario y abrir uno nuevo
			if(!servicioAdicionado){			

				//Almacenar el itinerario en el contenedor de P0
				cadenasPuntoP0_Modelo.push_back(turnoActual);
				//Almacenar el tiempo parcial en el contenedor de los y_i
				this->y_i_Constructivo.push_back(tiemposTurnoActual);
				//Almacenar la duración del turno creado para validaciones posteriores
				this->duracionTurnosConstructivo.push_back(duracionTurnoActual);
				//Limpiar el contenedor de itinerarios
				turnoActual.clear();
				//Limpiar el contenedor de tiempos parciales
				tiemposTurnoActual.clear();
				//Inicializar el itinerario con el primer servicio que se encuentre en los pendientes
				for(size_t i=0;i<num+1;++i){

					//Buscar si el servicio i está en pendientes para iniciar nuevo itinerario
					unordered_set<int>::const_iterator itServicioPendiente = serviciosPendientes.find(i);
					
					//Si fue encontrado, entonces inicializar y romper el recorrido
					if(itServicioPendiente != serviciosPendientes.end()){
						
						//Inicializar el servicio actual para las salidas en la siguiente iteración
						servicioActual = i;

						//Inicializar el turno
						turnoActual.push_back(servicioActual);												

						//Actualizar los grupos de control
						serviciosCubiertos.insert(servicioActual);
						serviciosPendientes.erase(servicioActual);

						//Inicializar el tiempo del turno
						duracionTurnoActual = this->listadoServiciosBeasley[servicioActual].duracion;

						//Almacenar el tiempo parcial
						tiemposTurnoActual.push_back(duracionTurnoActual);

						//Romper la búsqueda
						break;	
					}
					
				}//Terminar recorrido de inicialización

			}//Fin del if que inicializa itinerario

		}//Fin del recorrido de cobertura de todos los servicios

		//Tomar tiempo final del constructivo
		gettimeofday(&t_fin, NULL);	

		//Almacenar diferencia
		this->tiempoComputoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Almacenar el número de turnos del constructivo en el atributo de la instancia
		this->numeroConductoresConstructivo = cadenasPuntoP0_Modelo.size();

		//Copiar la especificación de turnos en el atributo correspondiente de la instancia
		this->turnosConstructivo = cadenasPuntoP0_Modelo;

		//Mostrar itinerarios construídos en pantalla
		cout<<endl<<"Especificación de turnos ("<<cadenasPuntoP0_Modelo.size()<<"): "<<endl;		
		for(size_t i = 0;i<cadenasPuntoP0_Modelo.size();++i){
			for(size_t j = 0;j<cadenasPuntoP0_Modelo[i].size();++j){
				cout<<cadenasPuntoP0_Modelo[i][j]<<" ("<< this->y_i_Constructivo[i][j] <<")- ";
			}
			cout<<"> "<<this->duracionTurnosConstructivo[i]<<" min"<<endl;			
			//cout<<endl;			
		}
		//getchar();//Pausar ejecución

		cout<<endl<<endl<<"-----------------------------------------";		
		cout<<endl<<this->nombreArchivo;
		cout<<endl<<"-----------------------------------------";
		cout<<endl<<"Cantidad de Servicios Cubiertos -> "<<serviciosCubiertos.size();
		cout<<endl<<"Cantidad de Servicios Pendientes -> "<<serviciosPendientes.size();
		cout<<endl<<"Turnos construídos -> "<<this->numeroConductoresConstructivo;
		cout<<endl<<"FO Constructivo -> "<<this->funcionObjetivoConstructivo;
		cout<<endl<<"Tiempo Cómputo -> "<<this->tiempoComputoConstructivo;
		cout<<endl;
		//getchar();//Parada de diagnóstico en la salida			

		//Traducir las aristas para visualizar el grafo
		this->arcosBeasleyModificado.clear();
		for(size_t i = 0;i<cadenasPuntoP0_Modelo.size();++i){			
			vector<int> turnoConstructivo;
			//Salida del depósito
			turnoConstructivo.push_back(0);
			turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][0]);
			arcosBeasleyModificado.push_back(turnoConstructivo);
			turnoConstructivo.clear();
			//Regreso al depósito			
			turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i].back());
			turnoConstructivo.push_back(num+1);
			arcosBeasleyModificado.push_back(turnoConstructivo);
			turnoConstructivo.clear();
			if(cadenasPuntoP0_Modelo[i].size()>1){
				for(size_t j = 0;j<cadenasPuntoP0_Modelo[i].size()-1;++j){
					turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][j]);
					turnoConstructivo.push_back(cadenasPuntoP0_Modelo[i][j+1]);					
					arcosBeasleyModificado.push_back(turnoConstructivo);
					turnoConstructivo.clear();				
				}
			}
									
		}
		
		////Visualizar traducción de los turnos a aristas para diagnóstico
		//cout<<endl<<"Aristas constructivo: "<<endl;
		//for(size_t i = 0;i<arcosBeasleyModificado.size();++i){
		//	for(size_t j = 0;j<arcosBeasleyModificado[i].size();++j){
		//		cout<<arcosBeasleyModificado[i][j]<<" - ";
		//	}
		//	cout<<endl;			
		//}

	}//Fin de la generación del punto inicial del modelo (P0)

	//Modelo BPP - Número de conductores
	void modeloBPP_CSP(){

		//Calcular punto inicial para obtener un límite superior ajustado del número de conductores (contenedores)
		puntoInicialModeloBeasleyModificado();
		//puntoInicialBarridoModeloBeasleyModificado();
		//generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable que controla los contenedores o turnos que se abren

		this->numeroConductoresConstructivo = 50;//Asignación termporal para diagnóstico

		IloBoolVarArray y(env, this->numeroConductoresConstructivo);
		std::stringstream name_y;
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i) {
			name_y << "y_" << i ;
			y[i] = IloBoolVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}		

		//El ítem o tarea j es asignada al contenedor i		
		IloArray<IloBoolVarArray> x(env, this->numeroConductoresConstructivo);
		std::stringstream name_x;
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i) {
			x[i] = IloBoolVarArray(env, num+1);//Incluyendo el depósito como una tarea sin costo
			for(size_t j=0;j<num+1;++j) {
				name_x << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name_x.str().c_str());			
				name_x.str(""); //Limpiar variable string que contiene el nombre
			}			
		}

		//Variable para linealización
		IloArray<IloBoolVarArray> z(env, num+1);
		std::stringstream name_z;
		for(size_t i=0;i<num+1;++i) {
			z[i] = IloBoolVarArray(env, num+1);//Incluyendo el depósito como una tarea sin costo
			for(size_t j=0;j<num+1;++j) {
				name_z << "z_" << i << "_" << j;
				z[i][j] = IloBoolVar(env, name_z.str().c_str());			
				name_z.str(""); //Limpiar variable string que contiene el nombre
			}			
		}			
		
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************

		//Restricción que controla la capacidad del contenedor (tareas y transiciones)
		IloRangeArray arregloControlCapacidad(env);
		IloRangeArray arregloLinealizacionControlCapacidad(env);
		stringstream nombreControlCapacidad;
		stringstream nombreLinealizacionControlCapacidad;
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i){
			IloExpr restriccionControlCapacidad(env);
			
			//Tiempo de servicio que aportan las tareas incluídas en el contenedor
			for(size_t j=1;j<num+1;++j){
				
				//Precarga del peso de la tarea (tiempo)
				int w_j = this->listadoServiciosBeasley[j].duracion;			
				
				//Agregar el término a la expresión
				restriccionControlCapacidad +=  w_j * x[i][j];

				//Peso del tiempo de desplazamiento entre las tareas (cambio de contexto dentro del turno)			
				for(size_t k=1;k<num+1;++k){
					//Transición a una tarea posterior siempre (las tareas están ordenadas cronológicamente)
					if(k>j){
						
						//Precarga del peso de la transición entre las tareas
						int w_jk = this->matrizTiempoTransiciones[j][k];

						//Agregar el término no lineal que se activa cuando ambas tareas han sido incluídas
						//restriccionControlCapacidad +=  w_jk * x[i][j] * x[i][k];

						//Alternativa lineal del término que se activa cuando ambas tareas han sido incluídas
						//restriccionControlCapacidad +=  w_jk * (x[i][j] + x[i][k] - 1);

						//Linealización en la restricción de capacidad
						restriccionControlCapacidad += w_jk * z[j][k];
						
						//Restricciones necesarias para la variable z_jk en cada caso
						//-----------------------------------------------------------

						//arregloLinealizacionControlCapacidad.add( z[j][k] - x[i][j] <=  0);
						IloExpr r1(env);						
						r1 += z[j][k] - x[i][j];
						stringstream nombre_r1;
						nombre_r1 << "r1_"<<i<<"_"<<j;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r1, 0, nombre_r1.str().c_str()) );

						//arregloLinealizacionControlCapacidad.add( z[j][k] - x[i][k] <=  0);
						IloExpr r2(env);						
						r2 += z[j][k] - x[i][k];
						stringstream nombre_r2;
						nombre_r2 << "r2_"<<i<<"_"<<k;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r2, 0, nombre_r2.str().c_str()) );

						//arregloLinealizacionControlCapacidad.add( x[i][j] + x[i][k] - z[j][k] <=  1);
						IloExpr r3(env);						
						r3 += x[i][j] + x[i][k] - z[j][k];
						stringstream nombre_r3;
						nombre_r3 << "r3_"<<j<<"_"<<k;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r3, 1, nombre_r3.str().c_str()) );						

					}
				}//Fin del recorrido de las tarjeas k posteriores a j

			}//Fin del recorrido de tareas j		
			

			//Adicionar la capacidad del contenedor (duración regrlamentaria de una jornada laboral)
			restriccionControlCapacidad -= this->ht * y[i];

			//Adicionar al conjunto de restricciones de capacidad por contenedor i-ésimo		
			nombreControlCapacidad << "ControlCapacidad_"<<i;		
			arregloControlCapacidad.add(IloRange(env, -IloInfinity, restriccionControlCapacidad, 0, nombreControlCapacidad.str().c_str()));
			nombreControlCapacidad.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(arregloControlCapacidad);
		//Adicionar restricciones de linealización
		modelo.add(arregloLinealizacionControlCapacidad);


		//Controlar que la tarea j sea asignada a un único turno o contenedor i		
		IloRangeArray AsignacionUnica(env);
		stringstream nombreAsignacionUnica;
		//Por cada ítem o tarea
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionAsignacionUnica(env);
			//Por cada contenedor o turno
			for(size_t i=0;i<this->numeroConductoresConstructivo;++i){
				restriccionAsignacionUnica += x[i][j];										
			}
			//Adicionar al conjunto que controla la asignación única			
			nombreAsignacionUnica << "AsignacionUnica_"<<j<<"_";			
			AsignacionUnica.add(IloRange(env, 1, restriccionAsignacionUnica, 1, nombreAsignacionUnica.str().c_str()));
			nombreAsignacionUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(AsignacionUnica);		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);

		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i){
			obj += y[i];			
		}		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver

		//Parámetro para resolver hasta optimalidad global
		//cplex.setParam(IloCplex::Param::OptimalityTarget,IloCplex::OptimalityOptimalGlobal);
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBPP_CSP.lp");		

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		/*

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}			

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}	
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		///////////////cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;

			getchar();//Pausar ejecución para daignóstico
			
			/*

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			
			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+2;++i){
				for(size_t j=0;j<num+2;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();

			*/		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"BPP CSP INFACTIBLE! (Revisar restricciones)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo bin packing para carga laboral de conductores
	
	//Modelo BPP - Factibilidad
	bool modeloBPP_CSP_Factibilidad(int K){

		//Calcular punto inicial para obtener un límite superior ajustado del número de conductores (contenedores)
		//puntoInicialModeloBeasleyModificado();
		//puntoInicialBarridoModeloBeasleyModificado();
		//generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable que controla los contenedores o turnos que se abren

		//this->numeroConductoresConstructivo = 50;//Asignación termporal para diagnóstico

		IloBoolVarArray y(env, K);
		std::stringstream name_y;
		for(size_t i=0;i<K;++i) {
			name_y << "y_" << i ;
			y[i] = IloBoolVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}		

		//El ítem o tarea j es asignada al contenedor i		
		IloArray<IloBoolVarArray> x(env, K);
		std::stringstream name_x;
		for(size_t i=0;i<K;++i) {
			x[i] = IloBoolVarArray(env, num+1);//Incluyendo el depósito como una tarea sin costo
			for(size_t j=0;j<num+1;++j) {
				name_x << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name_x.str().c_str());			
				name_x.str(""); //Limpiar variable string que contiene el nombre
			}			
		}


		/*
		//Variable para linealización
		IloArray<IloBoolVarArray> z(env, num+1);
		std::stringstream name_z;
		for(size_t i=0;i<num+1;++i) {
			z[i] = IloBoolVarArray(env, num+1);//Incluyendo el depósito como una tarea sin costo
			for(size_t j=0;j<num+1;++j) {
				name_z << "z_" << i << "_" << j;
				z[i][j] = IloBoolVar(env, name_z.str().c_str());			
				name_z.str(""); //Limpiar variable string que contiene el nombre
			}			
		}
		*/			
		
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************

		//Restricción que controla la capacidad del contenedor (tareas y transiciones)
		IloRangeArray arregloControlCapacidad(env);
		IloRangeArray arregloLinealizacionControlCapacidad(env);
		stringstream nombreControlCapacidad;
		stringstream nombreLinealizacionControlCapacidad;
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i){
			IloExpr restriccionControlCapacidad(env);
			
			//Tiempo de servicio que aportan las tareas incluídas en el contenedor
			for(size_t j=1;j<num+1;++j){
				
				//Precarga del peso de la tarea (tiempo)
				int w_j = this->listadoServiciosBeasley[j].duracion;			
				
				//Agregar el término a la expresión
				restriccionControlCapacidad +=  w_j * x[i][j];

				//Peso del tiempo de desplazamiento entre las tareas (cambio de contexto dentro del turno)			
				for(size_t k=1;k<num+1;++k){
					//Transición a una tarea posterior siempre (las tareas están ordenadas cronológicamente)
					if(k>j){
						
						//Precarga del peso de la transición entre las tareas
						int w_jk = this->matrizTiempoTransiciones[j][k];

						//Agregar el término no lineal que se activa cuando ambas tareas han sido incluídas
						restriccionControlCapacidad +=  w_jk * x[i][j] * x[i][k];

						//Alternativa lineal del término que se activa cuando ambas tareas han sido incluídas
						//restriccionControlCapacidad +=  w_jk * (x[i][j] + x[i][k] - 1);

						//Linealización en la restricción de capacidad
						//restriccionControlCapacidad += w_jk * z[j][k];
						
						//Restricciones necesarias para la variable z_jk en cada caso
						//-----------------------------------------------------------

						/*
						//arregloLinealizacionControlCapacidad.add( z[j][k] - x[i][j] <=  0);
						IloExpr r1(env);						
						r1 += z[j][k] - x[i][j];
						stringstream nombre_r1;
						nombre_r1 << "r1_"<<i<<"_"<<j;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r1, 0, nombre_r1.str().c_str()) );

						//arregloLinealizacionControlCapacidad.add( z[j][k] - x[i][k] <=  0);
						IloExpr r2(env);						
						r2 += z[j][k] - x[i][k];
						stringstream nombre_r2;
						nombre_r2 << "r2_"<<i<<"_"<<k;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r2, 0, nombre_r2.str().c_str()) );

						//arregloLinealizacionControlCapacidad.add( x[i][j] + x[i][k] - z[j][k] <=  1);
						IloExpr r3(env);						
						r3 += x[i][j] + x[i][k] - z[j][k];
						stringstream nombre_r3;
						nombre_r3 << "r3_"<<j<<"_"<<k;
						arregloLinealizacionControlCapacidad.add( IloRange(env, -IloInfinity, r3, 1, nombre_r3.str().c_str()) );						
						*/


					}
				}//Fin del recorrido de las tarjeas k posteriores a j

			}//Fin del recorrido de tareas j		
			

			//Adicionar la capacidad del contenedor (duración regrlamentaria de una jornada laboral)
			//restriccionControlCapacidad -= this->ht * y[i];

			//Adicionar al conjunto de restricciones de capacidad por contenedor i-ésimo		
			nombreControlCapacidad << "ControlCapacidad_"<<i;		
			//arregloControlCapacidad.add(IloRange(env, -IloInfinity, restriccionControlCapacidad, 0, nombreControlCapacidad.str().c_str()));
			arregloControlCapacidad.add(IloRange(env, -IloInfinity, restriccionControlCapacidad, this->ht, nombreControlCapacidad.str().c_str()));
			nombreControlCapacidad.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(arregloControlCapacidad);
		//Adicionar restricciones de linealización
		modelo.add(arregloLinealizacionControlCapacidad);


		//Controlar que la tarea j sea asignada a un único turno o contenedor i		
		IloRangeArray AsignacionUnica(env);
		stringstream nombreAsignacionUnica;
		//Por cada ítem o tarea
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionAsignacionUnica(env);
			//Por cada contenedor o turno
			for(size_t i=0;i<K;++i){
				restriccionAsignacionUnica += x[i][j];										
			}
			//Adicionar al conjunto que controla la asignación única			
			nombreAsignacionUnica << "AsignacionUnica_"<<j<<"_";			
			AsignacionUnica.add(IloRange(env, 1, restriccionAsignacionUnica, 1, nombreAsignacionUnica.str().c_str()));
			nombreAsignacionUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(AsignacionUnica);		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		/*

		//Adicionar la función objetivo
		IloExpr obj(env);

		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<this->numeroConductoresConstructivo;++i){
			obj += y[i];			
		}		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();

		*/
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver

		//Parámetro para resolver hasta optimalidad global
		//cplex.setParam(IloCplex::Param::OptimalityTarget,IloCplex::OptimalityOptimalGlobal);
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBPP_CSP_Factibilidad.lp");		

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		/*

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}			

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}	
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);

		//Enfatizar en factibilidad
		cplex.setParam(IloCplex::Param::Emphasis::MIP, CPX_MIPEMPHASIS_FEASIBILITY);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		///////////////cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);

			//Salida de diagnóstico
			cout<<endl<<"Para K = "<<K<<" Factible! BPP"<<endl;							

			//Retornar factibilidad para el valor de K recibido
			return true;
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;

			getchar();//Pausar ejecución para daignóstico
			
			/*

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			
			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+2;++i){
				for(size_t j=0;j<num+2;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();

			*/		
			
		}else{

			//Salida de diagnóstico
			cout<<endl<<"Para K = "<<K<<" INFACTIBLE BPP!"<<endl;

			//Retornar factibilidad para el valor de K recibido
			return false;
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"BPP CSP INFACTIBLE! (Revisar restricciones)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo bin packing para carga laboral de conductores
	


	//Modelo Beasley 96 CSP (Modificado)
	void modeloK_Variable(){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		puntoInicialBarridoModeloBeasleyModificado();
		generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable de la función objetivo
		stringstream name_K;
		name_K << "K";
		IloIntVar K;
		K = IloIntVar(env, name_K.str().c_str());
		name_K.str(""); //Limpiar variable string que contiene el nombre

		//Variable que se activa para transiciones entre servicios 
		//y comienzos de cadenas o turnos
		IloArray<IloBoolVarArray> x(env, num+2);
		std::stringstream name;
		for(size_t i=0;i<num+2;++i) {
			x[i] = IloBoolVarArray(env, num+2);
			for(size_t j=0;j<num+2;++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());			
				name.str(""); //Limpiar variable string que contiene el nombre
			}			
		}

		
		//Variable para diferenciación de cadenas o turnos 		
		IloIntVarArray y(env, num);
		std::stringstream name_y;
		for(size_t i=0;i<num;++i) {
			name_y << "y_" << i ;
			y[i] = IloIntVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}
		
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		bool desactivarControlLoops = false;

		/*
		//Restricciones (2) Modelo Beasley - Equilibrio entradas y salidas
		IloRangeArray EquilibrioArcosEntrantesSalientes(env);
		stringstream nombreEquilibrioArcosEntrantesSalientes;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionEquilibrioES(env);
			//Arcos de salida
			for(size_t k=1;k<num+2;++k){
				if(k!=j || desactivarControlLoops){
					restriccionEquilibrioES += x[j][k];
				}								
			}
			//Arcos de entrada
			for(size_t i=0;i<num+1;++i){
				if(i!=j || desactivarControlLoops){
					restriccionEquilibrioES -= x[i][j];
				}												
			}

			//Adicionar al conjunto (2) del modelo
			nombreEquilibrioArcosEntrantesSalientes << "EquilibrioES_{"<<j<<"}";
			EquilibrioArcosEntrantesSalientes.add(IloRange(env, -IloInfinity, restriccionEquilibrioES, 0, nombreEquilibrioArcosEntrantesSalientes.str().c_str()));
			nombreEquilibrioArcosEntrantesSalientes.str(""); // Limpiar variable para etiquetas de las restricciones

		}
		//Adicionar conjunto al modelo 
		//modelo.add(EquilibrioArcosEntrantesSalientes);
		*/


		//Restricciones (3) Modelo Beasley - Salida única, cadenas disjuntas
		IloRangeArray SalidaUnica(env);
		stringstream nombreSalidaUnica;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionSalidaUnica(env);
			//Arcos de salida
			for(size_t j=1;j<num+1;++j){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){
					restriccionSalidaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			//nombreSalidaUnica << "SalidaUnicaDesde_{"<<i<<"}";
			nombreSalidaUnica << "salida_"<<i;
			//SalidaUnica.add(IloRange(env, 1, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			SalidaUnica.add(IloRange(env, -IloInfinity, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			nombreSalidaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(SalidaUnica);

		//Restricciones (3b) Modelo Beasley - Entrada única, cadenas disjuntas
		IloRangeArray EntradaUnica(env);
		stringstream nombreEntradaUnica;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionnombreEntradaUnica(env);
			//Arcos de salida
			for(size_t i=0;i<num+1;++i){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){				
					restriccionnombreEntradaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			nombreEntradaUnica << "entrada_"<<j;
			EntradaUnica.add(IloRange(env, 1, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			//EntradaUnica.add(IloRange(env, -IloInfinity, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			nombreEntradaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(EntradaUnica);	

		//Restricción (4) Modelo Beasley - Restricción número de cadenas o turnos
		//numeroConductoresDisponibles = 208;//Asignación temporal depurando modelo
		//Cargar el límite de conductores del resultado del constructivo
		if(this->numeroConductoresConstructivo > 0){
			numeroConductoresDisponibles = this->numeroConductoresConstructivo;
		}else{
			//Valor máximo de conductores en la librería	
			//(Para mayor generalidad ilimitar con un número grande)
			numeroConductoresDisponibles = 208;
		}

		IloRangeArray LimiteSuperiorConductores(env);
		stringstream nombreLimiteSuperiorConductores;
		IloExpr restriccionLimiteSuperiorConductores(env);		
		IloExpr restriccionLimiteSuperiorConductoresRegreso(env);		
		for(size_t j=1;j<num+1;++j){

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductores += x[0][j];

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductoresRegreso += x[j][num+1];		
					
		}

		//Adicionar K como variable
		restriccionLimiteSuperiorConductores -= K;

		//Adicionar al conjunto (4) del modelo
		//nombreLimiteSuperiorConductores << "LimiteSuperiorConductores_{0_N+1}";	
		nombreLimiteSuperiorConductores << "conductores_";	
		LimiteSuperiorConductores.add(IloRange(env, 0, restriccionLimiteSuperiorConductores, 0, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, -IloInfinity, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, numeroConductoresDisponibles, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		nombreLimiteSuperiorConductores.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar conjunto al modelo 
		modelo.add(LimiteSuperiorConductores);

		////////////////Restricciones duración máxima del turno
		//Requiere revisar pertenencia a la cadena o turno

		

		//Restricciones (12) Modelo Borcinova 17 - Continuidad de cadena o turno
		IloRangeArray ContinuidadCadena(env);
		stringstream nombreContinuidadCadena;
		for(size_t t=0;t<this->listadoTransicionesPermitidasBeasley.size();++t){
		//for(size_t i=1;i<num+1;++i){
			//for(size_t j=1;j<num+1;++j){
				//if(i!=j){
				//if(j>i){

					//Precargar valores
					int i = listadoTransicionesPermitidasBeasley[t].i;
					int j = listadoTransicionesPermitidasBeasley[t].j;
					
					IloExpr restriccionContinuidadCadena(env);

					restriccionContinuidadCadena += y[i-1];
					//restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					//restriccionContinuidadCadena += (listadoServiciosBeasley[j].duracion + (listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ) ) * x[i][j];
					restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					int tiempoTransicion_ij = listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ;
					restriccionContinuidadCadena +=  tiempoTransicion_ij * x[i][j];
					restriccionContinuidadCadena -= ht * (1 -  x[i][j]);
					restriccionContinuidadCadena -= y[j-1];

					//Adicionar restricción al conjunto (12) del modelo
					nombreContinuidadCadena << "ContinuidadCadena_{"<<i<<","<<j<<"}";
					ContinuidadCadena.add(IloRange(env, -IloInfinity, restriccionContinuidadCadena, 0, nombreContinuidadCadena.str().c_str()));
					nombreContinuidadCadena.str(""); // Limpiar variable para etiquetas de las restricciones
				//}
			//}
		}
		//Adicionar conjunto al modelo 
		modelo.add(ContinuidadCadena);		

		//Límites alternativos variable y_i
		////////////////////////////////////
		IloRangeArray LimitesAlternativosA(env);
		IloRangeArray LimitesAlternativosB(env);
		stringstream nombreLimitesA;
		stringstream nombreLimitesB;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesA(env);
			IloExpr restriccionLimitesB(env);

			restriccionLimitesA += y[i-1];
			//restriccionLimitesA -= listadoServiciosBeasley[i].duracion;
			
			restriccionLimitesB += y[i-1];
			//restriccionLimitesB += ht;
			
			//Adicionar restricciones a los contenedores
			nombreLimitesA << "LimiteA_y_{"<<i<<"}";			
			LimitesAlternativosA.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesA, IloInfinity, nombreLimitesA.str().c_str()));
			nombreLimitesA.str(""); // Limpiar variable para etiquetas de las restricciones			

			nombreLimitesB << "LimiteB_y_{"<<i<<"}";
			LimitesAlternativosB.add(IloRange(env, -IloInfinity, restriccionLimitesB, ht, nombreLimitesB.str().c_str()));
			nombreLimitesB.str(""); // Limpiar variable para etiquetas de las restricciones			

		}
		//Adicionar conjuntos al modelo 
		modelo.add(LimitesAlternativosA);
		modelo.add(LimitesAlternativosB);

		//Restricciones (13) Modelo Borcinova 17 - Límites de la variable y_i para continuidad de cadena o turno
		IloRangeArray LimitesVariableContinuidad(env);
		stringstream nombreLimitesVariableContinuidad;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesVariableContinuidad(env);
			//restriccionLimitesVariableContinuidad += y[i-1] - listadoServiciosBeasley[i].duracion - ht;		
			restriccionLimitesVariableContinuidad += y[i-1];		
			//Adicionar restricción al conjunto (13) del modelo
			nombreLimitesVariableContinuidad << "LimiteVariableContinuidad_y_{"<<i<<"}";
			//LimitesVariableContinuidad.add(IloRange(env, -IloInfinity, restriccionLimitesVariableContinuidad, 0, nombreLimitesVariableContinuidad.str().c_str()));
			LimitesVariableContinuidad.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesVariableContinuidad, ht, nombreLimitesVariableContinuidad.str().c_str()));
			nombreLimitesVariableContinuidad.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		//Adicionar conjunto al modelo 
		//modelo.add(LimitesVariableContinuidad);
					

		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);

		/*
		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<num+2;++i){
			for(size_t j=0;j<num+2;++j){
				//if(i!=j){
				//if(j>i){
					obj += matrizCostosBeasley[i][j] * x[i][j];
				//}				
			}
		}
		*/

		//Minimizar K únicamente
		obj += K;		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBeasley_MinimizarK.lp");

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0

		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();

		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}		


		

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}

		/*
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;
			
			

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			/*

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			*/

			/*

			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+1;++i){
				for(size_t j=0;j<num+1;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = 0;//Cuando se minimiza K
			//double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);//Cuando se minimizan costos
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;
			foBeasley = cplex.getObjValue();

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			*/			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<cplex.getObjValue()<<" ";				
				//archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();

			

					
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"Beasle Modificado INFACTIBLE! (Revisar restricciones)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo esclavo
	
	//Modelo Beasley 96 CSP (Modificado)
	void modeloBeasleyModificadoArcos(){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		puntoInicialBarridoModeloBeasleyModificado();
		generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable que se activa para transiciones entre servicios 
		//y comienzos de cadenas o turnos
		IloArray<IloBoolVarArray> x(env, num+2);
		std::stringstream name;
		for(size_t i=0;i<num+2;++i) {
			x[i] = IloBoolVarArray(env, num+2);
			for(size_t j=0;j<num+2;++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());			
				name.str(""); //Limpiar variable string que contiene el nombre
			}			
		}

		
		//Variable para diferenciación de cadenas o turnos 		
		IloIntVarArray y(env, num);
		std::stringstream name_y;
		for(size_t i=0;i<num;++i) {
			name_y << "y_" << i ;
			y[i] = IloIntVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}
		
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		bool desactivarControlLoops = false;

		/*
		//Restricciones (2) Modelo Beasley - Equilibrio entradas y salidas
		IloRangeArray EquilibrioArcosEntrantesSalientes(env);
		stringstream nombreEquilibrioArcosEntrantesSalientes;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionEquilibrioES(env);
			//Arcos de salida
			for(size_t k=1;k<num+2;++k){
				if(k!=j || desactivarControlLoops){
					restriccionEquilibrioES += x[j][k];
				}								
			}
			//Arcos de entrada
			for(size_t i=0;i<num+1;++i){
				if(i!=j || desactivarControlLoops){
					restriccionEquilibrioES -= x[i][j];
				}												
			}

			//Adicionar al conjunto (2) del modelo
			nombreEquilibrioArcosEntrantesSalientes << "EquilibrioES_{"<<j<<"}";
			EquilibrioArcosEntrantesSalientes.add(IloRange(env, -IloInfinity, restriccionEquilibrioES, 0, nombreEquilibrioArcosEntrantesSalientes.str().c_str()));
			nombreEquilibrioArcosEntrantesSalientes.str(""); // Limpiar variable para etiquetas de las restricciones

		}
		//Adicionar conjunto al modelo 
		//modelo.add(EquilibrioArcosEntrantesSalientes);
		*/


		//Restricciones (3) Modelo Beasley - Salida única, cadenas disjuntas
		IloRangeArray SalidaUnica(env);
		stringstream nombreSalidaUnica;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionSalidaUnica(env);
			//Arcos de salida
			for(size_t j=1;j<num+1;++j){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){
					restriccionSalidaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			//nombreSalidaUnica << "SalidaUnicaDesde_{"<<i<<"}";
			nombreSalidaUnica << "salida_"<<i;
			//SalidaUnica.add(IloRange(env, 1, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			SalidaUnica.add(IloRange(env, -IloInfinity, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			nombreSalidaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(SalidaUnica);

		//Restricciones (3b) Modelo Beasley - Entrada única, cadenas disjuntas
		IloRangeArray EntradaUnica(env);
		stringstream nombreEntradaUnica;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionnombreEntradaUnica(env);
			//Arcos de salida
			for(size_t i=0;i<num+1;++i){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){				
					restriccionnombreEntradaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			nombreEntradaUnica << "entrada_"<<j;
			EntradaUnica.add(IloRange(env, 1, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			//EntradaUnica.add(IloRange(env, -IloInfinity, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			nombreEntradaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(EntradaUnica);

		//Restricción (4) Modelo Beasley - Restricción número de cadenas o turnos
		//numeroConductoresDisponibles = 208;//Asignación temporal depurando modelo
		//Cargar el límite de conductores del resultado del constructivo
		if(this->numeroConductoresConstructivo > 0){
			numeroConductoresDisponibles = this->numeroConductoresConstructivo;
		}else{
			//Valor máximo de conductores en la librería	
			//(Para mayor generalidad ilimitar con un número grande)
			numeroConductoresDisponibles = 208;
		}

		IloRangeArray LimiteSuperiorConductores(env);
		stringstream nombreLimiteSuperiorConductores;
		IloExpr restriccionLimiteSuperiorConductores(env);		
		IloExpr restriccionLimiteSuperiorConductoresRegreso(env);		
		for(size_t j=1;j<num+1;++j){

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductores += x[0][j];

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductoresRegreso += x[j][num+1];		
					
		}
		//Adicionar al conjunto (4) del modelo
		//nombreLimiteSuperiorConductores << "LimiteSuperiorConductores_{0_N+1}";	
		nombreLimiteSuperiorConductores << "conductores_";	
		LimiteSuperiorConductores.add(IloRange(env, -IloInfinity, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, numeroConductoresDisponibles, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		nombreLimiteSuperiorConductores.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar conjunto al modelo 
		modelo.add(LimiteSuperiorConductores);

		////////////////Restricciones duración máxima del turno
		//Requiere revisar pertenencia a la cadena o turno

		

		//Restricciones (12) Modelo Borcinova 17 - Continuidad de cadena o turno
		IloRangeArray ContinuidadCadena(env);
		stringstream nombreContinuidadCadena;
		for(size_t t=0;t<this->listadoTransicionesPermitidasBeasley.size();++t){
		//for(size_t i=1;i<num+1;++i){
			//for(size_t j=1;j<num+1;++j){
				//if(i!=j){
				//if(j>i){

					//Precargar valores
					int i = listadoTransicionesPermitidasBeasley[t].i;
					int j = listadoTransicionesPermitidasBeasley[t].j;
					
					IloExpr restriccionContinuidadCadena(env);

					restriccionContinuidadCadena += y[i-1];
					//restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					//restriccionContinuidadCadena += (listadoServiciosBeasley[j].duracion + (listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ) ) * x[i][j];
					restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					int tiempoTransicion_ij = listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ;
					restriccionContinuidadCadena +=  tiempoTransicion_ij * x[i][j];
					restriccionContinuidadCadena -= ht * (1 -  x[i][j]);
					restriccionContinuidadCadena -= y[j-1];

					//Adicionar restricción al conjunto (12) del modelo
					nombreContinuidadCadena << "ContinuidadCadena_{"<<i<<","<<j<<"}";
					ContinuidadCadena.add(IloRange(env, -IloInfinity, restriccionContinuidadCadena, 0, nombreContinuidadCadena.str().c_str()));
					nombreContinuidadCadena.str(""); // Limpiar variable para etiquetas de las restricciones
				//}
			//}
		}
		//Adicionar conjunto al modelo 
		modelo.add(ContinuidadCadena);		

		//Límites alternativos variable y_i
		////////////////////////////////////
		IloRangeArray LimitesAlternativosA(env);
		IloRangeArray LimitesAlternativosB(env);
		stringstream nombreLimitesA;
		stringstream nombreLimitesB;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesA(env);
			IloExpr restriccionLimitesB(env);

			restriccionLimitesA += y[i-1];
			//restriccionLimitesA -= listadoServiciosBeasley[i].duracion;
			
			restriccionLimitesB += y[i-1];
			//restriccionLimitesB += ht;
			
			//Adicionar restricciones a los contenedores
			nombreLimitesA << "LimiteA_y_{"<<i<<"}";			
			LimitesAlternativosA.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesA, IloInfinity, nombreLimitesA.str().c_str()));
			nombreLimitesA.str(""); // Limpiar variable para etiquetas de las restricciones			

			nombreLimitesB << "LimiteB_y_{"<<i<<"}";
			LimitesAlternativosB.add(IloRange(env, -IloInfinity, restriccionLimitesB, ht, nombreLimitesB.str().c_str()));
			nombreLimitesB.str(""); // Limpiar variable para etiquetas de las restricciones			

		}
		//Adicionar conjuntos al modelo 
		modelo.add(LimitesAlternativosA);
		modelo.add(LimitesAlternativosB);

		//Restricciones (13) Modelo Borcinova 17 - Límites de la variable y_i para continuidad de cadena o turno
		IloRangeArray LimitesVariableContinuidad(env);
		stringstream nombreLimitesVariableContinuidad;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesVariableContinuidad(env);
			//restriccionLimitesVariableContinuidad += y[i-1] - listadoServiciosBeasley[i].duracion - ht;		
			restriccionLimitesVariableContinuidad += y[i-1];		
			//Adicionar restricción al conjunto (13) del modelo
			nombreLimitesVariableContinuidad << "LimiteVariableContinuidad_y_{"<<i<<"}";
			//LimitesVariableContinuidad.add(IloRange(env, -IloInfinity, restriccionLimitesVariableContinuidad, 0, nombreLimitesVariableContinuidad.str().c_str()));
			LimitesVariableContinuidad.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesVariableContinuidad, ht, nombreLimitesVariableContinuidad.str().c_str()));
			nombreLimitesVariableContinuidad.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		//Adicionar conjunto al modelo 
		//modelo.add(LimitesVariableContinuidad);						

		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);

		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<num+2;++i){
			for(size_t j=0;j<num+2;++j){
				//if(i!=j){
				//if(j>i){
					obj += matrizCostosBeasley[i][j] * x[i][j];
				//}				
			}
		}		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBeasleyArcos.lp");

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0

		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();

		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}		


		

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}

		/*
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;
			

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			
			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+2;++i){
				for(size_t j=0;j<num+2;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"Beasle Modificado INFACTIBLE! (Revisar restricciones)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo Beasley Modificado Arcos
	

	//Modelo Beasley 96 CSP (Modificado)
	void modeloBeasleyModificado(){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		puntoInicialBarridoModeloBeasleyModificado();
		generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable que se activa para transiciones entre servicios 
		//y comienzos de cadenas o turnos
		IloArray<IloBoolVarArray> x(env, num+2);
		std::stringstream name;
		for(size_t i=0;i<num+2;++i) {
			x[i] = IloBoolVarArray(env, num+2);
			for(size_t j=0;j<num+2;++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());			
				name.str(""); //Limpiar variable string que contiene el nombre
			}			
		}

		
		//Variable para diferenciación de cadenas o turnos 		
		IloIntVarArray y(env, num);
		std::stringstream name_y;
		for(size_t i=0;i<num;++i) {
			name_y << "y_" << i ;
			y[i] = IloIntVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}
		
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		bool desactivarControlLoops = false;

		/*
		//Restricciones (2) Modelo Beasley - Equilibrio entradas y salidas
		IloRangeArray EquilibrioArcosEntrantesSalientes(env);
		stringstream nombreEquilibrioArcosEntrantesSalientes;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionEquilibrioES(env);
			//Arcos de salida
			for(size_t k=1;k<num+2;++k){
				if(k!=j || desactivarControlLoops){
					restriccionEquilibrioES += x[j][k];
				}								
			}
			//Arcos de entrada
			for(size_t i=0;i<num+1;++i){
				if(i!=j || desactivarControlLoops){
					restriccionEquilibrioES -= x[i][j];
				}												
			}

			//Adicionar al conjunto (2) del modelo
			nombreEquilibrioArcosEntrantesSalientes << "EquilibrioES_{"<<j<<"}";
			EquilibrioArcosEntrantesSalientes.add(IloRange(env, -IloInfinity, restriccionEquilibrioES, 0, nombreEquilibrioArcosEntrantesSalientes.str().c_str()));
			nombreEquilibrioArcosEntrantesSalientes.str(""); // Limpiar variable para etiquetas de las restricciones

		}
		//Adicionar conjunto al modelo 
		//modelo.add(EquilibrioArcosEntrantesSalientes);
		*/


		//Restricciones (3) Modelo Beasley - Salida única, cadenas disjuntas
		IloRangeArray SalidaUnica(env);
		stringstream nombreSalidaUnica;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionSalidaUnica(env);
			//Arcos de salida
			for(size_t j=1;j<num+1;++j){
				//if(j!=i || desactivarControlLoops){
				//if(j!=i){
				if(j!=i){
					restriccionSalidaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			//nombreSalidaUnica << "SalidaUnicaDesde_{"<<i<<"}";
			nombreSalidaUnica << "salida_"<<i;
			//SalidaUnica.add(IloRange(env, 1, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			SalidaUnica.add(IloRange(env, -IloInfinity, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			nombreSalidaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(SalidaUnica);

		//Restricciones (3b) Modelo Beasley - Entrada única, cadenas disjuntas
		IloRangeArray EntradaUnica(env);
		stringstream nombreEntradaUnica;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionnombreEntradaUnica(env);
			//Arcos de salida
			for(size_t i=0;i<num+1;++i){
				//if(j!=i || desactivarControlLoops){
				//if(j!=i){
				if(j!=i){
					restriccionnombreEntradaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			nombreEntradaUnica << "entrada_"<<j;
			EntradaUnica.add(IloRange(env, 1, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			//EntradaUnica.add(IloRange(env, -IloInfinity, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			nombreEntradaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(EntradaUnica);	

		//Restricción (4) Modelo Beasley - Restricción número de cadenas o turnos
		//numeroConductoresDisponibles = 208;//Asignación temporal depurando modelo
		//Cargar el límite de conductores del resultado del constructivo
		if(this->numeroConductoresConstructivo > 0){
			numeroConductoresDisponibles = this->numeroConductoresConstructivo;
		}else{
			//Valor máximo de conductores en la librería	
			//(Para mayor generalidad ilimitar con un número grande)
			numeroConductoresDisponibles = 208;
		}

		IloRangeArray LimiteSuperiorConductores(env);
		stringstream nombreLimiteSuperiorConductores;
		IloExpr restriccionLimiteSuperiorConductores(env);		
		IloExpr restriccionLimiteSuperiorConductoresRegreso(env);		
		for(size_t j=1;j<num+1;++j){

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductores += x[0][j];

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductoresRegreso += x[j][num+1];		
					
		}
		//Adicionar al conjunto (4) del modelo
		//nombreLimiteSuperiorConductores << "LimiteSuperiorConductores_{0_N+1}";	
		nombreLimiteSuperiorConductores << "conductores_";	
		LimiteSuperiorConductores.add(IloRange(env, -IloInfinity, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, numeroConductoresDisponibles, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		nombreLimiteSuperiorConductores.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar conjunto al modelo 
		modelo.add(LimiteSuperiorConductores);

		////////////////Restricciones duración máxima del turno
		//Requiere revisar pertenencia a la cadena o turno

		

		//Restricciones (12) Modelo Borcinova 17 - Continuidad de cadena o turno
		IloRangeArray ContinuidadCadena(env);
		stringstream nombreContinuidadCadena;
		for(size_t i=1;i<num+1;++i){
			for(size_t j=1;j<num+1;++j){
				//if(i!=j){
				if(j>i){
					
					IloExpr restriccionContinuidadCadena(env);

					restriccionContinuidadCadena += y[i-1];
					//restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					//restriccionContinuidadCadena += (listadoServiciosBeasley[j].duracion + (listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ) ) * x[i][j];
					restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					int tiempoTransicion_ij = listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ;
					restriccionContinuidadCadena +=  tiempoTransicion_ij * x[i][j];
					restriccionContinuidadCadena -= ht * (1 -  x[i][j]);
					restriccionContinuidadCadena -= y[j-1];

					//Adicionar restricción al conjunto (12) del modelo
					nombreContinuidadCadena << "ContinuidadCadena_{"<<i<<","<<j<<"}";
					ContinuidadCadena.add(IloRange(env, -IloInfinity, restriccionContinuidadCadena, 0, nombreContinuidadCadena.str().c_str()));
					nombreContinuidadCadena.str(""); // Limpiar variable para etiquetas de las restricciones
				}
			}
		}
		//Adicionar conjunto al modelo 
		modelo.add(ContinuidadCadena);		

		//Límites alternativos variable y_i
		////////////////////////////////////
		IloRangeArray LimitesAlternativosA(env);
		IloRangeArray LimitesAlternativosB(env);
		stringstream nombreLimitesA;
		stringstream nombreLimitesB;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesA(env);
			IloExpr restriccionLimitesB(env);

			restriccionLimitesA += y[i-1];
			//restriccionLimitesA -= listadoServiciosBeasley[i].duracion;
			
			restriccionLimitesB += y[i-1];
			//restriccionLimitesB += ht;
			
			//Adicionar restricciones a los contenedores
			nombreLimitesA << "LimiteA_y_{"<<i<<"}";			
			LimitesAlternativosA.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesA, IloInfinity, nombreLimitesA.str().c_str()));
			nombreLimitesA.str(""); // Limpiar variable para etiquetas de las restricciones			

			nombreLimitesB << "LimiteB_y_{"<<i<<"}";
			LimitesAlternativosB.add(IloRange(env, -IloInfinity, restriccionLimitesB, ht, nombreLimitesB.str().c_str()));
			nombreLimitesB.str(""); // Limpiar variable para etiquetas de las restricciones			

		}
		//Adicionar conjuntos al modelo 
		modelo.add(LimitesAlternativosA);
		modelo.add(LimitesAlternativosB);

		//Restricciones (13) Modelo Borcinova 17 - Límites de la variable y_i para continuidad de cadena o turno
		IloRangeArray LimitesVariableContinuidad(env);
		stringstream nombreLimitesVariableContinuidad;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesVariableContinuidad(env);
			//restriccionLimitesVariableContinuidad += y[i-1] - listadoServiciosBeasley[i].duracion - ht;		
			restriccionLimitesVariableContinuidad += y[i-1];		
			//Adicionar restricción al conjunto (13) del modelo
			nombreLimitesVariableContinuidad << "LimiteVariableContinuidad_y_{"<<i<<"}";
			//LimitesVariableContinuidad.add(IloRange(env, -IloInfinity, restriccionLimitesVariableContinuidad, 0, nombreLimitesVariableContinuidad.str().c_str()));
			LimitesVariableContinuidad.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesVariableContinuidad, ht, nombreLimitesVariableContinuidad.str().c_str()));
			nombreLimitesVariableContinuidad.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		//Adicionar conjunto al modelo 
		//modelo.add(LimitesVariableContinuidad);
					

		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);

		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<num+2;++i){
			for(size_t j=0;j<num+2;++j){
				//if(i!=j){
				//if(j>i){
					obj += matrizCostosBeasley[i][j] * x[i][j];
				//}				
			}
		}		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBeasley.lp");

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0

		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();

		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}		


		

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}

		/*
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;
			

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			
			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+2;++i){
				for(size_t j=0;j<num+2;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"Beasle Modificado INFACTIBLE! (Revisar restricciones)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo esclavo
	
	//Modelo Beasley Modificado Relajado en Tiempo
	int modeloBeasleyModificadoRelajado(){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		puntoInicialBarridoModeloBeasleyModificado();
		//generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable que se activa para transiciones entre servicios 
		//y comienzos de cadenas o turnos
		IloArray<IloBoolVarArray> x(env, num+2);
		std::stringstream name;
		for(size_t i=0;i<num+2;++i) {
			x[i] = IloBoolVarArray(env, num+2);
			for(size_t j=0;j<num+2;++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());			
				name.str(""); //Limpiar variable string que contiene el nombre
			}			
		}
		
		//Variable para diferenciación de cadenas o turnos 		
		IloIntVarArray y(env, num);
		std::stringstream name_y;
		for(size_t i=0;i<num;++i) {
			name_y << "y_" << i ;
			y[i] = IloIntVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}
		
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		bool desactivarControlLoops = false;

		/*
		//Restricciones (2) Modelo Beasley - Equilibrio entradas y salidas
		IloRangeArray EquilibrioArcosEntrantesSalientes(env);
		stringstream nombreEquilibrioArcosEntrantesSalientes;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionEquilibrioES(env);
			//Arcos de salida
			for(size_t k=1;k<num+2;++k){
				if(k!=j || desactivarControlLoops){
					restriccionEquilibrioES += x[j][k];
				}								
			}
			//Arcos de entrada
			for(size_t i=0;i<num+1;++i){
				if(i!=j || desactivarControlLoops){
					restriccionEquilibrioES -= x[i][j];
				}												
			}

			//Adicionar al conjunto (2) del modelo
			nombreEquilibrioArcosEntrantesSalientes << "EquilibrioES_{"<<j<<"}";
			EquilibrioArcosEntrantesSalientes.add(IloRange(env, -IloInfinity, restriccionEquilibrioES, 0, nombreEquilibrioArcosEntrantesSalientes.str().c_str()));
			nombreEquilibrioArcosEntrantesSalientes.str(""); // Limpiar variable para etiquetas de las restricciones

		}
		//Adicionar conjunto al modelo 
		//modelo.add(EquilibrioArcosEntrantesSalientes);
		*/


		//Restricciones (3) Modelo Beasley - Salida única, cadenas disjuntas
		IloRangeArray SalidaUnica(env);
		stringstream nombreSalidaUnica;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionSalidaUnica(env);
			//Arcos de salida
			for(size_t j=1;j<num+1;++j){
				//if(j!=i || desactivarControlLoops){
				//if(j!=i){
				if(j!=i){
					restriccionSalidaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			//nombreSalidaUnica << "SalidaUnicaDesde_{"<<i<<"}";
			nombreSalidaUnica << "salida_"<<i;
			//SalidaUnica.add(IloRange(env, 1, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			SalidaUnica.add(IloRange(env, -IloInfinity, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			nombreSalidaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(SalidaUnica);

		//Restricciones (3b) Modelo Beasley - Entrada única, cadenas disjuntas
		IloRangeArray EntradaUnica(env);
		stringstream nombreEntradaUnica;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionnombreEntradaUnica(env);
			//Arcos de salida
			for(size_t i=0;i<num+1;++i){
				//if(j!=i || desactivarControlLoops){
				//if(j!=i){
				if(j!=i){
					restriccionnombreEntradaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			nombreEntradaUnica << "entrada_"<<j;
			EntradaUnica.add(IloRange(env, 1, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			//EntradaUnica.add(IloRange(env, -IloInfinity, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			nombreEntradaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(EntradaUnica);	

		//Restricción (4) Modelo Beasley - Restricción número de cadenas o turnos
		//numeroConductoresDisponibles = 208;//Asignación temporal depurando modelo
		//Cargar el límite de conductores del resultado del constructivo
		if(this->numeroConductoresConstructivo > 0){
			numeroConductoresDisponibles = this->numeroConductoresConstructivo;
		}else{
			//Valor máximo de conductores en la librería	
			//(Para mayor generalidad ilimitar con un número grande)
			numeroConductoresDisponibles = 208;
		}

		IloRangeArray LimiteSuperiorConductores(env);
		stringstream nombreLimiteSuperiorConductores;
		IloExpr restriccionLimiteSuperiorConductores(env);		
		IloExpr restriccionLimiteSuperiorConductoresRegreso(env);		
		for(size_t j=1;j<num+1;++j){

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductores += x[0][j];

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductoresRegreso += x[j][num+1];		
					
		}
		//Adicionar al conjunto (4) del modelo
		//nombreLimiteSuperiorConductores << "LimiteSuperiorConductores_{0_N+1}";	
		nombreLimiteSuperiorConductores << "conductores_";	
		LimiteSuperiorConductores.add(IloRange(env, -IloInfinity, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, numeroConductoresDisponibles, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		nombreLimiteSuperiorConductores.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar conjunto al modelo 
		modelo.add(LimiteSuperiorConductores);

		////////////////Restricciones duración máxima del turno
		//Requiere revisar pertenencia a la cadena o turno

		

		//Restricciones (12) Modelo Borcinova 17 - Continuidad de cadena o turno
		IloRangeArray ContinuidadCadena(env);
		stringstream nombreContinuidadCadena;
		for(size_t i=1;i<num+1;++i){
			for(size_t j=1;j<num+1;++j){
				//if(i!=j){
				if(j>i){
					
					IloExpr restriccionContinuidadCadena(env);

					restriccionContinuidadCadena += y[i-1];
					//restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					//restriccionContinuidadCadena += (listadoServiciosBeasley[j].duracion + (listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ) ) * x[i][j];
					restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					int tiempoTransicion_ij = listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ;
					restriccionContinuidadCadena +=  tiempoTransicion_ij * x[i][j];
					restriccionContinuidadCadena -= ht * (1 -  x[i][j]);
					restriccionContinuidadCadena -= y[j-1];

					//Adicionar restricción al conjunto (12) del modelo
					nombreContinuidadCadena << "ContinuidadCadena_{"<<i<<","<<j<<"}";
					ContinuidadCadena.add(IloRange(env, -IloInfinity, restriccionContinuidadCadena, 0, nombreContinuidadCadena.str().c_str()));
					nombreContinuidadCadena.str(""); // Limpiar variable para etiquetas de las restricciones
				}
			}
		}
		//Adicionar conjunto al modelo 
		//modelo.add(ContinuidadCadena);		

		//Límites alternativos variable y_i
		////////////////////////////////////
		IloRangeArray LimitesAlternativosA(env);
		IloRangeArray LimitesAlternativosB(env);
		stringstream nombreLimitesA;
		stringstream nombreLimitesB;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesA(env);
			IloExpr restriccionLimitesB(env);

			restriccionLimitesA += y[i-1];
			//restriccionLimitesA -= listadoServiciosBeasley[i].duracion;
			
			restriccionLimitesB += y[i-1];
			//restriccionLimitesB += ht;
			
			//Adicionar restricciones a los contenedores
			nombreLimitesA << "LimiteA_y_{"<<i<<"}";			
			LimitesAlternativosA.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesA, IloInfinity, nombreLimitesA.str().c_str()));
			nombreLimitesA.str(""); // Limpiar variable para etiquetas de las restricciones			

			nombreLimitesB << "LimiteB_y_{"<<i<<"}";
			LimitesAlternativosB.add(IloRange(env, -IloInfinity, restriccionLimitesB, ht, nombreLimitesB.str().c_str()));
			nombreLimitesB.str(""); // Limpiar variable para etiquetas de las restricciones			

		}
		//Adicionar conjuntos al modelo 
		//modelo.add(LimitesAlternativosA);
		//modelo.add(LimitesAlternativosB);

		//Restricciones (13) Modelo Borcinova 17 - Límites de la variable y_i para continuidad de cadena o turno
		IloRangeArray LimitesVariableContinuidad(env);
		stringstream nombreLimitesVariableContinuidad;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesVariableContinuidad(env);
			//restriccionLimitesVariableContinuidad += y[i-1] - listadoServiciosBeasley[i].duracion - ht;		
			restriccionLimitesVariableContinuidad += y[i-1];		
			//Adicionar restricción al conjunto (13) del modelo
			nombreLimitesVariableContinuidad << "LimiteVariableContinuidad_y_{"<<i<<"}";
			//LimitesVariableContinuidad.add(IloRange(env, -IloInfinity, restriccionLimitesVariableContinuidad, 0, nombreLimitesVariableContinuidad.str().c_str()));
			LimitesVariableContinuidad.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesVariableContinuidad, ht, nombreLimitesVariableContinuidad.str().c_str()));
			nombreLimitesVariableContinuidad.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		//Adicionar conjunto al modelo 
		//modelo.add(LimitesVariableContinuidad);
					

		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);

		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<num+2;++i){
			for(size_t j=0;j<num+2;++j){
				//if(i!=j){
				//if(j>i){
					obj += matrizCostosBeasley[i][j] * x[i][j];
				//}				
			}
		}		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("modeloBeasley.lp");

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();
		
		/*

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0

		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();

		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}		

		
		
		

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}

		*/

		/*
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Establecer variantes de solución
		//int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		//cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			//cout<<"__________________________________________________________________________________________________"<<endl;
			//cout<<"__________________________________________________________________________________________________"<<endl;
			//cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			//cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;
			

			//Salidas del depósito (número de turnos)
			//cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					//cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			//cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			//cout<<"-------------------------------------"<<endl;
			//cout<<"-------------------------------------"<<endl;

			//Retornar la cantidad de turnos generados en la versión relajada
			return contadorTurnosGenerados;

			//Llegadas al depósito (número de turnos)
			//cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					//cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			//cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			//cout<<"-------------------------------------"<<endl;
			//cout<<"-------------------------------------"<<endl;
			
			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+2;++i){
				for(size_t j=0;j<num+2;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			//cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			//cout<<"-------------------------------------"<<endl;
			//cout<<"-------------------------------------"<<endl;

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;

			/*

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;			
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();
			*/		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"Beasley -Relajado- Modificado INFACTIBLE! (Revisar restricciones)"<<endl;
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo esclavo
	
	
	//Resolver hasta encontrar el K factible
	bool modeloExploracionK(int K_exploracion){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		//puntoInicialBarridoModeloBeasleyModificado();
		//generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla		

		//Ejemplo de toma de tiempos con la función 'timeval_diff'
		////Tomar tiempo inicial del constructivo
		//gettimeofday(&t_ini, NULL);		
		////Tomar tiempo final del constructivo
		//gettimeofday(&t_fin, NULL);		
		////Tomar tiempo de la solución relajada
		//tiempoTranscurridoConstructivo = timeval_diff(&t_fin, &t_ini);

		//Declaración de variables para medir el tiempo del constructivo
		struct timeval t_ini, t_fin;		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver

		//Variable de la función objetivo
		stringstream name_K;
		name_K << "K";
		IloIntVar K;
		K = IloIntVar(env, name_K.str().c_str());
		name_K.str(""); //Limpiar variable string que contiene el nombre

		//Variable que se activa para transiciones entre servicios 
		//y comienzos de cadenas o turnos
		IloArray<IloBoolVarArray> x(env, num+2);
		std::stringstream name;
		for(size_t i=0;i<num+2;++i) {
			x[i] = IloBoolVarArray(env, num+2);
			for(size_t j=0;j<num+2;++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());			
				name.str(""); //Limpiar variable string que contiene el nombre
			}			
		}

		
		//Variable para diferenciación de cadenas o turnos 		
		IloIntVarArray y(env, num);
		std::stringstream name_y;
		for(size_t i=0;i<num;++i) {
			name_y << "y_" << i ;
			y[i] = IloIntVar(env,name_y.str().c_str());			
			name_y.str(""); //Limpiar variable string que contiene el nombre						
		}
		
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		bool desactivarControlLoops = false;

		//Restricciones (3) Modelo Beasley - Salida única, cadenas disjuntas
		IloRangeArray SalidaUnica(env);
		stringstream nombreSalidaUnica;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionSalidaUnica(env);
			//Arcos de salida
			for(size_t j=1;j<num+1;++j){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){
					restriccionSalidaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			//nombreSalidaUnica << "SalidaUnicaDesde_{"<<i<<"}";
			nombreSalidaUnica << "salida_"<<i;
			//SalidaUnica.add(IloRange(env, 1, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			SalidaUnica.add(IloRange(env, -IloInfinity, restriccionSalidaUnica, 1, nombreSalidaUnica.str().c_str()));
			nombreSalidaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(SalidaUnica);

		//Restricciones (3b) Modelo Beasley - Entrada única, cadenas disjuntas
		IloRangeArray EntradaUnica(env);
		stringstream nombreEntradaUnica;
		for(size_t j=1;j<num+1;++j){
			IloExpr restriccionnombreEntradaUnica(env);
			//Arcos de salida
			for(size_t i=0;i<num+1;++i){

				//Precargar factibilidad
				int costo = this->matrizCostosBeasley[i][j];

				//Temporalidad y factibilidad
				if(j>i && costo != 100000000){				
					restriccionnombreEntradaUnica += x[i][j];
				}							
			}
			//Adicionar al conjunto (3) del modelo
			nombreEntradaUnica << "entrada_"<<j;
			EntradaUnica.add(IloRange(env, 1, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			//EntradaUnica.add(IloRange(env, -IloInfinity, restriccionnombreEntradaUnica, 1, nombreEntradaUnica.str().c_str()));
			nombreEntradaUnica.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar conjunto al modelo 
		modelo.add(EntradaUnica);	

		//Restricción (4) Modelo Beasley - Restricción número de cadenas o turnos
		//numeroConductoresDisponibles = 208;//Asignación temporal depurando modelo
		//Cargar el límite de conductores del resultado del constructivo
		if(this->numeroConductoresConstructivo > 0){
			numeroConductoresDisponibles = this->numeroConductoresConstructivo;
		}else{
			//Valor máximo de conductores en la librería	
			//(Para mayor generalidad ilimitar con un número grande)
			numeroConductoresDisponibles = 208;
		}

		IloRangeArray LimiteSuperiorConductores(env);
		stringstream nombreLimiteSuperiorConductores;
		IloExpr restriccionLimiteSuperiorConductores(env);		
		IloExpr restriccionLimiteSuperiorConductoresRegreso(env);		
		for(size_t j=1;j<num+1;++j){

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductores += x[0][j];

			//Salidas o cadenas partiendo del depósito (Nodo 0)		
			restriccionLimiteSuperiorConductoresRegreso += x[j][num+1];		
					
		}

		//Adicionar K como variable
		//restriccionLimiteSuperiorConductores -= K;

		//Adicionar al conjunto (4) del modelo
		//nombreLimiteSuperiorConductores << "LimiteSuperiorConductores_{0_N+1}";	
		nombreLimiteSuperiorConductores << "conductores_";	
		LimiteSuperiorConductores.add(IloRange(env, K_exploracion, restriccionLimiteSuperiorConductores, K_exploracion, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, 0, restriccionLimiteSuperiorConductores, 0, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, -IloInfinity, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		//LimiteSuperiorConductores.add(IloRange(env, numeroConductoresDisponibles, restriccionLimiteSuperiorConductores, numeroConductoresDisponibles, nombreLimiteSuperiorConductores.str().c_str()));		
		nombreLimiteSuperiorConductores.str(""); // Limpiar variable para etiquetas de las restricciones
		//Adicionar conjunto al modelo 
		modelo.add(LimiteSuperiorConductores);

		////////////////Restricciones duración máxima del turno
		//Requiere revisar pertenencia a la cadena o turno
		

		//Restricciones (12) Modelo Borcinova 17 - Continuidad de cadena o turno
		IloRangeArray ContinuidadCadena(env);
		stringstream nombreContinuidadCadena;
		for(size_t t=0;t<this->listadoTransicionesPermitidasBeasley.size();++t){
		//for(size_t i=1;i<num+1;++i){
			//for(size_t j=1;j<num+1;++j){
				//if(i!=j){
				//if(j>i){

					//Precargar valores
					int i = listadoTransicionesPermitidasBeasley[t].i;
					int j = listadoTransicionesPermitidasBeasley[t].j;
					
					IloExpr restriccionContinuidadCadena(env);

					restriccionContinuidadCadena += y[i-1];
					//restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					//restriccionContinuidadCadena += (listadoServiciosBeasley[j].duracion + (listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ) ) * x[i][j];
					restriccionContinuidadCadena += listadoServiciosBeasley[j].duracion * x[i][j];
					int tiempoTransicion_ij = listadoServiciosBeasley[j].t0 - listadoServiciosBeasley[i].tf ;
					restriccionContinuidadCadena +=  tiempoTransicion_ij * x[i][j];
					restriccionContinuidadCadena -= ht * (1 -  x[i][j]);
					restriccionContinuidadCadena -= y[j-1];

					//Adicionar restricción al conjunto (12) del modelo
					nombreContinuidadCadena << "ContinuidadCadena_{"<<i<<","<<j<<"}";
					ContinuidadCadena.add(IloRange(env, -IloInfinity, restriccionContinuidadCadena, 0, nombreContinuidadCadena.str().c_str()));
					nombreContinuidadCadena.str(""); // Limpiar variable para etiquetas de las restricciones
				//}
			//}
		}
		//Adicionar conjunto al modelo 
		modelo.add(ContinuidadCadena);		

		//Límites alternativos variable y_i
		////////////////////////////////////
		IloRangeArray LimitesAlternativosA(env);
		IloRangeArray LimitesAlternativosB(env);
		stringstream nombreLimitesA;
		stringstream nombreLimitesB;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesA(env);
			IloExpr restriccionLimitesB(env);

			restriccionLimitesA += y[i-1];
			//restriccionLimitesA -= listadoServiciosBeasley[i].duracion;
			
			restriccionLimitesB += y[i-1];
			//restriccionLimitesB += ht;
			
			//Adicionar restricciones a los contenedores
			nombreLimitesA << "LimiteA_y_{"<<i<<"}";			
			LimitesAlternativosA.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesA, IloInfinity, nombreLimitesA.str().c_str()));
			nombreLimitesA.str(""); // Limpiar variable para etiquetas de las restricciones			

			nombreLimitesB << "LimiteB_y_{"<<i<<"}";
			LimitesAlternativosB.add(IloRange(env, -IloInfinity, restriccionLimitesB, ht, nombreLimitesB.str().c_str()));
			nombreLimitesB.str(""); // Limpiar variable para etiquetas de las restricciones			

		}
		//Adicionar conjuntos al modelo 
		modelo.add(LimitesAlternativosA);
		modelo.add(LimitesAlternativosB);

		//Restricciones (13) Modelo Borcinova 17 - Límites de la variable y_i para continuidad de cadena o turno
		IloRangeArray LimitesVariableContinuidad(env);
		stringstream nombreLimitesVariableContinuidad;
		for(size_t i=1;i<num+1;++i){
			IloExpr restriccionLimitesVariableContinuidad(env);
			//restriccionLimitesVariableContinuidad += y[i-1] - listadoServiciosBeasley[i].duracion - ht;		
			restriccionLimitesVariableContinuidad += y[i-1];		
			//Adicionar restricción al conjunto (13) del modelo
			nombreLimitesVariableContinuidad << "LimiteVariableContinuidad_y_{"<<i<<"}";
			//LimitesVariableContinuidad.add(IloRange(env, -IloInfinity, restriccionLimitesVariableContinuidad, 0, nombreLimitesVariableContinuidad.str().c_str()));
			LimitesVariableContinuidad.add(IloRange(env, listadoServiciosBeasley[i].duracion, restriccionLimitesVariableContinuidad, ht, nombreLimitesVariableContinuidad.str().c_str()));
			nombreLimitesVariableContinuidad.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		//Adicionar conjunto al modelo 
		//modelo.add(LimitesVariableContinuidad);				

		

		/////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		//IloExpr obj(env);

		/*
		//Costo de todos los arcos seleccionados		
		for(size_t i=0;i<num+2;++i){
			for(size_t j=0;j<num+2;++j){
				//if(i!=j){
				//if(j>i){
					obj += matrizCostosBeasley[i][j] * x[i][j];
				//}				
			}
		}
		*/

		//Minimizar K únicamente
		//obj += K;		
		
		//Especificar si es un problema de maximización o de minimización
		//modelo.add(IloMaximize(env,obj)); 
		//modelo.add(IloMinimize(env,obj)); 
		//obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************		

		/////////////////////////////////////////Inicialización

		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloBeasley_Factibilidad_K.lp");

		////Ejemplo CPLEX-IBM inicialización del modelo
		//IloNumVarArray startVar(env);
		//IloNumArray startVal(env);
		//for (int i = 0; i < m; ++i)
		//	for (int j = 0; j < n; ++j) {
		//		startVar.add(x[i][j]);
		//		startVal.add(start[i][j]);
		//	}
		//cplex.addMIPStart(startVar, startVal);
		//startVal.end();
		//startVar.end();

		//Variable intermedia de x_ij para almacenar la solución generada por el constructivo
		IloArray<IloBoolArray> xSol(env, num+2);		
		for(auto i = 0u; i < num+2; ++i)
			xSol[i] = IloBoolArray(env, num+2);

		//Asignar cero a todos los valores de la solución
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){
				xSol[i][j] = 0;
			}
		}				

		//Cálculo de la función objetivo para diagnóstico
		int foInicializacion = 0;
		int foInicializacionSinDepositos = 0;
		
		//Activar los arcos seleccionados sin conexiones a depósitos
		for(size_t i=0;i<this->arcosBeasleyModificado.size();++i){

			//Salida de diagnóstico arcos
			//cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

			//Agregar el arco i del contenedor, teniendo P0
			//xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;	

			//Acumular el costo
			//foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			////Agregar sólo salidas e internos
			if(arcosBeasleyModificado[i][1] != num+1){
				
				xSol[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ] = 1;

				//Salida de diagnóstico arcos
				cout<<endl<<arcosBeasleyModificado[i][0]<<" - "<<arcosBeasleyModificado[i][1];

				//Cálculo de diagnóstico de la función objetivo
				foInicializacion += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];

			}
			
			//Cálculo de la función sin tener en cuenta los depósitos
			if(	arcosBeasleyModificado[i][0] != 0 		&& 
				arcosBeasleyModificado[i][0] != num+1 	&&
				arcosBeasleyModificado[i][1] != 0 		&&
				arcosBeasleyModificado[i][1] != num+1		){
				
				foInicializacionSinDepositos += this->matrizCostosBeasley[ arcosBeasleyModificado[i][0] ][ arcosBeasleyModificado[i][1] ];		

			}			

		}

		//Salida de diagnóstico
		cout<<endl<<"FO Punto Inicial Con Depósitos ->"<<foInicializacion;
		cout<<endl<<"FO Punto Inicial Sin Depósitos ->"<<foInicializacionSinDepositos;
		cout<<endl;
		//getchar();//Pausar ejecución


		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0
		////////////////////////////////////////Inicialización P0

		

		//Contenedores de inicialización
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(size_t i = 0; i < num+2; ++i){		
			for(size_t j = 0; j < num+2; ++j){				
				//if(i!=j){															
				//if(j>i){															
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				//}			
			}		
		}
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal);						

		////Liberar memoria
		//for(auto i = 0u; i < num+2; ++i){					
		//	xSol[i].end(); 
		//}
		//xSol.end();

		
		
		//Variable intermedia para el control de duración de turno
		IloIntArray ySol(env, num);

		//Cargar los tiempos del constructivo
		for(size_t i=0;i<this->y_i_Constructivo.size();++i){
			for(size_t j=0;j<this->y_i_Constructivo[i].size();++j){
				ySol[ this->turnosConstructivo[i][j] - 1 ] = this->y_i_Constructivo[i][j];
			}
		}		


		

		//Llevar tiempos a los contenedores de inicialización
		for(size_t i = 0; i < num; ++i){
			startVar.add(y[i]);
			startVal.add(ySol[i]);	
		}

		/*
		
		//Liberar memoria		
		ySol.end();	


		*/	

		//Limpiar el contenedor de los arcos solución para almacenar respuesta del modelo
		this->arcosBeasleyModificado.clear();

		/////////////////////////////////////////////////		
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos		
		//int timeLimit = 7200; //Dos horas
		int timeLimit = 3600; //Una hora
		//int timeLimit = 60; //Tiempo de prueba
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);

		//Enfatizar en factibilidad
		cplex.setParam(IloCplex::Param::Emphasis::MIP, CPX_MIPEMPHASIS_FEASIBILITY);

		//Establecer variantes de solución
		int optimization = 3;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		cplex.setParam(IloCplex::IntParam::RootAlg, optimization);	
		
		//Cargar solución inicial
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartCheckFeas, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveFixed, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartSolveMIP, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartRepair, "MIPStart");
		//cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartNoCheck, "MIPStart");
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Tomar tiempo inicial del solve
		gettimeofday(&t_ini, NULL);		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){

			//Reportar factibilidad para el K recibido
			return true;

			//Tomar tiempo final del solve
			gettimeofday(&t_fin, NULL);		
			//Tomar tiempo del solve
			this->tiempoComputoModelo = timeval_diff(&t_fin, &t_ini);					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Beasley96-Borcinova17: "<<cplex.getObjValue()<<endl;
			cout<<endl<<"-----> Tiempo cómputo modelo: "<<this->tiempoComputoModelo<<endl;
			
			

			//Salidas del depósito (número de turnos)
			cout<<endl<<"Salidas del depósito: "<<endl;
			int contadorTurnosGenerados = 0;			
			for(size_t j=1;j<num+1;++j){
				if(cplex.getValue(x[0][j])>0.95){
					++contadorTurnosGenerados;
					//Salida de diagnóstico
					cout<<"x[0]"<<"["<<j<<"] = "<<cplex.getValue(x[0][j])<<endl;
				}
			}
			cout<<endl<<"--->Número de turnos = "<< contadorTurnosGenerados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			/*

			//Llegadas al depósito (número de turnos)
			cout<<endl<<"Llegadas del depósito: "<<endl;
			int contadorRegresos = 0;
			vector<int> nodosRegreso;//Identificar los nodos terminales de los turnos o cadenas			
			for(size_t i=1;i<num+1;++i){
				//Si es un nodo terminal
				if(cplex.getValue(x[i][num+1])>0.95){
					++contadorRegresos;
					//Salida de diagnóstico
					cout<<"x["<<i<<"]"<<"[N+1] = "<<cplex.getValue(x[i][num+1])<<endl;
					//Almacenar los nodos terminales de los turnos
					nodosRegreso.push_back(i);
				}
			}
			cout<<endl<<"--->*Número de regresos = "<< contadorRegresos <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;
			*/

			/*

			//Todas las aristas seleccionadas
			int contadorArcosSeleccionados=0;
			for(size_t i=0;i<num+1;++i){
				for(size_t j=0;j<num+1;++j){
					//if(i!=j){
					if(j>i){
						if(cplex.getValue(x[i][j])>0.95){	
													
							//Informe en pantalla	
							++contadorArcosSeleccionados;
							//Salida de diagnóstico									
							//cout<<"x["<<i<<"]"<<"["<<j<<"] = "<<cplex.getValue(x[i][j])<<endl;
				
							//Recolectar los arcos seleccionados en un contenedor de la clase
							vector <int> arcoSeleccionado(3);//0 - i, 1 - j, 2 - selector inicializado en 0
							arcoSeleccionado[0] = i;
							arcoSeleccionado[1] = j;
							arcosBeasleyModificado.push_back(arcoSeleccionado);
				
						}

					}			
					
				}
			}
			cout<<endl<<"--->Número de arcos totales = "<< contadorArcosSeleccionados <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

			

			//Quitar penalizaciones de entrada y salida de la función objetivo y reportar
			double valorSalidas = contadorTurnosGenerados * ( mayorCostoReactivo * 10);
			double valorRegresos = 0;//Cuando se minimiza K
			//double valorRegresos = contadorRegresos * ( mayorCostoReactivo * 10);//Cuando se minimizan costos
			double foBeasley =  double(cplex.getObjValue()) - valorSalidas - valorRegresos;
			foBeasley = cplex.getObjValue();

			////Salida de diagnóstico
			//cout<<endl<<"valorSalidas="<<valorSalidas<<" valorRegresos="<<valorRegresos;
			//cout<<endl<<"contadorTurnosGenerados="<<contadorTurnosGenerados<<" contadorRegresos="<<contadorRegresos;
			//cout<<endl<<"cplexFO="<<double(cplex.getObjValue());			

			cout<<endl<<"--->FO Beasley = "<< foBeasley <<endl;
			cout<<"-------------------------------------"<<endl;
			cout<<"-------------------------------------"<<endl;

						
			
			//Archivo de salida para corridas en bloque
			//cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
			ofstream archivoSalidaSoluciones("./soluciones/salida.log",ios::app | ios::binary);		
			archivoSalidaSoluciones<<setprecision(10);//Cifras significativas para el reporte			
			if (!archivoSalidaSoluciones.is_open()){
				cout << "Fallo en la apertura del archivo salida.log - ModeloBeasleyModificado.";				
			}else{
				//Diferenciar el lote
				archivoSalidaSoluciones<<endl<< "Lote " <<  __DATE__ << " " << __TIME__ << " | ";				
				//Caso
				archivoSalidaSoluciones<<this->nombreArchivo.substr(21,this->nombreArchivo.length())<<" ";
				archivoSalidaSoluciones<<this->numeroConductoresConstructivo<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoConstructivo<<" ";
				archivoSalidaSoluciones<<this->funcionObjetivoConstructivo<<" ";
				archivoSalidaSoluciones<<"| ";
				archivoSalidaSoluciones<<contadorTurnosGenerados<<" ";
				archivoSalidaSoluciones<<this->tiempoComputoModelo<<" ";
				archivoSalidaSoluciones<<cplex.getObjValue()<<" ";				
				//archivoSalidaSoluciones<<foBeasley<<" ";				
				archivoSalidaSoluciones<<cplex.getMIPRelativeGap()<<" ";				
			}
			//Cerrar el archivo
			archivoSalidaSoluciones.close();

			*/
			

					
			
		}else{

			//Reportar infactibilidad para el K recibido
			return false;
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"Beasley Exploración de K INFACTIBLE! (Revisar restricciones)"<<endl;
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo esclavo

	//Búsqueda desde Upper Bound hasta el K_min
	void busquedaK_min(){

		//Calcular punto inicial
		//puntoInicialModeloBeasleyModificado();
		puntoInicialBarridoModeloBeasleyModificado();
		//generarGrafoDotConstructivo();
		//getchar();//Pausar ejecución para revisar salidas en pantalla	

		//Búsqueda por upper bound BPP
		////////////////////////////////
		
		/*

		//Mostrar valor inicial
		cout<<endl<<"--------- K_0 = "<<this->numeroConductoresConstructivo;

		//Forzar inicio de K
		//this->numeroConductoresConstructivo = 66;

		//Decrementar hasta que haya infactibilidad
		while(modeloBPP_CSP_Factibilidad(this->numeroConductoresConstructivo)){

			//Mostrar el K factible
			cout<<endl<<"K = "<<this->numeroConductoresConstructivo<<endl;

			//Decrementar el K que fue factible
			--this->numeroConductoresConstructivo;

		}

		//Mostrar valor final
		cout<<endl<<endl<<"-----------------------------------------";		
		cout<<endl<<this->nombreArchivo;
		cout<<endl<<"-----------------------------------------";
		++this->numeroConductoresConstructivo;
		cout<<endl<<"--------- K_min_BPP = "<<this->numeroConductoresConstructivo<<endl;
		getchar();//Pausar salida para diagnóstico	
		*/

		//Búsqueda por upper bound CSP
		//////////////////////////////
		/*		

		//Mostrar valor inicial
		cout<<endl<<"--------- K_0 = "<<this->numeroConductoresConstructivo;

		//Forzar inicio de K
		this->numeroConductoresConstructivo = 66;

		//Decrementar hasta que haya infactibilidad
		while(modeloExploracionK(this->numeroConductoresConstructivo)){

			//Mostrar el K factible
			cout<<endl<<"K = "<<this->numeroConductoresConstructivo<<endl;

			//Decrementar el K que fue factible
			--this->numeroConductoresConstructivo;

		}

		//Mostrar valor final
		cout<<endl<<endl<<"-----------------------------------------";		
		cout<<endl<<this->nombreArchivo;
		cout<<endl<<"-----------------------------------------";
		++this->numeroConductoresConstructivo;
		cout<<endl<<"--------- K_min = "<<this->numeroConductoresConstructivo<<endl;
		getchar();//Pausar salida para diagnóstico
		
		*/
		

		//Búsqueda por lower bound
		//////////////////////////
		

		//Mostrar valor inicial
		this->numeroConductoresConstructivo = this->modeloBeasleyModificadoRelajado();
		cout<<endl<<"--------- K_0 = "<<this->numeroConductoresConstructivo;

		//Decrementar hasta que haya infactibilidad
		while(!modeloExploracionK(this->numeroConductoresConstructivo)){

			//Mostrar el K infactible
			cout<<endl<<"K = "<<this->numeroConductoresConstructivo<<endl;

			//Incrementar el K que fue ifactible
			++this->numeroConductoresConstructivo;

		}	

		//Mostrar valor final
		cout<<endl<<endl<<"-----------------------------------------";		
		cout<<endl<<this->nombreArchivo;
		cout<<endl<<"-----------------------------------------";
		//--this->numeroConductoresConstructivo;
		cout<<endl<<"--------- K_min = "<<this->numeroConductoresConstructivo<<endl;
		getchar();//Pausar salida para diagnóstico		


	
	}//Fin de la búsqueda del K_min
	


	
	//Modelo esclavo Integra-UniAndes	
	void modeloEsclavo(){
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		
		//// variable que asigna eventos a un posible bloque de cada tabla
		char name_s[200];//Vector para nombrar el conjunto de variables de decisión'Y'
		BoolVar3Matrix Y(env, CantidadTablas);//Dimensión t = tablas   hay que poner la misma dimension que en el limite de el indice  
		for(unsigned int t=0;t< CantidadTablas;++t){
			Y[t]=BoolVarMatrix (env,TablasInstancia[t].EventosTabla);//Dimensión e = eventos de la tabla t
			for(unsigned int e=0;e<TablasInstancia[t].EventosTabla;++e){
				Y[t][e]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
				for(unsigned int b=0;b<PosiblesBloques;++b){					
					sprintf(name_s,"Y_%u_%u_%u",t,e,b);
					//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					Y[t][e][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
			}                
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////// Punto de corte de cada  bloque
		IloArray<IloNumVarArray> X(env, CantidadTablas);
		std::stringstream name;
		for(auto t=0;t<CantidadTablas;++t) {
			X[t] = IloNumVarArray(env, PosiblesBloques);
			for(auto b=0;b<PosiblesBloques;++b) {
				name << "X_" << t << "_" << b;
				X[t][b] = IloNumVar(env, name.str().c_str());			
				name.str(""); // Clean name
			}
		}	
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////// variable que describe si el bloque es no fantasma
		BoolVarMatrix nf (env,CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<CantidadTablas;++t){
			nf[t]=IloBoolVarArray(env,PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<PosiblesBloques;++b){					
				sprintf(name_s,"nf_%u_%u",t,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					nf[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}  
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//(R2)
		IloRangeArray NaturalezaX(env);
		stringstream nombreNaturalezaX;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			
			for(auto b = 0u; b < PosiblesBloques; ++b) { // la dimension de b = posibles bloques
				IloExpr restriccionNaturalezaX(env);
				restriccionNaturalezaX += X[t][b];				
				
				nombreNaturalezaX << "NaturalezaX_{"<< t<<","<<b<<"}";
				NaturalezaX.add (IloRange(env, 0, restriccionNaturalezaX, IloInfinity, nombreNaturalezaX.str().c_str()));
				nombreNaturalezaX.str(""); // Limpiar variable para etiquetas de las restricciones
				
			}			
			
		}	
		//Adicionar restricciones 
		modelo.add(NaturalezaX); 	
		
		
		//Conjunto de restricciones para cada evento este contenido solo en un bloque (R4)
		IloRangeArray ContenidoBolque(env);
		stringstream nombreContenidoBloque;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){//Una restricción por cada evento de cada tabla
				IloExpr restriccionContenidoBloque(env);
				for(auto b = 0u; b < PosiblesBloques; ++b) { // la dimension de b = posibles bloques
					restriccionContenidoBloque += Y[t][e][b];//Un término en la restricción por cada uno de los posibles bloques
				}
				nombreContenidoBloque << "AsignarBloque_{"<< t<<","<<e<<"}";
				ContenidoBolque.add (IloRange(env, 1, restriccionContenidoBloque, 1, nombreContenidoBloque.str().c_str()));
				nombreContenidoBloque.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		//Adicionar restricciones 
		modelo.add(ContenidoBolque); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		//Conjunto de restricciones para que los cortes de los bloques sean continuos (R5)
		IloRangeArray CorteContinuo(env);
		stringstream nombreCorteContinuo;
		for(auto t = 0u; t < CantidadTablas; ++t){ //Una restricción por tabla
			for(auto b = 0u; b < PosiblesBloques-1; ++b){
			IloExpr restriccionCorteContinuo(env);
			IloExpr cortePosterior(env);	
			
			restriccionCorteContinuo=X[t][b];
			cortePosterior=X[t][b+1];
			restriccionCorteContinuo-=cortePosterior;
			
			nombreCorteContinuo << "Corte_B{"<< t<<"}";
			CorteContinuo.add (IloRange(env, -IloInfinity, restriccionCorteContinuo, -1, nombreCorteContinuo.str().c_str()));
			nombreCorteContinuo.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}
		//Adicionar restricciones 
		modelo.add(CorteContinuo); 
		

		//Conjunto de restricciones para acotar ultimo corte 		(R6)
		IloRangeArray UltimoCorte(env);
		stringstream nombreUltimoCorte;
		for(auto t = 0u; t < CantidadTablas; ++t){ //Una restricción por tabla
			
			IloExpr restriccionUltimoCorte(env);	
			
			restriccionUltimoCorte=X[t][3];
			
			nombreUltimoCorte << "Corte_B{"<< 3<<"}";
			UltimoCorte.add (IloRange(env, -IloInfinity, restriccionUltimoCorte, TablasInstancia[t].EventosTabla+3, nombreUltimoCorte.str().c_str()));
			nombreUltimoCorte.str(""); // Limpiar variable para etiquetas de las restricciones
			
		}
		//Adicionar restricciones 
		modelo.add(UltimoCorte); 
		
                            
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////
		// Restricciones de relacion de variables X y Y	(R7)
		
		IloRangeArray RelacionVar(env);
		stringstream nombreRelacionVar;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques; ++b){
				IloExpr valorX(env);
				valorX=X[t][b];
					for(auto e=0u; e < TablasInstancia[t].EventosTabla; ++e){
						
						IloExpr restriccionRelacionVar(env);
						//restriccionRelacionVar=Y[t][e][b]*int(e+1);
						restriccionRelacionVar=Y[t][e][b]*TablasInstancia[t].eventos[e].evento;
						restriccionRelacionVar-=valorX;
						
						nombreRelacionVar << "RelacionVariables_{"<< t<<", "<< b<<", "<< e<<"}";
						RelacionVar.add (IloRange(env, -IloInfinity, restriccionRelacionVar	, 0 , nombreRelacionVar.str().c_str()));
						nombreRelacionVar.str(""); // Limpiar variable para etiquetas de las restricciones
			
					}
				
				}
		}
		//Adicionar restricciones 
		modelo.add(RelacionVar); 
		
		// Restricciones de relacion opuesta variables X y Y	(R8)
		
		IloRangeArray RelacionVarO(env);
		stringstream nombreRelacionVarO;
		for(auto t = 0u; t < CantidadTablas; ++t){ 
			for(auto b = 0u; b < PosiblesBloques-1; ++b){
				//IloExpr valorXO(env);
				//valorXO=X[t][b];
					for(auto e=0u; e < TablasInstancia[t].EventosTabla; ++e){
						
						IloExpr restriccionRelacionVarO(env);
						IloExpr granMM(env);	//Expresion para relajar la restriccion con un gran MM manual
						
						//restriccionRelacionVarO=Y[t][e][b+1]*int(e+1);
						restriccionRelacionVarO=Y[t][e][b+1]*TablasInstancia[t].eventos[e].evento;
						//granMM= (1-Y[t][e][b+1])*IloInfinity;
						granMM= (1-Y[t][e][b+1])*900;
						//restriccionRelacionVarO= restriccionRelacionVarO - valorXO + granMM;
						restriccionRelacionVarO= restriccionRelacionVarO - X[t][b] + granMM;
						
						nombreRelacionVarO << "RelacionVariables_{"<< t<<", "<< b<<", "<< e<<"}";
						RelacionVarO.add (IloRange(env, 1, restriccionRelacionVarO	, IloInfinity , nombreRelacionVarO.str().c_str()));
						nombreRelacionVarO.str(""); // Limpiar variable para etiquetas de las restricciones
					}
				}
		}
		//Adicionar restricciones 
		modelo.add(RelacionVarO);
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////
		//Restriccion de  activacion de variable no fantasma (R9)
		
		IloRangeArray ActivarNF(env);
		IloRangeArray ActivarNFO(env);
		stringstream nombreActivarNF;
		stringstream nombreActivarNFO;
		for(auto t = 0; t < CantidadTablas; ++t){
			for(auto b = 0u; b < PosiblesBloques; ++b){ 
				IloExpr restriccionActivarNF(env);
				IloExpr restriccionActivarNFO(env);
				//Una restricción por cada bloque de cada tabla
				
				
				for( auto e = 0u ; e < TablasInstancia[t].EventosTabla; e++){
					restriccionActivarNF+= Y[t][e][b]*TablasInstancia[t].eventos[e].duracion; 
					restriccionActivarNFO+= Y[t][e][b]*TablasInstancia[t].eventos[e].duracion; 
				}
				IloExpr auxActivacion(env);
				//auxActivacion=nf[t][b]*IloInfinity;							
				auxActivacion=nf[t][b]*900;							
				
				restriccionActivarNF-=auxActivacion;
				
				IloExpr auxActivacionO(env);
				auxActivacionO=nf[t][b];
				restriccionActivarNFO-=auxActivacionO;				
				
				//restriccionDiasTrabajados   
				nombreActivarNF << "ActivacionNF_{"<< t<<", " << b << "}";
				nombreActivarNFO << "ActivacionNFO_{"<< t<<", " << b << "}";
				ActivarNF.add (IloRange(env,-IloInfinity, restriccionActivarNF, 0 , nombreActivarNF.str().c_str()));
				ActivarNFO.add (IloRange(env,0, restriccionActivarNFO, IloInfinity, nombreActivarNFO.str().c_str()));
				//ActivarNF.str(""); // Limpiar variable para etiquetas de las restricciones
				nombreActivarNF.str(""); // Limpiar variable para etiquetas de las restricciones
				nombreActivarNFO.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(ActivarNF);
		modelo.add(ActivarNFO);
		
		 
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////
		//Restriccion de eventos de balance	(R10)
		
		
		IloRangeArray BalanceBloques(env);
		stringstream nombreBalanceBloques;
		for(auto t = 0; t < CantidadTablas; ++t){
			for(auto b = 0u; b < PosiblesBloques-1; ++b){ 
				//Una restricción por cada bloque de cada tabla
				IloExpr RestriccionBalance(env);
				IloExpr NumBalance(env);
				
				RestriccionBalance=X[t][b+1]-X[t][b];
				NumBalance=balance * nf[t][b+1];
				RestriccionBalance-=NumBalance;
				   
				nombreBalanceBloques << "Balance de Bloque_{"<< t <<", " << b<<"}";
				BalanceBloques.add (IloRange(env,0, RestriccionBalance, IloInfinity , nombreBalanceBloques.str().c_str()));
				nombreBalanceBloques.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(BalanceBloques);
		 
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////
		//Restricciones de maxima duracion 		(R11)
		IloRangeArray DuracionMaxima(env);
		stringstream nombreDuracionMaxima;
		for(auto t = 0; t < CantidadTablas; ++t){
			for(auto b = 0u; b < PosiblesBloques; ++b){ 
				//Una restricción por cada bloque generado
				IloExpr restriccionDuracion(env);
				for(auto e = 0u ; e < TablasInstancia[t].EventosTabla  ; e++){
					 restriccionDuracion+= Y[t][e][b] * TablasInstancia[t].eventos[e].duracion;
				}
				
				IloExpr Tope(env);
				//Tope = hb + extrab;
				
				//restriccion de Duracion de bloques  
				nombreDuracionMaxima << "Duracion Maxima_{"<< t<< ", " <<b <<"}";
				//DuracionMaxima.add (IloRange(env,-IloInfinity, restriccionDuracion, Tope , nombreDuracionMaxima.str().c_str()));
				DuracionMaxima.add (IloRange(env,-IloInfinity, restriccionDuracion, (hb + extrab) , nombreDuracionMaxima.str().c_str()));
				nombreDuracionMaxima.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(DuracionMaxima);
		
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////
		//Restriccion Ligadura de eventos	(R12)
		IloRangeArray LigaduraEventos(env);
		stringstream nombreLigaduraEventos;
		for(auto t = 0; t < CantidadTablas; ++t){
			for(auto b = 0u; b < PosiblesBloques; ++b){
				for(auto e = 1 ; e < TablasInstancia[t].EventosTabla; e++){
					//Una restricción por cada día de la semana por cada operario
					IloExpr restriccionLigadura(env);
					IloExpr Ligadura(env);
					
					//Ligadura=2*noI[t][e];
					//Ligadura=2*TablasInstancia[t].eventos[e].noI;
					restriccionLigadura= Y[t][e-1][b]+Y[t][e][b]-2*TablasInstancia[t].eventos[e].noI*Y[t][e-1][b];
					//restriccionLigadura-=Ligadura;
					
					//restriccion de eventos Ligados 
					nombreLigaduraEventos << "Ligadura Eventos_{"<< t<< ", "<< e<< ", " << b << "}";
					LigaduraEventos.add (IloRange(env,0, restriccionLigadura, IloInfinity , nombreLigaduraEventos.str().c_str()));
					nombreLigaduraEventos.str(""); // Limpiar variable para etiquetas de las restricciones
				}
			}	
		}
		//Adicionar restricciones 
		modelo.add(LigaduraEventos);
		

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		
		
		for (auto t = 0u; t < CantidadTablas; ++t) {
				for (auto b = 0u; b< PosiblesBloques; ++b){
					obj += X[t][b];
			}
		}		
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		//modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("modeloEsclavo.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){
					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<"__________________________________________________________________________________________________"<<endl;
			cout<<endl<<"-----> Valor Función Objetivo Esclavo: "<<cplex.getObjValue()<<endl;
			
			//Salida X
			for(int t=0;t<CantidadTablas;++t){		
				cout<<"- Tabla : "<<TablasInstancia[t].nombreTabla<<endl;
				cout<<"Cantidad Bloque 1= "<<cplex.getValue(X[t][0])<< " nf = "<< cplex.getValue(nf[t][0]) <<endl;
				for(int b=1;b<PosiblesBloques;++b){					
					cout<<"Cantidad Bloque "<<b+1<<" = " <<cplex.getValue(X[t][b])-cplex.getValue(X[t][b-1])<< " nf = "<< cplex.getValue(nf[t][b]) <<endl;
					//cout<<"Cantidad Bloque "<<b+1<<" = " <<cplex.getValue(X[t][b])<< " nf = "<< cplex.getValue(nf[t][b]) <<endl;
				}				
			}
			
			////Salida Y
			//cout<<endl;
			//for(int t=0;t<CantidadTablas;++t){								
			//	for(int e=0;e<TablasInstancia[t].EventosTabla;++e){					
			//		for(int b=0;b<PosiblesBloques;++b){						
			//			if(cplex.getValue(Y[t][e][b])>0.95){										
			//				//cout<<"Cantidad Bloque "<<b+1<<" = " <<cplex.getValue(X[t][b])-cplex.getValue(X[t][b-1])<<endl;
			//				cout<<"Y["<<t<<"]"<<"["<<e<<"]"<<"["<<b<<"]"<<cplex.getValue(Y[t][e][b])<<endl;
			//			}
			//		}
			//	}				
			//}
			
			
			////Salida de diagnóstico Y_sol
			//cout<<endl<<"Dimensiones de Y_sol -> "<<Y_sol.size()<<" "<<Y_sol[0].size()<<" "<<Y_sol[0][0].size();
			//cout<<endl<<CantidadTablas<<endl;
			//cout<<endl<<PosiblesBloques<<endl;			
			//getchar();
			
			//Guardar los bloques generados por el esclavo
			cout<<endl;
			for(int t=0;t<CantidadTablas;++t){								
				for(int b=0;b<PosiblesBloques;++b){
					float minimoAux;
					minimoAux=1440;	
					string inicioAux;
					float maximoAux;	
					maximoAux=0;	
					string finAux;	
						
					float duracionAux = 0;
					
					bloque bloqueAux;
													
					for(int e=0;e<TablasInstancia[t].EventosTabla;++e){										
						if(cplex.getValue(Y[t][e][b])>0.95 && TablasInstancia[t].eventos[e].t0<minimoAux){										
							minimoAux=TablasInstancia[t].eventos[e].t0;
							inicioAux=TablasInstancia[t].eventos[e].inicio;
						}															
						if(cplex.getValue(Y[t][e][b])>0.95 && TablasInstancia[t].eventos[e].tf>maximoAux){										
							maximoAux=TablasInstancia[t].eventos[e].tf;
							finAux=TablasInstancia[t].eventos[e].fin;
						}
						if(cplex.getValue(Y[t][e][b])>0.95){
							duracionAux+=TablasInstancia[t].eventos[e].duracion;
							Y_sol[t][e][b]=1;
						}
						
		
					
					}
					
					bloqueAux.t0b = minimoAux;
					bloqueAux.iniciob = inicioAux;
					bloqueAux.tfb = maximoAux;
					bloqueAux.finb = finAux;
					bloqueAux.duracionb = duracionAux;
					TablasInstancia[t].bloques.push_back(bloqueAux);
					
					if(duracionAux>=0){
						//Salida prueba
						cout<<endl<<"Agregando el bloque: "<< b << " de tabla "<< TablasInstancia[t].nombreTabla <<endl;
						cout<<"t0b: "<< bloqueAux.t0b<<endl; 
						cout<<"iniciob: "<< bloqueAux.iniciob<<endl;
						cout<<"tfb: "<< bloqueAux.tfb<<endl;
						cout<<"finb: "<< bloqueAux.finb<<endl;
						cout<<"duracionb: "<< bloqueAux.duracionb<<endl;
					}
									
				}				
			}
			
			
			////Archivo de salida para corridas en bloque
			//ofstream archivoSalidaEsclavo("esclavoSalidas.log",ios::app | ios::binary);		
			//archivoSalidaEsclavo<<setprecision(10);//Cifras significativas para el reporte
			//if (!archivoSalidaEsclavo.is_open()){
			//	cout << "Fallo en la apertura del archivo de salida MODELO ESCLAVO.";				
			//}
			//archivoSalidaEsclavo.close();		
			
		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo esclavo)";
			
		}
					
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}//Fin modelo esclavo
	
	
	//Generación de columnas CrewScheduling Problem Integra-UniAndes
	void generacionTurnosTrabajoDanielUniAndes(){
		
		//Variables locales del método para iniciar
		bool continuar = true;
		//int iteracion = num;
		
		//Iniciar el proceso con las particiones
		modeloEsclavo();
		
		// (NUEVO) inicializar la matriz A según las particiones
			//saber cuantos bloques quedaron
			int totalBloques=0;
			for(auto t=0 ; t < CantidadTablas; ++t){					
				for(auto b=0 ; b < PosiblesBloques; ++b){							
					if(TablasInstancia[t].bloques[b].duracionb>0){
						totalBloques+=1;
					}
				}
			}
		int iteracion= totalBloques;
			//redimensionar
			//vector< vector< vector< int > > > A;//Matriz Identidad de turnos iniciales
			A.resize(CantidadTablas);	
			for(auto t=0 ; t < CantidadTablas; ++t){
				A[t].resize(TablasInstancia[t].EventosTabla);						
				for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){							
					A[t][e].resize(totalBloques);
					
				}
			}
			int contadorA=0;
			//asignar a la matriz A bloque a bloque
			for(auto t=0; t < CantidadTablas; ++t){
				for(auto b=0; b < PosiblesBloques; ++b){
					if(TablasInstancia[t].bloques[b].duracionb>0){
						for(auto e=0; e < TablasInstancia[t].EventosTabla; ++e){
							if(Y_sol[t][e][b]==1){
								A[t][e][contadorA]=1;
							}
						}
						contadorA+=1;
					}
				}				
			}
			cout<<endl<<"     ---> fueron inicializadas "<< contadorA<<" columnas iniciales"<<endl;
			cout<<totalBloques<<endl;
			cout<<contadorA<<endl;
			
			for(auto i=0; i< totalBloques; ++i){
				cout<<"- Columna "<<i<<endl;
				for(auto t=0; t < CantidadTablas; ++t){
					for(auto e=0; e < TablasInstancia[t].EventosTabla; ++e){
						if(A[t][e][i]>0.5){
							cout<<"A_["<<t<<"]["<<e<<"]["<<i<<"] = "<<A[t][e][i]<<endl;
						}
					}
				}
			}

		
		
		
		
		
		//Realizar el proceso hasta que se active la bandera
		while(continuar){
			cout<<endl<<"     !!!!!!!!!"<<endl;
			cout<<"     Iteración "<<iteracion-totalBloques<<endl;			
			cout<<"     !!!!!!!!!"<<endl;
			cout<<endl<<"     FO Maestro Relajado = "<<modeloGCMaestroRelajado()<<endl;
			
			
			cout<<endl<<"     Pasa del modelo relajado!!!"<<endl;
			
			//Validación criterio de parada
			if(modeloGCAuxiliar()<=1.1){
				//Bandera de salida
				continuar = false;	
				cout<<endl<<"                    GENERACIÓN DE COLUMNAS TERMINADA"<<endl;
				cout<<"     ======================================================================="<<endl;
			}else{
				
				cout<<endl<<"     Se debe generar la columna "<<iteracion<<endl;
				
				//Insertar la columna en A
				for(auto t = 0u; t< CantidadTablas; ++t){					
					for(auto e = 0u; e < TablasInstancia[t].EventosTabla; ++e){						
						A[t][e].push_back(columnaAuxiliar[t][e]);						
					}
				}				
				
			}//Fin de la validación
			
			//Actualizar la iteración del método
			++iteracion;
			
			
		}//Fin del ciclo general
		
		//Resolver maestro sin relajar
		modeloGCMaestro();
		
		
		
		cout<<endl<<"FINAL!"<<endl;
		cout<<endl<<"FINAL!"<<endl;
		cout<<endl<<"FINAL!"<<endl;
		
		
		
		/// /// ///
		
		
		
		////Archivo de salida para corridas en bloque
		//ofstream archivoMatriz("matrizRevisar.csv",ios::app | ios::binary);		
		////archivoMatriz<<setprecision(10);//Cifras significativas para el reporte
		//if (!archivoMatriz.is_open()){
		//	cout << "Fallo en la apertura del archivo de salida MATRIZ A.";				
		//}else{
		//	
		//	int sumatoriaUnosA = 0;
		//	for(auto t=0 ; t < CantidadTablas; ++t){						
		//		for(auto e=0 ; e < TablasInstancia[t].EventosTabla; ++e){
		//			
		//			archivoMatriz<<TablasInstancia[t].nombreTabla<<";"<<e+1<<";";
		//			
		//			
		//			for(auto i=0 ; i < A[t][e].size(); ++i){
		//				
		//				
		//				archivoMatriz<<A[t][e][i]<<";";
		//				
		//				
		//			}
		//			
		//			archivoMatriz<<endl;
		//									
		//		}
		//		
		//	}
		//	
		//	
		//	
		//}
		//archivoMatriz.close();			
		////exit(1);
		////getchar();
		
		
		
		/// /// ///
		
		
		
		//Retroalimentación de salida
		//Retroalimentación de salida
		//Retroalimentación de salida
		//Retroalimentación de salida		
		
	}//Fin de la metodología
	
	
	//Mostrar bloques solución
	void mostrarBloquesEsclavo(){
				
		//Salida en pantalla
		cout<<endl;
		for(int t=0;t<CantidadTablas;++t){
			for(int b=0;b<TablasInstancia[t].bloques.size();++b){				
				cout<<"El Bloque de la Tabla "<<t<<" dura "<<TablasInstancia[t].bloques[b].duracionb<<" minutos"<<endl;								
			}
			cout<<endl;		
		}	
		
	}
	
	
};



struct casoRostering {
	
	//Atributos para los diferentes modelos
	string nombreArchivo;
	int numeroTurnosMaximo;
	int numeroConductores;
	vector<dia> semana; //Recordar que este es un periodo de rotación, pueden ser menos días que una semana o ciclos mayores como mes o año
	int longitudProgramacion;
	vector< vector<int> > matrizRequerimientoPersonal;
	vector< tipoTurnoMusliu > descripcionTurnosMusliu;
	dia diaHabil;//Atributo para descripción de turnos día hábil integra
	int tiposTurno;
	int numeroMinimoDiasLibre;
	int numeroMaximoDiasLibre;
	int numeroMinimoDiasTrabajo;
	int numeroMaximoDiasTrabajo;
	int longitudProhibicionTamanio2;
	int longitudProhibicionTamanio3;
	vector<string> secuenciasProhibidasTamanio2;
	vector<string> secuenciasProhibidasTamanio3;
	
	//Caso integra donde los tipos de turno y el requerimiento difieren
	int numeroTurnosRequeridos;
	
	//Posibles constructores de los días
	casoRostering() : numeroTurnosMaximo(0), numeroConductores(0) {}
	casoRostering(int a, int b) : numeroTurnosMaximo(a), numeroConductores(b) {}
	
	
	//Constructor: Seleccionar el tipo de variante de programación de rutas para proceder a realizar la carga
	casoRostering(string testname, int idVarianteRostering){
		
		//Identificar el tipo de carga especificado por el parámetro de entrada 
		switch (idVarianteRostering){
			
			//Variante Datos Bohórquez
			case 0:
			{ 
					///////////////////////
					//// Lectura de datos//
					///////////////////////
					//
					////Abrir el archivo con la matriz de costos e información de los depósitos
					//ifstream data_input((testname).c_str());				
					//if (! data_input.is_open() ) {
					//	cout << "Can not read the rostering file " << (testname).c_str() << endl;
					//	//return EXIT_FAILURE;
					//	//return 1;
					//}
					//
					////Nombre de la instancia
					//nombreArchivo = testname;
					//cout<<endl<<"Nombre del archivo -> "<<testname;//getchar();
					//
					////Número máximo de turnos
					//data_input >> numeroTurnosMaximo;
					//if (numeroTurnosMaximo <= 0) cout << "Número turnos máximo <= 0" << endl;
					////Salida en pantalla
					//cout<<endl<<"Número de turnos máximo: "<< numeroTurnosMaximo;
					//
					////Número conductores
					//data_input >> numeroConductores;
					//if (numeroConductores <= 0) cout << "Número conductores <= 0" << endl;
					////Salida en pantalla
					//cout<<endl<<"Número de conductores: "<< numeroConductores;
					//
					////Carga de turnos de 'n' número de días
					//int n = 7;//Una semana 
					//for(int i=0;i<n;++i){
					//	
					//	//Contenedor auxiliar de cada día
					//	dia diaAux;
					//	
					//	//Número de turnos por día
					//	data_input >> diaAux.numeroTurnos;
					//	
					//	//Carga de turnos
					//	for(int j=0;j<diaAux.numeroTurnos;++j){
					//		
					//		//Turno actual
					//		turno turnoAux;
					//		data_input >> turnoAux.id;
					//		data_input >> turnoAux.hora_entrada;
					//		data_input >> turnoAux.hora_salida;
					//		data_input >> turnoAux.duracion_turno;
					//		
					//		//Cargar el 'j'-ésimo turno al día actual
					//		diaAux.turnos.push_back(turnoAux);
					//	}
					//	
					//	//Una vez cargados los turnos del día, cargar a la programación semanal de turnos
					//	semana.push_back(diaAux);
					//	
					//	
					//}//Fin de la carga de turnos en 'n' días
					//
					//
					////Cerrar el archivo que contiene la matriz de costos
					//data_input.close();
					//
					////Salida en pantalla de los 'n' días cargados
					//cout<<endl;
					//for(int i=0;i<semana.size();++i){
					//	
					//	cout<<semana[i].numeroTurnos<<endl;
					//	
					//	for(int j=0;j<semana[i].numeroTurnos;++j){
					//		
					//		cout<<semana[i].turnos[j].id<<" "<<semana[i].turnos[j].hora_entrada<<" "<<semana[i].turnos[j].hora_salida<<" "<<semana[i].turnos[j].duracion_turno<<endl;
					//		
					//	}
					//	
					//}
					//cout<<endl;
					
			}break;//Fin de carga de datos Modelo Bohórquez
			
			
			//Variante Datos Musliu
			case 1:
			{ 
			
					/////////////////////
					// Lectura de datos//
					/////////////////////
					
					//Abrir el archivo con la matriz de costos e información de los depósitos
					ifstream data_input((testname).c_str());				
					if (! data_input.is_open() ) {
						cout << "Can not read the rostering file " << (testname).c_str() << endl;
						//return EXIT_FAILURE;
						//return 1;
					}
					
					//Nombre de la instancia
					nombreArchivo = testname;
					cout<<endl<<"Nombre del archivo -> "<<testname;//getchar();
					
					
					//Omitir enunciado de la longitud de la programación
					string foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					//Longitud de programación
					data_input >> longitudProgramacion;
					cout<<endl<<"Longitud de Programación : "<<longitudProgramacion;//getchar();
					
					
					//Omitir enunciado del número de conductores
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					//Número conductores
					data_input >> numeroConductores;
					if (numeroConductores <= 0) cout << "Número conductores <= 0" << endl;
					//Salida en pantalla
					cout<<endl<<"Número de conductores: "<< numeroConductores;
					
					//Omitir enunciado del número de turnos
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					//Número tipos de turno
					data_input >> tiposTurno;
					if (tiposTurno <= 0) cout << "Número tipos de turno <= 0" << endl;
					//Salida en pantalla
					cout<<endl<<"Número tipos de turno: "<< tiposTurno;
					
					
					//Omitir enunciado de la matriz de requerimiento de personal
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					for(int i=0;i<tiposTurno;++i){
						vector<int> filaAuxiliar(longitudProgramacion);
						for(int j=0;j<longitudProgramacion;++j){
							data_input >> filaAuxiliar[j];
						}
						matrizRequerimientoPersonal.push_back(filaAuxiliar);
					}
					//Salida en pantalla
					cout<<endl<<"Matriz de requerimiento de personal: "<<endl;
					for(auto& fila_i : matrizRequerimientoPersonal){
						for(auto& posicion_i_j : fila_i){
							cout<<posicion_i_j<<" ";
						}
						cout<<endl;	
					}
					//getchar();
					
		
					//Omitir enunciado de la especificación de turnos
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					descripcionTurnosMusliu.resize(tiposTurno);
					for(int i=0;i<tiposTurno;++i){
						
						tipoTurnoMusliu turnoAuxiliar;
						data_input >> turnoAuxiliar.ShiftName;
						data_input >> turnoAuxiliar.Start;
						data_input >> turnoAuxiliar.Length;
						data_input >> turnoAuxiliar.MinlengthOfBlocks;
						data_input >> turnoAuxiliar.MaxLengthOfBlocks;
						
						descripcionTurnosMusliu[i] = turnoAuxiliar;	
					}
					
					
					
					//Omitir enunciado de días libre
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> numeroMinimoDiasLibre;
					data_input >> numeroMaximoDiasLibre;
					//Salida en pantalla
					cout<<endl<<"Número días libre -> "<< numeroMinimoDiasLibre <<" "<<numeroMaximoDiasLibre<<endl;
					
					
					//Adicionar turno de descanso
					tipoTurnoMusliu turnoDescanso;
					turnoDescanso.ShiftName = "B";
					turnoDescanso.Start = 0;
					turnoDescanso.Length = 1440;
					turnoDescanso.MinlengthOfBlocks = numeroMinimoDiasLibre;
					turnoDescanso.MaxLengthOfBlocks = numeroMaximoDiasLibre ;
					descripcionTurnosMusliu.push_back(turnoDescanso);	
					++tiposTurno;//Aumentar el número de turnos después de adicionar el turno de descanso
					
					//Salida en pantalla
					cout<<endl<<"Descripción de tipos de turno: "<<endl;
					for(auto& turno_i : descripcionTurnosMusliu){
						cout<<turno_i.ShiftName;
						cout<<" "<<turno_i.Start;
						cout<<" "<<turno_i.Length;
						cout<<" "<<turno_i.MinlengthOfBlocks;
						cout<<" "<<turno_i.MaxLengthOfBlocks;
						cout<<endl;	
					}
					//getchar();
					
					//Omitir enunciado de días de trabajo
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> numeroMinimoDiasTrabajo;
					data_input >> numeroMaximoDiasTrabajo;
					//Salida en pantalla
					cout<<endl<<"Número días trabajo -> "<< numeroMinimoDiasTrabajo <<" "<<numeroMaximoDiasTrabajo<<endl;
					
					//Omitir enunciado de prohibiciones
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> foo;
					data_input >> longitudProhibicionTamanio2;
					data_input >> longitudProhibicionTamanio3;
					//Salida en pantalla
					cout<<endl<<"Prohibición Tamaño 2 y 3 -> "<< longitudProhibicionTamanio2 <<" "<<longitudProhibicionTamanio3<<endl;
					
					////Omitir enunciado
					//data_input >> foo;
					//data_input >> foo;
					//data_input >> foo;
					//data_input >> foo;
					//data_input >> foo;
					//for(int i=0;i<longitudProhibicionTamanio2;++i){
					//	secuenciasProhibidasTamanio2
					//}
					//
					//
					//
					//
					//vector<string> secuenciasProhibidasTamanio2;
					//vector<string> secuenciasProhibidasTamanio3;
					
					
					
					
					
				
					
					
					
					
					
					
					////Número conductores
					//data_input >> numeroConductores;
					//if (numeroConductores <= 0) cout << "Número conductores <= 0" << endl;
					////Salida en pantalla
					//cout<<endl<<"Número de conductores: "<< numeroConductores;
					//
					////Carga de turnos de 'n' número de días
					//int n = 7;//Una semana 
					//for(int i=0;i<n;++i){
					//	
					//	//Contenedor auxiliar de cada día
					//	dia diaAux;
					//	
					//	//Número de turnos por día
					//	data_input >> diaAux.numeroTurnos;
					//	
					//	//Carga de turnos
					//	for(int j=0;j<diaAux.numeroTurnos;++j){
					//		
					//		//Turno actual
					//		turno turnoAux;
					//		data_input >> turnoAux.id;
					//		data_input >> turnoAux.hora_entrada;
					//		data_input >> turnoAux.hora_salida;
					//		data_input >> turnoAux.duracion_turno;
					//		
					//		//Cargar el 'j'-ésimo turno al día actual
					//		diaAux.turnos.push_back(turnoAux);
					//	}
					//	
					//	//Una vez cargados los turnos del día, cargar a la programación semanal de turnos
					//	semana.push_back(diaAux);
					//	
					//	
					//}//Fin de la carga de turnos en 'n' días
					//
					//
					////Cerrar el archivo que contiene la matriz de costos
					//data_input.close();
					//
					////Salida en pantalla de los 'n' días cargados
					//cout<<endl;
					//for(int i=0;i<semana.size();++i){
					//	
					//	cout<<semana[i].numeroTurnos<<endl;
					//	
					//	for(int j=0;j<semana[i].numeroTurnos;++j){
					//		
					//		cout<<semana[i].turnos[j].id<<" "<<semana[i].turnos[j].hora_entrada<<" "<<semana[i].turnos[j].hora_salida<<" "<<semana[i].turnos[j].duracion_turno<<endl;
					//		
					//	}
					//	
					//}
					//cout<<endl;
					
			}break;//Fin de carga de datos Modelo Musliu		
			
			
			//Variante Integra-UniAndes
			case 2:
			{
				
				/////////////////////
				// Lectura de datos//
				/////////////////////
				
				//Abrir el archivo con la matriz de costos e información de los depósitos
				ifstream data_input((testname).c_str());				
				if (! data_input.is_open() ) {
					cout << "Can not read the rostering file " << (testname).c_str() << endl;
					//return EXIT_FAILURE;
					//return 1;
				}else{
					cout<<endl<<"Lectura exitosa del archivo."<<endl;
				}
				
				//Nombre de la instancia
				nombreArchivo = testname;
				cout<<endl<<"Nombre del archivo -> "<<testname;//getchar();				
				
				//Longitud de programación
				data_input >> longitudProgramacion;
				cout<<endl<<"Longitud de Programación : "<<longitudProgramacion<<endl;//getchar();
				
				//Número conductores
				data_input >> numeroConductores;				
				cout<<endl<<"Número de conductores: "<< numeroConductores<<endl;//getchar();			
				
				//Número tipos de turno
				data_input >> tiposTurno;
				if (tiposTurno <= 0) cout << "Número tipos de turno <= 0"<<endl;				
				cout<<endl<<"Número tipos de turno: "<< tiposTurno<<endl;//getchar();								
				
				//Especificación de turnos (caso Integra)				
				descripcionTurnosMusliu.resize(tiposTurno);
				
				//Turno de la mañana Integra
				tipoTurnoMusliu turnoManiana;
				turnoManiana.ShiftName = "D";
				turnoManiana.Start = 240;
				turnoManiana.End = 1260 + 15;
				//turnoManiana.Length;
				turnoManiana.MinlengthOfBlocks = 1;
				turnoManiana.MaxLengthOfBlocks = 7;
				//Adición tel tipo de turno
				descripcionTurnosMusliu[0] = turnoManiana;	
				
				//Turno de la tarde Integra
				tipoTurnoMusliu turnoTarde;
				turnoTarde.ShiftName = "A";
				turnoTarde.Start = 480;
				turnoTarde.End = 1260 + 15;
				//turnoTarde.Length;
				turnoTarde.MinlengthOfBlocks = 1;
				turnoTarde.MaxLengthOfBlocks = 7;
				//Adición tel tipo de turno
				descripcionTurnosMusliu[1] = turnoTarde;
				
				//Turno de la noche Integra
				tipoTurnoMusliu turnoNoche;
				turnoNoche.ShiftName = "N";
				turnoNoche.Start = 480;
				turnoNoche.End = 1440;
				//turnoNoche.Length;
				turnoNoche.MinlengthOfBlocks = 1;
				turnoNoche.MaxLengthOfBlocks = 7;
				//Adición tel tipo de turno
				descripcionTurnosMusliu[2] = turnoNoche;
				
				//Turno mixto (madrugada y trasnocho) Integra
				tipoTurnoMusliu turnoMixto;
				turnoMixto.ShiftName = "M";
				turnoMixto.Start = 240;
				turnoMixto.End = 1440;
				//turnoMixto.Length;
				turnoMixto.MinlengthOfBlocks = 1;
				turnoMixto.MaxLengthOfBlocks = 7;
				//Adición tel tipo de turno
				descripcionTurnosMusliu[3] = turnoMixto;
				
				//Requerimiento de turnos
				data_input >> numeroTurnosRequeridos;
				if (numeroTurnosRequeridos <= 0) cout << "Número turnos que deben ser atendidos <= 0"<<endl;				
				cout<<endl<<"Número turnos que deben ser atendidos: "<< numeroTurnosRequeridos<<endl;//getchar();	
				
				//Cargar cada uno de los turnos
				//dia diaHabil(numeroTurnosRequeridos);				
				diaHabil.numeroTurnos = numeroTurnosRequeridos;
				diaHabil.turnos.resize(numeroTurnosRequeridos);
				for(int i=0;i<numeroTurnosRequeridos;++i){						
					
					//Obtener el número de bloques de trabajo o sub turnos del día
					int numeroBloquesTrabajo;
					data_input >> numeroBloquesTrabajo;
					
					//Arreglo auxiliar bloques de trabajo
					vector< vector< int > > bloquesTrabajo(numeroBloquesTrabajo);								
					
					//Calcular la duración del turno a partir de los bloques de trabajo
					int duracionTurno = 0;
					for(int j = 0; j<numeroBloquesTrabajo;++j){
						
						//Contenedor auxiliar del bloque
						vector<int> bloque(3);//Declarar inicio, finalización y  duración
						
						//Hora de inicio del j-ésimo bloque de trabajo
						data_input >> bloque[0];
						
						//Hora de finalización del j-ésimo bloque de trabajo
						data_input >> bloque[1];
						
						//Duración del j-ésimo bloque
						bloque[2] = bloque[1] - bloque[0];
						
						//Acumular el tiempo trabajado del bloque al turno
						duracionTurno += bloque[2];
						
						//Adicionar el bloque de trabajo al contenedor de los bloques de trabajo del turno i-ésimo
						bloquesTrabajo[j] = bloque;					
								
					}//Fin del recorrido de los bloques de trabajo
					
					//Contenedor del turno que se está clasificando
					turno turnoEnClasificacion;
					turnoEnClasificacion.hora_entrada = bloquesTrabajo[0][0];
					turnoEnClasificacion.hora_salida = bloquesTrabajo[ bloquesTrabajo.size() - 1 ][1];										
					turnoEnClasificacion.duracion_turno = duracionTurno;
					
					////Salida de diagnóstico
					//cout<<endl<<"Turno en clasificación: "<<turnoEnClasificacion.hora_entrada<<" "<<turnoEnClasificacion.hora_salida<<" "<<turnoEnClasificacion.duracion_turno;
					//getchar();
					
					//Clasificar en la mañana
					if( turnoEnClasificacion.hora_entrada < turnoTarde.Start &&  turnoEnClasificacion.hora_salida <= turnoManiana.End ){						
						
						turnoEnClasificacion.clasificacion = "D";
					
					//Clasificar en la tarde	
					}else if( turnoEnClasificacion.hora_entrada >= turnoTarde.Start && turnoEnClasificacion.hora_salida <= turnoTarde.End ){
						
						turnoEnClasificacion.clasificacion = "A";
						
					//Clasificar en la noche	
					}else if( turnoEnClasificacion.hora_entrada >= turnoNoche.Start && turnoEnClasificacion.hora_salida > turnoTarde.End ){
						
						turnoEnClasificacion.clasificacion = "N";
						
					}else{//Mixto o turno extenso en los demás casos
						
						turnoEnClasificacion.clasificacion = "M";
						
					}//Final de las clasificaciones				
					
					
					//Adicionar el turno clasificado
					//diaHabil.turnos.push_back(turnoEnClasificacion);
					diaHabil.turnos[i] = turnoEnClasificacion;
					
					
				}//Finalización de la carga y clasificación de los turnos		
				
				
				//Salida en pantalla de los turnos con su clasificación
				cout<<endl<<"Número de turnos requeridos--->"<<diaHabil.numeroTurnos<<endl;
				for(int i=0; i<diaHabil.numeroTurnos;i++ ){
										
					cout<<endl<<i+1<<") Hora entrada: "<<diaHabil.turnos[i].hora_entrada<<" ("<<diaHabil.turnos[i].hora_entrada/60<<") "<<
					"Hora salida: "<<diaHabil.turnos[i].hora_salida<<" ("<<diaHabil.turnos[i].hora_salida/60<<") "<<
					"Duración: "<<diaHabil.turnos[i].duracion_turno<<" ("<<diaHabil.turnos[i].duracion_turno/60<<") "<<
					"Clasificación -> "<<diaHabil.turnos[i].clasificacion;
										
				}
				
				//Generar matriz de requerimientos
				matrizRequerimientoPersonal.resize(numeroTurnosRequeridos);
				for(int i=0;i<numeroTurnosRequeridos;++i){					
					vector <int> arregloSemana(longitudProgramacion,1);
					//Casos últimos dos días de la programación
					if(i>=39 && i<44){
						arregloSemana[longitudProgramacion-2] = 1;
						arregloSemana[longitudProgramacion-1] = 0;						
					}else if(i>=44){
						arregloSemana[longitudProgramacion-2] = 0;
						arregloSemana[longitudProgramacion-1] = 0;
					}					
					matrizRequerimientoPersonal[i] = arregloSemana;
				}
				//Agregar descanso a los requerimientos
				vector <int> arregloSemana(longitudProgramacion);
				matrizRequerimientoPersonal.push_back(arregloSemana);
				
				//Salida en pantalla
				cout<<endl<<"Matriz de requerimiento de personal: "<<endl;
				int contador = 1;
				for(auto& fila_i : matrizRequerimientoPersonal){
					cout<<contador<<")";
					for(auto& posicion_i_j : fila_i){
						cout<<posicion_i_j<<" ";
					}
					cout<<endl;	
					++contador;
				}
				//getchar();			
				
				
				//Días libre
				data_input >> numeroMinimoDiasLibre;
				data_input >> numeroMaximoDiasLibre;
				//Salida en pantalla
				cout<<endl<<"Número días libre -> "<< numeroMinimoDiasLibre <<" "<<numeroMaximoDiasLibre<<endl;				
				
				//Adicionar turno de descanso
				tipoTurnoMusliu turnoDescanso;
				turnoDescanso.ShiftName = "B";
				turnoDescanso.Start = 0;
				turnoDescanso.Length = 1440;
				turnoDescanso.MinlengthOfBlocks = numeroMinimoDiasLibre;
				turnoDescanso.MaxLengthOfBlocks = numeroMaximoDiasLibre ;
				descripcionTurnosMusliu.push_back(turnoDescanso);	
				++tiposTurno;//Aumentar el número de turnos después de adicionar el turno de descanso
				
				//Salida en pantalla
				cout<<endl<<"Descripción de tipos de turno: "<<endl;
				for(auto& turno_i : descripcionTurnosMusliu){
					cout<<turno_i.ShiftName;
					cout<<" "<<turno_i.Start;
					cout<<" "<<turno_i.Length;
					cout<<" "<<turno_i.MinlengthOfBlocks;
					cout<<" "<<turno_i.MaxLengthOfBlocks;
					cout<<endl;	
				}
				//getchar();				
				
				//Días de trabajo				
				data_input >> numeroMinimoDiasTrabajo;
				data_input >> numeroMaximoDiasTrabajo;
				//Salida en pantalla
				cout<<endl<<"Número días trabajo -> "<< numeroMinimoDiasTrabajo <<" "<<numeroMaximoDiasTrabajo<<endl;
				
				//Prohibiciones				
				data_input >> longitudProhibicionTamanio2;
				data_input >> longitudProhibicionTamanio3;
				//Salida en pantalla
				cout<<endl<<"Prohibición Tamaño 2 y 3 -> "<< longitudProhibicionTamanio2 <<" "<<longitudProhibicionTamanio3<<endl;
				
				////Omitir enunciado
				//data_input >> foo;
				//data_input >> foo;
				//data_input >> foo;
				//data_input >> foo;
				//data_input >> foo;
				//for(int i=0;i<longitudProhibicionTamanio2;++i){
				//	secuenciasProhibidasTamanio2
				//}
				//
				//
				//
				//
				//vector<string> secuenciasProhibidasTamanio2;
				//vector<string> secuenciasProhibidasTamanio3;
				 			
				
			}break;//Fin de carga de datos Integra-UniAndes
			
			//Indeterminado
			default: cout<<endl<< "Variante de carga de datos para los modelos inexistente!!!!!";
			break;
			
		}//Final del selector de carga de datos
		
			
		
	}//Fin del constructor para carga de datos desde archivo
	
};


//////////////////////////////////////////////////////////////////


//Contenedor para almacenar los viajes de cada caso o instancia
struct viaje {
	
	//Identificador del viaje para las asignaciones
	int idViaje;
	
	//Tiempos en minutos, desde media noche (minuto 0) hasta la media noche siguiente (minuto 1440)
	int tiempoInicio;
	int tiempoFin;
	
	//Ubicación de los viajes
	string lugarInicio;//Punto de salida para el caso Megabús (tanto MDVSP como CSP)
	int idLugarInicio;
	string lugarFinal;
	int idLugarFinal;
	
	//Número de transiciones posibles a otros viajes 
	int numeroTransicionesPosibles;
	
	//Id's de los viajes que se pueden atender después del viaje actual
	unordered_set<int> viajesTransicion;
	
	//Tiempo de duración del viaje en minutos
	int duracionViaje;	
	
	//Bandera para controlar la asignación de los viajes a alguno de los vehículos disponibles (cuando forma parte de la solución)
	bool asignado;
	
	//Atributo que almacena el viaje a qué depósito ha sido asignado
	int idDepositoAsignado;
	
	//Atributos del caso real de Megabús
	string rutaTabla;//Aplica tanto para MDVSP como CSP
	
	//Atributos correspondientes al CSP para empalme con MDVSP
	string evento;
	string nombreRuta;
	int idViajeArchivoEntrada;
	vector<int> idsViajesCubiertosArchivoEntrada;
	
	//Itinerario donde el viaje ha sido insertado (Atributo para constructivo tipo bin-packing)
	list<viaje> itinerarioViajeActualInsertado;
	
	//Itinerario de preprocesamiento
	list<viaje> itinerarioAsociado;
	int idPrimerViaje;
	int idUltimoViaje;
	
	//Posibles constructores de los viajes
	viaje() : tiempoInicio(0), tiempoFin(0), asignado(false) {}		
};


//Contenedor para almacenar la información de los depósitos depósitos
struct deposito {
	
	//Atributos del depósito
	int idDeposito;
	int numeroVehiculos;
	string nombreDeposito;
	
	//Posibles constructores de los depósitos
	deposito() : idDeposito(0), numeroVehiculos(0) {}
};

//Contenedor para instancias o casos MD-VSP (Librería Fischetti et al.)
struct mdvsp {	
	
	//Atributos de la instancia o caso
	//int idLibreria;
	int idInstancia;
	string nombreArchivoInstancia;
	int numeroDepositos;	
	int numeroViajes;	
	int numeroTotalVehiculos;
	vector<deposito> depositos;
	vector<viaje> viajes;	
	vector < vector<double> > matrizViajes;//Matriz explícita con el costo que implica pasar de una ruta o tarea a otra y los depósitos
	
	//Parámetros a definir
	int tiempoServicioVehiculos;
	
	//Atributos adicionales (Adaptación a Crew Scheduling caso Integra S.A.)
	vector< vector <viaje> > servicios;
	int numeroPuntosCambio;
	vector< vector <int> > tiemposDesplazamientoPuntosCambio;
	unordered_map<string,int> codigosPuntosCambio;	
	int numeroPuntosCorteViajes;
	unordered_map<string,int> codigosCorteViajes;
	int duracionBloqueTrabajo;//Duración definida en minutos. Usualmente 300 minutos, es decir, 5 horas
	int tiempoDescansoReglamentario;//Tiempo de descanso reglamentario en minutos. (40 minutos)
	int numeroConductoresLicenciaC2;//Parámetro para control de número de turnos mixtos
	int numeroConductoresLicenciaC3;//Parámetro para control de número de turnos mixtos
	int inicioJornada;//Hora de inicio en minutos de la jornada (menor a este parámetro se considera madrugada)
	int finalJornada;//Hora de finalización en minutos de la jornada (mayor a este parámetro se considera trasnocho)
	int limiteInferiorTiempoTurno;//Número mínimo de horas de trabajo para construir un turno (se recomienda 0 para permitir exploración libre del algoritmo)
	int limiteSuperiorTiempoTurno;//Número máximo de horas de trabajo para construir un turno
	bool restriccionMadrugadaTrasnocho;//Activar la restricción de turnos extensos (con madrugada y trasnocho)
	
	//Atributos adicionales (MDVSP caso Integra S.A.)
	vector< vector <double> > distanciasEntrePuntosCambio;
		
	//Variables para algoritmos genéticos
	int t_0_max;//Tiempo total de viaje mayor en la población inicial
	int v_0_max;//Solución que utiliza la mayor cantidad de vehículos en una población
	int d_0_max;//Solución que presenta el mayor tiempo de ocio
	int banderaCostoDepositos;//Bandera para que la función fitness incluya o no los costos de los depósitos
	
	//Posibles constructores de la instancia
    //mdvsp() : have_st(false), min_x(INT_MAX), min_y(INT_MAX) {}
};




//Contenedor para CSP (Caso Integra S.A.)
struct csp{	
	
	//Atributos de la instancia o caso
	int idInstancia;
	string nombreArchivoInstancia;
	int numeroDepositos;	
	int numeroViajes;	
	int numeroTotalVehiculos;
	vector<deposito> depositos;
	vector<viaje> viajes;	
	vector < vector<double> > matrizViajes;//Matriz explícita con el costo que implica pasar de una ruta o tarea a otra y los depósitos
	
	//Parámetros a definir
	int tiempoServicioVehiculos;
	
	//Posibles constructores de la instancia
    //mdvsp() : have_st(false), min_x(INT_MAX), min_y(INT_MAX) {}
};

//Contenedor o clase que representa un vehículo del problema
struct vehiculo{
	int idVehiculo;
	int idDeposito;
	int tiempoServicio;
	int tiempoInactivo;
	int tiempoTransicional;
	int tiempoOcio;
	bool utilizado;
	bool copado;
	double densidadItinerario;
	list <viaje> listadoViajesAsignados;	
	int numeroViajesAsignados;
	//list <int> listadoViajesAsignados;
	int tiempoInicioItinerarioVehiculo;
	int tiempoFinItinerarioVehiculo;
	int tiempoTotalViaje;
		
	//Atributos adicionados para constructivo basado en bin-packing
	int tiempoFinUltimoServicio;
	bool abierto;	
	
	//Constructores de los vehículos (principalmente utilizados en soluciones)
	vehiculo() : utilizado(false), copado(false), tiempoServicio(0), tiempoTransicional(0), tiempoInactivo(0), densidadItinerario(0), numeroViajesAsignados(0), tiempoOcio(0), tiempoInicioItinerarioVehiculo(0), tiempoFinItinerarioVehiculo(0), tiempoTotalViaje(0), abierto(true)  {}	
};

//Contenedor o clase que representa un conductor en el problema
struct conductor{
	int idVehiculo;
	int idDeposito;
	int tiempoServicio;
	int tiempoInactivo;
	bool utilizado;
	bool copado;
	list <viaje> listadoViajesAsignados;
	//list <int> listadoViajesAsignados;
	
	//Constructores de los vehículos (principalmente utilizados en soluciones)
	conductor() : utilizado(false), copado(false), tiempoServicio(0)  {}
	
};

//Contenedor para soluciones MD-VSP
struct mdvsp_solution{
		
	//Atributos del contenedor de solución
	int numeroTotalVehiculosEmpleados;
	int numeroTotalVehiculosLibres;
	int numeroVehiculosFlota;
	vector<vehiculo> flotaVehiculos;
	vector< vector<int> > arregloAristasDepositos;
	double funcionObjetivo;//Unidades de costo (tiempo o distancia)
	
	//Atributos que codifican el cromosoma de la solución mdvsp (algoritmo genético)
	vector<int> cromosomaViajes;
	vector<int> cromosomaVehiculos;
	bool cromosomaFactible;
	int FOnumeroVehiculos;
	double funcionObjetivoCompuesta;
	int funcionObjetivoTiempoOcio;
	
	//Salida en pantalla y en archivo con formato CSV de los itinerarios generados
	void mostrarEstadoFlotaDepositos(mdvsp mdvspdata){	
		
		//Método que muestra en pantalla y en un archivo csv los viajes que tienen asignados cada uno de los vehículos agrupados por deposito	
		ofstream archivoDetalleScheduling("detalleScheduling.csv",ios::binary);		
		if (archivoDetalleScheduling.is_open()){
			//Colocar el nombre de la instancia en el archivo de salida
			archivoDetalleScheduling<<mdvspdata.nombreArchivoInstancia<<endl;
		}else{
			cout << "Fallo en la apertura del archivo de salida que contiene el detalle del scheduling!";
		}
		
		cout<<endl<<"----------------Estado de la flota----------------"<<endl;
		for(size_t i=0;i<mdvspdata.numeroDepositos;++i){			
			cout<<"---->Depósito "<<i<<" :"<<endl;
			archivoDetalleScheduling<<"-----;-----;-----;-----;-----;"<<endl;			
			archivoDetalleScheduling<<"Deposito;"<<i<<";"<<endl;
			archivoDetalleScheduling<<"-----;-----;-----;-----;-----;"<<endl;
			for(size_t j=0;j<this->flotaVehiculos.size();++j){				
				//Agrupar los vehículos por depósito en la salida
				if(this->flotaVehiculos[j].idDeposito == mdvspdata.depositos[i].idDeposito){
					cout<<"Vehículo "<<this->flotaVehiculos[j].idVehiculo<<" = ";
					archivoDetalleScheduling<<"-----;-----;-----;-----;-----;"<<endl;			
					archivoDetalleScheduling<<"Vehiculo;"<<this->flotaVehiculos[j].idVehiculo<<endl;
					archivoDetalleScheduling<<"Id_Viaje;Inicio;Fin;Duracion;"<<endl;			
					int tiempoOcupacionVehiculo = 0;
					for(list<viaje>::iterator itViajesAsignados = flotaVehiculos[j].listadoViajesAsignados.begin(); itViajesAsignados != flotaVehiculos[j].listadoViajesAsignados.end() ; ++itViajesAsignados){
						cout<<(*itViajesAsignados).idViaje<<" ["<<(*itViajesAsignados).tiempoInicio<<","<<(*itViajesAsignados).tiempoFin<<"] ";
						archivoDetalleScheduling<<(*itViajesAsignados).idViaje<<";"<<(*itViajesAsignados).tiempoInicio<<";"<<(*itViajesAsignados).tiempoFin<<";"<<(*itViajesAsignados).duracionViaje<<endl;
						tiempoOcupacionVehiculo += (*itViajesAsignados).duracionViaje;
					}
					cout<<"("<<flotaVehiculos[j].listadoViajesAsignados.size()<<")"<<endl;
					archivoDetalleScheduling<<"Viajes asignados;"<<flotaVehiculos[j].listadoViajesAsignados.size()<<endl;
					archivoDetalleScheduling<<"Tiempo ocupacion;"<<tiempoOcupacionVehiculo<<endl;
					//archivoDetalleScheduling<<"Tiempo ocupacion;"<<flotaVehiculos[j].tiempoServicio<<endl;
				}
			}
		}
		cout<<endl<<"----------------Fin estado de la flota----------------"<<endl;
		
		
		//Cierre del archivo csv con los itinerarios
		archivoDetalleScheduling.close();
	}
		
	//Constructores para el contenedor de una solución de viajes
	mdvsp_solution() : numeroTotalVehiculosEmpleados(0), numeroTotalVehiculosLibres(0), numeroVehiculosFlota(0), funcionObjetivoCompuesta(0), funcionObjetivoTiempoOcio(0), cromosomaFactible(true) {}	
	mdvsp_solution(int a) : flotaVehiculos(a), numeroTotalVehiculosEmpleados(0), numeroTotalVehiculosLibres(a), numeroVehiculosFlota(a), funcionObjetivo(0), funcionObjetivoCompuesta(0), funcionObjetivoTiempoOcio(0), cromosomaFactible(true) {}	
	
};

//Contenedor para la clusterización de viajes por depósito
struct asignacionViajeDeposito{
	//Atributos del contenedor
	int idDeposito;	
	int idViaje;	
	double costoDepositoViajeActual;	
	
	//Operadores del contenedor
	bool operator<(const asignacionViajeDeposito& b) const { return (this->costoDepositoViajeActual < b.costoDepositoViajeActual); }
	bool operator>(const asignacionViajeDeposito& b) const { return (this->costoDepositoViajeActual > b.costoDepositoViajeActual); }
	
};

//Contenedor para las aristas o transiciones entre viajes o depósitos
struct arista{
	//Atributos del contenedor
	double costo;
	int i;
	int j;
	int idDeposito;
	
	//Constructores de las aristas o transiciones del grafo asociado al MD-VSP
	arista() : costo(0)  {}
};

//Contenedor para segmentos de una secuencia de viajes
struct segmentoSecuenciaViajes{
	
	//Atributos del contenedor
	list<viaje> viajes;
	int tiempoInicioSegmento;
	int tiempoFinSegmento;
	int tiempoOcioSegmento;
	int numeroViajes;
	double costo;
	bool segmentoEliminado;
	
	//Posibles constructores
	segmentoSecuenciaViajes() : costo(0), numeroViajes(0), tiempoOcioSegmento(0), segmentoEliminado(false)  {}
	
};

//Contenedor para rankear conexiones entre itinerarios o supernodos
struct conexionItinerario{
	
	//Atributos para rankear la conexión de itinerarios
	long int subIndiceContenedorViajeFinal;
	long int subIndiceContenedorViajeInicial;
	double costo;
	int id_viaje_final;
	int id_viaje_inicial;
	
	//Operadores de ordenamiento
	bool operator<(const conexionItinerario& b) const { return (this->costo < b.costo); }
    bool operator>(const conexionItinerario& b) const { return (this->costo > b.costo); }
    
    
    //Posibles constructores
	conexionItinerario() : costo(0) {}
    
};

//Contenedor para modelar los super nodos (opcional)
struct nodoItinerario {
	long int idViajeEntrada;
	long int idViajeSalida;
};

//Contenedor para almacenar información correspondiente a una secuencia de atención
struct secuenciaAtencion{
	
	//Atributos para almacenar la información e la secuencia	
	double costoSinDepositos;
	double costoConDepositos;
	vector<segmentoSecuenciaViajes> itinerarios;
	
	//Operadores de ordenamiento 
	//bool operator<(const secuenciaAtencion& b) const { return (this->costoSinDepositos < b.costoSinDepositos); }
    //bool operator>(const secuenciaAtencion& b) const { return (this->costoSinDepositos > b.costoSinDepositos); }   
    
    //Posibles constructores
	secuenciaAtencion() : costoSinDepositos(0), costoConDepositos(0)  {}
    
};

//Contenedor para las aristas del digrafo Prins-Liu
struct aristaDigrafoPrinsLiu{
		
	//Atributos asociados a la arista
	double costo;
	viaje viajeInicial;
	viaje viajeFinal;
	deposito depositoArista;
	list<viaje> viajesCubiertos;
	int numeroViajesCubiertos;
	int subIndiceContenedorSalidaArista;
	int subIndiceContenedorLlegadaArista;
	
	//Posibles constructores
	aristaDigrafoPrinsLiu() : costo(0), numeroViajesCubiertos(0) {}
	aristaDigrafoPrinsLiu(double a, viaje b, viaje c, deposito d, list<viaje> e, int f, int g, int h) : costo(a), viajeInicial(b), viajeFinal(c), depositoArista(d), viajesCubiertos(e), numeroViajesCubiertos(f), subIndiceContenedorSalidaArista(g), subIndiceContenedorLlegadaArista(h) {}
	
	//Ejemplo de constructor
	//int id;
	//int demanda;
	//rutaCorta(int a, int b) : id(a), demanda(b) {}
	
};

//Contenedor para los nodos del digrafo Prins-Liu
struct nodoDigrafoPrinsLiu{
	
	//Atributos asociados a los nodos
	//vector<int> aristasSalientes;//Contiene los id's o subíndices de las aristas salientes
	//vector<int> aristasEntrantes;//Contiene los id's o subíndices de las aristas entrantes
	unordered_set<int> aristasSalientes;//Contiene los id's o subíndices de las aristas salientes
	unordered_set<int> aristasEntrantes;//Contiene los id's o subíndices de las aristas entrantes
	int numeroAristasSalientes;
	int numeroAristasEntrantes;
	bool nodoInicialGrafo;
	bool nodoFinalGrafo;
	
	//Posibles constructores
	nodoDigrafoPrinsLiu(): numeroAristasSalientes(0), numeroAristasEntrantes(0), nodoInicialGrafo(false), nodoFinalGrafo(false)  {}
	
};

//Contenedor para el digrafo Prins-Liu organizado por nodos
struct digrafoPrinsLiu{
	
	//Atributos del digrafo Prins-Liu
	vector<nodoDigrafoPrinsLiu> nodos;
	vector<aristaDigrafoPrinsLiu> aristas;
	int numeroNodos;
	int numeroAristas;
	
	//Posibles constructores
	digrafoPrinsLiu (): numeroNodos(0), numeroAristas(0) {}
	
};

//Contenedor para almacenar y rankear movimientos shift(1,0) entre itinerarios
struct movimientoShift1_0{
	
	//Atributos del movimiento
	int idItinerarioPerdiendoViaje;
	int idItinerarioGanandoViaje;
	
	//Posibles constructores
	movimientoShift1_0 (): idItinerarioGanandoViaje(0), idItinerarioPerdiendoViaje(0) {}
	
};


//////////////////////////////////////////////////////////////////////////
//Predicados de ordenamiento de vectores con dato tipo estructura o TAD // 
//////////////////////////////////////////////////////////////////////////
struct depositosNumeroVehiculosAscendente{
	bool operator() (const deposito& a, const deposito& b) const { return a.numeroVehiculos < b.numeroVehiculos; }
};
struct depositosNumeroVehiculosDescendente{
	bool operator() (const deposito& a, const deposito& b) const { return a.numeroVehiculos > b.numeroVehiculos; }
};
struct ordenarViajesDuracionAscendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.duracionViaje < b.duracionViaje; }
};
struct ordenarViajesDuracionDescendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.duracionViaje > b.duracionViaje; }
};
struct ordenarViajesTiempoInicioAscendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.tiempoInicio < b.tiempoInicio; }
};
struct ordenarViajesTiempoInicioDescendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.tiempoInicio > b.tiempoInicio; }
};
struct ordenarViajesTiempoFinAscendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.tiempoFin < b.tiempoFin; }
};
struct ordenarViajesTiempoFinDescendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.tiempoFin > b.tiempoFin; }
};
struct ordenarViajesNumeroTransicionesSalidaAscendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.numeroTransicionesPosibles < b.numeroTransicionesPosibles; }
};
struct ordenarViajesNumeroTransicionesSalidaDescendente{
	bool operator() (const viaje& a, const viaje& b) const { return a.numeroTransicionesPosibles > b.numeroTransicionesPosibles; }
};
struct ordenarAristasCostoAscendente{
	bool operator() (const arista& a, const arista& b) const { return a.costo < b.costo; }
};
struct ordenarAristasCostoDescendente{
	bool operator() (const arista& a, const arista& b) const { return a.costo > b.costo; }
};
struct ordenarSegmentosSecuenciaTiempoInicioAscendente{
	bool operator() (const segmentoSecuenciaViajes& a, const segmentoSecuenciaViajes& b) const { return a.tiempoInicioSegmento < b.tiempoInicioSegmento; }
};
struct ordenarSegmentosSecuenciaTiempoInicioDescendente{
	bool operator() (const segmentoSecuenciaViajes& a, const segmentoSecuenciaViajes& b) const { return a.tiempoInicioSegmento > b.tiempoInicioSegmento; }
};
struct ordenarSegmentosSecuenciaCostoAscendente{
	bool operator() (const segmentoSecuenciaViajes& a, const segmentoSecuenciaViajes& b) const { return a.tiempoInicioSegmento < b.tiempoInicioSegmento; }
};
struct ordenarSegmentosSecuenciaCostoDescendente{
	bool operator() (const segmentoSecuenciaViajes& a, const segmentoSecuenciaViajes& b) const { return a.costo > b.costo; }
};
struct ordenarSecuenciasCostoSinDeposito{	
	bool operator() (const secuenciaAtencion& a, const secuenciaAtencion& b) const { return a.costoSinDepositos > b.costoSinDepositos; }
};
struct ordenarSecuenciasCostoConDeposito{	
	bool operator() (const secuenciaAtencion& a, const secuenciaAtencion& b) const { return a.costoConDepositos > b.costoConDepositos; }
};
//Predicados para ordenar vehículos

////////////////////////////////////////////Adicionados para mutaciones presentadas en (Park, 2001)
struct ordenarVehiculosTiempoTotalViajeAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoTotalViaje < b.tiempoTotalViaje; }
};
struct ordenarVehiculosTiempoTotalViajeDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoTotalViaje > b.tiempoTotalViaje; }
};
struct ordenarVehiculosTiempoInicioItinerarioAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoInicioItinerarioVehiculo < b.tiempoInicioItinerarioVehiculo; }
};
struct ordenarVehiculosTiempoInicioItinerarioDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoInicioItinerarioVehiculo > b.tiempoInicioItinerarioVehiculo; }
};
struct ordenarVehiculosTiempoFinItinerarioAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoFinItinerarioVehiculo < b.tiempoFinItinerarioVehiculo; }
};
struct ordenarVehiculosTiempoFinItinerarioDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoFinItinerarioVehiculo > b.tiempoFinItinerarioVehiculo; }
};
////////////////////////////////////////////
struct ordenarVehiculosTiempoFinUltimoServicioAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoFinUltimoServicio < b.tiempoFinUltimoServicio; }
};
struct ordenarVehiculosTiempoFinUltimoServicioDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoFinUltimoServicio > b.tiempoFinUltimoServicio; }
};
struct ordenarVehiculosTiempoServicioAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoServicio < b.tiempoServicio; }
};
struct ordenarVehiculosTiempoServicioDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoServicio > b.tiempoServicio; }
};
struct ordenarVehiculosTiempoTransicionalAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoTransicional < b.tiempoTransicional; }
};
struct ordenarVehiculosTiempoTransicionalDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.tiempoTransicional > b.tiempoTransicional; }
};
struct ordenarVehiculosDensidadItinerarioAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.densidadItinerario < b.densidadItinerario; }
};
struct ordenarVehiculosDensidadItinerarioDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.densidadItinerario > b.densidadItinerario; }
};
//struct ordenarVehiculosNumeroViajesAsignadosAscendente{
//	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.numeroViajesAsignados < b.numeroViajesAsignados; }
//};
//struct ordenarVehiculosNumeroViajesAsignadosDescendente{
//	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.numeroViajesAsignados > b.numeroViajesAsignados; }
//};
struct ordenarVehiculosNumeroViajesAsignadosAscendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.listadoViajesAsignados.size() < b.listadoViajesAsignados.size(); }
};
struct ordenarVehiculosNumeroViajesAsignadosDescendente{
	bool operator() (const vehiculo& a, const vehiculo& b) const { return a.listadoViajesAsignados.size() > b.listadoViajesAsignados.size(); }
};
//Predicados para ordenar soluciones
struct ordenarSolucionesNumeroVehiculosAscendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.FOnumeroVehiculos < b.FOnumeroVehiculos; }
};
struct ordenarSolucionesNumeroVehiculosDescendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.FOnumeroVehiculos > b.FOnumeroVehiculos; }
};
struct ordenarSolucionesCostoAscendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivo < b.funcionObjetivo; }
};
struct ordenarSolucionesCostoDescendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivo > b.funcionObjetivo; }
};
struct ordenarSolucionesFuncionObjetivoCompuestaAscendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivoCompuesta < b.funcionObjetivoCompuesta; }
};
struct ordenarSolucionesFuncionObjetivoCompuestaDescendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivoCompuesta > b.funcionObjetivoCompuesta; }
};

struct ordenarSolucionesFuncionObjetivoTiempoOcioAscendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivoTiempoOcio < b.funcionObjetivoTiempoOcio; }
};
struct ordenarSolucionesFuncionObjetivoTiempoOcioDescendente{
	bool operator() (const mdvsp_solution& a, const mdvsp_solution& b) const { return a.funcionObjetivoTiempoOcio > b.funcionObjetivoTiempoOcio; }
};


//Clase general para problemas de programación de rutas (struct se comporta como clase con el 100% de sus atributos públicos)
struct VSP{
	
	//////////////////////////////////////////////////////////////
	//Atributos de la clase genérica de variantes de ruteamiento//
	//////////////////////////////////////////////////////////////
	
	//Atributos generales
	int idVarianteVSP;	
	
	//Atributos Multi-depot Vehicle Scheduling Problem
	mdvsp mdvspdata, mdvspdataRestaurar;
	mdvsp_solution solucionmdvsp, solucionmdvspRestaurar;	
	
		
	//Constructor: Seleccionar el tipo de variante de programación de rutas para proceder a realizar la carga
	VSP(int varianteSeleccionada, string testname){		
		
		//1 - Incluye aristas entrando y saliendo a los depósitos, 0 - Lo contrario.
		mdvspdata.banderaCostoDepositos = 1;
		
		//Cargar el id de la variante en el atributo correspondiente
		idVarianteVSP = varianteSeleccionada;
		
		//Estructura de decisión múltiple para seleccionar el tipo de carga		
		switch(idVarianteVSP){								
				
			//0 = MDVSP
			case 0:
			{				
				/////////////////////
				// Lectura de datos//
				/////////////////////
				
				//Abrir el archivo con la matriz de costos e información de los depósitos
				ifstream data_input_cst((testname + ".cst").c_str());				
				if (! data_input_cst.is_open() ) {
					cout << "Can not read the .cst file " << (testname + "/" + "input.txt") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				
				//Nombre de la instancia
				mdvspdata.nombreArchivoInstancia = testname;
				
				//Número de depósitos
				data_input_cst >> mdvspdata.numeroDepositos;
				if (mdvspdata.numeroDepositos <= 0) cout << "Número de depósitos <= 0" << endl;
				//Salida en pantalla
				//cout<<endl<<"Número de depósitos: "<< mdvspdata.numeroDepositos;
				
				//Número de viajes que se deben atender con la flota
				data_input_cst >> mdvspdata.numeroViajes;
				if (mdvspdata.numeroViajes <= 0) cout << "Número de viajes <= 0" << endl;
				//Salida en pantalla
				//cout<<endl<<"Número de viajes que se deben atender: "<< mdvspdata.numeroViajes;
				
				//Por cada depósito almacenar el número de vehículos de los que dispone y el tamaño total de la flota para la instancia
				mdvspdata.depositos.resize(mdvspdata.numeroDepositos);
				mdvspdata.numeroTotalVehiculos = 0;
				for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
					data_input_cst >> mdvspdata.depositos[i].numeroVehiculos;
					mdvspdata.depositos[i].idDeposito = i;
					mdvspdata.numeroTotalVehiculos += mdvspdata.depositos[i].numeroVehiculos;
				}
				////Salida en pantalla
				//cout<<endl;
				//for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
				//	cout<<"Vehículos disponibles en el depósito "<<(i+1)<<": "<<mdvspdata.depositos[i].numeroVehiculos<<endl;
				//}
				
				//Redimensionar matriz de costos (explícita)
				//mdvspdata.matrizViajes.resize(mdvspdata.numeroViajes + mdvspdata.numeroDepositos);
				//Recorrer la matriz en formato "row-wise" para pasarla a la matriz de costos explícita
				int contadorCostos = 0;
				vector<double> filaTemp;
				int numeroAristasInfactibles = 0;				
				double costoTemp;
				for(size_t i=0;i<(mdvspdata.numeroViajes + mdvspdata.numeroDepositos)*(mdvspdata.numeroViajes + mdvspdata.numeroDepositos);++i){
					data_input_cst >> costoTemp;
					if(costoTemp == 100000000){
						++numeroAristasInfactibles;
					}
					filaTemp.push_back(costoTemp);
					if(filaTemp.size() == mdvspdata.numeroViajes + mdvspdata.numeroDepositos){
						mdvspdata.matrizViajes.push_back(filaTemp);
						filaTemp.clear();
					}
				}
				////Salida en pantalla (pasar la matriz a un editor de texto plano para revisar su contenido)
				////getchar();
				//cout<<endl;
				//for(size_t i=0;i<mdvspdata.matrizViajes.size();++i){
				//	for(size_t j=0;j<mdvspdata.matrizViajes[i].size();++j){
				//		cout<<mdvspdata.matrizViajes[i][j]<<" ";
				//	}
				//	cout<<endl;
				//}
				
				//Cerrar el archivo que contiene la matriz de costos
				data_input_cst.close();
				
				//Salida de diagnóstico
				int numeroAristasGrafo = (mdvspdata.numeroViajes+mdvspdata.numeroDepositos)*(mdvspdata.numeroViajes+mdvspdata.numeroDepositos);
				//cout<<"Porcentaje de aristas desactivadas -> "<<(double(numeroAristasInfactibles)/double(numeroAristasGrafo))*100.0;
				//getchar();				
				
				//Abrir el archivo con los tiempos de inicio y fin de los viajes
				ifstream data_input_tim((testname + ".tim").c_str());				
				if (! data_input_tim.is_open() ) {
					cout << "Can not read the .tim file " << (testname + "/" + "input.txt") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				
				//Precargar todos los tiempos de inicio
				vector <int> tiemposInicioTemp;
				tiemposInicioTemp.resize(mdvspdata.numeroViajes);
				for(size_t i=0;i<mdvspdata.numeroViajes;++i){
					data_input_tim >> tiemposInicioTemp[i];
				}
				
				//Precargar todos los tiempos de finalización de las rutas
				vector <int> tiemposFinTemp;
				tiemposFinTemp.resize(mdvspdata.numeroViajes);
				for(size_t i=0;i<mdvspdata.numeroViajes;++i){
					data_input_tim >> tiemposFinTemp[i];
				}
				
				//Cargar en la estructura correspondiente los tiempos de los viajes
				mdvspdata.viajes.resize(mdvspdata.numeroViajes);
				for(size_t i=0;i<mdvspdata.numeroViajes;++i){
					viaje viajeTemp;
					viajeTemp.idViaje = i;
					viajeTemp.tiempoInicio = tiemposInicioTemp[i];
					viajeTemp.tiempoFin = tiemposFinTemp[i];					
					mdvspdata.viajes[i] = viajeTemp;
				}
				
				////Sallida en pantalla de los viajes
				//cout<<endl<<"Tiempos de inicio y finalización de viajes"<<endl;
				//cout<<"------------------------------------------"<<endl;
				//for(size_t i=0;i<mdvspdata.numeroViajes;++i){
				//	cout<<"Viaje "<<i+1<<endl;
				//	cout<<"T_inico = "<<mdvspdata.viajes[i].tiempoInicio<<endl;
				//	cout<<"T_fin = "<<mdvspdata.viajes[i].tiempoFin<<endl;
				//	cout<<endl;
				//	
				//	//Validación de la lectura de los viajes
				//	if(mdvspdata.viajes[i].tiempoInicio > mdvspdata.viajes[i].tiempoFin){
				//		cout<<endl<<"------> ERROR!!!!!!!!!"<<endl;getchar();
				//	}
				//	
				//}
				////getchar();
								
				//Cerrar el archivo que contiene los tiempos de inicio y finalización de los viajes
				data_input_tim.close();								
				
				//Preparar contenedor de solución a partir de la información cargada de la instancia
				solucionmdvsp.flotaVehiculos.resize(mdvspdata.numeroTotalVehiculos);
				solucionmdvsp.numeroTotalVehiculosEmpleados = 0;
				//Asignar código a los vehículos de la solución y diferenciar el depósito al cual pertenecen
				int autonumericoIdVehiculos = 0;
				for(size_t i=0;i<mdvspdata.depositos.size();++i){
					for(size_t j=0;j<mdvspdata.depositos[i].numeroVehiculos;++j){
						solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idDeposito = mdvspdata.depositos[i].idDeposito;  
						solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idVehiculo = autonumericoIdVehiculos;
						++autonumericoIdVehiculos;
					}
				}
				
				//Contenedores de restauración
				solucionmdvspRestaurar = solucionmdvsp;
				mdvspdataRestaurar = mdvspdata;			
				
				
			}break;
			
			//1 = Omitir carga VSP
			case 1:
			{
				
				cout<<"Omitir carga VSP!";
				
			}break;			
			
			//2 = MDVSP - Megabús (Caso real, sólo articulados)
			case 2:
			{	
				
				/////////////////////
				// Lectura de datos//
				/////////////////////
				
				//Nombre de la instancia
				mdvspdata.nombreArchivoInstancia = "CSP_Integra_MEGABUS";
				cout<<endl<<"Nombre de la instancia---> "<<mdvspdata.nombreArchivoInstancia<<endl;
				
				////////////////////////////////////////////////////////////////////////////////////////////////			
				
				//Obtener la información de los puntos de corte para el preprocesamiento del archivo de servicios o viajes de entrada 'servicios.csp'
				ifstream data_input_puntos_corte((testname + "puntosCorteViajes.csp").c_str());				
				if (! data_input_puntos_corte.is_open() ) {
					cout << "Can not read the file " << (testname + "puntosCorteViajes.csp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				
				//Obtener el número de puntos de cambio
				data_input_puntos_corte >> mdvspdata.numeroPuntosCorteViajes;
				if (mdvspdata.numeroPuntosCorteViajes <= 0) cout << "Número de puntos de corte <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Número de puntos de corte de eventos: "<< mdvspdata.numeroPuntosCorteViajes;
				
				//Mapa para la valicación de puntos de corte
				for(size_t i=0;i<mdvspdata.numeroPuntosCorteViajes;++i){				
					
					//Cargar el nombre del punto de corte de servicios
					string nombrePuntoCorte;
					data_input_puntos_corte >> nombrePuntoCorte;
					
					//Adicionarlo a la tabla hash correspondiente
					mdvspdata.codigosCorteViajes[nombrePuntoCorte] = i;
					
				}
				//Salida en pantalla
				cout<<endl<<"Codificación Puntos de Corte: ";
				for(auto& puntoCorte:mdvspdata.codigosCorteViajes){
					cout<<endl<<puntoCorte.second<<" - "<<puntoCorte.first;
				}
				//getchar();			
				
				
				//Cerrar el archivo de entrada de los puntos de cambio del caso CSP Integra S.A.
				data_input_puntos_corte.close();
				
				////////////////////////////////////////////////////////////////////////////////////////////////			
				
				//Abrir el archivo con la información de los viajes o servicios
				vector< viaje > viajesAuxiliar;
				ifstream data_input_viajes((testname + "servicios.csp").c_str());				
				if (! data_input_viajes.is_open() ) {
					cout << "Can not read the file " << (testname + "servicios.csp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}				
				
				//Número de viajes que se deben atender con la flota
				data_input_viajes >> mdvspdata.numeroViajes;
				if (mdvspdata.numeroViajes <= 0) cout << "Número de viajes <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Número de viajes que se deben atender: "<< mdvspdata.numeroViajes;								
				
				//Arreglo auxiliar para diferenciar las tablas (rutas) del caso Integra S.A.
				vector <viaje> bloqueRutaTabla;
				
				//Arreglo auxiliar general de servicios (primer paso de procesamiento de datos) caso Integra S.A.
				vector < vector <viaje> > serviciosAuxiliar;				
				
				//Variable para diferenciar la ruta sobre la cual se está trabajando
				string rutaTablaActual;				
				
				//Cargar en la estructura correspondiente los tiempos de los viajes
				mdvspdata.viajes.resize(mdvspdata.numeroViajes);
				for(size_t i=0;i<mdvspdata.numeroViajes;++i){
					
					//Salida de diagnóstico
					//cout<<endl<<"ITERACIÓN "<<i<<endl;
					//getchar();
					
					//Cargar en estructuras temporales la información contenida en el archivo para preprocesamiento
					viaje viajeTemp;
					viajeTemp.idViaje = i;
					//viaje viajeTempSiguiente;
					//viajeTempSiguiente.idViaje = i + 1;
					
					//Viaje actual
					data_input_viajes >> viajeTemp.idViajeArchivoEntrada;
					data_input_viajes >> viajeTemp.evento;
					data_input_viajes >> viajeTemp.nombreRuta;
					data_input_viajes >> viajeTemp.lugarInicio;
					data_input_viajes >> viajeTemp.tiempoInicio;
					data_input_viajes >> viajeTemp.tiempoFin;
					viajeTemp.duracionViaje = viajeTemp.tiempoFin - viajeTemp.tiempoInicio;					
					data_input_viajes >> viajeTemp.rutaTabla;
					
					////Viaje siguiente
					//data_input_viajes >> viajeTempSiguiente.evento;
					//data_input_viajes >> viajeTempSiguiente.nombreRuta;
					//data_input_viajes >> viajeTempSiguiente.lugarInicio;
					//data_input_viajes >> viajeTempSiguiente.tiempoInicio;
					//data_input_viajes >> viajeTempSiguiente.tiempoFin;
					//viajeTempSiguiente.duracionViaje = viajeTempSiguiente.tiempoFin - viajeTempSiguiente.tiempoInicio;					
					//data_input_viajes >> viajeTempSiguiente.rutaTabla;					
					
					//Cargar la estructura temporal en el contenedor general de la instancia o caso de estudio			
					mdvspdata.viajes[i] = viajeTemp;//Conservar para llenar las estructuras base
					
					////Filtrar los vaijes y permitir solamente los que correspondan a articulados
					//size_t busquedaT1 = mdvspdata.viajes[i].rutaTabla.find("T1");
					//size_t busquedaT2 = mdvspdata.viajes[i].rutaTabla.find("T2");
					//size_t busquedaT3 = mdvspdata.viajes[i].rutaTabla.find("T3");					
					//size_t busquedaTroncal_1 = mdvspdata.viajes[i].nombreRuta.find("Troncal_1");
					//size_t busquedaTroncal_2 = mdvspdata.viajes[i].nombreRuta.find("Troncal_2");
					//size_t busquedaTroncal_3 = mdvspdata.viajes[i].nombreRuta.find("Troncal_3");					
					////if(busquedaTroncal_1 == string::npos && busquedaTroncal_2 == string::npos && busquedaT1 == string::npos && busquedaT2 == string::npos){					
					//if(busquedaT1 != string::npos || busquedaT2 != string::npos || busquedaT3 != string::npos || busquedaTroncal_1 != string::npos || busquedaTroncal_2 != string::npos || busquedaTroncal_3 != string::npos){					
					//	
					//	//Adicionar el servicio
					//	mdvspdata.viajes[i] = viajeTemp;
					//	
					//	//Establecer el lugar de finalización del servicio
					//	
					//	
					//}
					
					//Validar si se está cargando el primer registro
					if(i==0){
						
						//Si es la primera vez, inicializar con la primera ruta presente en el archivo de entrada
						rutaTablaActual = viajeTemp.rutaTabla;
						
						//Adicionar el primer viaje
						bloqueRutaTabla.push_back(viajeTemp);
						
					}else{
						
						//Validar que pertenece a la misma ruta-tabla y que haya continuidad en los servicios (una espera máxima de dos minutos entre servicios), sinó se requiere una partición
						if(!bloqueRutaTabla.empty() && bloqueRutaTabla[bloqueRutaTabla.size()-1].rutaTabla == viajeTemp.rutaTabla && (viajeTemp.tiempoInicio - bloqueRutaTabla[bloqueRutaTabla.size()-1].tiempoFin) <= 2 ){
							
							//Adicionar el viaje al bloque en construcción
							bloqueRutaTabla.push_back(viajeTemp);
							
							//Adicionar la última tabla, porque en esta no sucede cambio
							if(i==mdvspdata.numeroViajes-1){
								
								//Realizar el cambio de ruta
								rutaTablaActual = viajeTemp.rutaTabla;
								
								//Almacenar el bloque de la ruta en la estructura auxiliar de procesamiento de servicios
								serviciosAuxiliar.push_back(bloqueRutaTabla);
								
								//Limpiar el contenedor porque se inicia una nueva ruta
								bloqueRutaTabla.clear();
								
								//Iniciar la construcción del nuevo bloque
								bloqueRutaTabla.push_back(viajeTemp);
								
							}
							
						}else{//Cambio de ruta
							
							//Realizar el cambio de ruta
							rutaTablaActual = viajeTemp.rutaTabla;
							
							//Almacenar el bloque de la ruta en la estructura auxiliar de procesamiento de servicios
							serviciosAuxiliar.push_back(bloqueRutaTabla);
							
							//Limpiar el contenedor porque se inicia una nueva ruta
							bloqueRutaTabla.clear();
							
							//Iniciar la construcción del nuevo bloque
							bloqueRutaTabla.push_back(viajeTemp);
						
						}//Fin del if que valida cambio de ruta en la carga de la información
						
					}//Fin del if que valida la carga del primer registro 					
					
				}//Fin del for que recorre los registros especificados en el archivo de entrada				
				
				//Cerrar el archivo de entrada del caso CSP Integra S.A.
				data_input_viajes.close();
				
				//Salida de diagnóstico
				cout<<endl<<"Terminó la carga del archivo de entrada!!!! ("<<mdvspdata.numeroViajes<<" registros cargados)"<<endl;
				cout<<endl<<"Número de bloques-ruta ("<<serviciosAuxiliar.size()<<")"<<endl;
				//getchar();
				
				//Limpiar el contenedor auxiliar para diferenciar rutas
				bloqueRutaTabla.clear();
				
				//Variable auxiliar para procesar la ruta antes de adicionarla a la estructura de servicios
				vector <viaje> auxiliarBloqueRutaTabla;
				
				//Contador general de servicios troncales (id general para viajes)
				size_t contadorGeneralServiciosTroncal = 0;
				
				//Recorrer los bloques de la estructura de procesamiento de servicios
				for(size_t i=0;i<serviciosAuxiliar.size();++i){
					
					//Cargar cada una de las rutas
					bloqueRutaTabla.clear();//Limpiar el contenedor auxiliar para diferenciar rutas
					bloqueRutaTabla = serviciosAuxiliar[i];
					
					//Limpiar el contenedor auxiliar de la segunda fase de procesamiento
					auxiliarBloqueRutaTabla.clear();
					
					/////////////////////////////////////////////////////////////////////////////
					////Salida de diagnóstico en pantalla de los servicios que se están construyendo
					//cout<<endl<<"Primer registro"<<endl;
					//cout<<"------------------------------------------"<<endl;
					//cout<<"Viaje "<<bloqueRutaTabla[0].idViaje<<endl;
					//cout<<"Evento = "<<bloqueRutaTabla[0].evento<<endl;
					//cout<<"T_inico = "<<bloqueRutaTabla[0].tiempoInicio<<endl;
					//cout<<"T_fin = "<<bloqueRutaTabla[0].tiempoFin<<endl;
					//cout<<"Duración Viaje = "<<bloqueRutaTabla[0].duracionViaje<<endl;
					//cout<<"Punto Salida = "<<bloqueRutaTabla[0].lugarInicio<<endl;
					//cout<<"Punto Llegada = "<<bloqueRutaTabla[0].lugarFinal<<endl;
					////cout<<"Ruta = "<<bloqueRutaTabla[0].rutaTabla.substr(0,2)<<endl;
					//cout<<"Ruta = "<<bloqueRutaTabla[0].rutaTabla<<endl;
					//cout<<endl;
					////Validación de la lectura de los viajes
					//if(bloqueRutaTabla[0].tiempoInicio > bloqueRutaTabla[0].tiempoFin){
					//	cout<<endl<<"------> ERROR (Infactibilidad en tiempo de inicio y tiempo de finalización)!!!!!!!!!"<<endl;getchar();
					//}
					////getchar();
					///////////////////////////////////////////////////////////////////////////////
					
					
					//Validar si son bloques dobles (diferentes de "Troncal_1" y "Troncal_2")
					size_t busquedaTroncal_1 = bloqueRutaTabla[0].nombreRuta.find("Troncal_1");
					size_t busquedaTroncal_2 = bloqueRutaTabla[0].nombreRuta.find("Troncal_2");
					size_t busquedaT1 = bloqueRutaTabla[0].nombreRuta.find("T1");
					size_t busquedaT2 = bloqueRutaTabla[0].nombreRuta.find("T2");
					if(busquedaTroncal_1 == string::npos && busquedaTroncal_2 == string::npos && busquedaT1 == string::npos && busquedaT2 == string::npos || true){								
						
						//Autonumérico para id de los servicios
						size_t autonumServicios = 0;
						
						//Procesar el bloque
						for(size_t ii = 0; ii<bloqueRutaTabla.size();++ii){							
							
							//Salida de diagnóstico
							//cout<<endl<<"Entra a iterar sobre el bloque"<<endl;							
							
							//Variable auxiliar para la construcción del servicio con las restricciones operativas
							viaje servicioTemporal;
							
							//Bandera para separar los servicios
							bool particionEncontrada = false;									
							
							//Construir servicio con las restricciones operativas
							servicioTemporal.idViajeArchivoEntrada = bloqueRutaTabla[ii].idViajeArchivoEntrada;
							servicioTemporal.idsViajesCubiertosArchivoEntrada.push_back(bloqueRutaTabla[ii].idViajeArchivoEntrada);
							//servicioTemporal.idViaje = autonumServicios;
							servicioTemporal.idViaje = contadorGeneralServiciosTroncal;
							servicioTemporal.evento = bloqueRutaTabla[ii].evento;
							servicioTemporal.nombreRuta = bloqueRutaTabla[ii].nombreRuta;
							servicioTemporal.lugarInicio = bloqueRutaTabla[ii].lugarInicio;
							servicioTemporal.tiempoInicio = bloqueRutaTabla[ii].tiempoInicio;
							servicioTemporal.duracionViaje = bloqueRutaTabla[ii].duracionViaje;
							servicioTemporal.rutaTabla = bloqueRutaTabla[ii].rutaTabla;									
							
							//Iterar hasta encontrar la partición del servicio
							while(!particionEncontrada){
								
								//Validación generalizada (validación con los lugares ingresados a través del archivo 'puntosCorteViajes.csp')
								bool banderaValidacionGeneralizadaParticionViajes = true;//Asumir que se trata del mismo viaje (que no es un punto de corte)
								for(auto& puntoCorte:mdvspdata.codigosCorteViajes){
									//Controlar desbordamiento antes de revisar
									if((ii+1) < bloqueRutaTabla.size()){
										//Si es un punto de corte, se debe cambiar de viaje y para esto se cambia el valor de la bandera
										if(bloqueRutaTabla[ii+1].lugarInicio == puntoCorte.first){
											banderaValidacionGeneralizadaParticionViajes = false;
											break;
										}
									}else{
										//Si hay desbordamiento, parar la revisión
										break;
									}
																	
								}
								
								//Validar que no se trata de una partición del servicio								
								//if( (ii+1) < bloqueRutaTabla.size() && bloqueRutaTabla[ii+1].lugarInicio != "Intercambiador_Cuba" && bloqueRutaTabla[ii+1].lugarInicio != "Intercambiador_DOSQ_Progreso" && bloqueRutaTabla[ii+1].lugarInicio != "Viajero" && bloqueRutaTabla[ii+1].lugarInicio != "Estación_Milán" && bloqueRutaTabla[ii+1].lugarInicio != "Patio_Integra" && bloqueRutaTabla[ii+1].lugarInicio != "Patios_Villa_ligia"){//Primera versión, problemas con los eventos y los patios
								//if( (ii+1) < bloqueRutaTabla.size() && bloqueRutaTabla[ii+1].lugarInicio != "Intercambiador_Cuba" && bloqueRutaTabla[ii+1].lugarInicio != "Intercambiador_DOSQ_Progreso" && bloqueRutaTabla[ii+1].lugarInicio != "Viajero" && bloqueRutaTabla[ii+1].lugarInicio != "Estacion_Milan" && bloqueRutaTabla[ii+1].evento != "Salida_de_Patio"){//Sgunda versión, condicionales quemados en código
								if( (ii+1) < bloqueRutaTabla.size() && banderaValidacionGeneralizadaParticionViajes && bloqueRutaTabla[ii+1].evento != "Salida_de_Patio"){//Tercera versión, validación de puntos de corte generalizada
									
									//Adicionar a los lugares del servicio
									servicioTemporal.lugarInicio += "-" + bloqueRutaTabla[ii+1].lugarInicio; 
									
									//Adicionar al arreglo de id's de los viajes o servicios cubiertos (id's provenientes del archivo de entrada servicios.csp)
									servicioTemporal.idsViajesCubiertosArchivoEntrada.push_back(bloqueRutaTabla[ii+1].idViajeArchivoEntrada);
									
									//Incrementar el tiempo del servicio
									servicioTemporal.duracionViaje += bloqueRutaTabla[ii+1].duracionViaje;
									
									//Saltar el siguiente, porque no se trataba de una partición
									++ii;
									
								}else{//Si el siguiente es una partición permitida del servicio
									
									//Evitar desbordamiento de memoria en los cambios de bloques asociados a rutas
									if( (ii+1) < bloqueRutaTabla.size() ){
										
										//El lugar final del servicio que se está construyendo, es el punto de salida del servicio siguiente
										servicioTemporal.lugarFinal = bloqueRutaTabla[ii+1].lugarInicio;										
										
									}else{
										
										//No hacer nada y dejar el lugar final del último registro
										
									}									
									
									//Actualizar el tiempo de finalización cuando ya se tiene la duración total del viaje
									servicioTemporal.tiempoFin = servicioTemporal.tiempoInicio + servicioTemporal.duracionViaje;
									
									//Adicionar el servicio construído a la estructura general											
									auxiliarBloqueRutaTabla.push_back(servicioTemporal);
									
									//Filtrar los vaijes y permitir solamente los que correspondan a articulados
									size_t busquedaT1 = servicioTemporal.rutaTabla.find("T1");
									size_t busquedaT2 = servicioTemporal.rutaTabla.find("T2");
									size_t busquedaT3 = servicioTemporal.rutaTabla.find("T3");					
									size_t busquedaTroncal_1 = servicioTemporal.nombreRuta.find("Troncal_1");
									size_t busquedaTroncal_2 = servicioTemporal.nombreRuta.find("Troncal_2");
									size_t busquedaTroncal_3 = servicioTemporal.nombreRuta.find("Troncal_3");														
									if(busquedaT1 != string::npos || busquedaT2 != string::npos || busquedaT3 != string::npos || busquedaTroncal_1 != string::npos || busquedaTroncal_2 != string::npos || busquedaTroncal_3 != string::npos){
									//if(busquedaT3 != string::npos || busquedaTroncal_3 != string::npos ){
										
										//if(servicioTemporal.evento != "Salida_de_Patio" && servicioTemporal.evento != "Entrada_Patio"){
										if(servicioTemporal.evento == "Opera"){
											viajesAuxiliar.push_back(servicioTemporal);
										
											//Contador general servicios troncal
											++contadorGeneralServiciosTroncal;	
										}
										
										
									}									
									
									//Actualizar el autonumérico de servicios
									++autonumServicios;
									
																	
									
									//Reportar a través de la bandera que la partición ha sido encontrada
									particionEncontrada = true;
									
								}//Fin del if que encuentra puntos para partir el servicio							
								
								
							}//Fin del while que funciona hasta encontrar partición de tabla de servicios								
							
							////Salida de diagnóstico en pantalla de los servicios que se están construyendo
							//cout<<endl<<"Tiempos de inicio y finalización de viajes"<<endl;
							//cout<<"------------------------------------------"<<endl;
							//cout<<"Viaje "<<servicioTemporal.idViaje<<endl;
							//cout<<"Evento = "<<servicioTemporal.evento<<endl;
							//cout<<"T_inico = "<<servicioTemporal.tiempoInicio<<endl;
							//cout<<"T_fin = "<<servicioTemporal.tiempoFin<<endl;
							//cout<<"Duración Viaje = "<<servicioTemporal.duracionViaje<<endl;
							//cout<<"Punto Salida = "<<servicioTemporal.lugarInicio<<endl;
							//cout<<"Punto Llegada = "<<servicioTemporal.lugarFinal<<endl;
							////cout<<"Ruta = "<<servicioTemporal.rutaTabla.substr(0,2)<<endl;
							//cout<<"Ruta = "<<servicioTemporal.rutaTabla<<endl;
							//cout<<"IDs Viajes cubiertos: "<<endl;
							//for(auto& idArchivoEntrada:servicioTemporal.idsViajesCubiertosArchivoEntrada){
							//	cout<<idArchivoEntrada<<" ";
							//}
							//cout<<endl;
							////Validación de la lectura de los viajes
							//if(servicioTemporal.tiempoInicio > servicioTemporal.tiempoFin){
							//	cout<<endl<<"------> ERROR (Infactibilidad en tiempo de inicio y tiempo de finalización)!!!!!!!!!"<<endl;getchar();
							//}
							////getchar();
							
							
						}//Fin del for que recorre el bloque
						
						//Adicionar el bloque con las restricciones operativas a la estructura del caso de estudio
						mdvspdata.servicios.push_back(auxiliarBloqueRutaTabla);
														
					}else{//De lo contrario, se trata de las rutas Troncal_1 o de Troncal_2
						
						////////////////////////Tratamiento de las troncales 1 y 2
						////////////////////////Tratamiento de las troncales 1 y 2
						////////////////////////Tratamiento de las troncales 1 y 2
						////////////////////////Tratamiento de las troncales 1 y 2
						////////////////////////Tratamiento de las troncales 1 y 2
						
					}//Fin del if que diferencia las rutas Troncal_1 y Troncal_2 de las demás							

					
					
										
				}//Fin del recorrido de la estructura de servicios auxiliar									
				
				
				//Obtener los viajes de la variable auxiliar
				mdvspdata.viajes = viajesAuxiliar;
				mdvspdata.numeroViajes = viajesAuxiliar.size();				
				
				//Salida de diagnóstico
				cout<<endl<<"Número de viajes (size): "<<mdvspdata.viajes.size();
				cout<<endl<<"Número de viajes (atributo): "<<mdvspdata.numeroViajes;
				//getchar();	
				
				//Ordenar viajes cronológicamente (desde el que inicia más temprano, hasta el que inicia más tarde)
				//sort(mdvspdata.viajes.begin(), mdvspdata.viajes.end(),ordenarViajesTiempoInicioAscendente());
				
				//Salida en pantalla y archivo de los viajes cargados
				ofstream archivoViajesCargados("viajesCargados.mdvsp", ios::binary);
				if (archivoViajesCargados.is_open()){
					//Colocar el nombre de la instancia en el archivo de salida
					archivoViajesCargados<<endl<<this->mdvspdata.nombreArchivoInstancia<<endl;
				}else{
					cout << "Fallo en la apertura del archivo de salida para los viajes cargados (viajesCargados.mdvsp)";
				}
				cout<<endl<<"Tiempos de inicio y finalización de viajes"<<endl;
				cout<<"------------------------------------------"<<endl;
				for(size_t i=0;i<mdvspdata.numeroViajes;++i){
					cout<<"Viaje "<<i+1<<endl;
					archivoViajesCargados<<"Viaje "<<i+1<<endl;
					cout<<"Id_Viaje = "<<mdvspdata.viajes[i].idViaje<<endl;
					archivoViajesCargados<<"Id_Viaje = "<<mdvspdata.viajes[i].idViaje<<endl;
					cout<<"T_inico = "<<mdvspdata.viajes[i].tiempoInicio<<endl;
					archivoViajesCargados<<"T_inico = "<<mdvspdata.viajes[i].tiempoInicio<<endl;
					cout<<"T_fin = "<<mdvspdata.viajes[i].tiempoFin<<endl;
					archivoViajesCargados<<"T_fin = "<<mdvspdata.viajes[i].tiempoFin<<endl;
					cout<<"Duración = "<<mdvspdata.viajes[i].duracionViaje<<endl;
					archivoViajesCargados<<"Duración = "<<mdvspdata.viajes[i].duracionViaje<<endl;
					cout<<"Punto Salida = "<<mdvspdata.viajes[i].lugarInicio<<endl;
					archivoViajesCargados<<"Punto Salida = "<<mdvspdata.viajes[i].lugarInicio<<endl;
					cout<<"Punto Llegada = "<<mdvspdata.viajes[i].lugarFinal<<endl;
					archivoViajesCargados<<"Punto Llegada = "<<mdvspdata.viajes[i].lugarFinal<<endl;
					cout<<"Evento = "<<mdvspdata.viajes[i].evento<<endl;
					archivoViajesCargados<<"Evento = "<<mdvspdata.viajes[i].evento<<endl;
					cout<<"Ruta = "<<mdvspdata.viajes[i].rutaTabla.substr(0,2)<<endl;
					archivoViajesCargados<<"Ruta = "<<mdvspdata.viajes[i].rutaTabla.substr(0,2)<<endl;
					cout<<endl;
					archivoViajesCargados<<endl;
					
					//Validación de la lectura de los viajes
					if(mdvspdata.viajes[i].tiempoInicio > mdvspdata.viajes[i].tiempoFin){
						cout<<endl<<"------> ERROR!!!!!!!!!"<<endl;getchar();
						archivoViajesCargados<<endl<<"------> ERROR!!!!!!!!!"<<endl;getchar();
					}
					
				}				
				//Cerrar el archivo para diagnóstico de viajes
				archivoViajesCargados.close();
				//getchar();			
				
				////Generación de depósito virtual para conexión con el algoritmo empleado en MDVSP				
				////Número de depósitos
				//mdvspdata.numeroDepositos = 1;				
				////Salida en pantalla
				//cout<<endl<<"Número de depósitos: "<< mdvspdata.numeroDepositos;			
				//
				////Número ilimitado de vehículos para dejar libre la selección de arcos del digrafo
				//mdvspdata.depositos.resize(mdvspdata.numeroDepositos);
				//mdvspdata.numeroTotalVehiculos = 0;
				//for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
				//	mdvspdata.depositos[i].nombreDeposito = "Depósito_Virtual";
				//	mdvspdata.depositos[i].numeroVehiculos = 100000000;
				//	mdvspdata.depositos[i].idDeposito = i;
				//	mdvspdata.numeroTotalVehiculos += mdvspdata.depositos[i].numeroVehiculos;
				//}
				////Salida en pantalla
				//cout<<endl;
				//for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
				//	cout<<"Vehículos disponibles en el depósito virtual "<<(i+1)<<": "<<mdvspdata.depositos[i].numeroVehiculos<<endl;
				//}
				
				////Salida en pantalla de los viajes
				//cout<<endl<<"Tiempos de inicio y finalización de viajes"<<endl;
				//cout<<"------------------------------------------"<<endl;
				//for(size_t i=0;i<mdvspdata.numeroViajes;++i){
				//	cout<<"Viaje "<<i+1<<endl;
				//	cout<<"T_inico = "<<mdvspdata.viajes[i].tiempoInicio<<endl;
				//	cout<<"T_fin = "<<mdvspdata.viajes[i].tiempoFin<<endl;
				//	cout<<"Punto Salida = "<<mdvspdata.viajes[i].lugarInicio<<endl;
				//	cout<<"Punto Llegada = "<<mdvspdata.viajes[i].lugarFinal<<endl;
				//	cout<<"Ruta = "<<mdvspdata.viajes[i].rutaTabla.substr(0,2)<<endl;
				//	cout<<endl;
				//	
				//	//Validación de la lectura de los viajes
				//	if(mdvspdata.viajes[i].tiempoInicio > mdvspdata.viajes[i].tiempoFin){
				//		cout<<endl<<"------> ERROR!!!!!!!!!"<<endl;getchar();
				//	}
				//	
				//}
				//getchar();
				
				////////////////////////////////////////////////////////////////////////////////////////////////				
				
				
				//Obtener la información de los puntos de cambio para los conductores del archivo correspondiente				
				ifstream data_input_puntos_cambio((testname + "puntosCambio.csp").c_str());				
				if (! data_input_puntos_cambio.is_open() ) {
					cout << "Can not read the file " << (testname + "puntosCambio.csp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				ifstream data_input_distancia_puntos_cambio((testname + "distanciaPuntosCambio.mdvsp").c_str());				
				if (! data_input_puntos_cambio.is_open() ) {
					cout << "Can not read the file " << (testname + "distanciaPuntosCambio.mdvsp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				
				//Obtener el número de puntos de cambio
				data_input_puntos_cambio >> mdvspdata.numeroPuntosCambio;
				if (mdvspdata.numeroPuntosCambio <= 0) cout << "Número de puntos de cambio <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Número de puntos de cambio: "<< mdvspdata.numeroPuntosCambio;
				
				//Mapa para la codificación de los puntos de cambio
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){				
					
					//Cargar el nombre del punto de cambio
					string nombrePuntoCambio;					
					data_input_puntos_cambio >> nombrePuntoCambio;
					
					//Adicionarlo a la tabla hash correspondiente
					mdvspdata.codigosPuntosCambio[nombrePuntoCambio] = i;
					
				}
				//Salida en pantalla
				cout<<endl<<"Codificación Puntos de Cambio: ";
				for(auto& puntoCambio:mdvspdata.codigosPuntosCambio){
					cout<<endl<<puntoCambio.second<<" - "<<puntoCambio.first;
				}
				
				//Redimensionar la matriz que tendrá los tiempos de desplazamiento en minutos para los diferentes puntos de cambio				
				mdvspdata.tiemposDesplazamientoPuntosCambio.reserve(mdvspdata.numeroPuntosCambio);
				mdvspdata.tiemposDesplazamientoPuntosCambio.resize(mdvspdata.numeroPuntosCambio);
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					mdvspdata.tiemposDesplazamientoPuntosCambio[i].reserve(mdvspdata.numeroPuntosCambio);
					mdvspdata.tiemposDesplazamientoPuntosCambio[i].resize(mdvspdata.numeroPuntosCambio);
				}						
				
				//Cargar los tiempos de desplazamiento en la matriz de puntos de cambio
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					for(size_t j=0;j<mdvspdata.numeroPuntosCambio;++j){
						data_input_puntos_cambio >> mdvspdata.tiemposDesplazamientoPuntosCambio[i][j];						 
					}
				}
				//Salida en pantalla
				cout<<endl<<"Tiempos de desplazamiento entre puntos: "<<endl;
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					for(size_t j=0;j<mdvspdata.numeroPuntosCambio;++j){
						cout<<mdvspdata.tiemposDesplazamientoPuntosCambio[i][j]<<" ";
					}
					cout<<endl;
				}
				//getchar();
				
				//Redimensionar la matriz que tendrá las distancias en metros entre los diferentes puntos de cambio				
				mdvspdata.distanciasEntrePuntosCambio.reserve(mdvspdata.numeroPuntosCambio);
				mdvspdata.distanciasEntrePuntosCambio.resize(mdvspdata.numeroPuntosCambio);
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					mdvspdata.distanciasEntrePuntosCambio[i].reserve(mdvspdata.numeroPuntosCambio);
					mdvspdata.distanciasEntrePuntosCambio[i].resize(mdvspdata.numeroPuntosCambio);
				}	
				
				//Cargar las distancias en la matriz de distancias entre puntos de cambio
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					for(size_t j=0;j<mdvspdata.numeroPuntosCambio;++j){
						data_input_distancia_puntos_cambio >> mdvspdata.distanciasEntrePuntosCambio[i][j];						 
					}
				}
				//Salida en pantalla
				cout<<endl<<"Distancias entre puntos (m): "<<endl;
				for(size_t i=0;i<mdvspdata.numeroPuntosCambio;++i){
					for(size_t j=0;j<mdvspdata.numeroPuntosCambio;++j){
						cout<<mdvspdata.distanciasEntrePuntosCambio[i][j]<<" ";
					}
					cout<<endl;
				}
				//getchar();
				
				//Cerrar el archivo de entrada de los puntos de cambio del caso CSP Integra S.A.
				data_input_puntos_cambio.close();				
				
				//Cerrar el archivo de entrada de las distancias entre los puntos de cambio del caso CSP Integra S.A.
				data_input_distancia_puntos_cambio.close();
				
				////////////////////////////////////////////////////////////////////////////////////////////////				
				
				//Obtener los parámetros generales del algoritmo
				ifstream data_input_parametros_generales((testname + "parametrosGenerales.csp").c_str());
				if (! data_input_parametros_generales.is_open() ) {
					cout << "Can not read the file " << (testname + "parametrosGenerales.csp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}
				
				//Obtener la duración de los bloques de trabajo
				data_input_parametros_generales >> mdvspdata.duracionBloqueTrabajo;
				if (mdvspdata.duracionBloqueTrabajo <= 0) cout << "Duración de bloques de trabajo <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Duración de bloques de trabajo: "<< mdvspdata.duracionBloqueTrabajo <<"("<<double(mdvspdata.duracionBloqueTrabajo/60)<<" horas)";
				
				//Obtener la duración del descanso reglamentario (40 minutos)
				data_input_parametros_generales >> mdvspdata.tiempoDescansoReglamentario;
				if (mdvspdata.tiempoDescansoReglamentario <= 0) cout << "Tiempo de descanso reglamentario <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Tiempo de descanso reglamentario: "<< mdvspdata.tiempoDescansoReglamentario;			
				
				//Obtener el número de conductores con Licencia 2
				data_input_parametros_generales >> mdvspdata.numeroConductoresLicenciaC2;
				if (mdvspdata.numeroConductoresLicenciaC2 <= 0) cout << "Número de conductores con Licencia C2 <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Número de conductores con Licencia C2: "<< mdvspdata.numeroConductoresLicenciaC2;
				
				//Obtener el número de conductores con Licencia 3
				data_input_parametros_generales >> mdvspdata.numeroConductoresLicenciaC3;
				if (mdvspdata.numeroConductoresLicenciaC3 <= 0) cout << "Número de conductores con Licencia C3 <= 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Número de conductores con Licencia C3: "<< mdvspdata.numeroConductoresLicenciaC3;				
				
				//Obtener la hora de inicio de la jornada (8:00 am en minutos)
				data_input_parametros_generales >> mdvspdata.inicioJornada;				
				if (mdvspdata.inicioJornada < 0) cout << "Hora de Inicio de la Jornada < 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Hora de Inicio de la Jornada (minutos): "<< mdvspdata.inicioJornada;				
				
				//Obtener la hora de finalización de la jornada (10:00 pm en minutos)	
				data_input_parametros_generales >> mdvspdata.finalJornada;							
				if (mdvspdata.finalJornada < 0) cout << "Hora de Finalización de la Jornada < 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Hora de Finalización de la Jornada (minutos): "<< mdvspdata.finalJornada;			
				
				//Número de horas mínimo de un turno de trabajo en minutos
				data_input_parametros_generales >> mdvspdata.limiteInferiorTiempoTurno;							
				if (mdvspdata.limiteInferiorTiempoTurno < 0) cout << "Límite inferior de horas de turno de trabajo < 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Límite inferior de horas de turno de trabajo (minutos): "<< mdvspdata.limiteInferiorTiempoTurno;			
				
				//Número de horas máximo de un turno de trabajo en minutos
				data_input_parametros_generales >> mdvspdata.limiteSuperiorTiempoTurno;							
				if (mdvspdata.limiteSuperiorTiempoTurno < 0) cout << "Límite superior de horas de turno de trabajo < 0" << endl;
				//Salida en pantalla
				cout<<endl<<"Límite superior de horas de turno de trabajo (minutos): "<< mdvspdata.limiteSuperiorTiempoTurno;										
				
				//Bandera de restricciones de trasnocho la duración de los bloques de trabajo
				data_input_parametros_generales >> mdvspdata.restriccionMadrugadaTrasnocho;
				if (mdvspdata.restriccionMadrugadaTrasnocho < 0) cout << "Bandera de restricción de turnos extensos (madrugada y trasnocho)" << endl;
				//Salida en pantalla
				cout<<endl<<"Restricción de turnos extensos: ";
				if(mdvspdata.restriccionMadrugadaTrasnocho){
					cout<<"Activa";
				}else{
					cout<<"Inactiva";
				}
				cout<<endl;
				//getchar();	
				
				//Cerrar el archivo de entrada de parámetros generales
				data_input_parametros_generales.close();
				
				////////////////////////////////////////////////////////////////////////////////////////////////				
				
				//Obtener información de los depósitos del caso
				ifstream data_input_depositos((testname + "depositos.mdvsp").c_str());
				if (! data_input_depositos.is_open() ) {
					cout << "Can not read the file " << (testname + "depositos.mdvsp") << endl;
					//return EXIT_FAILURE;
					//return 1;
				}			
				
				//Número de depósitos
				data_input_depositos >> mdvspdata.numeroDepositos;
				
				//Salida en pantalla
				cout<<endl<<"Número de depósitos: "<< mdvspdata.numeroDepositos;			
				
				//Cargar los detalles de cada depósito
				mdvspdata.depositos.resize(mdvspdata.numeroDepositos);
				mdvspdata.numeroTotalVehiculos = 0;
				for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
					data_input_depositos >> mdvspdata.depositos[i].nombreDeposito;
					data_input_depositos >> mdvspdata.depositos[i].numeroVehiculos;
					mdvspdata.depositos[i].idDeposito = i;
					mdvspdata.numeroTotalVehiculos += mdvspdata.depositos[i].numeroVehiculos;
					//mdvspdata.depositos[i].nombreDeposito = "Depósito_Virtual";
					//mdvspdata.depositos[i].numeroVehiculos = 100000000;
					//mdvspdata.depositos[i].idDeposito = i;
					//mdvspdata.numeroTotalVehiculos += mdvspdata.depositos[i].numeroVehiculos;
				}
				//Salida en pantalla
				cout<<endl;
				for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
					cout<<"Vehículos disponibles en el depósito virtual "<<(i+1)<<": "<<mdvspdata.depositos[i].numeroVehiculos<<endl;
				}
				//getchar();		
				
				//Cerrar el archivo con la información de los depósitos
				data_input_depositos.close();
				
				////////////////////////////////////////////////////////////////////////////////////////////////
				
				//Construcción de la matriz de costos
				
				//Redimensionar matriz de costos (explícita)
				double infactible = 100000000;//Bandera de infactibilidad
				int numeroAristasInfactibles = 0;
				int dimensionMatrizViajes = mdvspdata.numeroDepositos + mdvspdata.numeroViajes;//Matriz cuadrada
				mdvspdata.matrizViajes.resize(dimensionMatrizViajes);
				for(size_t i=0;i<dimensionMatrizViajes;++i){
					vector < double > arregloCambiosContexto(dimensionMatrizViajes);
					for(size_t j=0;j<dimensionMatrizViajes;++j){
						
						
						//Si se están relacionando depósitos entre sí, infactibilizar
						if(i < mdvspdata.numeroDepositos && j < mdvspdata.numeroDepositos && i!=j){
							arregloCambiosContexto[j] = infactible;
							++numeroAristasInfactibles;
						}else{
							
							//Si se está relacionando un depósito con él mismo
							if(i < mdvspdata.numeroDepositos && j < mdvspdata.numeroDepositos && i==j){								
								
								//arregloCambiosContexto[j] = 0;//0 unidades de costo
								arregloCambiosContexto[j] = infactible;
								++numeroAristasInfactibles;
																
							}else{//Los demás casos, viajes entre sí y depósitos con viajes
								
								//Caso en el que se sale de un depósito a atender un viaje
								if(i<mdvspdata.numeroDepositos){									
									
									//Obtener el código del punto de salida
									size_t puntoSalida = mdvspdata.codigosPuntosCambio[ mdvspdata.depositos[i].nombreDeposito ];
									//Obtener el código del punto de llegada (inicio de un servicio)
									char * cstr = new char [ mdvspdata.viajes[ j - mdvspdata.numeroDepositos ].lugarInicio.length()+1 ];
									strcpy (cstr, mdvspdata.viajes[ j - mdvspdata.numeroDepositos ].lugarInicio.c_str());
									string lugarInicio = strtok(cstr,"-");
									size_t puntoLLegada = mdvspdata.codigosPuntosCambio[ lugarInicio ];									
									
									//if(!mdvspdata.viajes[j].lugarFinal.empty()){
									//	puntoLLegada = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[j].lugarFinal ];
									//}else{									
									//	char * cstr_final = new char [ mdvspdata.viajes[j].lugarInicio.length()+1 ];
									//	strcpy (cstr_final, mdvspdata.viajes[j].lugarInicio.c_str());				
									//	char * p = strtok (cstr_final,"-");
									//	while (p!=0){
									//		mdvspdata.viajes[j].lugarFinal = p;
									//		p = strtok(NULL,"-");
									//	}
									//	puntoLLegada = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[j].lugarFinal ];
									//	
									//}						
									
									//Costo por tiempos
									//arregloCambiosContexto[j] = mdvspdata.tiemposDesplazamientoPuntosCambio[puntoSalida][puntoLLegada];																	
									//Costo por distancias
									arregloCambiosContexto[j] = mdvspdata.distanciasEntrePuntosCambio[puntoSalida][puntoLLegada];																	
								
								//Caso en el que se llega a un depósito después de atender un viaje
								}else if(j<mdvspdata.numeroDepositos){
									
									//Obtener el código del punto de salida
									size_t puntoSalida;
									if(!mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarFinal.empty()){
										puntoSalida = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[ i - mdvspdata.numeroDepositos ].lugarFinal ];
									}else{
										char * cstr_final = new char [ mdvspdata.viajes[ i - mdvspdata.numeroDepositos ].lugarInicio.length()+1 ];
										strcpy (cstr_final, mdvspdata.viajes[i].lugarInicio.c_str());				
										char * p = strtok (cstr_final,"-");
										while (p!=0){
											mdvspdata.viajes[ i - mdvspdata.numeroDepositos ].lugarFinal = p;
											p = strtok(NULL,"-");
										}
										puntoSalida = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[ i - mdvspdata.numeroDepositos ].lugarFinal ];
										
									}								
									//Obtener el código del punto de llegada
									size_t puntoLLegada = mdvspdata.codigosPuntosCambio[ mdvspdata.depositos[j].nombreDeposito ];
									
									//Costo por tiempos
									//arregloCambiosContexto[j] = mdvspdata.tiemposDesplazamientoPuntosCambio[puntoSalida][puntoLLegada];																	
									//Costo por distancias
									arregloCambiosContexto[j] = mdvspdata.distanciasEntrePuntosCambio[puntoSalida][puntoLLegada];
								
								//Caso en el que se relacionan viajes o servicios entre sí
								}else{
									
									//Obtener el código del punto de salida
									size_t puntoSalida;
									if(!mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarFinal.empty()){
										puntoSalida = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarFinal ];
									}else{
										char * cstr_final = new char [ mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarInicio.length()+1 ];
										strcpy (cstr_final, mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarInicio.c_str());				
										char * p = strtok (cstr_final,"-");
										while (p!=0){
											mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarFinal = p;
											p = strtok(NULL,"-");
										}
										puntoSalida = mdvspdata.codigosPuntosCambio[ mdvspdata.viajes[i - mdvspdata.numeroDepositos].lugarFinal ];
										
									}																	
									//Obtener el código del punto de llegada (inicio de un servicio)
									char * cstr = new char [ mdvspdata.viajes[ j - mdvspdata.numeroDepositos ].lugarInicio.length()+1 ];
									strcpy (cstr, mdvspdata.viajes[ j - mdvspdata.numeroDepositos ].lugarInicio.c_str());
									string lugarInicio = strtok(cstr,"-");
									size_t puntoLLegada = mdvspdata.codigosPuntosCambio[ lugarInicio ];									
																		
									//Revisar factibilidad de itinerarios (tiempos de finalización del servicio i + tiempo de desplazamiento ij <= inicio del servicio o viaje j)
									if( mdvspdata.viajes[i - mdvspdata.numeroDepositos].tiempoFin + mdvspdata.tiemposDesplazamientoPuntosCambio[puntoSalida][puntoLLegada] <=  mdvspdata.viajes[j - mdvspdata.numeroDepositos].tiempoInicio){
										//Costo por tiempo									
										//arregloCambiosContexto[j] = mdvspdata.tiemposDesplazamientoPuntosCambio[puntoSalida][puntoLLegada];
										//Costo por distancias
										arregloCambiosContexto[j] = mdvspdata.distanciasEntrePuntosCambio[puntoSalida][puntoLLegada];
									}else{
										
										//Salida de diagnóstico
										if(puntoLLegada == puntoSalida && mdvspdata.viajes[i - mdvspdata.numeroDepositos].tiempoFin <= mdvspdata.viajes[j - mdvspdata.numeroDepositos].tiempoInicio){
											cout<<endl<<"Error!";getchar();
										}
										
										//Infactible, porque no se cumple la restricción de tiempo entre viajes para formar parte del mismo itinerario
										arregloCambiosContexto[j] = infactible;
										++numeroAristasInfactibles;
									}
									
								}				
								
								
							}//Fin del if que valida relación bucle para cada depósito
											
							
						}//Fin del if que valida la relación entre depósitos (prohibida)						
						
					}//Fin del for que construye la fila de la matriz (subíndice j)
					
					//Actualizar la fila de la matriz de costos de cambio de contexto
					mdvspdata.matrizViajes[i] = arregloCambiosContexto;
					
				}//Fin del for que construye la matriz fila a fila (subíndice i)			
				
				//Salida de diagnóstico				
				//ofstream archivoMatrizCostos("matrizCostos.csv",ios::app | ios::binary);		
				ofstream archivoMatrizCostos("matrizCostos.csv", ios::binary);		
				if (archivoMatrizCostos.is_open()){
					//Colocar el nombre de la instancia en el archivo de salida
					archivoMatrizCostos<<endl<<this->mdvspdata.nombreArchivoInstancia<<endl;
				}else{
					cout << "Fallo en la apertura del archivo de salida.";
				}
				for(size_t i=0;i<mdvspdata.matrizViajes.size();++i){
					for(size_t j=0;j<mdvspdata.matrizViajes[i].size();++j){
						archivoMatrizCostos<<mdvspdata.matrizViajes[i][j]<<";";
					}
					archivoMatrizCostos<<endl;
				}
				//Cerrar el archivo de salida al finalizar el constructivo
				archivoMatrizCostos.close();
				
				//Salida de diagnóstico
				int numeroAristasGrafo = (mdvspdata.numeroViajes+mdvspdata.numeroDepositos)*(mdvspdata.numeroViajes+mdvspdata.numeroDepositos);
				cout<<"Porcentaje de aristas desactivadas -> "<<(double(numeroAristasInfactibles)/double(numeroAristasGrafo))*100.0;
				//getchar();				
				
				//Preparar contenedor de solución a partir de la información cargada de la instancia
				solucionmdvsp.flotaVehiculos.resize(mdvspdata.numeroTotalVehiculos);
				solucionmdvsp.numeroTotalVehiculosEmpleados = 0;
				//Asignar código a los vehículos de la solución y diferenciar el depósito al cual pertenecen
				int autonumericoIdVehiculos = 0;
				for(size_t i=0;i<mdvspdata.depositos.size();++i){
					for(size_t j=0;j<mdvspdata.depositos[i].numeroVehiculos;++j){
						solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idDeposito = mdvspdata.depositos[i].idDeposito;  
						solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idVehiculo = autonumericoIdVehiculos;
						++autonumericoIdVehiculos;
					}
				}
				
				//Salida del caso de Integra al formato solicitado para algoritmo de caminos más cortos (Luciano Costa)
				ofstream archivoIntegraFormatoShortestPaths("caseIntegraSA.mdvsp", ios::binary);		
				if (archivoIntegraFormatoShortestPaths.is_open()){
					//Colocar el nombre de la instancia en el archivo de salida				
					//archivoIntegraFormatoShortestPaths<<endl<<this->mdvspdata.nombreArchivoInstancia<<endl;
				}else{
					cout << "Fallo en apertura del archivo de salida caseIntegraSA.mdvsp";
				}
				//Número de depósitos				
				archivoIntegraFormatoShortestPaths<<mdvspdata.numeroDepositos<<endl;
				//Número de vehículos por depósito
				for(auto& deposito_i:mdvspdata.depositos){
					archivoIntegraFormatoShortestPaths<<deposito_i.numeroVehiculos<<endl;
				}
				//Número de viajes (clientes)
				archivoIntegraFormatoShortestPaths<<mdvspdata.numeroViajes<<endl;			
				//Viajes (clientes) (Id open_time close_time service_time)
				int idViajesCasoIntegra = 0;
				for(auto& viaje_i:mdvspdata.viajes){
					archivoIntegraFormatoShortestPaths<<idViajesCasoIntegra<<" "<<viaje_i.tiempoInicio<<" "<<viaje_i.tiempoFin<<" "<<viaje_i.duracionViaje<<endl;
					++idViajesCasoIntegra;
				}
				//Matriz costos				
				for(size_t i=0;i<mdvspdata.matrizViajes.size();++i){
					for(size_t j=0;j<mdvspdata.matrizViajes[i].size();++j){
						archivoIntegraFormatoShortestPaths<<mdvspdata.matrizViajes[i][j]<<" ";
					}
					archivoIntegraFormatoShortestPaths<<endl;
				}
				//Cerrar el archivo de salida al finalizar el constructivo
				archivoIntegraFormatoShortestPaths.close();
				
				////Recorrer la matriz en formato "row-wise" para pasarla a la matriz de costos explícita
				//int contadorCostos = 0;
				//vector<double> filaTemp;
				//double costoTemp;
				//for(size_t i=0;i<(mdvspdata.numeroViajes + mdvspdata.numeroDepositos)*(mdvspdata.numeroViajes + mdvspdata.numeroDepositos);++i){
				//	data_input_cst >> costoTemp;
				//	filaTemp.push_back(costoTemp);
				//	if(filaTemp.size() == mdvspdata.numeroViajes + mdvspdata.numeroDepositos){
				//		mdvspdata.matrizViajes.push_back(filaTemp);
				//		filaTemp.clear();
				//	}
				//}
				//////Salida en pantalla (pasar la matriz a un editor de texto plano para revisar su contenido)
				//////getchar();
				////cout<<endl;
				////for(size_t i=0;i<mdvspdata.matrizViajes.size();++i){
				////	for(size_t j=0;j<mdvspdata.matrizViajes[i].size();++j){
				////		cout<<mdvspdata.matrizViajes[i][j]<<" ";
				////	}
				////	cout<<endl;
				////}
				
				
				
				//Primera versión de lectura de casos reales//
				
				///////////////////////
				//// Lectura de datos//
				///////////////////////
				//
				////Abrir el archivo con la información de los depósitos
				//ifstream data_input_depositos((testname + "depositos.dat").c_str());				
				//if (! data_input_depositos.is_open() ) {
				//	cout << "Can not read the file " << (testname + "depositos.dat") << endl;
				//	//return EXIT_FAILURE;
				//	//return 1;
				//}
				//
				////Nombre de la instancia
				//mdvspdata.nombreArchivoInstancia = "ArticuladosMEGABUS";				
				//
				////Número de depósitos
				//data_input_depositos >> mdvspdata.numeroDepositos;
				//if (mdvspdata.numeroDepositos <= 0) cout << "Número de depósitos <= 0" << endl;
				////Salida en pantalla
				//cout<<endl<<"Número de depósitos: "<< mdvspdata.numeroDepositos;
				//
				////Por cada depósito almacenar el número de vehículos de los que dispone y el tamaño total de la flota para la instancia
				//mdvspdata.depositos.resize(mdvspdata.numeroDepositos);
				//mdvspdata.numeroTotalVehiculos = 0;
				//for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
				//	data_input_depositos >> mdvspdata.depositos[i].nombreDeposito;
				//	data_input_depositos >> mdvspdata.depositos[i].numeroVehiculos;
				//	mdvspdata.depositos[i].idDeposito = i;
				//	mdvspdata.numeroTotalVehiculos += mdvspdata.depositos[i].numeroVehiculos;
				//}
				////Salida en pantalla
				//cout<<endl;
				//for(size_t i = 0;i<mdvspdata.numeroDepositos;++i){
				//	cout<<"Vehículos disponibles en el depósito "<<(i+1)<<": "<<mdvspdata.depositos[i].numeroVehiculos<<endl;
				//}
				//
				////Cerrar el archivo que contiene la información de los depósitos
				//data_input_depositos.close();
				//
				/////////////////////////////////////////////////////////////////////				
				//
				////Abrir el archivo con la información de los viajes
				//ifstream data_input_viajes((testname + "viajes.dat").c_str());				
				//if (! data_input_viajes.is_open() ) {
				//	cout << "Can not read the file " << (testname + "viajes.dat") << endl;
				//	//return EXIT_FAILURE;
				//	//return 1;
				//}
				//
				////Número de viajes que se deben atender con la flota
				//data_input_viajes >> mdvspdata.numeroViajes;
				//if (mdvspdata.numeroViajes <= 0) cout << "Número de viajes <= 0" << endl;
				////Salida en pantalla
				//cout<<endl<<"Número de viajes que se deben atender: "<< mdvspdata.numeroViajes;								
				//
				////Cargar en la estructura correspondiente los tiempos de los viajes
				//mdvspdata.viajes.resize(mdvspdata.numeroViajes);
				//for(size_t i=0;i<mdvspdata.numeroViajes;++i){
				//	//Cargar en una estructura temporal la información contenida en el archivo
				//	viaje viajeTemp;
				//	viajeTemp.idViaje = i;					
				//	data_input_viajes >> viajeTemp.lugarInicio;
				//	data_input_viajes >> viajeTemp.rutaTabla;
				//	data_input_viajes >> viajeTemp.tiempoInicio;
				//	data_input_viajes >> viajeTemp.tiempoFin;
				//	//Establecer el lugar de llegada dependiendo del punto de partida y la ruta
				//	string ruta = viajeTemp.rutaTabla.substr(0,2);
				//	if( ruta != "T3" ){
				//		if( viajeTemp.lugarInicio == "Intercambiador_DOSQ_Progreso" ){
				//			viajeTemp.lugarFinal = "Intercambiador_Cuba";
				//		}else{
				//			viajeTemp.lugarFinal = "Intercambiador_DOSQ_Progreso";
				//		}						
				//	}else{
				//		if( viajeTemp.lugarInicio == "Intercambiador_Cuba" ){
				//			viajeTemp.lugarFinal = "Libertad";
				//		}else{
				//			viajeTemp.lugarFinal = "Intercambiador_Cuba";
				//		}					
				//	}					
				//	//Cargar la estructura temporal en el contenedor general de la instancia o caso de estudio			
				//	mdvspdata.viajes[i] = viajeTemp;
				//}
				//
				////Salida en pantalla de los viajes
				//cout<<endl<<"Tiempos de inicio y finalización de viajes"<<endl;
				//cout<<"------------------------------------------"<<endl;
				//for(size_t i=0;i<mdvspdata.numeroViajes;++i){
				//	cout<<"Viaje "<<i+1<<endl;
				//	cout<<"T_inico = "<<mdvspdata.viajes[i].tiempoInicio<<endl;
				//	cout<<"T_fin = "<<mdvspdata.viajes[i].tiempoFin<<endl;
				//	cout<<"Punto Salida = "<<mdvspdata.viajes[i].lugarInicio<<endl;
				//	cout<<"Punto Llegada = "<<mdvspdata.viajes[i].lugarFinal<<endl;
				//	cout<<"Ruta = "<<mdvspdata.viajes[i].rutaTabla.substr(0,2)<<endl;
				//	cout<<endl;
				//	
				//	//Validación de la lectura de los viajes
				//	if(mdvspdata.viajes[i].tiempoInicio > mdvspdata.viajes[i].tiempoFin){
				//		cout<<endl<<"------> ERROR!!!!!!!!!"<<endl;getchar();
				//	}
				//	
				//}
				//getchar();			
				//				
				////Cerrar el archivo que contiene la información de los viajes
				//data_input_viajes.close();				
				//
				//
				//////Redimensionar matriz de costos (explícita)
				//////mdvspdata.matrizViajes.resize(mdvspdata.numeroViajes + mdvspdata.numeroDepositos);
				//////Recorrer la matriz en formato "row-wise" para pasarla a la matriz de costos explícita
				////int contadorCostos = 0;
				////vector<double> filaTemp;
				////double costoTemp;
				////for(size_t i=0;i<(mdvspdata.numeroViajes + mdvspdata.numeroDepositos)*(mdvspdata.numeroViajes + mdvspdata.numeroDepositos);++i){
				////	data_input_cst >> costoTemp;
				////	filaTemp.push_back(costoTemp);
				////	if(filaTemp.size() == mdvspdata.numeroViajes + mdvspdata.numeroDepositos){
				////		mdvspdata.matrizViajes.push_back(filaTemp);
				////		filaTemp.clear();
				////	}
				////}
				////////Salida en pantalla (pasar la matriz a un editor de texto plano para revisar su contenido)
				////////getchar();
				//////cout<<endl;
				//////for(size_t i=0;i<mdvspdata.matrizViajes.size();++i){
				//////	for(size_t j=0;j<mdvspdata.matrizViajes[i].size();++j){
				//////		cout<<mdvspdata.matrizViajes[i][j]<<" ";
				//////	}
				//////	cout<<endl;
				//////}		
				//							
				//
				//////Preparar contenedor de solución a partir de la información cargada de la instancia
				////solucionmdvsp.flotaVehiculos.resize(mdvspdata.numeroTotalVehiculos);
				//////Asignar código a los vehículos de la solución y diferenciar el depósito al cual pertenecen
				////int autonumericoIdVehiculos = 0;
				////for(size_t i=0;i<mdvspdata.depositos.size();++i){
				////	for(size_t j=0;j<mdvspdata.depositos[i].numeroVehiculos;++j){
				////		solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idDeposito = mdvspdata.depositos[i].idDeposito;  
				////		solucionmdvsp.flotaVehiculos[autonumericoIdVehiculos].idVehiculo = autonumericoIdVehiculos;
				////		++autonumericoIdVehiculos;
				////	}
				////}
				
				//Contenedores de restauración
				solucionmdvspRestaurar = solucionmdvsp;
				mdvspdataRestaurar = mdvspdata;				
				
			}break;
			
			//Reportar error en ID de variante de VSP
			default: 
				cout<<endl<< "ID de variante de vehicle scheduling inválida!"<<endl;
			break;
			
		
		}//Fin del switch
		
		
		
	}//Fin del constructor general
	
	
	//Implementación en CPLEX del modelo de asignación generalizada para proceder a asignar depósitos a servicios o a itinerarios
	void modeloGAP(secuenciaAtencion secuenciaServicios, double &costoG, double &costoSpuerG, double &tiempoSolucionModeloAsignacionGeneralizada, bool &banderaFactibilidadVehiculos, vector< vector<int> > &aristasDepositos ){
		
		//Declaración de variables locales
		struct timeval t_inicio, t_actual;
		double tiempolimite;
		long int n = secuenciaServicios.itinerarios.size();//Número de servicios que se deben asignar (servicios o itinerarios en el caso de super nodos)
		long int m = this->mdvspdata.numeroDepositos;//Número de depósitos		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver					
		
		IloTimer tiempo(env); //Tiempo de ejecución (Sólo cuenta luego de la lectura de datos)	
		//tiempo.reset(), tiempo.start(); tiempo.start(); //Instrucciones del cronómetro interno
		
		//Creación de variables x
		//x_ij = 1, si el depósito 'j' atiende al servicio 'i'. De lo contrario 0
		IloArray<IloNumVarArray> x(env, n);
		std::stringstream name;
		for(auto i = 0u; i < n; ++i) {
			x[i] = IloNumVarArray(env, m);
			for(auto j = 0u; j < m; ++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
				name.str(""); // Clean name
			}
		}			
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//Conjunto de restricciones para controlar que cada itinerario sea atendido por uno y sólo uno de los depósitos
		IloRangeArray atencionServicioDepositoUnico(env, n);
		stringstream nombreRestriccionesAtencionServicioDepositoUnico;
		for(auto i = 0u; i < n; ++i){//Una restricción por cada uno de los servicios o itinerarios
			//Crear una expresión por cada restricción, adicionarla al modelo y reiniciarla
			IloExpr restriccion(env);
			for(auto j = 0u; j < m; ++j) {
				restriccion += x[i][j];//Un término en la restricción por cada uno de los depósitos que atenderán servicios o itinerarios
			}
			nombreRestriccionesAtencionServicioDepositoUnico << "ServiceDepotOnce_" << i;
			atencionServicioDepositoUnico[i] = IloRange(env, 1, restriccion, 1, nombreRestriccionesAtencionServicioDepositoUnico.str().c_str());
			nombreRestriccionesAtencionServicioDepositoUnico.str(""); // Limpiar variable para etiquetas de las restricciones
		}
		//Adicionar el conjunto de restricciones que controlan que todos los servicios sean atendidos y por un solo depósito
		modelo.add(atencionServicioDepositoUnico);	
		
		//Conjunto de restricciones para limitar el uso de vehículos de cada uno de los depósitos
		IloRangeArray flotaDepositos(env, m);
		stringstream nombreRestriccionFlotaDisponible;
		for(auto j = 0u; j < m; ++j){//Una restricción por cada uno de los depósitos
			//Crear una expresión por cada restricción, adicionarla al modelo y reiniciarla
			IloExpr restriccion(env);
			for(auto i = 0u; i < n; ++i) {
				restriccion += x[i][j];//Un término en la restricción por cada uno de los servicios que serán atendidos por el depósito 'j' (a_ij = 1, porque un servicio consume sólo un vehículo)
			}
			restriccion -= this->mdvspdata.depositos[j].numeroVehiculos;//Limitar con el número de vehículos (flota) disponibles en el depósito 'j'			
			nombreRestriccionFlotaDisponible << "FleetConstraintDepot_"<<j;
			flotaDepositos[j] = IloRange(env, -IloInfinity, restriccion, 0, nombreRestriccionFlotaDisponible.str().c_str());
			nombreRestriccionFlotaDisponible.str(""); // Limpiar variable para etiquetas de las restricciones
			
			
		}
		//Adicionar el conjunto de restricciones que controlan que todos los servicios sean atendidos y por un solo depósito
		modelo.add(flotaDepositos);		
		
						
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (GAP)
		//******************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);				
		
		//Costos de asignación entre los depósitos y los servicios (o conjuntos de servicios)
		for(auto i=0u;i<n;++i){				
			for(auto j=0u;j<m;++j){				
				
				//Costo de asignación c_ij (c_ij = costo salida al primer viaje del itinerario + costo de regreso del último viaje del itinerario)
				double costoAsignacion = 0;
								
				//Apuntar al primer viaje del itinerario (super nodo)
				list<viaje>::iterator itPrimerViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.begin();
				//Apuntar al último viaje del itinerario (super nodo)
				list<viaje>::iterator itUltimoViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.end();
				--itUltimoViajeSuperNodo;				
				
				//Salida del depósito a atender este itinerario o servicio
				costoAsignacion += this->mdvspdata.matrizViajes[j][this->mdvspdata.numeroDepositos +  (*itPrimerViajeSuperNodo).idViaje ];
				//Regreso del depósito después de terminar el itinerario o servicio
				costoAsignacion += this->mdvspdata.matrizViajes[this->mdvspdata.numeroDepositos + (*itUltimoViajeSuperNodo).idViaje ][j];
				
				//Sumatoria del costo de todas las asignaciones seleccionadas al resolver el modelo
				obj += costoAsignacion * x[i][j];			
				
			}
		}
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO GAP
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("GAP.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//Variable para validar la duración total del tour según la velocidad y los tiempos de servicio de los clientes
		double tiempoTotalSolucion = 0;
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		tiempo.start();//Iniciar el cronómetro
		if(cplex.solve()){
			
			//Parar el cronómetro del entorno
			tiempo.stop();		
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo GAP: "<<cplex.getObjValue()<<endl;		
			
			//Mostrar en pantalla el tiempo transcurrido en segundos
			cout<<endl<<"Tiempo solución GAP (segundos) = "<<tiempo.getTime()<<endl;
			
			
			//Mostrar en pantalla los arcos que se activan e información adicional
			vector<long int> itinerariosAsignados(m,0);//Itinerarios asignados por depósito
			for(auto i=0u;i<n;++i){			
				for(auto j=0u;j<m;++j){
					
					//Vector para contener la arista
					vector<int> aristaAuxiliarSalidaDeposito(2);
					vector<int> aristaAuxiliarEntradaDeposito(2);
									
					//Sólo las aristas activas
					if(cplex.getValue(x[i][j])>0.95){
						
						//Salida en pantalla de las aristas activas
						//cout<<"x("<<i<<","<<j<<") = "<<cplex.getValue(x[i][j])<<endl;						
						
						//Contador de viajes asignados al depósito
						++itinerariosAsignados[j];
						
						//Obtener y almacenar las aristas de conexión (seleccionadas por la solución del GAP)
						///////////////////////////////////////////////////////////////////////////////////////
						
						//Apuntar al primer viaje del itinerario (super nodo)
						list<viaje>::iterator itPrimerViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.begin();
						//Apuntar al último viaje del itinerario (super nodo)
						list<viaje>::iterator itUltimoViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.end();
						--itUltimoViajeSuperNodo;
						
						//Crear aristas (con los subíndices del modelo Mesquita 1992)
						aristaAuxiliarSalidaDeposito[0] = j + this->mdvspdata.numeroViajes;
						aristaAuxiliarSalidaDeposito[1] = (*itPrimerViajeSuperNodo).idViaje;
						//------------------------------------------------------------------
						aristaAuxiliarEntradaDeposito[0] = (*itUltimoViajeSuperNodo).idViaje;
						aristaAuxiliarEntradaDeposito[1] = j + this->mdvspdata.numeroViajes;
						
						//Adicionar aristas
						aristasDepositos.push_back(aristaAuxiliarSalidaDeposito);						
						aristasDepositos.push_back(aristaAuxiliarEntradaDeposito);						
						
					}//Fin de la validación de activación									
				}//Fin del for que varía los depósitos 		
			}//Fin del for que varía los supernodos
			
			////Ejemplo de acceso (borrar)
			//for(auto i=0u;i<n;++i){				
			//	for(auto j=0u;j<m;++j){				
			//		
			//		//Costo de asignación c_ij (c_ij = costo salida al primer viaje del itinerario + costo de regreso del último viaje del itinerario)
			//		double costoAsignacion = 0;
			//						
			//		//Apuntar al primer viaje del itinerario (super nodo)
			//		list<viaje>::iterator itPrimerViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.begin();
			//		//Apuntar al último viaje del itinerario (super nodo)
			//		list<viaje>::iterator itUltimoViajeSuperNodo = secuenciaServicios.itinerarios[i].viajes.end();
			//		--itUltimoViajeSuperNodo;				
			//		
			//		//Salida del depósito a atender este itinerario o servicio
			//		costoAsignacion += this->mdvspdata.matrizViajes[j][this->mdvspdata.numeroDepositos +  (*itPrimerViajeSuperNodo).idViaje ];
			//		//Regreso del depósito después de terminar el itinerario o servicio
			//		costoAsignacion += this->mdvspdata.matrizViajes[this->mdvspdata.numeroDepositos + (*itUltimoViajeSuperNodo).idViaje ][j];
			//		
			//		//Sumatoria del costo de todas las asignaciones seleccionadas al resolver el modelo
			//		obj += costoAsignacion * x[i][j];			
			//		
			//	}
			//}		
			
			//Mostrar el uso de cada depósito
			cout<<endl;
			long int autonumericoDepositos = 0;
			for(auto& depositosUsados: itinerariosAsignados){
				cout<<"Depósito "<<this->mdvspdata.depositos[autonumericoDepositos].nombreDeposito<<" ("<<autonumericoDepositos<<") = "<<depositosUsados<<" vehículos usados de "<<this->mdvspdata.depositos[autonumericoDepositos].numeroVehiculos<<endl;
				//Validar que no se exceda la flota del depósito
				if(depositosUsados > this->mdvspdata.depositos[autonumericoDepositos].numeroVehiculos){
					banderaFactibilidadVehiculos = false;
				}				
				++autonumericoDepositos;
			}
			
			//Mostrar en pantalla costos finales
			cout<<endl<<"Costo sin asignación de depósitos = "<<secuenciaServicios.costoSinDepositos;			
			secuenciaServicios.costoConDepositos += cplex.getObjValue() + secuenciaServicios.costoSinDepositos;
			cout<<endl<<"Costo con asignación de depósitos = "<<secuenciaServicios.costoConDepositos<<endl;
			
			//Pasar por referencia los valores obtenidos a las variables del constructivo
			costoG = secuenciaServicios.costoSinDepositos;
			costoSpuerG = secuenciaServicios.costoConDepositos;
			tiempoSolucionModeloAsignacionGeneralizada = tiempo.getTime();			
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo GAP)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP




	
	
	

/*

void modeloGCMaestro(casoRostering instancia){
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		// los parametros de entrada son dinamicos para resolverf el problema maestro.

		char name_s[200];//Vector para nombrar el conjunto de variables de decisión'Y'
		W=IloBoolVarArray(env);//Dimensión b = posibles bloques (de 1 a 4 posibles)
		for(unsigned int i=0;i<instancia.num;++i){					
			sprintf(name_s,"W_%u_%u_%u",i);
			//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
			W[i] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		IloArray<IloNumVarArray> X(env, instancia.CantidadTablas);
		std::stringstream name;
		for(auto t=0;t<instancia.CantidadTablas;++t) {
			X[t] = IloNumVarArray(env, instancia.PosiblesBloques);
			for(auto b=0;b<instancia.PosiblesBloques;++b) {
				name << "X_" << t << "_" << b;
				X[t][b] = IloNumVar(env, name.str().c_str());			
				name.str(""); // Clean name
			}
		}	
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		nf[t]=BoolVarMatrix (env,instancia.CantidadTablas);//Dimensión t = tablas
		for(unsigned int t=0;t<instancia.CantidadTablas;++t){
			nf[t]=IloBoolVarArray(env,instancia.PosiblesBloques);//Dimensión b = posibles bloques (de 1 a 4 posibles)
			for(unsigned int b=0;b<instancia.PosiblesBloques;++b){					
				sprintf(name_s,"nf_%u_%u_%u",t,e,b);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
					nf[t][b] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                               
				}
		}  
		
		/////////////////////////////////////////////////////////////////
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//Conjunto de restricciones para cada evento este contenido solo en un bloque (R4)
		IloRangeArray ContenidoBolque(env);
		stringstream nombreContenidoBloque;
		for(auto t = 0u; t < instancia.CantidadTablas; ++t){ 
			for(auto e = 0u; e < instancia.EventosTabla[t]; ++e){//Una restricción por cada evento de cada tabla
				IloExpr restriccionContenidoBloque(env);
				for(auto b = 0u; b < instancia.PosiblesBloques; ++b) { // la dimension de b = posibles bloques
					restriccionContenidoBloque += Y[t][e][b];//Un término en la restricción por cada uno de los posibles bloques
				}
				nombreContenidoBloque << "AsignarBloque_{"<< t<<","<<e<<"}";
				ContenidoBolque.add (IloRange(env, 1, restriccionContenidoBloque, 1, nombreContenidoBloque.str().c_str()));
				nombreContenidoBloque.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}	
		//Adicionar restricciones 
		modelo.add(ContenidoBolque); 
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		//Conjunto de restricciones para que los cortes de los blques sean continuos (R5)
		IloRangeArray CorteContinuo(env);
		stringstream nombreCorteContinuo;
		for(auto t = 0u; t < instancia.CantidadTablas; ++t){ //Una restricción por tabla
			for(auto b = 0u; b < instancia.PosiblesBlques-1; ++b){
			IloExpr restriccionCorteContinuo(env);
			IloExpr cortePosterior(env);	
			
			restriccionCorteContinuo=X[t][b];
			cortePosterior=X[t][b+1];
			retriccionCorteContinuo-=cortePosterior;
			
			nombreCorteContinuo << "Corte_B{"<< t<<"}";
			CorteContinuo.add (IloRange(env, -IloInfinity, restriccionCorteContinuo, -1, nombreCorteContinuo.str().c_str()));
			nombreCorteContinuo.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}
		//Adicionar restricciones 
		modelo.add(CorteContinuo); 
		

		//Conjunto de restricciones para acotar ultimo corte 		(R6)
		IloRangeArray UltimoCorte(env);
		stringstream nombreUtimoCorte;
		for(auto t = 0u; t < instancia.CantidadTablas; ++t){ //Una restricción por tabla
			
			IloExpr restriccionUltimoCorte(env);	
			
			restriccionUltimoCorte=X[t][3];
			
			nombreUltimoCorte << "Corte_B{"<< 3<<"}";
			UltimoCorte.add (IloRange(env, -IloInfinity, restriccionUltimoCorte, EventosTabla[t]+3, nombreUltimoCorte.str().c_str()));
			nombreUltimoCorte.str(""); // Limpiar variable para etiquetas de las restricciones
			
		}
		//Adicionar restricciones 
		modelo.add(CorteContinuo); 
		
                            
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////
		// Restricciones de relacion de variables X y Y	(R7)
		
		IloRangeArray RelacionVar(env);
		stringstream nombreRelacionVar;
		for(auto t = 0u; t < instancia.CantidadTablas; ++t){ 
			for(auto b = 0u; b < instancia.PosiblesBloques; ++b){
				IloExpr valorX(env);
				valorX=X[t][b];
					for(auto e=0u; e < instancia.EventosTabla[t]; ++e){
						
						IloExpr restriccionRelacionVar(env);
						restriccionRelacionVar=Y[t][e][b]*int(e+1);
						restriccionRelacionVar-=valorX;
						
						nombreRelacionVar << "RelacionVariables_{"<< t<<", "<< b<<", "<< e<<"}";
						RelacionVar.add (IloRange(env, -IloInfinity, restriccionRelacionVar	, 0 , nombreRelacionVar.str().c_str()));
						nombreRelacionVar.str(""); // Limpiar variable para etiquetas de las restricciones
			
					}
				
				}
		}
		//Adicionar restricciones 
		modelo.add(RelacionVar); 
		
		// Restricciones de relacion opuesta variables X y Y	(R8)
		
		IloRangeArray RelacionVarO(env);
		stringstream nombreRelacionVarO;
		for(auto t = 0u; t < instancia.CantidadTablas; ++t){ 
			for(auto b = 0u; b < instancia.PosiblesBloques-1; ++b){
				IloExpr valorXO(env);
				valorXO=X[t][b];
					for(auto e=0u; e < instancia.EventosTabla[t]; ++e){
						
						IloExpr restriccionRelacionVarO(env);
						IloExpr granMM(env);	//Expresion para relajar la restriccion con un gran MM manual
						
						restriccionRelacionVarO=Y[t][b+1][e]*int(e+1);
						granMM= (1-Y[t][e][b+1])*999999999999;
						restriccionRelacionVarO= restriccionRelacionVarO - valorXO + granMM;
						
						nombreRelacionVarO << "RelacionVariables_{"<< t<<", "<< b<<", "<< e<<"}";
						RelacionVarO.add (IloRange(env, 1, restriccionRelacionVarO	, IloInfinity , nombreRelacionVarO.str().c_str()));
						nombreRelacionVarO.str(""); // Limpiar variable para etiquetas de las restricciones
					}
				}
		}
		//Adicionar restricciones 
		modelo.add(RelacionVarO);
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////
		//Restriccion que  activacion de variable no fantasma (R9)
		
		IloRangeArray ActivarNF(env);
		stringstream nombreActivarNF;
		for(auto t = 0; t < instancia.CantidadTablas; ++t){
			for(auto b = 0u; b < instancia.PosiblesBloques; ++b){ 
				IloExpr restriccionActivarNF(env);
				//Una restricción por cada bloque de cada tabla
				for( auto e = 0u ; e < instancia.CantidadEventos[t]; e++){
					restriccionActivarNF+= Y[t][e][b]*instancia.duracion[t][e]; 
				}
				IloExpr auxActivacion(env);
				auxActivacion=nf[t][b]*99999999;
				
				restriccionActivarNF-=auxActivacion;
				
				
				//restriccionDiasTrabajados   
				nombreActivarNF << "ActivacionNF_{"<< t<<", " << b << "}";
				ActivarNF.add (IloRange(env,-IloInfinity, restriccionProhibicionAsignacionNA, 0 , nombreActivarNF.str().c_str()));
				ActivarNF.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(ActivarNF); 
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////
		//Restriccion de eventos de balance	(R10)
		
		
		IloRangeArray BalanceBloques(env);
		stringstream nombreBalanceBloques;
		for(auto t = 0; t < instancia.CantidadTablas; ++t){
			for(auto b = 0u; b < instancia.PosiblesBloques-1; ++b){ 
				//Una restricción por cada bloque de cada tabla
				IloExpr RestriccionBalance(env);
				IloExpr NumBalance(env);
				
				RestriccionBalance=X[t][b+1]-X[t][b];
				NumBalance=instancia.balance * nf[t][b+1];
				RestriccionBalance-=NumBalance;
				   
				nombreBalanceBLoques << "Balance de Bloque_{"<< t <<", " << b<<"}";
				BalanceBloques.add (IloRange(env,0, restriccionBalance, IloInfinity , nombreBalanceBloques.str().c_str()));
				nombreBalanceBloques.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(BalanceBloques); 
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////
		//Restricciones de maxima duracion 		(R11)
		IloRangeArray DuracionMaxima(env);
		stringstream nombreDuracionMaxima;
		for(auto t = 0; t < instancia.CantidadTablas; ++t){
			for(auto b = 0u; b < instancia.PosiblesBloques; ++b){ 
				//Una restricción por cada bloque generado
				IloExpr restriccionDuracion(env);
				for(auto e = 0u ; e < instancia.CantidadEventos[t] ; e++){
					 restriccionDuracion+= Y[t][e][b] * instancia.duracion[t][e];
				}
				IloExpr Tope(env);
				Tope= instancia.hb + instancia.extrab;
				
				//restriccion de Duracion de bloques  
				nombreDuracionMaxima << "Duracion Maxima_{"<< t<< ", " <<b <<"}";
				DuracionMaxima.add (IloRange(env,-IloInfinity, restriccionDuracion, Tope , nombreDuracionMaxima.str().c_str()));
				nombreDuracionMaxima.str(""); // Limpiar variable para etiquetas de las restricciones
			}	
		}
		//Adicionar restricciones 
		modelo.add(DuracionMaxima);
		
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////
		//Restriccion Ligadura de eventos	(R12)
		IloRangeArray LigaduraEventos(env);
		stringstream nombreLigaduraEventos;
		for(auto t = 0; t < instancia.CantidadTablas; ++t){
			for(auto b = 0u; b < instancia.PosiblesBloques; ++b){
				for(auto e = 1 ; e < instancia.CantidadEventos[t] ; e++){
					//Una restricción por cada día de la semana por cada operario
					IloExpr restriccionLigadura(env);
					IloExpr Ligadura(env);
					
					Ligadura=2*instancia.noI[t][e];
					RestriccionLigadura= Y[t][e-1][b]+Y[t][e][b];
					RestriccionLigadura-=Ligadura;
					
					//restriccion de eventos Ligados 
					nombreLigaduraEventos << "Ligadura Eventos_{"<< t<< ", "<< e<< ", " << b << "}";
					LigaduraEventos.add (IloRange(env,0, restriccionLigadura, IloInfinity , nombreLigaduraEventos.str().c_str()));
					nombreLigadura.str(""); // Limpiar variable para etiquetas de las restricciones
				}
			}	
		}
		//Adicionar restricciones 
		modelo.add(LigaduraEventos);
		

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//******************************************************************
		//* 					FUNCIÓN OBJETIVO (Crew Scheduling)
		//**************************************************************
				
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		
		
		for (auto t = 0u; t < instancia.CantidadTablas; ++t) {
				for (auto b = 0u; b< instancia.PosiblesBloqes; ++b){
					obj += X[t][b];
			}
		}		
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		//modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO Crew Scheduling
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		//env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		cplex.exportModel("modeloMusliu.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){	
			
			//Subíndices variable de decisión del modelo
			//i - Corresponde al conductor
			//j - Corresponde al día
			//k - Corresponde al turno
			
			
			//Contenedores de la solución (salida en formato de horario)
			vector< vector< vector < int > > > programacionTurnos;
			programacionTurnos.resize( instancia.longitudProgramacion );//Todos los días de la programación
			//Por cada uno de los días, dividir en los tipos de turno que se tengan
			for(auto& dia_j : programacionTurnos){
				dia_j.resize(instancia.tiposTurno);
			}
					
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo: "<<cplex.getObjValue()<<endl;					
			
			for( size_t i=0;i<instancia.numeroConductores;++i){
				for(size_t j=0;j<instancia.longitudProgramacion;++j){
					for (size_t k=0;k<instancia.tiposTurno;++k){
						if(cplex.getValue(X[i][j][k])>0.95){										
							cout<<"X("<<i<<","<<j<<","<<k<<") = "<<cplex.getValue(X[i][j][k])<<endl;
							//cout<<"X("<<i<<","<<0<<","<<k<<") = "<<cplex.getValue(X[i][0][k])<<endl;
							
							//Acumular los códigos de los conductores asignados a cada día y segmento o turno del día
							programacionTurnos[j][k].push_back(i);
							
						}
					}
				}
			}
			// no imporime porque el vector es de 3 dimensiones y estoy imprimiendo solo 2n
			
			////Salida diagnóstico			
			//cout<<endl<<"Tamanio Y ->"<<Y.getSize()<<" "<<instancia.numeroConductores;
			//cout<<endl<<"Tamanio Y[i] ->"<<Y[0].getSize()<<" "<<instancia.tiposTurno;
			//getchar();			
			
			
			
			cout<<endl<<"--------------------------"<<endl;
			for( size_t i=0;i<instancia.numeroConductores;++i){				
				for (size_t k=0;k<instancia.tiposTurno-1;++k){
					//if(cplex.getValue(Y[i][k])>0.0){										
						cout<<"Y("<<i<<","<<k<<") = "<<cplex.getValue(Y[i][k])<<endl;
					//}
				}		
			}
			cout<<endl<<"--------------------------"<<endl;
			
			
			
			
			//Archivo de salida para corridas en bloque
			ofstream archivoFormatoHorario("rosteringFormatoHorario.log",ios::app | ios::binary);		
			archivoFormatoHorario<<setprecision(10);//Cifras significativas para el reporte
			if (!archivoFormatoHorario.is_open()){
				cout << "Fallo en la apertura del archivo de salida FORMATO HORARIO.";				
			}
			
			
			//Salida Horario
			cout<<endl<<"Salida en Formato Horario"<<endl;
			archivoFormatoHorario<<endl<<"Salida en Formato Horario -> "<<instancia.nombreArchivo<<endl;
			vector<string> arregloDias(7);//Arreglo para documentar salida
			arregloDias[0] = "L";
			arregloDias[1] = "M";
			arregloDias[2] = "X";
			arregloDias[3] = "J";
			arregloDias[4] = "V";
			arregloDias[5] = "S";
			arregloDias[6] = "D";
			for(auto& etiquetaDia_j : arregloDias){
				cout<<etiquetaDia_j<<"	";
			}
			cout<<endl;
			archivoFormatoHorario<<endl;
			for(int k=0;k<instancia.tiposTurno;++k){
				
				for(int j=0;j<instancia.longitudProgramacion;++j){
					
					cout<<" (";
					archivoFormatoHorario<<" (";
					
					for(int i=0;i<programacionTurnos[j][k].size();++i){
						
						cout<<programacionTurnos[j][k][i]<<" ";
						archivoFormatoHorario<<programacionTurnos[j][k][i]<<" ";
						
					}
					
					cout<<"), ";
					archivoFormatoHorario<<"), ";
					
			
				}
				
				cout<<endl<<"----------------------------------------------------------------"<<endl;
				archivoFormatoHorario<<endl<<"----------------------------------------------------------------"<<endl;
			
			}			
			
			//Cerrar el controlador del archivo de salida en formato horario
			archivoFormatoHorario.close();	
			
		
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo)";
			
		}			
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		env.end();//Cierre del entorno CPLEX
		
		
	}


*////Cierre comentario Modelos Integra-Uniandes	
	
	
	void modeloSudoku(){	
		
		
		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		
		
		//Creación de variables x
		/////////////////////////
		

		//Creación de una variable z_{ij}^r binaria que relaciona los arcos x_ij con niveles de velocidad 'r'             
		char name_s[200];//Vector para nombrar el conjunto de variables 'z'
		BoolVar3Matrix s(env,9);//Dimensión i    
		//Bool3Matrix z0(env,cardinalidadN);
		for(unsigned int i=0;i< 9;++i){
			s[i]=BoolVarMatrix (env,9);//Dimensión j
			//z0[i]=BoolMatrix(env,cardinalidadN);
			for(unsigned int j=0;j<9;++j){
				s[i][j]=IloBoolVarArray(env,9);//Dimensión r (nivel de velocidad al cual es atravesado la arista)
				//z0[i][j]=IloBoolArray(bb,cardinalidadConjuntoNivelesVelocidad);
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
				for(unsigned int k=0;k<9;++k){
					
					sprintf(name_s,"s_%u_%u_%u",i,j,k);
					
					//sprintf(name_s,"s_",i,"_",j,"_",k+1);
					s[i][j][k] = IloBoolVar(env,name_s);//Asociación explícita de la variable de decisión del problema                
					//z0[i][j][r]=0;//Inicialización de la variable para puntos de partida de solución                
				}
			}                
		}

		
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//Conjunto de restricciones para controlar las columnas 
		IloRangeArray RF(env);
		stringstream nombreRF;
		for(auto i = 0u; i < 9; ++i){
			for(auto k = 0u; k < 9; ++k){//Una restricción por cada uno de los servicios o itinerarios
				//Crear una expresión por cada restricción, adicionarla al modelo y reiniciarla
				IloExpr restriccionRF(env);
				for(auto j = 0u; j < 9; ++j) {
					restriccionRF += s[i][j][k];//Un término en la restricción por cada uno de las posiciones del cuadro
				}
				restriccionRF-=1;
				nombreRF << "FilaAsignada_{"<< i<<","<<k<<"}";
				RF.add (IloRange(env, 0, restriccionRF, 0, nombreRF.str().c_str()));
				nombreRF.str(""); // Limpiar variable para etiquetas de las restricciones
			}
		}
		
		//Adicionar el conjunto de restricciones 
		modelo.add(RF);	
		
		
		
		
		
		////////////////////////////////////////////////////////////////////////////7
		// restricciones de las columnas
	IloRangeArray RC(env);
		stringstream nombreRC;
		for(auto j = 0u; j < 9; ++j){
			for(auto k = 0u; k < 9; ++k){//Una restricción por cada uno de las filas y columnas
			//Crear una expresión por cada restricción, adicionarla al modelo y reiniciarla
			IloExpr restriccionRC(env);
			for(auto i = 0u; i < 9; ++i) {
				restriccionRC += s[i][j][k];//Un término en la restricción por cada uno de las posiciones del cuadro
			}
			restriccionRC-=1;
			nombreRC << "ColumnaAsignada_{"<< j<<","<<k<<"}";
			RC.add (IloRange(env, 0, restriccionRC, 0, nombreRC.str().c_str()));
			nombreRC.str(""); // Limpiar variable para etiquetas de las restricciones
		}
				}
				

		//Adicionar el conjunto de restricciones 
		modelo.add(RC);
		
			
		
		////// mirar bien plox 
		IloRangeArray RCuadro(env);
		stringstream nombreRCuadro;
		for(auto j = 1; j < 3; ++j){
			for(auto i = 1; i < 3; ++i){
				for(auto k = 0u; k < 9; ++k){       // un numero por posición en el sudoku 
			IloExpr restriccionRCuadro(env);
			for(auto a = 0u; a < 3; ++a) {
				for(auto b = 0u; b < 3; ++b){ 
			//	restriccionRCuadro += s[a+3*(i-1)][b+3*(j-1)][k];//Un término en la restricción por cada uno de las posiciones del cuadro
				restriccionRCuadro += s[a+3*(i)][b+3*(j)][k];//Un término en la restricción por cada uno de las posiciones del cuadro
		}
			}
			restriccionRCuadro-=1;
			nombreRCuadro << "Cuadro_{"<< i<<","<<j<<","<<k<<"}";
			RCuadro.add (IloRange(env, 0, restriccionRCuadro, 0, nombreRCuadro.str().c_str()));
			nombreRCuadro.str(""); // Limpiar variable para etiquetas de las restricciones
		}
			}
				}
		
		//Adicionar el conjunto de restricciones 
		modelo.add(RCuadro);	
		
		cout<<"Entra al SUDOKU!!!"<<endl;
		getchar();
		
						
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (GAP)
		//******************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		obj += s[1][1][1];
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO GAP
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		//env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("modeloSudoku.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){			
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo: "<<cplex.getObjValue()<<endl;					
			
			for( size_t i=0;i<9;++i){
				for(size_t j=0;j<9;++j){
					for (size_t k=0;k<9;++k){
						if(cplex.getValue(s[i][j][k])>0.95){					
							cout<<"s("<<i<<","<<j<<","<<k<<") = "<<cplex.getValue(s[i][j][k])<<endl;
						}
					}
				}
			}
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP
	
	
	//Ejemplo 2 Clase
	void modeloEjemplo2(){	
		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		
		
		//Creación de variables x
		/////////////////////////
		
		//Uno a uno
		IloArray<IloNumVar> x(env, 2);//Tamaño!!!
		x[0] = IloNumVar(env, "x_1");
		x[1] = IloNumVar(env, "x_2");		
		
		////Generalización
		//IloArray<IloNumVarArray> x(env, 2);//Tamaño!!!
		//std::stringstream name;
		//for(auto i = 0u; i < 2; ++i) {
		//	name << "x_" << i+1 ;
		//	x[i] = IloNumVar(env, name.str().c_str());
		//	name.str(""); // Clean name
		//}			
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//Restricciones Ejemplo 1
		IloRangeArray restricciones(env, 4);
		stringstream nombreRestricciones;
				//double para decimales 
		//Restricción 1
		IloExpr restriccion1(env);		
		restriccion1 += 13*x[0] + 19*x[1];		
		nombreRestricciones<<"r_1";
		restricciones[0] = IloRange(env, -IloInfinity, restriccion1, 2400, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
		//Restricción 2
		IloExpr restriccion2(env);		
		restriccion2 += 20*x[0] + 29*x[1];
		nombreRestricciones<<"r_2";
		restricciones[1] = IloRange(env, -IloInfinity, restriccion2, 2100, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
		//Restricción 3
		IloExpr restriccion3(env);		
		restriccion3 -= x[0];		
		nombreRestricciones<<"r_3";
		restricciones[2] = IloRange(env, -IloInfinity, restriccion3, -10, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
			
		
		//Adicionar el conjunto de restricciones al modelo
		modelo.add(restricciones);			
			
		
						
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO 
		//******************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		obj += (double(103.0)/6.0)*x[0];
		obj += (double(388.0)/15.0)*x[1];
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO GAP
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		//env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("Ejemplo2.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){			
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo: "<<cplex.getObjValue()<<endl;					
			
			//Valores de las variables
			cout<<endl<<"x_1 = "<<cplex.getValue(x[0]);
			cout<<endl<<"x_2 = "<<cplex.getValue(x[1])<<endl;					
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP
	
void modeloEjemplo1(){	
		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		
		
		//Creación de variables x
		/////////////////////////
		
		//Uno a uno
		IloArray<IloNumVar> x(env, 2);//Tamaño!!!
		x[0] = IloNumVar(env, "x_1");
		x[1] = IloNumVar(env, "x_2");		
		
		////Generalización
		//IloArray<IloNumVarArray> x(env, 2);//Tamaño!!!
		//std::stringstream name;
		//for(auto i = 0u; i < 2; ++i) {
		//	name << "x_" << i+1 ;
		//	x[i] = IloNumVar(env, name.str().c_str());
		//	name.str(""); // Clean name
		//}			
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************	
		
		//Restricciones Ejemplo 1
		IloRangeArray restricciones(env, 4);
		stringstream nombreRestricciones;
				//double para decimales 
		//Restricción 1
		IloExpr restriccion1(env);		
		restriccion1 += (double(50.0)/8.0)*x[0] + 24*x[1];		
		nombreRestricciones<<"r_1";
		restricciones[0] = IloRange(env, -IloInfinity, restriccion1, 2400, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
		//Restricción 2
		IloExpr restriccion2(env);		
		restriccion2 += 30*x[0] + 33*x[1];
		nombreRestricciones<<"r_2";
		restricciones[1] = IloRange(env, -IloInfinity, restriccion2, 2100, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
		//Restricción 3
		IloExpr restriccion3(env);		
		restriccion3 -= x[0];		
		nombreRestricciones<<"r_3";
		restricciones[2] = IloRange(env, -IloInfinity, restriccion3, -45, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones
		
		//Restricción 4
		IloExpr restriccion4(env);		
		restriccion4 -= x[1];		
		nombreRestricciones<<"r_4";
		restricciones[3] = IloRange(env, -IloInfinity, restriccion4, -5, nombreRestricciones.str().c_str());	
		nombreRestricciones.str(""); // Limpiar variable para etiquetas de las restricciones		
		
		//Adicionar el conjunto de restricciones al modelo
		modelo.add(restricciones);			
			
		
						
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO (GAP)
		//******************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);
		obj += x[0];
		obj += x[1];
		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO GAP
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		//env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("Ejemplo1.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){			
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo: "<<cplex.getObjValue()<<endl;					
			
			//Valores de las variables
			cout<<endl<<"x_1 = "<<cplex.getValue(x[0]);
			cout<<endl<<"x_2 = "<<cplex.getValue(x[1])<<endl;					
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP
	
	//Ejemplo Avión Clase
	void modeloAvion(){
		
		//Lectura de datos
		//////////////////
		
		//Abrir el archivo con la información
		ifstream data_input_avion("./instances/avion/datos_avion_cpp.dat");				
		if (! data_input_avion.is_open() ) {
			cout << "No es posible abrir el archivo " << "./instances/avion/datos_avion_cpp.dat" << endl;getchar();						
		}
		
		//Variable número lecturas
		int numeroLecturas;
		
		//Lectura ingresos
		data_input_avion >> numeroLecturas;
		vector<int> ingresos_carga(numeroLecturas);
		for(size_t i=0;i<numeroLecturas;++i){
			data_input_avion >> ingresos_carga[i];
		}
		
		//Lectura disponibilidad de la carga
		data_input_avion >> numeroLecturas;
		vector<int> disp_carga(numeroLecturas);
		for(size_t i=0;i<numeroLecturas;++i){
			data_input_avion >> disp_carga[i];
		}
		
		//Lectura volumen de la carga
		data_input_avion >> numeroLecturas;
		vector<int> vol_carga(numeroLecturas);
		for(size_t i=0;i<numeroLecturas;++i){
			data_input_avion >> vol_carga[i];
		}
		
		//Lectura volumen del compartimiento
		data_input_avion >> numeroLecturas;
		vector<int> vol_compar(numeroLecturas);
		for(size_t i=0;i<numeroLecturas;++i){
			data_input_avion >> vol_compar[i];
		}
		
		//Lectura peso del compartimiento
		data_input_avion >> numeroLecturas;
		vector<int> peso_compar(numeroLecturas);
		for(size_t i=0;i<numeroLecturas;++i){
			data_input_avion >> peso_compar[i];
		}	
		
		//Cerrar el archivo que contiene la matriz de costos
		data_input_avion.close();
		
		
		//Salida de datos cargados
		cout<<endl<<"Datos Cargados:"<<endl;		
		cout<<"Ingresos Carga ("<<ingresos_carga.size()<<"): "<<endl;
		for(auto& ingreso_i : ingresos_carga){
			cout<<ingreso_i<<endl;
		}
		cout<<"Disponibilidad de la Carga ("<<disp_carga.size()<<"): "<<endl;
		for(auto& disp_i : disp_carga){
			cout<<disp_i<<endl;
		}
		cout<<"Volumen de la Carga ("<<vol_carga.size()<<"): "<<endl;
		for(auto& vol_i : vol_carga){
			cout<<vol_i<<endl;
		}
		cout<<"Volumen del Compartimiento ("<<vol_compar.size()<<"): "<<endl;
		for(auto& vol_i : vol_compar){
			cout<<vol_i<<endl;
		}
		cout<<"Peso del Compartimiento ("<<peso_compar.size()<<"): "<<endl;
		for(auto& peso_i : peso_compar){
			cout<<peso_i<<endl;
		}
		
		//getchar();
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver
		
		
		//Creación de variables x
		/////////////////////////
		
		//Dimensiones X
		int numeroCargas = ingresos_carga.size();//Límite 'i'	
		int numeroCompartimientos = vol_compar.size();//Límite 'j'	
		
		//Generalización
		IloArray<IloNumVarArray> x(env, numeroCargas);//Tamaño!!!
		std::stringstream name;
		for(auto i = 0u; i < numeroCargas; ++i) {
			x[i] = IloNumVarArray(env, numeroCompartimientos);
			for(auto j = 0u; j < numeroCompartimientos; ++j) {				
				name << "x_" << i+1 << "_" << j+1 ;
				//IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
				x[i][j] = IloNumVar(env,0,IloInfinity, name.str().c_str());
				name.str(""); // Clean name
			}			
		}			
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************			
		
		//Restricciones de disponibilidad de la carga
		IloRangeArray res_disp_carga(env, numeroCargas);
		stringstream nom_res_disp_carga;
		for(auto i = 0u ; i < numeroCargas ; ++i ){			
			IloExpr restriccion(env);			
			for(auto j = 0u ; j < numeroCompartimientos ; ++j ){				
				restriccion += x[i][j];				
			}
			nom_res_disp_carga<<"rdc_"<<i;
			res_disp_carga[i] = IloRange(env, -IloInfinity, restriccion, disp_carga[i], nom_res_disp_carga.str().c_str());	
			nom_res_disp_carga.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		modelo.add(res_disp_carga);//Adicionar restricciones al modelo		
		
		//Restricciones de volumen de compartimiento
		IloRangeArray res_vol_compar(env, numeroCompartimientos);
		stringstream nom_res_vol_compar;
		for(auto j = 0u ; j < numeroCompartimientos ; ++j ){			
			IloExpr restriccion(env);			
			for(auto i = 0u ; i < numeroCargas ; ++i ){				
				restriccion += vol_carga[i] * x[i][j];				
			}
			nom_res_vol_compar<<"rvc_"<<j;
			res_vol_compar[j] = IloRange(env, -IloInfinity, restriccion, vol_compar[j], nom_res_vol_compar.str().c_str());	
			nom_res_vol_compar.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		modelo.add(res_vol_compar);//Adicionar restricciones al modelo
		
		//Restricciones de peso de compartimiento
		IloRangeArray res_peso_compar(env, numeroCompartimientos);
		stringstream nom_res_peso_compar;
		for(auto j = 0u ; j < numeroCompartimientos ; ++j ){			
			IloExpr restriccion(env);			
			for(auto i = 0u ; i < numeroCargas ; ++i ){				
				restriccion += x[i][j];				
			}
			nom_res_peso_compar<<"rvc_"<<j;
			res_peso_compar[j] = IloRange(env, -IloInfinity, restriccion, peso_compar[j], nom_res_peso_compar.str().c_str());	
			nom_res_peso_compar.str(""); // Limpiar variable para etiquetas de las restricciones			
		}
		modelo.add(res_peso_compar);//Adicionar restricciones al modelo		
		
		//Relación entre compartimientos 1 y 2
		IloExpr restriccion1_2(env);
		for(auto i = 0u;i<numeroCargas;++i){
			restriccion1_2 += 1.6 * x[i][0];
		}			
		for(auto i = 0u;i<numeroCargas;++i){
			restriccion1_2 -= x[i][1]; 
		}			
		modelo.add( IloRange(env, -IloInfinity, restriccion1_2, 0, "restriccion1_2" ) );	
		
		//Relación entre compartimientos 2 y 3
		IloExpr restriccion2_3(env);
		for(auto i = 0u;i<numeroCargas;++i){
			restriccion2_3 += 2.0 * x[i][2];
		}			
		for(auto i = 0u;i<numeroCargas;++i){
			restriccion2_3 -= x[i][1]; 
		}			
		modelo.add( IloRange(env, -IloInfinity, restriccion2_3, 0, "restriccion2_3" ) );			
						
		//******************************************************************
		//* 					FUNCIÓN OBJETIVO 
		//******************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);		
		
		for(auto i = 0u ; i < numeroCargas ; ++i ){						
			for(auto j = 0u ; j < numeroCompartimientos ; ++j ){				
				obj += ingresos_carga[i] * x[i][j];				
			}			
		}	
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMaximize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 					SOLUCIÓN DEL MODELO GAP
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		//env.setOut(env.getNullStream());	
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("EjemploAvion.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 4000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************	
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes		
		if(cplex.solve()){			
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Valor Función Objetivo: "<<cplex.getObjValue()<<endl;					
			
			////Valores de las variables
			//cout<<endl<<"x_1 = "<<cplex.getValue(x[0]);
			//cout<<endl<<"x_2 = "<<cplex.getValue(x[1])<<endl;					
			
			//Mostrar en pantalla los arcos que se activan e información adicional						
			for(size_t i=0;i<numeroCargas;++i){			
				for(size_t j=0;j<numeroCompartimientos;++j){					
					
					//Validación de la activación de una arista x_{ij}^m
					//if(cplex.getValue(x[i][j])>0.95){
					//if(cplex.getValue(x[i][j])>0.0){				
						
						//Salida en pantalla de las aristas seleccionadas
						cout<<"x("<<i+1<<","<<j+1<<") = "<<cplex.getValue(x[i][j])<<endl;
					
					//}
					
				}
			}		
			
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP
	
	
		
	//Implementación en CPLEX para solucionar el problema de flujo mínimo en una red y encontrar el camino en el digrafo Prins-Liu
	void modeloFlujoMinimo(digrafoPrinsLiu digrafo, vector<viaje> secuenciaGeneralViajesNN, double &costoSuperG, int &numeroVehiculosSolucion){
		
		//Declaración de variables locales
		double tiempolimite;
		long int n = this->mdvspdata.numeroViajes;//Número de servicios 
		long int M = this->mdvspdata.numeroDepositos;//Número de depósitos		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver					
		
		IloTimer tiempo(env); //Tiempo de ejecución (Sólo cuenta luego de la lectura de datos)	
		//tiempo.reset(), tiempo.start(); tiempo.start(); //Instrucciones del cronómetro interno
		
		////Creación de variables x (ejemplo)
		////x_ij = 1, si el depósito 'j' atiende al servicio 'i'. De lo contrario 0
		//IloArray<IloNumVarArray> x(env, n);
		//std::stringstream name;
		//for(auto i = 0u; i < n; ++i) {
		//	x[i] = IloNumVarArray(env, m);
		//	for(auto j = 0u; j < m; ++j) {
		//		name << "x_" << i << "_" << j;
		//		x[i][j] = IloNumVar(env, 0, 1, IloNumVar::Bool, name.str().c_str());
		//		name.str(""); // Clean name
		//	}
		//}		
		
		//Creación de una variable x_{ij}^r binaria por acada uno de los arcos x_ij que son itinerarios atendidos con el depósito 'm' 			
		char name_x[100];//Vector para nombrar el conjunto de variables 'x'
		BoolVar3Matrix x(env,digrafo.numeroNodos);//Dimensión i (número de nodos del grafo)	
		for(unsigned int i=0;i<digrafo.numeroNodos;++i){
			//x[i]=BoolVarMatrix (env,digrafo.numeroAristas);//Dimensión j (número de aristas del grafo)
			x[i] = BoolVarMatrix (env,digrafo.numeroNodos);//Dimensión j (número de nodos del grafo)
			for(unsigned int j=0;j<digrafo.numeroNodos;++j){
				x[i][j]=IloBoolVarArray(env,M);//Dimensión M (número de depósitos que podrían atender el itinerario)				
				//Adicionar el nombre a cada una de las variables en cada iteración para la observación del modelo
				for(unsigned int m=0;m<M;++m){
					sprintf(name_x,"x_{%u,%u}%u",i,j,m);
					x[i][j][m] = IloBoolVar(env,name_x);//Asociación explícita de la variable de decisión del problema
				}
			}				
		}
		
		//Salida de diagnóstico
		//cout<<endl<<"Número de nodos del digrafo -> "<<digrafo.numeroNodos<<endl;getchar();
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************
				
		//Conjunto de restricciones para controlar que cada itinerario sea atendido por uno y sólo uno de los depósitos
		IloRangeArray restriccionesFlujoNodos(env, digrafo.numeroNodos);
		stringstream nombreRestriccionesFlujoNodos;
		unordered_set <string> conjuntoAristasActivas;//Tabla hash con las aristas activas del posible digrafo compreto Prins-Liu
		
		//Una restricción por cada uno de los nodos del digrafo
		for(auto i = 0u; i < digrafo.numeroNodos; ++i){
			
			//Crear una expresión por cada restricción de flujo en cada nodo, adicionarla al modelo y reiniciarla
			IloExpr restriccion(env);			
				
			//Por cada una de las aristas, establecer si es saliente, entrante o no tiene conexión con el nodo 'i'
			for(auto j = 0u; j < digrafo.numeroAristas; ++j) {
				
				//String para representar las aristas en la tabla hash de las aristas activas del posible digrafo completo
				stringstream aristaString;
					
				//Revisar si se trata de una arista saliente del nodo 'i'
				unordered_set<int>::const_iterator itAristasSalientes = digrafo.nodos[i].aristasSalientes.find(j);
				if(itAristasSalientes != digrafo.nodos[i].aristasSalientes.end()){
					
					//Adicionar a la restricción de flujo del nodo 'i', una arista saliendo del mismo, la cual representa un itinerario atendido por el depósito que esté codificado en ella
					restriccion += -1 * x[i][ digrafo.aristas[j].subIndiceContenedorLlegadaArista ][ digrafo.aristas[j].depositoArista.idDeposito ];
					
					//Codificación de la arista presente en el grafo para el conjunto de control de aristas activas
					aristaString<<i<<"_"<<digrafo.aristas[j].subIndiceContenedorLlegadaArista<<"_"<<digrafo.aristas[j].depositoArista.idDeposito;
					//Validar su presencia en el conjunto, antes de agregarlo a la tabla hash correspondiente
					unordered_set<string>::const_iterator itAristasActivas = conjuntoAristasActivas.find(aristaString.str());
					if(itAristasActivas == conjuntoAristasActivas.end()){
						conjuntoAristasActivas.insert(aristaString.str());
					}				
						
					//Salida de diagnóstico
					cout<<endl<<"Arista Saliente"<<i<<"_"<<digrafo.aristas[j].subIndiceContenedorLlegadaArista<<"_"<<digrafo.aristas[j].depositoArista.idDeposito<<" ";					
					
				}else{
					
					//Revisar si se trata de una arista entrante al nodo 'i'
					unordered_set<int>::const_iterator itAristasEntrantes = digrafo.nodos[i].aristasEntrantes.find(j);
					if(itAristasEntrantes != digrafo.nodos[i].aristasEntrantes.end()){
						
						//Adicionar a la restricción de flujo del nodo 'i', una arista entrando a dicho nodo, la cual representa el último itinerario atendido antes de este nodo
						restriccion += x[ digrafo.aristas[j].subIndiceContenedorSalidaArista ][i][ digrafo.aristas[j].depositoArista.idDeposito ];
						
						//Codificación de la arista presente en el grafo para el conjunto de control de aristas activas
						aristaString<<digrafo.aristas[j].subIndiceContenedorSalidaArista<<"_"<<i<<"_"<<digrafo.aristas[j].depositoArista.idDeposito;
						//Validar su presencia en el conjunto, antes de agregarlo a la tabla hash correspondiente
						unordered_set<string>::const_iterator itAristasActivas = conjuntoAristasActivas.find(aristaString.str());
						if(itAristasActivas == conjuntoAristasActivas.end()){
							conjuntoAristasActivas.insert(aristaString.str());
						}						
						
						//Salida de diagnóstico
						cout<<endl<<"Arista Entrante"<<digrafo.aristas[j].subIndiceContenedorLlegadaArista<<"_"<<i<<"_"<<digrafo.aristas[j].depositoArista.idDeposito<<" ";						
						
					}					
					
				}								
				
			}//Fin del for que recorre las aristas del grafo			
			//getchar();//Pausar para diagnóstico de términos de la restricción
			
			
			//Asignar etiqueta a la restricción
			nombreRestriccionesFlujoNodos << "FlujoNodo_" << i;			
			//Si es punto de entrada, salida o tránsito de la red, establecer la restricción
			if(i == 0 ){//Nodo de entrada a la red
				restriccionesFlujoNodos[i] = IloRange(env, -1, restriccion, -1, nombreRestriccionesFlujoNodos.str().c_str());
			}else{				
				if(i == (digrafo.numeroNodos-1) ){//Nodo de salida de la red
					restriccionesFlujoNodos[i] = IloRange(env, 1, restriccion, 1, nombreRestriccionesFlujoNodos.str().c_str());
				}else{//Nodo de tránsito en la red
					restriccionesFlujoNodos[i] = IloRange(env, 0, restriccion, 0, nombreRestriccionesFlujoNodos.str().c_str());
				}
			}
			
			nombreRestriccionesFlujoNodos.str(""); // Limpiar variable para etiquetas de las restricciones		
			
			
			//Separar salidas de diagnóstico
			cout<<endl<<"----------------------------------"<<endl;//getchar();
				
			
		}
		//Adicionar el conjunto de restricciones de flujo en los nodos del grafo o de la red
		modelo.add(restriccionesFlujoNodos);	
							
		
		//Conjunto de restricciones para limitar el uso de vehículos de cada uno de los depósitos
		IloRangeArray flotaDepositos(env, M);
		stringstream nombreRestriccionFlotaDisponible;
		for(auto m = 0u; m < M; ++m){//Una restricción por cada uno de los depósitos		
			
			//Crear una expresión por cada restricción, adicionarla al modelo y reiniciarla
			IloExpr restriccion(env);
			
			//Todas las conexiones posibles del grafo		
			for(auto i = 0u; i < digrafo.numeroNodos; ++i) {				
				for(auto j = 0u; j < digrafo.numeroNodos; ++j) {
					
					restriccion += x[i][j][m];
					
				}
			}
			restriccion -= this->mdvspdata.depositos[m].numeroVehiculos;//Limitar con el número de vehículos (flota) disponibles en el depósito 'm'			
			nombreRestriccionFlotaDisponible << "FleetConstraintDepot_"<<m;
			flotaDepositos[m] = IloRange(env, -IloInfinity, restriccion, 0, nombreRestriccionFlotaDisponible.str().c_str());
			nombreRestriccionFlotaDisponible.str(""); // Limpiar variable para etiquetas de las restricciones
			
			
		}
		//Adicionar el conjunto de restricciones que controlan que todos los servicios sean atendidos y por un solo depósito
		//modelo.add(flotaDepositos);		
		
						
		//************************************************************************
		//* FUNCIÓN OBJETIVO (Flujo Mínimo o camino más corto en una red o grafo)
		//************************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);				
		
		//Costos de asignación entre los depósitos y los servicios (o conjuntos de servicios)
		for(auto& aristaDigrafo : digrafo.aristas){
			obj += aristaDigrafo.costo * x[aristaDigrafo.subIndiceContenedorSalidaArista][aristaDigrafo.subIndiceContenedorLlegadaArista][aristaDigrafo.depositoArista.idDeposito];
		}			
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 		 	SOLUCIÓN DEL MODELO DE FLUJO MÍNIMO
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("MinFlow.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 8000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		int timeLimit = 7200; //Dos horas
		//int timeLimit = 14400; //Cuatro horas
		//int timeLimit = 43200; //Doce horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Tolerancia para el criterio de parada de CPLEX
		//cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-7);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Variable para validar la duración total del tour según la velocidad y los tiempos de servicio de los clientes
		double tiempoTotalSolucion = 0;
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		tiempo.start();//Iniciar el cronómetro
		if(cplex.solve()){
			
			//Parar el cronómetro del entorno
			tiempo.stop();		
			
			//Valor de la función objetivo al solucionar el problema por primera vez
			cout<<endl<<"-----> Objective Function Value: "<<cplex.getObjValue()<<endl;		
			costoSuperG = cplex.getObjValue();
			
			//Mostrar en pantalla el tiempo transcurrido en segundos
			cout<<endl<<"Solution time (seconds) = "<<tiempo.getTime()<<endl;
			
			//Mostrar en pantalla los arcos que se activan e información adicional			
			int numeroAristasSeleccionadas = 0;//Contador de las aristas o itinerarios seleccionados en la solución
			vector<int> vehiculosUsadosPorDeposito(M,0);//Contador de los vehículos utilizados por depósito
			for(size_t i=0;i<digrafo.numeroNodos;++i){			
				for(size_t j=0;j<digrafo.numeroNodos;++j){					
					for(size_t m=0;m<M;++m){						
						
						//Codificación de la arista presente en el grafo para el conjunto de control de aristas activas
						stringstream aristaString;
						aristaString<<i<<"_"<<j<<"_"<<m;
						//Validar su presencia en el conjunto, antes de revisar si hace parte del conjunto solución
						unordered_set<string>::const_iterator itAristasActivas = conjuntoAristasActivas.find(aristaString.str());
						if(itAristasActivas != conjuntoAristasActivas.end()){							
							
							//Validación de la activación de una arista x_{ij}^m
							if(cplex.getValue(x[i][j][m])>0.95){
							
								//Incrementar el contador de itinerarios o aristas seleccionadas en la solución
								++numeroAristasSeleccionadas;
								
								//Salida en pantalla de las aristas seleccionadas
								cout<<"x("<<i<<","<<j<<","<<m<<") = "<<cplex.getValue(x[i][j][m])<<endl;
								
								//Encontrar la arista en el conjunto correspondiente							
								int auxiliarSubIndiceArista = 0;//Contador para conocer el subíndice de la arista
								for(auto& arista_i : digrafo.aristas){
									
									//Condicional para encontrar la arista
									if(arista_i.subIndiceContenedorSalidaArista == i && arista_i.subIndiceContenedorLlegadaArista == j && arista_i.depositoArista.idDeposito == m){
										
										//Salida de diagnóstico									
										cout<<endl<<"--------------------------------"<<endl;
										cout<<"Viajes cubiertos por la arista ("<<auxiliarSubIndiceArista<<"): ";			
										for(auto& viaje_i : arista_i.viajesCubiertos){
											cout<<viaje_i.idViaje<<" ";
										}
										cout<<endl<<"Primer viaje = "<<arista_i.viajeInicial.idViaje<<endl;	
										cout<<"Último viaje = "<<arista_i.viajeFinal.idViaje<<endl;
										cout<<"Depósito = "<<arista_i.depositoArista.idDeposito<<endl;
										cout<<"Número de viajes cubiertos = "<<arista_i.numeroViajesCubiertos<<endl;
										cout<<"Subíndice Salida = "<<arista_i.subIndiceContenedorSalidaArista<<endl;
										cout<<"Subíndice Llegada = "<<arista_i.subIndiceContenedorLlegadaArista<<endl;
										cout<<"Costo del itinerario atendido por el depósito ("<<arista_i.depositoArista.idDeposito<<") = "<<arista_i.costo<<endl;	
										//getchar();//Pausar la ejecución
										
										//Actualizar el contador de depósitos
										++vehiculosUsadosPorDeposito[m];
																			
										break;//Salir del ciclo, una vez se ha identificado la arista									
										
									}
																		
									++auxiliarSubIndiceArista;//Incremento del subíndice en el conjunto de aristas
									
								}//For que recorre las aristas del grafo para encontrar la arista asociada a la variable activa en el conjunto solución						
								
							}//Fin del if que valida si una arista x_{ij}^m está activa							
							
						}//Fin de validación con la tabla hash de aristas activas					
						
					}//Recorrido de los M depósitos
					
				}//Recorrido de los nodos de llegada 'j'
				
			}//Recorrido de los nodos de llegada 'i'
			
			//Salidas de diagnóstico
			for(auto m = 0; m<M; ++m){
				cout<<"Vehículos empleados en el depósito "<<m<<" = "<<vehiculosUsadosPorDeposito[m]<<"/"<<this->mdvspdata.depositos[m].numeroVehiculos<<endl;				
				numeroVehiculosSolucion += vehiculosUsadosPorDeposito[m];
			}			
			cout<<"Número de itinerarios seleccionados = "<<numeroAristasSeleccionadas<<endl;			
			cout<<endl<<"-----> F.O. = "<<costoSuperG<<endl;					
			
			////Mostrar el uso de cada depósito
			//cout<<endl;
			//long int autonumericoDepositos = 0;
			//for(auto& depositosUsados: itinerariosAsignados){
			//	cout<<"Depósito "<<autonumericoDepositos<<" = "<<depositosUsados<<" vehículos usados de "<<this->mdvspdata.depositos[autonumericoDepositos].numeroVehiculos<<endl;
			//	//Validar que no se exceda la flota del depósito
			//	if(depositosUsados > this->mdvspdata.depositos[autonumericoDepositos].numeroVehiculos){
			//		banderaFactibilidadVehiculos = false;
			//	}				
			//	++autonumericoDepositos;
			//}
			//
			////Mostrar en pantalla costos finales
			//cout<<endl<<"Costo sin asignación de depósitos = "<<secuenciaServicios.costoSinDepositos;			
			//secuenciaServicios.costoConDepositos += cplex.getObjValue() + secuenciaServicios.costoSinDepositos;
			//cout<<endl<<"Costo con asignación de depósitos = "<<secuenciaServicios.costoConDepositos<<endl;
			//
			////Pasar por referencia los valores obtenidos a las variables del constructivo
			//costoG = secuenciaServicios.costoSinDepositos;
			//costoSpuerG = secuenciaServicios.costoConDepositos;
			//tiempoSolucionModeloAsignacionGeneralizada = tiempo.getTime();			
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo de flujo mínimo en una red)";
			
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP
	
	
	//Implementación del modelo matemático para MDVSP planteado en Mesquita (1992)
	//void modeloMDVSP_Mesquita1992(){
	void modeloMDVSP_Mesquita1992(vector<vehiculo> itinerariosGA, vector< vector<int> > arregloAristasDepositos){
		
		//Declaración de variables locales (siguiendo notación modelo Mesquita, 1992)
		double tiempolimite;
		long int n = this->mdvspdata.numeroViajes;//Número de servicios
		long int k = this->mdvspdata.numeroDepositos;//Número de depósitos		
		vector <long int> d(k);//Número de vehículos en cada uno de los depósitos
		for(size_t l=0;l<k;++l){
			d[l] = this->mdvspdata.depositos[l].numeroVehiculos;//Obtenerlo de los datos leídos
		}		
					
		//////////////////////CONCERT & CPLEX
			
		//******************************************************************
		//* 			VARIABLES DEL ENTORNO CPLEX Y DEL MODELO
		//******************************************************************
		
		IloEnv env; //Entorno de trabajo		
		IloModel modelo(env); //Modelo que se va a construir y resolver					
		
		IloTimer tiempo(env); //Tiempo de ejecución (Sólo cuenta luego de la lectura de datos)	
		//tiempo.reset(), tiempo.start(); tiempo.start(); //Instrucciones del cronómetro interno		
		
		//Salida de dignóstico
		//cout<<endl<<"Tamaño (n+k) ----> "<<(n+k)<<endl;getchar();
		
		//Creación de variables x (Transiciones entre servicios)
		//x_ij = 1, cuando el servicio 'j' es atendido después de completar el servicio 'i'. De lo contrario 0.
		//x_{n+l,j} = 1, cuando el servicio 'j' es atendido desde el depósito 'l'. De lo contrario 0 (por lo tanto es atendido por otro depósito)
		//x_{i,n+l} = 1, cuando después de atender el servicio 'i' el vehículo regresa al depósito l
		IloArray<IloBoolVarArray> x(env, n+k);
		stringstream name;
		for(auto i = 0u; i < n+k; ++i) {
			x[i] = IloBoolVarArray(env, n+k);
			for(auto j = 0u; j < n+k; ++j) {
				name << "x_" << i << "_" << j;
				x[i][j] = IloBoolVar(env, name.str().c_str());
				name.str(""); // Clean name
			}
		}
		
		////Salida de dignóstico
		//cout<<endl<<"GetSize x ----> "<<x.getSize()<<endl;//getchar();
		//cout<<endl<<"GetSize x[n+k] ----> (n+k = "<< n+k <<" ) "<<x[(n+k)-1].getSize()<<endl;//getchar();		
		
		//Creación de variables y_il (asignación de servicios a depósitos)
		//y_il = 1, cuando el servicio 'i' es asignado al depósito 'l'. De lo contrario 0 (será asignado a otro depósito).		
		name.str(""); // Clean name
		IloArray<IloBoolVarArray> y(env, n);		
		for(auto i = 0u; i < n; ++i) {
			y[i] = IloBoolVarArray(env, k);
			for(auto l = 0u; l < k; ++l) {
				name << "y_" << i << "_" << l;
				y[i][l] = IloBoolVar(env, name.str().c_str());
				name.str(""); // Clean name
			}
		}
		
		///////////////////////////////////////////////////////////////////////////////////
		//Construir solución inicial para el modelo a partir de los itinerarios recibidos//
		///////////////////////////////////////////////////////////////////////////////////		
		
		//Variable para almacenar la solución recibida del genético
		IloArray<IloBoolArray> xSol(env, n+k);		
		for(auto i = 0u; i < n+k; ++i)
			xSol[i] = IloBoolArray(env, n+k);
		
		//Recorrer los itinerarios de la solución
		for(auto& vehiculo_i : itinerariosGA){			
			
			//Apuntar al penúltimo viaje para controlar los límites al iterar y obtener transiciones (arcos)
			list<viaje>::iterator itUltimoViajeSecuencia = vehiculo_i.listadoViajesAsignados.end();
			--itUltimoViajeSecuencia;//Límite del for correspondiente
			
			//Por cada una de las transiciones, actualizar los contenedores
			for(list<viaje>::iterator itListaViajes = vehiculo_i.listadoViajesAsignados.begin(); itListaViajes != itUltimoViajeSecuencia ; ++itListaViajes){			
				
				//Apuntar al viaje siguiente para la construcción de los arcos
				list<viaje>::iterator itViajeSiguiente = itListaViajes;
				++itViajeSiguiente;
				
				//Activar el arco correspondiente				
				xSol[ (*itListaViajes).idViaje ][ (*itViajeSiguiente).idViaje ] = 1;
				
			}		
			
		}//Fin del recorrido de los itinerarios
		
		////Mostrar aristas de salida y retorno (salida de diagnóstico)
		//cout<<endl<<"Aristas de salida y retorno: "<<endl;
		//for(auto& arista_i:arregloAristasDepositos){
		//	cout<<endl<<arista_i[0]<<"----"<<arista_i[1];
		//}
		//cout<<endl;getchar();		
		
		//Activar las aristas de salida y retorno
		for(size_t i=0;i<arregloAristasDepositos.size();++i){			
			//Activar el arco correspondiente				
			xSol[ arregloAristasDepositos[i][0] ][ arregloAristasDepositos[i][1] ] = 1;
		}
		
		//Cargar la solución del genético en los contenedores correspondientes 
		IloNumVarArray startVar(env);
		IloNumArray startVal(env);
		for(auto i = 0u; i < n; ++i){		
			for(auto j = 0u; j < n; ++j){				
				if(i!=j){
					
					//cout<<endl<<"Entra! "<<i<<", "<<j<<endl;
										
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				}									
				
			}
			//xSol[i].end(); // free
		}
		//xSol.end(); // free
		
		//Salidas del depósito
		for(auto i = n; i < n+k; ++i){		
			for(auto j = 0u; j < n; ++j){				
				if(i!=j){			
										
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				}									
				
			}
			//xSol[i].end(); // free
		}
		//xSol.end(); // free
		
		//Entradas a los depósitos
		for(auto i = 0u; i < n; ++i){		
			for(auto j = n; j < n+k; ++j){				
				if(i!=j){			
										
					startVar.add(x[i][j]);
					startVal.add(xSol[i][j]);
				}									
				
			}
			//xSol[i].end(); // free
		}
		//xSol.end(); // free
		
		//Liberar memoria
		for(auto i = 0u; i < n+k; ++i){					
			xSol[i].end(); // free
		}
		xSol.end(); // free
		
		///////////////////////////////////////////////////////////////////////////////////			
				
		//******************************************************************
		//* 					RESTRICCIONES DEL MODELO
		//******************************************************************
		
		//(1) Conjunto de restricciones para asegurar que después de atender el servicio 'i' se proceda, o a atender otro servicio o, regresar a un depósito, pero no ambas situaciones.			
		IloRangeArray restriccionesDespuesDeServicio(env, n);
		stringstream nombreRestriccionesDespuesDeServicio;		
		for(auto i = 0u; i < n; ++i){		
			//Crear una expresión por cada restricción y luego adicionarla al modelo y reiniciarla
			IloExpr restriccionDespuesDeServicio(env);
			for(auto j = 0u; j < n+k; ++j) {
				if(i!=j){
					restriccionDespuesDeServicio += x[i][j];										
				}				
			}			
			nombreRestriccionesDespuesDeServicio<<"DespuesServicio_"<<i;		
			restriccionesDespuesDeServicio[i] = IloRange(env, 1, restriccionDespuesDeServicio, 1, nombreRestriccionesDespuesDeServicio.str().c_str());
			nombreRestriccionesDespuesDeServicio.str(""); // Limpiar variable para etiquetas de las restricciones		
		}//Fin de la generación del conjunto de restricciones		
		modelo.add(restriccionesDespuesDeServicio);//Adicionar el conjunto de restricciones (1) Mesquita (1992)		
		
		//(2) Conjunto de restricciones para que los vehículos saliendo desde un depósito o desde un servicio, pasen solamente a servicios
		IloRangeArray restriccionesAntesDeServicio(env, n);
		stringstream nombreRestriccionAntesDeServicio;		
		for(auto j = 0u; j < n; ++j){		
			//Crear una expresión por cada restricción y luego adicionarla al modelo y reiniciarla
			IloExpr restriccionAntesDeServicio(env);
			for(auto i = 0u; i < n+k; ++i) {
				if(i!=j){
					restriccionAntesDeServicio += x[i][j];										
				}				
			}			
			nombreRestriccionAntesDeServicio<<"AntesServicio_"<<j;		
			restriccionesAntesDeServicio[j] = IloRange(env, 1, restriccionAntesDeServicio, 1, nombreRestriccionAntesDeServicio.str().c_str());
			nombreRestriccionAntesDeServicio.str(""); // Limpiar variable para etiquetas de las restricciones		
		}//Fin de la generación del conjunto de restricciones		
		modelo.add(restriccionesAntesDeServicio);//Adicionar el conjunto de restricciones (2) Mesquita (1992)
		
		
		//(3) Conjunto de restricciones para no sobrepasar la capacidad de atención de los depósitos (número de vehículos de cada depósito)
		IloRangeArray restriccionesCapacidadDepositos(env, k);
		stringstream nombreRestriccionCapacidadDeposito;
		for(auto l = 0u; l < k; ++l){//Por cada uno de los depósitos (nodos n+l)			
			
			//Despejar al lado izquierdo la restricción, para adaptarla al formato IloRange
			IloExpr ladoIzquierdoRestriccion(env);					
			for(auto j=0u; j<n ;++j){				
				ladoIzquierdoRestriccion += x[n+l][j];		
			}
			ladoIzquierdoRestriccion -= d[l];
			
			//Nombrar y almacenar en el arreglo que contiene este conjunto de restricciones			
			nombreRestriccionCapacidadDeposito << "VehiculosDeposito_"<<n+l<<"||"<<l;						
			restriccionesCapacidadDepositos[l] = IloRange(env, -IloInfinity, ladoIzquierdoRestriccion, 0, nombreRestriccionCapacidadDeposito.str().c_str());
			nombreRestriccionCapacidadDeposito.str(""); // Limpiar variable para etiquetas de las restricciones			
			
		}
		modelo.add(restriccionesCapacidadDepositos);//Adicionar el conjunto de restricciones (3) Mesquita (1992)		
		
		//(3') Restricción uso general de flota (UB vehículos)		
		IloExpr restriccionUsoFlota(env);
		IloRangeArray contenedorRestriccionUsoFlota(env);
		stringstream nombreRestriccionUsoFlota;
		int limiteGeneralVehiculos = 0;
		for(auto& vehiculo_i : itinerariosGA){
			if(!vehiculo_i.listadoViajesAsignados.empty())
				++limiteGeneralVehiculos;
		}
		//limiteGeneralVehiculos = 35;		
		//Iterar por cada depósito
		for(auto l = 0u; l < k; ++l){						
			//Una salida a cada servicio desde el depósito 'l'			
			for(auto j = 0u; j < n; ++j){
				restriccionUsoFlota += x[n+l][j];
			}											
		}	
		restriccionUsoFlota -= limiteGeneralVehiculos;	
		//Nombrar restricción y adicionarla al modelo
		nombreRestriccionUsoFlota << "UB_Flota";						
		contenedorRestriccionUsoFlota.add(IloRange(env, -IloInfinity, restriccionUsoFlota, 0, nombreRestriccionUsoFlota.str().c_str()) );
		nombreRestriccionUsoFlota.str("");		
		modelo.add(contenedorRestriccionUsoFlota);//Adicionar restricción obtenida con el UB obtenido por heurística		
		
		//(4) Conjunto de restricciones para controlar asignación, cuando 'j' es el primer viaje del itinerario, cadena o camino
		IloRangeArray restriccionesAsignacionPrimerViaje(env);
		stringstream nombreRestriccionAsignacionPrimerViaje;
		for(auto l = 0u; l < k; ++l){//Por cada uno de los depósitos (nodos n+l)		
			for(auto j = 0u; j < n; ++j) {//Por cada uno de los servicios con los que se inician itinerarios 									
				//Crear una expresión por cada restricción
				IloExpr restriccionAsignacionPrimerViaje(env);					
				restriccionAsignacionPrimerViaje = x[n+l][j] - y[j][l];					
				nombreRestriccionAsignacionPrimerViaje<<"AsignacionPrimerViaje_j_l_"<<j<<"_"<<l;
				restriccionesAsignacionPrimerViaje.add( IloRange(env, -IloInfinity, restriccionAsignacionPrimerViaje, 0, nombreRestriccionAsignacionPrimerViaje.str().c_str()) );
				nombreRestriccionAsignacionPrimerViaje.str(""); // Limpiar variable para etiquetas de las restricciones	
			}			
			
		}
		modelo.add(restriccionesAsignacionPrimerViaje);//Adicionar el conjunto de restricciones (4) Mesquita (1992)	
		
		//(5) Conjunto de restricciones para controlar asignación, cuando 'i' es el último viaje del itinerario, cadena o camino
		IloRangeArray restriccionesAsignacionUltimoViaje(env);
		stringstream nombreRestriccionAsignacionUltimoViaje;
		for(auto l = 0u; l < k; ++l){//Por cada uno de los depósitos (nodos n+l)		
			for(auto i = 0u; i < n; ++i) {//Por cada uno de los servicios con los que se finalizan los itinerarios 									
				//Crear una expresión por cada restricción
				IloExpr restriccionAsignacionUltimoViaje(env);					
				restriccionAsignacionUltimoViaje = x[i][n+l] - y[i][l];					
				nombreRestriccionAsignacionUltimoViaje<<"AsignacionUltimoViaje_i_l_"<<i<<"_"<<l;
				restriccionesAsignacionUltimoViaje.add( IloRange(env, -IloInfinity, restriccionAsignacionUltimoViaje, 0, nombreRestriccionAsignacionPrimerViaje.str().c_str()) );
				nombreRestriccionAsignacionUltimoViaje.str(""); // Limpiar variable para etiquetas de las restricciones	
			}			
			
		}
		modelo.add(restriccionesAsignacionUltimoViaje);//Adicionar el conjunto de restricciones (5) Mesquita (1992)			
		
		//(6) Conjunto de restricciones para controlar que si dos servicios están conectados entre sí, estén asignados entonces al mismo depósito
		IloRangeArray restriccionesAsignacionConectados(env);
		stringstream nombreRestriccionAsignacionConectados;
		for(auto l = 0u; l < k; ++l){//Por cada uno de los depósitos (nodos n+l)
			//Relacionar los servicios entre sí (todos con todos sin bucles)
			for(auto i = 0u; i < n; ++i){
				for(auto j = 0u; j < n; ++j){
					if(i!=j){						
						IloExpr restriccionAsignacionConectados(env);						
						restriccionAsignacionConectados = y[i][l] + x[i][j] - y[j][l];
						nombreRestriccionAsignacionConectados<<"ServiciosConectadosDeposito_i_j_l_"<<i<<"_"<<j<<"_"<<l;
						restriccionesAsignacionConectados.add( IloRange(env, -IloInfinity, restriccionAsignacionConectados, 1, nombreRestriccionAsignacionConectados.str().c_str()) );
						nombreRestriccionAsignacionConectados.str(""); // Limpiar variable para etiquetas de las restricciones
					}
				}								
			}			
		}
		modelo.add(restriccionesAsignacionConectados);//Adicionar el conjunto de restricciones (6) Mesquita (1992)			
		
		//(7) Conjunto de restricciones para controlar asignación de cada servicio a un único depósito 
		IloRangeArray restriccionesAsignacionUnica(env, n);
		stringstream nombreRestriccionAsignacionUnica;		
		for(auto i = 0u; i < n; ++i){		
			//Crear una expresión por cada restricción y luego adicionarla al modelo y reiniciarla
			IloExpr restriccionAsignacionUnica(env);
			for(auto l = 0u; l < k; ++l) {				
				restriccionAsignacionUnica += y[i][l];														
			}			
			nombreRestriccionAsignacionUnica<<"AsignacionUnica_i_"<<i;		
			restriccionesAsignacionUnica[i] = IloRange(env, 1, restriccionAsignacionUnica, 1, nombreRestriccionAsignacionUnica.str().c_str());
			nombreRestriccionAsignacionUnica.str(""); // Limpiar variable para etiquetas de las restricciones		
		}//Fin de la generación del conjunto de restricciones		
		modelo.add(restriccionesAsignacionUnica);//Adicionar el conjunto de restricciones (7) Mesquita (1992)					
		
						
		//************************************************************************
		//* FUNCIÓN OBJETIVO (Circuitos o cadenas mínimas en el grafo)
		//************************************************************************		
		
		//Adicionar la función objetivo
		IloExpr obj(env);	
		
		//Primer término de la función objetivo (costos de transiciones entre servicios)			
		for(auto i = 0u; i < n; ++i){		
			for(auto j = 0u; j < n; ++j){
				if(i!=j){					
					obj += this->mdvspdata.matrizViajes[i + k][j + k] * x[i][j];					
				}
			}			
		}
		
		//Segundo término de la función objetivo (costos de entradas y salidas a los diferentes depósitos)
		for(auto l = 0u; l < k; ++l){
			
			//Sumatoria de regresos al depósito
			IloExpr sumatoriaRetornos(env);	
			for(auto i = 0u; i < n; ++i){
				sumatoriaRetornos += this->mdvspdata.matrizViajes[i + k][l] * x[i][n+l];
			}			
			
			//Sumatoria de salidas del depósito
			IloExpr sumatoriaSalidas(env);	
			for(auto j = 0u; j < n; ++j){
				sumatoriaSalidas += this->mdvspdata.matrizViajes[l][j + k] * x[n+l][j];				
			}
			
			//Sumatoria de entradas y salidas al depósito
			obj += sumatoriaRetornos + sumatoriaSalidas;
			
		}		
		
		//Especificar si es un problema de maximización o de minimización
		modelo.add(IloMinimize(env,obj)); 
		obj.end();
		
		//******************************************************************
		//* 		 	SOLUCIÓN DEL MODELO DE FLUJO MÍNIMO
		//******************************************************************
		
		//IloCplex cplex(env); //Invocación del solver
		IloCplex cplex(modelo); //Invocación del solver
		
		//Silenciar el solver (salidas en pantalla)
		env.setOut(env.getNullStream());	
		
		//******************************************************************
		//* 					FEEDBACK 
		//******************************************************************
		
		//Generación del archivo lp para observar el modelo que fue construído
		cplex.extract(modelo);
		//cplex.exportModel("MDVSP_Mesquita1992.lp");
		
		//Limitar el tamaño del árbol solución en MB
		cplex.setParam(IloCplex::TreLim, 25000.0);
		
		//Límite de tiempo de ejecución en segundos
		//int timeLimit = 10; 
		//int timeLimit = 7200; //Dos horas
		//int timeLimit = 14400; //Cuatro horas
		int timeLimit = 43200; //Doce horas
		//int timeLimit = 21600; //Seis horas
		cplex.setParam(IloCplex::TiLim, timeLimit);
		
		//Tolerancia para el criterio de parada de CPLEX
		cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1e-5);
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		
		//Establecer variantes de solución
		int optimization = 1;// optimization -> 1: Primal, 2: Dual, 3: Network, 4: Barrier, 5: Sifting, 6: Concurrent
		cplex.setParam(IloCplex::IntParam::RootAlg, optimization);
		
		//Variable para validar la duración total del tour según la velocidad y los tiempos de servicio de los clientes
		double tiempoTotalSolucion = 0;	
		
		// Load initial Solution
		cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartAuto, "MIPStart");		
		
		//Variables para imprimir el tiempo de solución del modelo y la función objetivo
		double tiempoSolucionModelo;
		long double funcionObjetivoModeloMesquita1992;
		
		//Validar si el solver encontró una solución y cargarla en las variables correspondientes
		//cplex.setParam(IloCplex::Threads, 1);//Establecer el número de hilos del solver
		tiempo.start();//Iniciar el cronómetro
		if(cplex.solve()){
			
			//Parar el cronómetro del entorno
			tiempo.stop();		
			
			//Valor de la función objetivo al solucionar el problema
			cout<<endl<<"-----> Valor de la función objetivo MDVSP Mesquita1992: "<<cplex.getObjValue()<<endl;
			funcionObjetivoModeloMesquita1992 = cplex.getObjValue();
			
			//Mostrar en pantalla el tiempo transcurrido en segundos
			cout<<endl<<"Tiempo solución modelo MDVSP Mesquita1992 (segundos) = "<<tiempo.getTime()<<endl;		
			tiempoSolucionModelo = tiempo.getTime();
			
			//Pausar salida para apreciar la función objetivo (salida de diagnóstico)
			//getchar();
			
			//Mostrar en pantalla los arcos que se activan para solucionar la instancia
			double costoTotal = 0;
			
			////Salida de dignóstico
			//cout<<endl<<"GetSize x ----> "<<x.getSize()<<endl;getchar();
			//cout<<endl<<"GetSize x[n+k] ----> "<<x[(n+k)-1].getSize()<<endl;getchar();
			
			////Salida de diagnóstico
			//size_t sub_i = 100;
			//size_t sub_j = 101;			
			//cout<<endl<<"Prueba-> x("<<sub_i<<","<<sub_j<<") = "<<cplex.getValue(x[sub_i][sub_j])<<endl;getchar();
			
			//Variables correspondientes a los arcos
			unordered_map <int,int> contenedorAristas;
			for(auto i = 0u; i < n; ++i){
				for(auto j = 0u; j < n; ++j){				
					if(i!=j && cplex.getValue(x[i][j])>0.95){					
					//if(i!=j && cplex.getValue(x[i][j])>0.95 && (i+j)<(n*2) ){					
					//if(i!=j && cplex.getValue(x[i][j])==1){
					//if(i!=j && cplex.getValue(x[i][j])>0.00){	
						
						//double costoRelacionDeposito = 0;
						//if(i>=n){
						//	if(j>=n){
						//		cout<<"x("<<i<<","<<j<<") = "<<cplex.getValue(x[i][j])<< " Costo = "<<this->mdvspdata.matrizDistancias[ik][j]<<endl;;
						//	}							
						//	
						//}
						
						//Salida sin costo
						cout<<"x("<<i<<","<<j<<") = "<<cplex.getValue(x[i][j])<<endl;
						
						//Acumular costo
						//costoTotal += this->prpdata.distanceij[i][j];
						
					}				
				}		
			}
			
			cout<<"---------------------------------------------------------------------------"<<endl;
			
			//Variables correspondientes a la asignación
			for(size_t l=0;l<k;++l){
				for(size_t i=0;i<n;++i){
					if(cplex.getValue(y[i][l])>0.95){					
						cout<<"y("<<i<<","<<l<<") = "<<cplex.getValue(y[i][l])<<endl;
					}
				}
			}
			
			//Agragar columnas con la información de la solución del modelo a la salida del genético
			////////////////////////////////////////////////////////////////////////////////////////
					
			//Creación o apertura del archivo de salida para registrar la incumbente del algoritmo genético
			ofstream archivoSalida("salidaGenetico.log",ios::app | ios::binary);		
			archivoSalida<<setprecision(10);//Cifras significativas para el reporte
			if (archivoSalida.is_open()){
				//Reporte del constructivo en el archivo de salida				
				archivoSalida<<" "<<funcionObjetivoModeloMesquita1992;
				archivoSalida<<" "<<tiempoSolucionModelo;				
			}else{
				cout << "Fallo en la apertura del archivo de salida para poner resultados del modelo Mesquita1992";
			}			
			
			//Cerrar el controlador del archivo de salida
			archivoSalida.close();		
			
			//Decodificar solución
			
			////Revisar si el tour obtenido es válido				
			//unordered_set<int> clientesCicloTSP;
			//
			////Inicializar conjunto de validación con la salida del depósito (Arista 0-j)
			//clientesCicloTSP.insert(0);
			//int nodo_j_ultimaAristaAdicionada = contenedorAristas[0];
			//clientesCicloTSP.insert(nodo_j_ultimaAristaAdicionada);				
			//contenedorAristas.erase(0);				
			//
			////Realizar el proceso hasta que todos los nodos del grafo estén cubiertos
			//while(clientesCicloTSP.size() < cardinalidadN){					
			//	
			//	//Buscar la conexión del ciclo parcial con las aristas de la solución del modelo
			//	unordered_map<int,int>::iterator itContenedorAristas = contenedorAristas.find(nodo_j_ultimaAristaAdicionada);
			//	
			//	//Si se encontró la conexión, incluír el cliente j de dicha conexión en el ciclo TSP
			//	if( itContenedorAristas != contenedorAristas.end()){
			//		
			//		//Antes de incluirlo, validar que no forme subtour (que ya se encuentre en el ciclo TSP)
			//		unordered_set<int>::iterator itTSP = clientesCicloTSP.find( itContenedorAristas->second );
			//		if( itTSP == clientesCicloTSP.end()){
			//			
			//			clientesCicloTSP.insert(itContenedorAristas->second);
			//			nodo_j_ultimaAristaAdicionada = itContenedorAristas->second;
			//			
			//		}else{//Si ya se encuentra en el ciclo, terminar
			//			
			//			break;
			//			
			//		}
			//		
			//		////Salida de diagnóstico
			//		//cout<<endl<<"Estado actual del conjunto de aristas: "<<endl;
			//		//for(auto& aristas: contenedorAristas){
			//		//	cout<<aristas.first<<" - "<<aristas.second<<endl;
			//		//}
			//		//cout<<endl<<"Estado actual del conjunto de nodos: "<<endl;
			//		//for(auto& nodos: clientesCicloTSP){
			//		//	cout<<nodos<<endl;
			//		//}
			//		//cout<<endl<<"--------------------------------"<<endl;
			//		//getchar();						
			//		
			//	}else{
			//		
			//		break;//Solución inválida o incompleta
			//		
			//	}
			//	
			//}//Fin del while que revisa si todos los clientes han sido incluídos en la solución			
			
			//Parar salida en pantalla (diagnóstico)
			//getchar();					
			
		}else{
			
			//Reportar si el modelo construído es infactible
			cout<<endl<<"INFACTIBLE! (Revisar modelo Mesquita (1992) )";
						
		}			
			
		env.end();//Cierre del entorno CPLEX
		/////////////////////////////////////////////////////////////CONCERT & CPLEX
	
		
	}//Fin del modelo para resolver el problema de asignación generalizada GAP	
	
	
	
	
	
	
};//Fin de la estructura general



//Sección principal
int main(int argc, char *argv[]) {
	
	
	//Obtener la semilla al comienzo de la ejecución de la rutina	
	srand(time(NULL));	
		
	cerr << "CSP build " <<  __DATE__ << " " << __TIME__ << endl;
    
	//Códigos de librerías y/o variantes de scheduling
	// 0 - CSP	
	
	//Establecer instancia por defecto o la que se ha entrado por línea de comandos
	//string testname = argc > 1 ? argv[1] : "./instances/mdvrptw/pr01";	
	string testname = argc > 1 ? argv[1] : "instances/prueba1.csp";       
	
	//Establecer la variante por defecto o la que se ha entrado por línea de comandos
	unsigned int codigoVariante;
	if(argc > 2){
		//Pasar el parámetro de entrada correspondiente a la variante de csp a entero
		stringstream strValue;
		strValue << argv[2];			
		strValue >> codigoVariante;			
	}else{
		codigoVariante = 0;			
	}
	
	//Evaluar si se está cargando BKS, sinó colocar el BKS para la opción por defecto
	//string bksname = argc > 3 ? argv[3] : "./instances/prp/BKS/UK10_01.txt";			
	
	//Estructura del llamado genérico
	//VSP caso(1,testname);//Caso 1, omite carga de datos de Vehicle o Crew Scheduling
	//VSP caso();
	
	/// ///////////////////////////////////////////
	///Carga y llamado para Modelo Bohórquez 2015//
	/// ///////////////////////////////////////////
	
	////Cargar información del archivo especificado por parámetro
	//casoRostering instancia(testname, codigoVariante);
	//
	////Llamado al modelo de rostering Bohórquez, 2015
	//caso.modeloBohorquez(instancia);
	
	/// /////////////////////////////////////////
	///Carga y llamado para Modelo Musliu 2006 //
	/// /////////////////////////////////////////
	
	////Cargar información del archivo especificado por parámetro
	//casoRostering instancia(testname, codigoVariante);
	//
	////Llamado al modelo de rostering Musliu, 2006
	//caso.modeloMusliu(instancia);
	
	/// /////////////////////////////////////////////////
	///Carga y llamado para Modelo Integra - UniAndes  //
	/// /////////////////////////////////////////////////
	
	//Cargar información del archivo especificado por parámetro
	casoCrewScheduling instancia(testname, codigoVariante);	
	
	//Llamado al modelo esclavo Integra UniAndes
	//instancia.modeloEsclavo();getchar();
	
	//Mostrar solución del esclavo
	//instancia.mostrarBloquesEsclavo();getchar();
	
	//Mostrar resultados de la heurística
	//instancia.HeuristicaParticion();	
	
	//Llamado al modelo maestro Integra UniAndes
	//instancia.modeloGCMaestro();getchar();	
	//instancia.modeloGCMaestroRelajado();getchar();	
	
	//Llamado al modelo auxiliar Integra UniAndes
	//instancia.modeloGCAuxiliar();	
	
	//Llamado a la metodología general
	//instancia.generacionTurnosTrabajoDanielUniAndes();

	//Llamado al modelo de BPP para obtener el número mínimo de conductores
	//instancia.modeloBPP_CSP();
	//instancia.generarGrafoDot();//Generar el grafo solución

	//Llamado a la búsqueda del K_min
	//instancia.busquedaK_min();

	//Llamado a la modelo modificado Beasley96-Borcinova17 - Instancias Literatura
	//instancia.modeloBeasleyModificado();
	//instancia.generarGrafoDot();//Generar el grafo solución	

	//Llamado a la modelo modificado (arcos) Beasley96-Borcinova17 - Instancias Literatura
	instancia.modeloBeasleyModificadoArcos();
	instancia.generarGrafoDot();//Generar el grafo solución

	//Llamado al modelo con K como variable y objetivo de minimización
	//instancia.modeloK_Variable();
	//instancia.generarGrafoDot();//Generar el grafo solución


	/// /////////////////////////////////////////
	///Ejercicios de Implementación			   //
	/// /////////////////////////////////////////
	
	//Llamado Ejemplo 1
	//caso.modeloEjemplo1();	
	
	//Llamado Ejemplo 2
	//caso.modeloEjemplo2();
	
	//Llamado Ejemplo Compartimientos Avión
	//caso.modeloAvion();
	
	//Llamado Sudoku Kenny
	//caso.modeloSudoku();
    
	//Terminación sin errores
    return 0;
}
