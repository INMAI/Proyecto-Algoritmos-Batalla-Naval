#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <windows.h>
using namespace std;

const int TAM = 8; // Tamaño del tablero 8x8

// Datos de cada jugador y sus estadísticas
struct Jugador {
    string nombre, alias, codigo;
    int partidasJugadas = 0, partidasGanadas = 0;
    int disparosRealizados = 0, disparosAcertados = 0;
};

// Tablero de un jugador
struct Tablero {
    char celda[TAM][TAM];  // ~ agua, B barco, X impacto, O disparo fallido
    int idBarco[TAM][TAM]; // número del barco que ocupa cada casilla (-1 si es agua)
    vector<int> vida;      // casillas que le quedan vivas a cada barco
};

// Aquí se guardan todos los jugadores registrados
vector<Jugador> jugadores;

// Cambia el color del texto que se imprime después
void setCursorColor(int color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); }

// Imprime un texto en el color indicado y vuelve al color normal (7)
void msg(int color, const string& texto) { setCursorColor(color); cout << texto; setCursorColor(7); }

// Borra la pantalla
void limpiar() { system("cls"); }

// Espera a que el usuario presione una tecla
void pausa() { system("pause"); }

// Limpia la pantalla y muestra el título de la sección
void pantalla(int color, const string& titulo) { limpiar(); msg(color, titulo); }

// Calcula un porcentaje; si el total es 0 devuelve 0 para no dividir entre cero
double porcentaje(int parte, int total) { return (total > 0) ? parte * 100.0 / total : 0.0; }

// Pasa un texto a mayúsculas para comparar sin importar cómo lo escribieron
string mayus(string s) {
    for (size_t i = 0; i < s.length(); i++)
        s[i] = toupper((unsigned char)s[i]); // el (unsigned char) evita problemas con letras con tilde
    return s;
}

// Arma el código: 3 primeras letras del nombre + 1ra del alias + 2 últimas del alias (Carlos, Destructor -> CARDOR)
string generarCodigo(const string& nombre, const string& alias) {
    return mayus(nombre.substr(0, 3) + alias.substr(0, 1) + alias.substr(alias.length() - 2)); // substr(alias.length() - 2) toma las últimas 2 letras
}

// Busca un jugador por código o por alias y devuelve su posición en el vector (-1 si no existe)
int buscarIndice(string termino) {
    termino = mayus(termino); // así da igual si escribieron en minúsculas
    for (int i = 0; i < (int)jugadores.size(); i++)
        if (jugadores[i].codigo == termino || mayus(jugadores[i].alias) == termino) return i;
    return -1;
}

// Quita los espacios sobrantes al inicio y al final de un texto
string trim(const string& s) {
    size_t primero = s.find_first_not_of(" \t\r\n"); // posición de la primera letra que no es espacio
    if (primero == string::npos) return ""; // npos = no encontró ninguna, o sea que solo había espacios
    size_t ultimo = s.find_last_not_of(" \t\r\n");  // posición de la última letra que no es espacio
    return s.substr(primero, (ultimo - primero + 1)); // se queda con lo que hay entre las dos
}

// Lee jugadores.txt; los campos van separados por '|' para que nombre y alias puedan llevar espacios
void cargarJugadores() {
    jugadores.clear();
    ifstream archivo("jugadores.txt"); // si todavía no existe, simplemente no se lee nada
    string linea;
    while (getline(archivo, linea)) { // cada vuelta lee una línea completa, que es un jugador
        if (linea.empty()) continue;
        stringstream ss(linea); // deja leer la línea por pedazos, como si fuera un archivo
        Jugador j;
        string temp;

        getline(ss, j.nombre, '|'); // lee hasta encontrar el '|'
        getline(ss, j.alias, '|');
        getline(ss, j.codigo, '|');
        if (getline(ss, temp, '|')) j.partidasJugadas = stoi(temp); // stoi convierte el texto a número
        if (getline(ss, temp, '|')) j.partidasGanadas = stoi(temp);
        if (getline(ss, temp, '|')) j.disparosRealizados = stoi(temp);
        if (getline(ss, temp, '|')) j.disparosAcertados = stoi(temp);

        jugadores.push_back(j);
    }
}

