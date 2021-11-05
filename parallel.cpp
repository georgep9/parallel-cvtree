#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <map>
#include <chrono>

int number_bacteria;
char** bacteria_name;
long M, M1, M2;
short code[27] = { 0, 2, 1, 2, 3, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, 12, 13, 14, 15, 16, 1, 17, 18, 5, 19, 3};
#define encode(ch)		code[ch-'A']
#define LEN				6
#define AA_NUMBER		20
#define	EPSILON			1e-010

void Init()
{
    M2 = 1;
    for (int i=0; i<LEN-2; i++)	// M2 = AA_NUMBER ^ (LEN-2);
        M2 *= AA_NUMBER;
    M1 = M2 * AA_NUMBER;		// M1 = AA_NUMBER ^ (LEN-1);
    M  = M1 *AA_NUMBER;			// M  = AA_NUMBER ^ (LEN);
}

class Bacteria
{
private:
    long* vector;
    long* second;
    long one_l[AA_NUMBER];
    long indexs;
    long total;
    long total_l;
    long complement;

    void InitVectors()
    {
        vector = new long [M];
        second = new long [M1];
        memset(vector, 0, M * sizeof(long));
        memset(second, 0, M1 * sizeof(long));
        memset(one_l, 0, AA_NUMBER * sizeof(long));
        total = 0;
        total_l = 0;
        complement = 0;
    }

    void init_buffer(char* buffer)
    {
        complement++;
        indexs = 0;
        for (int i=0; i<LEN-1; i++)
        {
            short enc = encode(buffer[i]);
            one_l[enc]++;
            total_l++;
            indexs = indexs * AA_NUMBER + enc;
        }
        second[indexs]++;
    }

    void cont_buffer(char ch)
    {
        short enc = encode(ch);
        one_l[enc]++;
        total_l++;
        long index = indexs * AA_NUMBER + enc;
        vector[index]++;
        total++;
        indexs = (indexs % M2) * AA_NUMBER + enc;
        second[indexs]++;
    }

public:
    long count;
    double* tv;
    long *ti;

    Bacteria(char* filename)
    {

        std::ifstream bacteria_file{filename};

        if (!bacteria_file)
        {
            // Print an error and exit
            std::string fname = filename;
            std::cerr << "Uh oh, " + fname + " could not be opened for reading!" << std::endl;
            exit(1);
        }

        InitVectors();

        char ch;
        while (bacteria_file.get(ch))
        {

            if (ch == '>')
            {
                //while (bacteria_file.get(ch) && ch != '\n'); // skip rest of line
                std::string skip;
                std::getline(bacteria_file, skip);

                char buffer[LEN-1];
                bacteria_file.read(buffer,LEN-1);
                init_buffer(buffer);
            }
            else if (ch != '\n' && ch != '\r')
                cont_buffer(ch);
        }

        long total_plus_complement = total + complement;
        double total_div_2 = total * 0.5;
        int i_mod_aa_number = 0;
        int i_div_aa_number = 0;
        long i_mod_M1 = 0;
        long i_div_M1 = 0;

        double one_l_div_total[AA_NUMBER];
        for (int i=0; i<AA_NUMBER; i++)
            one_l_div_total[i] = (double)one_l[i] / total_l;

        double* second_div_total = new double[M1];
        for (int i=0; i<M1; i++)
            second_div_total[i] = (double)second[i] / total_plus_complement;

        count = 0;
        double* t = new double[M];

        for(long i=0; i<M; i++)
        {
            double p1 = second_div_total[i_div_aa_number];
            double p2 = one_l_div_total[i_mod_aa_number];
            double p3 = second_div_total[i_mod_M1];
            double p4 = one_l_div_total[i_div_M1];
            double stochastic =  (p1 * p2 + p3 * p4) * total_div_2;

            if (i_mod_aa_number == AA_NUMBER-1)
            {
                i_mod_aa_number = 0;
                i_div_aa_number++;
            }
            else
                i_mod_aa_number++;

            if (i_mod_M1 == M1-1)
            {
                i_mod_M1 = 0;
                i_div_M1++;
            }
            else
                i_mod_M1++;

            if (stochastic > EPSILON)
            {
                t[i] = (vector[i] - stochastic) / stochastic;
                count++;
            }
            else
                t[i] = 0;
        }

        delete second_div_total;
        delete vector;
        delete second;

        tv = new double[count];
        ti = new long[count];

        int pos = 0;
        for (long i=0; i<M; i++)
        {
            if (t[i] != 0)
            {
                tv[pos] = t[i];
                ti[pos] = i;
                pos++;
            }
        }
        delete t;

        //fclose (bacteria_file);
    }
};

