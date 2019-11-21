/*
    Melih Kurtaran Homework #1 - Airline Reservation System
*/

#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

using namespace std;

bool isFinished = false;
vector<vector<int> > Matrix(2,vector<int>(50,0));
int turn = 0;

void *bookSeat1(void *)
{
    while(!isFinished)
    {
        srand(time(NULL)); //to have different random numbers for every trial
        while(turn == 0 && !isFinished)
        {
            bool reserved = false;
            int row = rand() % 2;
            int col = rand() % 50;
            if(!Matrix[row][col])
            {
                cout << "1st thread enters to the critical region" << endl;
                cout << "Seat number " << row*50 + col + 1<< " reserved by agency 1" << endl;
                Matrix[row][col] = 1;
                reserved = true;
                cout << "1st thread exits from the critical region" << endl << endl;
            }if(reserved)
                turn = 1;
        }
    }
    return 0;
}

void *bookSeat2( void * )
{
    while(!isFinished)
    {
        while(turn == 1 && !isFinished)
        {
            bool reserved = false;
            int row = rand() % 2;
            int col = rand() % 50;
            if(!Matrix[row][col])
            {
                cout << "2nd thread enters to the critical region" << endl;
                cout << "Seat number " << row*50 + col + 1<< " reserved by agency 2" << endl;
                Matrix[row][col] = 2;
                reserved = true;
                cout << "2nd thread exits from the critical region" << endl << endl;
            }if(reserved)
                turn = 2;
        }
    }
    return 0;
}

void* bookSeat3( void * )
{
    while(!isFinished)
    {
        while(turn == 2 && !isFinished)
        {
            bool reserved = false;
            int row = rand() % 2;
            int col = rand() % 50;
            if(!Matrix[row][col])
            {
                cout << "3rd thread enters to the critical region" << endl;
                cout << "Seat number " << row*50 + col + 1<< " reserved by agency 3" << endl;
                Matrix[row][col] = 3;
                reserved = true;
                cout << "3rd thread exits from the critical region" << endl << endl;
            }if(reserved)
                turn = 0;
        }
    }
    return 0;
}

int main()
{
    pthread_t travelAgency1, travelAgency2, travelAgency3;
    
    pthread_create( &travelAgency1, NULL, bookSeat1, NULL);
    pthread_create( &travelAgency2, NULL, bookSeat2, NULL);
    pthread_create( &travelAgency3, NULL, bookSeat3, NULL);
    
    int counter = 0;
    while(!isFinished)
    {
        for(int i=0;i < Matrix.size(); i++)
        {
            for(int j=0;j < Matrix[0].size(); j++)
            {
                if(Matrix[i][j])
                    counter++;
            }
        }
        if(counter == 100)
            isFinished = true;
        else
            counter = 0;
    }
    
    pthread_join(travelAgency1,NULL);
    pthread_join(travelAgency2,NULL);
    pthread_join(travelAgency3,NULL);

    
    cout << endl << "All seats are reserved by 3 agencies." << endl << endl;
    for(int i=0;i < Matrix.size(); i++)
    {
        for(int j=0;j < Matrix[0].size(); j++)
        {
            cout << Matrix[i][j] << " ";
        }
        cout << endl;
    }
    
    return 0;
}