// Reescribe jugadores.txt con todos los jugadores, uno por línea y separados por '|'
void guardarJugadores() {
    ofstream archivo("jugadores.txt"); // Esto borra lo anterior y escribe todo de nuevo, no se debe usar ios::app 
    for (size_t i = 0; i < jugadores.size(); i++)
        archivo << jugadores[i].nombre << "|" << jugadores[i].alias << "|" << jugadores[i].codigo << "|"
                << jugadores[i].partidasJugadas << "|" << jugadores[i].partidasGanadas << "|"
                << jugadores[i].disparosRealizados << "|" << jugadores[i].disparosAcertados << "\n";
}

// Agrega una línea al final de partidas.txt con el resultado de la partida
void guardarHistorial(const string& j1, const string& j2, const string& ganador, int turnos) {
    ofstream archivo("partidas.txt", ios::app); // ios::app agrega al final sin borrar lo anterior
    archivo << "Enfrentamiento: " << j1 << " vs " << j2 << " | Ganador: " << ganador << " | Turnos totales: " << turnos << "\n";
}

// Deja el tablero con puras casillas de agua y sin barcos
void inicializarTablero(Tablero& t) {
    for (int i = 0; i < TAM; i++)
        for (int j = 0; j < TAM; j++) { t.celda[i][j] = '~'; t.idBarco[i][j] = -1; }
    t.vida.clear();
}

// Dibuja el tablero; si ocultarBarcos es true, los barcos se ven como agua (para que el rival no los vea)
void mostrarTablero(const Tablero& t, bool ocultarBarcos) {
    msg(15, "     0 1 2 3 4 5 6 7\n");
    for (int i = 0; i < TAM; i++) {
        msg(15, "   " + to_string(i) + " ");
        for (int j = 0; j < TAM; j++) {
            char c = t.celda[i][j];
            if (c == 'B' && ocultarBarcos) c = '~'; // solo cambia la copia, no el tablero real
            int color = (c == '~') ? 9 : (c == 'B') ? 8 : (c == 'X') ? 12 : 11; // azul agua, gris barco, rojo impacto, cian fallo
            msg(color, string(1, c) + " "); // string(1, c) convierte el char en texto para poder usar msg
        }
        cout << "\n";
    }
}

// Cuenta cuántas casillas de barco siguen sin ser alcanzadas
int calcularCasillasRestantes(const Tablero& t) {
    int vivas = 0;
    for (int i = 0; i < TAM; i++)
        for (int j = 0; j < TAM; j++)
            if (t.celda[i][j] == 'B') vivas++;
    return vivas;
}

// Cuenta cuántos barcos todavía tienen casillas vivas
int barcosAflote(const Tablero& t) {
    int n = 0;
    for (size_t i = 0; i < t.vida.size(); i++)
        if (t.vida[i] > 0) n++;
    return n;
}