void ReadInputFile(const char* input_name)
{

    std::ifstream input_file{input_name};

    if (!input_file)
    {
        std::string iname = input_name;
        std::cerr << "Error: failed to open file " + iname + " (Hint: check your working directory)\n" << std::endl;
        exit(1);
    }

    input_file >> number_bacteria;
    bacteria_name = new char*[number_bacteria];

    for(long i=0;i<number_bacteria;i++)
    {
        char name[10];
        input_file >> name;
        bacteria_name[i] = new char[20];
        sprintf(bacteria_name[i], "data/%s.faa", name);
    }
    //fclose(input_file);
}

double CompareBacteria(Bacteria* b1, Bacteria* b2)
{
    double correlation = 0;
    double vector_len1=0;
    double vector_len2=0;
    long p1 = 0;
    long p2 = 0;
    while (p1 < b1->count && p2 < b2->count)
    {
        long n1 = b1->ti[p1];
        long n2 = b2->ti[p2];
        if (n1 < n2)
        {
            double t1 = b1->tv[p1];
            vector_len1 += (t1 * t1);
            p1++;
        }
        else if (n2 < n1)
        {
            double t2 = b2->tv[p2];
            p2++;
            vector_len2 += (t2 * t2);
        }
        else
        {
            double t1 = b1->tv[p1++];
            double t2 = b2->tv[p2++];
            vector_len1 += (t1 * t1);
            vector_len2 += (t2 * t2);
            correlation += t1 * t2;
        }
    }
    while (p1 < b1->count)
    {
        long n1 = b1->ti[p1];
        double t1 = b1->tv[p1++];
        vector_len1 += (t1 * t1);
    }
    while (p2 < b2->count)
    {
        long n2 = b2->ti[p2];
        double t2 = b2->tv[p2++];
        vector_len2 += (t2 * t2);
    }

    return correlation / (sqrt(vector_len1) * sqrt(vector_len2));
}

Bacteria** LoadAllBacteriaParallel() {
    Bacteria** b = new Bacteria*[number_bacteria];
    #pragma omp parallel for schedule(dynamic)
    for(int i=0; i<number_bacteria; i++) {
        printf("[Thread: %d] load %d of %d\n", 
            omp_get_thread_num(), i+1, number_bacteria);
        b[i] = new Bacteria(bacteria_name[i]);
    }

    return b;
}

void CompareAllBacteriaParallel(Bacteria** b){
    std::map<int,int> comb_i;
    std::map<int,int> comb_j;
    int comb_n = 0;

    for(int i=0; i<number_bacteria-1; i++) {
        for(int j=i+1; j<number_bacteria; j++) {
            comb_i[comb_n] = i;
            comb_j[comb_n] = j;
            comb_n++;
        }
    }

    #pragma omp parallel for schedule(dynamic)
    for(int k=0; k<comb_n; k++){
        int i = comb_i[k];
        int j = comb_j[k];
        double correlation = CompareBacteria(b[i], b[j]);
        printf("[Thread: %d] %2d %2d -> %.20lf\n", 
            omp_get_thread_num(), i, j, correlation);
    }
}

int main(int argc,char * argv[])
{
    Init();
    ReadInputFile("list.txt");

    using clock = std::chrono::high_resolution_clock;
    auto start_t = clock::now();
    auto now_t = start_t;

    Bacteria** b = LoadAllBacteriaParallel();

    auto dur = (float) std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - now_t).count()
        / 1000;
    printf("\nLoadAllBacteria: %6.2lf seconds\n",  dur);
    now_t = clock::now();

    CompareAllBacteriaParallel(b);

    dur = (float) std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - now_t).count()
        / 1000;
    printf("\nCompareAllBacteria: %6.2lf seconds\n",  dur);

    float total_t = (float) std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start_t).count()
        / 1000;
    printf("\nTotal time elapsed: %6.2lf seconds\n",  total_t);
    return 0;
}