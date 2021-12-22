// OS Project.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>
using namespace std;

//Global variables
const int mm_size = 512;
int sys_clock = 0;
int fault_count = 0; //to track the number of page faults

struct page {
    int valid_bit = 0;
    int last_accessed;
    int page_number = 0;
    int entry_time;
    int second_chance = 0;
};

struct page_table {
    int bal_alloc = 0;
    int no_of_pages = 0;
    int offset = 0;
    bool has_space = true;
    vector<page> pp;
    int counter = 0;
};

/*void swap_page(int index_to_swap, int proc, int req, page_table* v, page** mm) {
    //in->last_accessed = sys_clock;
    //in->valid_bit = 1;
    //out->valid_bit = 0;
    fault_count++;
};*/

int main(int argc, char* argv[])
{
    /*Takes in 5 arguments
    * program_name proc_list proc_trace page_size replacement_algo
    if (argc != 5) {
        printf("Invalid number of inputs!");
        return 0;
    }
    int i = 0;
    */

    for (int i = 0; i < argc; i++) {
        printf("Arg %d: %s\n", i, argv[i]);
    }
       
    int page_size = stoi(argv[3]);
    int algo;

    if (strcmp(argv[4],"FIFO")==0) {
        algo = 1;
    }
    else if (strcmp(argv[4], "LRU") == 0) {
        algo = 2;
    }
    else if (strcmp(argv[4], "Clock") == 0) {
        algo = 3;
    }
    else {
        printf("Invalid replacement algorithm!\n");
        return 0;
    }

    ifstream f(argv[1],ifstream::in);
    int num_of_proc = 0;
    map<int, int> p_map;
    string s;
    vector <page_table> v;
    
    if (f.is_open())
    {
        int sub, sub2;
        string delimit = " ";
        while (getline(f, s))
        {
            //skip empty lines
            if (s.length() == 0)
                continue;
            sub = stoi(s.substr(0, s.find(delimit)));
            sub2 = stoi(s.substr(s.find(delimit) + 1, s.length()));
            //map to keep track of number of pages for each process
            p_map[sub] = sub2;
            //page table for each process
            page_table* temp = new page_table;
            temp->no_of_pages = sub2;
            temp->pp.resize(sub2);
            for (int i = 0; i < sub2; i++) {
                temp->pp[i].page_number = i;
            }
            v.push_back(*temp);
            num_of_proc++;
            delete temp;
        }
        f.close();
    }


    //for the purpose of this simulation, each process gets equal number of pages in memory
    //if it is not evenly divisible, the leftover pages in memory will be left blank
    //this is an area of improvement
    //at this point the page table for each process has been created
    //int mloc_per_proc = mm_size / num_of_proc;
    int max_page = mm_size / num_of_proc / page_size;
    int total_pages = max_page * num_of_proc;
    for (int i = 0 ; i < num_of_proc; i++) {
        v[i].offset = i*max_page;
        v[i].bal_alloc = max_page;
    }

    vector <page*> main_memory(total_pages);
    printf("max page %d\n", max_page);
    printf("num of proc %d\n", num_of_proc);
    printf("total pages %d\n", total_pages);

    //start reading from ptrace
    ifstream tracefile(argv[2], ifstream::in);
    if (tracefile.is_open())
    {
        int proc, req, index_to_swap = 0;
        string delimit = " ";
        while (getline(tracefile, s))
        {
            //skip empty lines
            if (s.length() == 0)
                continue;

            proc = stoi(s.substr(0, s.find(delimit)));
            req = stoi(s.substr(s.find(delimit) + 1, s.length())) - 1;
            //printf("%d %d\n",proc,req);
            if (v[proc].pp[req].valid_bit == 1)
            //page is in mm, just update time accessed

                v[proc].pp[req].last_accessed = sys_clock;
                v[proc].pp[req].second_chance = 1;
                sys_clock++;

                continue;
            }
            else if (v[proc].has_space)
            //process still has allocated space for pages
            {
                v[proc].pp[req].valid_bit = 1;
                v[proc].pp[req].last_accessed = sys_clock;
                v[proc].pp[req].entry_time = sys_clock;
                main_memory[v[proc].offset + (max_page- v[proc].bal_alloc)] = &v[proc].pp[req];
                v[proc].bal_alloc = v[proc].bal_alloc - 1;
                if (v[proc].bal_alloc == 0)
                    v[proc].has_space = false;
                sys_clock++;
                
            }
            //otherwise, it is a page fault
            else {
                if (algo == 1) //FIFO
                {
                    v[proc].pp[req].entry_time = sys_clock;
                    //find the page with minimum entry time
                    index_to_swap = v[proc].offset;
                    int min_entry = main_memory[v[proc].offset]->entry_time;
                    for (int i = 1; i < max_page; i++) {
                        if (main_memory[v[proc].offset + i]->entry_time < min_entry) 
                        {
                            min_entry = main_memory[v[proc].offset + i]->entry_time;
                            index_to_swap = (v[proc].offset) + i;
                        }
                    }
                }
                else if (algo == 2) //LRU
                {
                    v[proc].pp[req].last_accessed = sys_clock;
                    //find the page with the minimum time accessed
                    index_to_swap = v[proc].offset;
                    int min_accessed = main_memory[v[proc].offset]->last_accessed;
                    for (int i = 1; i < max_page; i++) {
                        if (main_memory[v[proc].offset + i]->last_accessed < min_accessed) 
                        {
                            min_accessed = main_memory[v[proc].offset + i]->last_accessed;
                            index_to_swap = (v[proc].offset) + i;
                        }
                            
                    }
                }
                else if (algo == 3) //Clock
                {
                    //go round the clock to find the first page with second chance bit as 0
                    //if counter points to a page with second chance bit as 1, the bit is flipped to 0
                    //and counter advances
                    v[proc].pp[req].entry_time = sys_clock;
                    bool found = false;
                    while (!found) 
                    {
                        //find the page with minimum entry time
                        index_to_swap = v[proc].offset;
                        int min_entry = main_memory[v[proc].offset]->entry_time;
                        for (int i = 1; i < max_page; i++) {
                            if (main_memory[v[proc].offset + i]->entry_time < min_entry)
                            {
                                min_entry = main_memory[v[proc].offset + i]->entry_time;
                                index_to_swap = (v[proc].offset) + i;
                            }
                        }

                        if (main_memory[index_to_swap]->second_chance == 0) {
                            found = true;
                        }
                        else {
                            //flip referenced bit to 0 and update arrival time to current time
                            main_memory[index_to_swap]->second_chance = 0;
                            main_memory[index_to_swap]->entry_time = sys_clock;
                        }
                    }
                    
                }
                main_memory[index_to_swap]->valid_bit = 0;
                main_memory[index_to_swap] = &v[proc].pp[req];
                v[proc].pp[req].valid_bit = 1;
                fault_count++;
                sys_clock++;
            }
        }
    }

    printf("Final sys_clock is %d\n", sys_clock);
    printf("Total page fault count is %d\n", fault_count);
    cout << "Percentage of page fault is " << (fault_count*100 / sys_clock) << "%" << endl;

    ofstream out;
    out.open("output.txt");
    out << "Final memory status" << endl;
    for (int i = 0; i < num_of_proc; i++) 
    {
        out << "Process " << i << endl;
        for (int j = 0; j < max_page; j++) 
        {
        out << main_memory[v[i].offset + j]->page_number + 1 << endl;
        }
    }
    out.close();

    return 0;
}