// Le pide al jugador que coloque sus barcos y revisa que no se salgan del tablero ni se encimen
void faseColocacion(Tablero& t, const string& nombre) {
    vector<int> barcos = {4, 3, 2, 2}; // tamaños de la flota: uno de 4, uno de 3 y dos de 2
    for (size_t b = 0; b < barcos.size(); b++) {
        int n = barcos[b];
        while (true) { // se repite hasta que el barco quede bien colocado
            pantalla(14, "\n FASE DE COLOCACION DE: " + nombre + "\n\n");
            mostrarTablero(t, false);
            cout << "\nColocando barcos de " << n << " casillas\n";

            int y, x;
            string ori;
            cout << "Fila (0-7): ";
            cin >> y;
            while (cin.fail() || y < 0 || y >= TAM) {
                cin.clear();            // quita el error de cin para poder leer otra vez
                cin.ignore(1000, '\n'); // borra lo que escribieron mal
                msg(12, "Entrada invalida. Ingrese un numero entre 0 y 7.\n");
                cout << "Fila (0-7): ";
                cin >> y;
            }
            cout << "Columna (0-7): ";
            cin >> x;
            while (cin.fail() || x < 0 || x >= TAM) {
                cin.clear();
                cin.ignore(1000, '\n');
                msg(12, "Entrada invalida. Ingrese un numero entre 0 y 7.\n");
                cout << "Columna (0-7): ";
                cin >> x;
            }
            cout << "Orientacion (H/V): ";
            cin >> ori;
            ori = mayus(ori); // así acepta h o H
            if (ori != "H" && ori != "V") { msg(12, "Orientacion invalida.\n"); pausa(); continue; }

            // dx y dy dicen hacia dónde crece el barco
            int dx = (ori == "H") ? 1 : 0, dy = 1 - dx;
            // se revisa la última casilla del barco para ver si cabe en el tablero
            if (x + dx * (n - 1) >= TAM || y + dy * (n - 1) >= TAM) {
                msg(12, "El barco no cabe en el tablero.\n"); pausa(); continue;
            }
            bool libre = true;
            for (int k = 0; k < n; k++)
                if (t.celda[y + dy * k][x + dx * k] != '~') libre = false; // ya hay otro barco ahí
            if (!libre) { msg(12, "Casillas ocupadas.\n"); pausa(); continue; }

            for (int k = 0; k < n; k++) {
                t.celda[y + dy * k][x + dx * k] = 'B';
                t.idBarco[y + dy * k][x + dx * k] = (int)b; // anota a qué barco pertenece la casilla
            }
            t.vida.push_back(n); // el barco b empieza con n casillas vivas
            break;
        }
    }
    pantalla(14, "\n  FLOTA DE " + nombre + " LISTA\n");
    mostrarTablero(t, false);
    msg(14, "\nMemorice su flota. Luego pase el control al siguiente jugador.\n");
    pausa();
}

// Pide la coordenada, dispara al tablero enemigo y avisa si fue impacto, agua o barco hundido
void turnoDisparo(Tablero& enemigo, Jugador& atacante) {
    int x, y;
    while (true) { // se repite hasta que la casilla sea válida y no se haya disparado antes
        cout << "\nIngrese Fila de ataque (0-7): ";
        cin >> y;
        while (cin.fail() || y < 0 || y >= TAM) {
            cin.clear();
            cin.ignore(1000, '\n');
            msg(12, "Entrada invalida. Ingrese un numero entre 0 y 7.\n");
            cout << "Ingrese Fila de ataque (0-7): ";
            cin >> y;
        }
        cout << "Ingrese Columna de ataque (0-7): ";
        cin >> x;
        while (cin.fail() || x < 0 || x >= TAM) {
            cin.clear();
            cin.ignore(1000, '\n');
            msg(12, "Entrada invalida. Ingrese un numero entre 0 y 7.\n");
            cout << "Ingrese Columna de ataque (0-7): ";
            cin >> x;
        }
        if (enemigo.celda[y][x] != 'X' && enemigo.celda[y][x] != 'O') break; // casilla nueva
        msg(12, "Ya habias disparado aqui. Elige otra casilla.\n");
    }
    atacante.disparosRealizados++; // solo cuentan los disparos válidos
    if (enemigo.celda[y][x] == 'B') {
        enemigo.celda[y][x] = 'X';
        atacante.disparosAcertados++;
        msg(12, "\n>>> IMPACTO DIRECTO <<<\n");
        // idBarco dice qué barco es; se le resta 1 de vida y si llega a 0 está hundido
        if (--enemigo.vida[enemigo.idBarco[y][x]] == 0) msg(14, ">>> BARCO HUNDIDO! <<<\n");
    } else {
        enemigo.celda[y][x] = 'O';
        msg(11, "\n>>> AGUA <<<\n");
    }
}

