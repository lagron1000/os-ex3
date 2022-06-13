/*
README
Using Option 2 for my config scheme
*/


#include <iostream>
#include <queue>
#include <thread>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

#define S "SPORT"
#define N "NEWS"
#define W "WEATHER"
#define DONE "DONE"
#define P_PROMPT "Producer %c %s %c"

/*******************************************CLASSES*********************************************/


class UnboundedQ : public queue<string> {
private:
    sem_t full;
    sem_t mutex;
public:
    UnboundedQ () {
        sem_init(&full, 0, 0);
        sem_init(&mutex, 0, 1);
    }

    void push(string product) {
        sem_wait(&mutex);
        queue<string>::push(product);
        sem_post(&mutex);
        sem_post(&full);
    }

    string pop() {
        sem_wait(&full);
        sem_wait(&mutex);
        string item = front();
        queue<string> :: pop();
        sem_post(&mutex);
        return item;
    }
    
    // string front() {
    //     sem_wait(&full);
    //     sem_wait(&mutex);
    //     string f = queue<string> :: front();
    //     sem_post(&mutex);
    //     sem_post(&full);
    //     return f;
    // }
    ~UnboundedQ() {
    sem_destroy(&mutex);
    sem_destroy(&full);
    }
};

class BoundedQ : public UnboundedQ {
private:
    int sizeOfQ;
    sem_t empty;

public:
    BoundedQ (int size) : UnboundedQ() {
        sizeOfQ = size;
        sem_init(&empty, 0, sizeOfQ);
    }

    void push(string product) {
        sem_wait(&empty);
        UnboundedQ :: push(product);
    }

    string pop() {
        string item = UnboundedQ :: pop();
        sem_post(&empty);
        return item;
    }
    // string front() {
    //     string f = UnboundedQ :: front();
    //     return f;
    // }
    ~BoundedQ() {
    sem_destroy(&empty);
    }
};

class Producer {
    private :
        int id;
        int productCount;

        int productType = 0;
        int sCount = 0;
        int nCount = 0;
        int wCount = 0;
        BoundedQ* q;

    public :
        Producer(int _id, int _count, BoundedQ* _q) {
            id = _id;
            productCount = _count;
            q = _q;
        }

        void produce() {
            for(int i = 0; i < productCount; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                switch (productType % 3)
                {
                case 0:{
                    // printf(P_PROMPT, id, S, sCount);
                    // cout << "Producer " << id << " " << S << " " << sCount << endl;
                    string p = "Producer " + to_string(id) + " " + S + " " + to_string(sCount);
                    q->push(p);
                    sCount++;
                    break;
                }

                case 1:{
                    string p = "Producer " + to_string(id) + " " + N + " " + to_string(nCount);
                    q->push(p);
                    nCount++;
                    break;
                }
                    
                case 2:{
                    string p = "Producer " + to_string(id) + " " + W + " " + to_string(wCount);
                    q->push(p);
                    wCount++;
                    break;
                }
                    
                default:
                    break;
                }
                // productType++;
                productType = std::rand() % ( 3 );
            }
            q->push(DONE);
        }
        thread produceT() {
            return thread(&Producer::produce, this);
        }

        ~Producer() {
            // free(q);
        }

};

class Dispatcher {
    private:
    //   int numOfQs;
      vector<BoundedQ*> queues;
      vector<int> doneQs;
      UnboundedQ* sQ;
      UnboundedQ* nQ;
      UnboundedQ* wQ;

    public:

    Dispatcher(UnboundedQ* _sQ, UnboundedQ* _nQ, UnboundedQ* _wQ) {
        sQ = _sQ;
        nQ = _nQ;
        wQ = _wQ;
        // numOfQs = num;
        // queues = new vector<BoundedQ>()
    }

    void dispatch() {
        int i = 0;
        //while not all queues in vector are done
        while(doneQs.size() != queues.size()) {
            //if Q not in done list
            if (std::count(doneQs.begin(), doneQs.end(), i) == 0) {
                BoundedQ* q = queues[i];
                string item = q->pop();
                // if Q done
                if (item.compare(DONE) == 0) {
                    doneQs.push_back(i);
                }
                // if Q not done
                else {
                    if(item.find(S) != string::npos) {
                        string p = "SPORT Inserted to the “S dispatcher queue”";
                        sQ->push(item);
                        // cout << p << endl;
                    } else if(item.find(N) != string::npos) {
                        string p = "NEWS Inserted to the “N dispatcher queue”";
                        nQ->push(item);
                        // cout << p << endl;
                    } else if(item.find(W) != string::npos) {
                        string p = "WEATHER Inserted to the “W dispatcher queue”";
                        wQ->push(item);
                        // cout << p << endl;

                    }
                }
            }
            
            i = (i+1) % (queues.size());
        }
        sQ->push(DONE);
        nQ->push(DONE);
        wQ->push(DONE);

    }

