/* PRL 2018 - Projekt 2 - Merge-splitting sort
 * Autor: Marian Orszagh (xorsza00)
 * 
 */

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <limits>
#include <iomanip>    

using namespace std;

#define PLACEHOLDER numeric_limits<int>::max();
// Ak je zadefinovana, normalny vystup je potlaceny a na vystup je vypisany
// cas behu
// #define TIME true

void print_vec(vector<int> input, int size) {
    for (int i=0; i<size; i++) {
        cout << input[i] << endl;
    }
}

/* Nacitanie suboru masterom (id 0) a distribucia hodnot medzi ostatne.
*/
vector<int> load_file(int ratio, int proc_count) {
    vector<int> input_vec(ratio); 
    vector<int> master_vec(ratio);
    fstream f;
    f.open("numbers", ios::in);

    for (int pid = 0; pid < proc_count; pid++){
        for (int i=0; i<ratio; i++) {
            if (!f.eof()) {
                input_vec[i] = f.get();
                if (input_vec[i] == EOF) {
                    input_vec.at(i) = PLACEHOLDER;
                    continue;
                }
                else {
                    // Vypis vsetkych cisel podla zadania
                    #ifndef TIME
                    cout << input_vec[i] << " ";
                    #endif
                }
            }
            // V pripade, ze sme precitali vsetky cisla, do zvysnych slotov
            // v procesoroch vlozime maximalny mozny integer ako
            // placeholder
            else {
                input_vec.at(i) = PLACEHOLDER;
            }
        }
        // Procesor 0 neposle svoju cast, ale rovno si ju uchova
        // Je to zabrana proti situacii, kedy sa MPI buffer zahlti pri send/
        // receive operacii.
        if (pid == 0) {
            master_vec = input_vec;
        }
        else MPI_Send(&input_vec[0], ratio, MPI_INT, pid, 0, MPI_COMM_WORLD);
    }

    f.close();
    #ifndef TIME
    cout << endl;
    #endif
    return master_vec;
}

/* Procesor odosle pravemu susedovi svoj vektor a caka na mensiu cast spojenej
 * zoradenej postupnosti
 */
void send_and_wait(vector<int> &mynumbers, int ratio, int pid) {
    MPI_Status status;
    MPI_Send(&mynumbers[0], ratio, MPI_INT, pid+1, 0, MPI_COMM_WORLD);
    MPI_Recv(&mynumbers[0], ratio, MPI_INT, pid+1, 0, MPI_COMM_WORLD, 
             &status);
}

/* Procesor prijme od svojeho laveho suseda jeho vektor, spoji ho so svojim,
 * zoradi ich a susedovi posle mensiu polovicu.
 */
void sort_and_return(vector<int> &mynumbers, vector<int> &sort_v, int ratio, 
                     int pid) {
    MPI_Status status;

    MPI_Recv(&sort_v[0], ratio, MPI_INT, pid-1, 0, MPI_COMM_WORLD, &status); 
    // Poskladam obe vektory
    sort_v.insert(sort_v.end(), mynumbers.begin(), mynumbers.end());  
    // Zoradenie
    sort(sort_v.begin(), sort_v.end());

    // Druhu polovicu da procesor sebe, prvu posle dolava
    copy(sort_v.begin() + ratio, sort_v.end(), mynumbers.begin());
    MPI_Send(&sort_v[0], ratio, MPI_INT, pid-1, 0, MPI_COMM_WORLD); 
}

int main(int argc, char *argv[])
{
    // Init
    int numb_count = stoi(argv[1]);
    int proc_count;
    int pid;
    double start, finish;

    MPI_Status stat;

    //MPI init
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
 
    // Kolko cisel pripada jednemu procesoru
    int ratio = 1 + ((numb_count - 1) / proc_count);

    vector<int> mynumbers(ratio);

    #ifdef TIME
        if (!pid) start=MPI_Wtime();
    #endif

    // Master nacita a rozdistribuuje hodnoty
    if (!pid) mynumbers = load_file(ratio, proc_count);

    // cout<<pid<<":";
    // print_vec(mynumbers, mynumbers.size());
    // Kazdy procesor si nacita svoju cast cisel a predspracuje ju
    if (pid) MPI_Recv(&mynumbers[0], ratio, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

    sort(mynumbers.begin(), mynumbers.end());               


    for(int i=1; i<= 1 + ((proc_count - 1) / 2); i++) {
        vector<int> sort_v(ratio);

        // Parne okrem posledneho, ak je parny poslu svoj vektor doprava
        if (!(pid%2) && (pid != (proc_count-1))){
            send_and_wait(mynumbers, ratio, pid);
        }
        // Neparne prijmu, zoradia a poslu mensiu polovicu naspat dolava
        else if (pid%2) {
            sort_and_return(mynumbers, sort_v, ratio, pid);
        }

        sort_v.resize(ratio);

        // To iste, akurat sa vymeni pozicia parnych a neparnych procesorov
        if((pid%2) && (pid != (proc_count-1))){
            send_and_wait(mynumbers, ratio, pid);
        }
        else if(!(pid%2) && (pid != 0)){
            sort_and_return(mynumbers, sort_v, ratio, pid);
        }
    }
    // cout<<pid<<":";
    // print_vec(mynumbers, mynumbers.size());

    // Master si doalokuje pamat a postupne prijima hodnoty od ostatnych 
    // procesorov
    if(pid == 0) mynumbers.resize(proc_count * ratio);
    


    for(int i=1; i<proc_count; i++){
        if(pid == i) MPI_Send(&mynumbers[0], ratio, MPI_INT, 0, 0,
                               MPI_COMM_WORLD);

        if(pid == 0){
            MPI_Recv(&mynumbers[i*ratio], ratio, MPI_INT, i, 0, 
                     MPI_COMM_WORLD, &stat);
        }
    }
    // Vysledok
    if(pid == 0) {
        #ifndef TIME
        print_vec(mynumbers, numb_count);
        #else
        finish=MPI_Wtime();
        cout<<std::setprecision(40)<<(finish-start)<<endl;
        #endif
    }

    MPI_Finalize(); 
    return 0;
}