// Opción 1 del menú: registra un jugador nuevo (nombre y alias pueden llevar espacios)
void registrarJugador() {
    pantalla(11, "--------------------------------------------------------\n               REGISTRAR NUEVO JUGADOR\n--------------------------------------------------------\n");
    Jugador j;
    cin.ignore(1000, '\n'); // quita el Enter que quedó del menú, si no el primer getline leería una línea vacía

    cout << "Ingrese nombre (minimo 3 letras): ";
    getline(cin, j.nombre); // getline lee la línea completa, con espacios
    j.nombre = trim(j.nombre);

    cout << "Ingrese alias (minimo 3 letras): ";
    getline(cin, j.alias);
    j.alias = trim(j.alias);

    // nombre o alias muy cortos (si escribieron solo espacios, trim los deja vacíos y también entran aquí)
    if (j.nombre.length() < 3 || j.alias.length() < 3) {
        msg(12, "Error: el nombre y el alias deben tener al menos 3 caracteres reales.\n");
        pausa();
        return;
    }

    // el '|' separa los campos en jugadores.txt, si estuviera en el nombre se dañaría el archivo
    if (j.nombre.find('|') != string::npos || j.alias.find('|') != string::npos) {
        msg(12, "Error: el nombre y el alias no pueden llevar el simbolo |.\n");
        pausa();
        return;
    }

    j.codigo = generarCodigo(j.nombre, j.alias);
    if (buscarIndice(j.codigo) != -1 || buscarIndice(j.alias) != -1) { // código o alias repetido
        msg(12, "Error: ya existe un jugador con el codigo " + j.codigo + " o con ese alias.\n");
        pausa();
        return;
    }

    jugadores.push_back(j);
    guardarJugadores();
    msg(10, "Jugador guardado. Codigo: " + j.codigo + "\n");
    pausa();
}

// Opción 2 del menú: busca un jugador por código o por alias y muestra sus datos
void buscarJugador() {
    pantalla(11, "-----------------------\n     BUSCAR JUGADOR    \n-----------------------");
    int op;
    cout << "\n1. Buscar por Codigo\n2. Buscar por Alias\nOpcion: ";
    cin >> op;
    while (cin.fail() || op < 1 || op > 2) {
        cin.clear();
        cin.ignore(1000, '\n');
        msg(12, "Entrada invalida. Ingrese un numero entre 1 y 2.\n");
        cout << "Opcion: ";
        cin >> op;
    }

    cin.ignore(1000, '\n'); // quita el Enter que quedó del número, para que getline no lea una línea vacía
    string busqueda;
    if (op == 1) cout << "Ingrese el codigo del jugador que desea buscar: ";
    else cout << "Ingrese el alias del jugador que desea buscar: ";
    getline(cin, busqueda);
    busqueda = mayus(trim(busqueda)); // trim quita espacios sobrantes por si escribieron de más

    for (size_t i = 0; i < jugadores.size(); i++) {
        const Jugador& j = jugadores[i]; // j es el mismo jugador del vector, solo con nombre más corto
        // según la opción que eligieron, compara el código o el alias
        if ((op == 1 && j.codigo == busqueda) || (op == 2 && mayus(j.alias) == busqueda)) {
            msg(10, "\nJugador Encontrado:\n");
            cout << "Nombre: " << j.nombre << "\nAlias: " << j.alias << "\nCodigo: " << j.codigo << "\n"
                 << "Partidas jugadas: " << j.partidasJugadas << " | Ganadas: " << j.partidasGanadas << "\n";
            pausa(); return; // el return evita que salga el mensaje de "no encontrado"
        }
    }
    msg(12, "Jugador no encontrado.\n");
    pausa();
}