    void addQ(BoundedQ* q) {
        queues.push_back(q);
    }

        thread dispatchT() {
            return thread(&Dispatcher::dispatch, this);
        }

    ~Dispatcher(){
        free(sQ);
        free(nQ);
        free(wQ);
    }

};

class CoEditor {
private:
    UnboundedQ* myQ;
    BoundedQ* sharedQ;
public:
    CoEditor(UnboundedQ* q, BoundedQ* sQ) {
        myQ = q;
        sharedQ = sQ;
    }
    void edit() {
        string front = myQ->pop();
        while(front.compare(DONE) != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // cout << "Co-Editor edited for 0.1 seconds" << endl;
            sharedQ->push(front);
            front = myQ->pop();
        }
        sharedQ->push(front);

    }

    thread editT() {
        return thread(&CoEditor::edit, this);
    }
    ~CoEditor();
};

class ScreenManager {
private:
    BoundedQ* sharedQ;
    int doneCount = 0;

public:
    ScreenManager(BoundedQ* q) {
        sharedQ = q;
    }

    void displayMsgs() {
        while(doneCount < 3) {
            string item = sharedQ->pop();
            if(item.compare(DONE) == 0) {
                doneCount++;
            }
            else {
                cout << item << endl;
            }
            // sharedQ->pop();
        }
                cout << DONE << endl;
    }

    thread displayT() {
        return thread(&ScreenManager::displayMsgs, this);
    }
};


/*************************************************************************************************/


int main(int argc, char *argv[]) {
    if (argc<2) exit(-1);

    std::fstream config;
    config.open(argv[1], std::fstream::in);

    vector<Producer*> producers;
    vector<string> lines;
    vector<queue<string>*> allQs;

    string line;
    while(config >> line) {
        lines.push_back(line);
    }

    int numoflines = lines.size();

    int sharedQSize = stoi(lines[numoflines-1]);

    BoundedQ* sharedQ = new BoundedQ(sharedQSize);
    UnboundedQ* sQ = new UnboundedQ();
    UnboundedQ* nQ = new UnboundedQ();
    UnboundedQ* wQ = new UnboundedQ();

    
    Dispatcher* dispatcher = new Dispatcher(sQ, nQ, wQ);
    CoEditor* sEditor = new CoEditor(sQ, sharedQ);
    CoEditor* nEditor = new CoEditor(nQ, sharedQ);
    CoEditor* wEditor = new CoEditor(wQ, sharedQ);
    ScreenManager* SM = new ScreenManager(sharedQ);

    int proId;
    int numOfProducts;
    int sizeOfQ;
    int lineIndex = 0;
    for (int i = numoflines-2; i >= 0; i--) {
        switch (lineIndex)
        {
        case 0:
            sizeOfQ = stoi(lines[i]);
            break;

        case 1:
            numOfProducts = stoi(lines[i]);
            break;

        case 2:{
            proId = stoi(lines[i]);
            BoundedQ* q = new BoundedQ(sizeOfQ);
            Producer* p = new Producer(proId, numOfProducts, q);
            producers.push_back(p);
            dispatcher->addQ(q);
            allQs.push_back(q);
            break;
        }
            
        
        default:
            break;
        }

        lineIndex = (lineIndex+1)%3;
    }


    vector<thread> threads;
    int pSize = producers.size();
    for (int i = 0; i < pSize; i++) {
        threads.push_back(move(producers[i]->produceT()));
    }
    threads.push_back(move(dispatcher->dispatchT()));
    threads.push_back(move(sEditor->editT()));
    threads.push_back(move(nEditor->editT()));
    threads.push_back(move(wEditor->editT()));
    threads.push_back(move(SM->displayT()));

    int tsize = threads.size();
    for (int j = 0; j < tsize; j++) {
        threads[j].join();
    }

    /*TODO: DELETE ALL PRODUCERS*/ 
    // for (Producer* x : producers) {
    //     free(x);
    // }
    // free(dispatcher);
    // free(CE);
    // free(SM);
    // exit(1);


    config.close();

    return 1;
    
}

 