// Opción 3 del menú: controla toda la partida, desde elegir jugadores hasta guardar el resultado
void iniciarPartida() {
    limpiar();
    if (jugadores.size() < 2) { msg(12, "Se necesitan al menos 2 jugadores.\n"); pausa(); return; }
    int idx[2]; // posición en el vector de cada uno de los 2 jugadores
    string entrada;
    cin.ignore(1000, '\n'); // quita el Enter que quedó del menú, para que getline no lea una línea vacía
    cout << "Codigo o Alias Jugador 1: ";
    getline(cin, entrada);
    idx[0] = buscarIndice(trim(entrada)); // trim quita espacios sobrantes por si escribieron de más
    cout << "Codigo o Alias Jugador 2: ";
    getline(cin, entrada);
    idx[1] = buscarIndice(trim(entrada));
    if (idx[0] == -1 || idx[1] == -1 || idx[0] == idx[1]) { // no existen o son la misma persona
        msg(12, "Jugadores invalidos (no existen o son el mismo).\n"); pausa(); return;
    }

    Tablero tab[2]; // tab[0] es el tablero del jugador 1 y tab[1] el del jugador 2
    for (int p = 0; p < 2; p++) { // cada jugador coloca sus barcos, uno después del otro
        inicializarTablero(tab[p]);
        faseColocacion(tab[p], jugadores[idx[p]].nombre);
    }

    int turno = 0, turnosTotales = 0; // turno 0 = Jugador 1, turno 1 = Jugador 2
    while (true) { // cada vuelta es un disparo
        turnosTotales++;
        Jugador& atacante = jugadores[idx[turno]]; // referencia: lo que cambie aquí se cambia en el vector
        Tablero& enemigo = tab[1 - turno];         // el tablero del otro jugador
        pantalla(14, "\nTURNO DE ATAQUE DE: " + atacante.nombre + " \n");
        cout << "Barcos enemigos restantes: " << calcularCasillasRestantes(enemigo)
             << " | Flotas restantes: " << barcosAflote(enemigo) << "\n\n";
        mostrarTablero(enemigo, true); // se ocultan los barcos del rival
        turnoDisparo(enemigo, atacante);
        if (calcularCasillasRestantes(enemigo) == 0) break; // al rival no le quedan barcos: hay ganador
        pausa();
        turno = 1 - turno; // cambia el turno (si era 0 pasa a 1 y viceversa)
    }
    // al salir del ciclo, turno todavía apunta al jugador que ganó
    msg(10, "\n" + jugadores[idx[turno]].nombre + " HA DESTRUIDO LAS FLOTAS ENEMIGAS Y GANA!\n");
    jugadores[idx[turno]].partidasGanadas++;
    jugadores[idx[0]].partidasJugadas++;
    jugadores[idx[1]].partidasJugadas++;
    guardarHistorial(jugadores[idx[0]].nombre, jugadores[idx[1]].nombre, jugadores[idx[turno]].nombre, turnosTotales);
    guardarJugadores();
    pausa();
}

// Opción 4 del menú: muestra las estadísticas de cada jugador
void reporteEstadisticas() {
    pantalla(14, "------------------------------------------------------\n               REPORTE DE ESTADISTICAS \n------------------------------------------------------\n");
    if (jugadores.empty()) { cout << "No hay datos.\n"; pausa(); return; }

    for (size_t i = 0; i < jugadores.size(); i++) {
        const Jugador& j = jugadores[i];
        msg(11, "Jugador: " + j.nombre + " [" + j.codigo + "]\n");
        // las perdidas y los fallados no se guardan, se calculan restando
        cout << "Partidas Jugadas: " << j.partidasJugadas << " | Ganadas: " << j.partidasGanadas
             << " | Perdidas: " << j.partidasJugadas - j.partidasGanadas << "\n"
             << "Tasa de Victorias: " << porcentaje(j.partidasGanadas, j.partidasJugadas) << "%\n"
             << "Disparos Realizados: " << j.disparosRealizados << " | Acertados: " << j.disparosAcertados
             << " | Fallados: " << j.disparosRealizados - j.disparosAcertados << "\n"
             << "Precision de Tiro: " << porcentaje(j.disparosAcertados, j.disparosRealizados) << "%\n";
        cout << "------------------------------------------------------\n";
    }
    pausa();
}

// Opción 5 del menú: muestra a los jugadores ordenados por victorias, del que más ganó al que menos
void reporteRanking() {
    pantalla(14, "---------------------------------------------------\n          RANKING GLOBAL (Por Victorias) \n---------------------------------------------------\n");
    if (jugadores.empty()) { cout << "No hay datos.\n"; pausa(); return; }

    vector<Jugador> ranking = jugadores; // copia del vector, así no se altera el orden original
    // ordenamiento burbuja: compara cada pareja de vecinos y los intercambia si están al revés
    for (size_t i = 0; i + 1 < ranking.size(); i++)
        for (size_t k = 0; k + 1 < ranking.size() - i; k++) // en cada vuelta ya queda uno ordenado al final, por eso se resta i
            if (ranking[k].partidasGanadas < ranking[k + 1].partidasGanadas) swap(ranking[k], ranking[k + 1]);

    // setw(n) deja un espacio de n caracteres para que las columnas queden alineadas
    cout << left << setw(5) << "Top" << setw(15) << "Nombre" << setw(11) << "Victorias" << setw(10) << "Jugadas" << "Precision\n";
    cout << "---------------------------------------------------\n";
    for (size_t i = 0; i < ranking.size(); i++)
        // substr(0, 14) corta los nombres largos para que no se peguen a la columna de victorias
        cout << setw(5) << i + 1 << setw(15) << ranking[i].nombre.substr(0, 14) << setw(11) << ranking[i].partidasGanadas
             << setw(10) << ranking[i].partidasJugadas
             << porcentaje(ranking[i].disparosAcertados, ranking[i].disparosRealizados) << "%\n";
    pausa();
}

// Opción 6 del menú: muestra todas las partidas guardadas en partidas.txt
void reporteHistorial() {
    pantalla(14, "-----------------------------------------------------------------\n                     HISTORIAL DE PARTIDAS\n-----------------------------------------------------------------\n");
    ifstream archivo("partidas.txt");
    string linea;
    int n = 0;
    while (getline(archivo, linea)) cout << ++n << ". " << linea << "\n-----------------------------------------------------------------\n"; // getline lee la línea completa, con espacios
    if (n == 0) msg(12, "Aun no hay partidas registradas.\n");
    else cout << "\nTotal de partidas jugadas: " << n << "\n";
    pausa();
}

// Carga los datos, muestra el menú y llama a la opción que elija el usuario
int main() {
    cargarJugadores();
    cout << fixed << setprecision(1); // los porcentajes salen con 1 decimal
    int op;
    do {
        limpiar();
        msg(10, "=============================\n        BATALLA NAVAL        \n=============================\n");
        cout << "1. Registrar Jugador\n2. Buscar Jugador\n3. Iniciar Nueva Partida\n4. Reporte de Estadisticas\n"
             << "5. Ranking de Victorias\n6. Historial de Partidas\n7. Salir\n------------------------------\n";
        cout << "Seleccione: ";
        cin >> op;
        while (cin.fail() || op < 1 || op > 7) { // si escriben una letra o un número fuera del menú, vuelve a preguntar
            cin.clear();
            cin.ignore(1000, '\n');
            msg(12, "Entrada invalida. Ingrese un numero entre 1 y 7.\n");
            cout << "Seleccione: ";
            cin >> op;
        }

        switch (op) {
            case 1: registrarJugador(); break;
            case 2: buscarJugador(); break;
            case 3: iniciarPartida(); break;
            case 4: reporteEstadisticas(); break;
            case 5: reporteRanking(); break;
            case 6: reporteHistorial(); break;
            case 7: cout << "Saliendo...\n"; break;
        }
    } while (op != 7);
    return 0;
}
