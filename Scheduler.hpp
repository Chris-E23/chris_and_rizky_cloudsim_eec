//
//  Scheduler.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#ifndef Scheduler_hpp
#define Scheduler_hpp

#include <vector>

#include "Interfaces.h"

class GreedyScheduler {
public:
    GreedyScheduler()                 {}
    void Init();
    void MigrationComplete(Time_t time, VMId_t vm_id);
    void NewTask(Time_t now, TaskId_t task_id);
    void PeriodicCheck(Time_t now);
    void Shutdown(Time_t now);
    void TaskComplete(Time_t now, TaskId_t task_id);
private:
    vector<VMId_t> vms;
    vector<MachineId_t> machines;
};

class pMapper{

public: 
    pMapper() {};
    void Init();
    void MigrationComplete(Time_t time, VMId_t vm_id);
    void NewTask(Time_t now, TaskId_t task_id);
    void PeriodicCheck(Time_t now);
    void Shutdown(Time_t now);
    void TaskComplete(Time_t now, TaskId_t task_id);
    private:
        vector<VMId_t> vms;
        vector<MachineId_t> machines;
};

class MinMin{
    public: 
        MinMin() {};
        void Init();
        void MigrationComplete(Time_t time, VMId_t vm_id);
        void NewTask(Time_t now, TaskId_t task_id);
        void PeriodicCheck(Time_t now);
        void Shutdown(Time_t now);
        void TaskComplete(Time_t now, TaskId_t task_id);
        void ScheduleBatch();
        private:
            vector<VMId_t> vms;
            vector<MachineId_t> machines;
    };

#endif /* Scheduler_hpp */


class EEco{
     public: 
        EEco() {};
        void Init();
        void MigrationComplete(Time_t time, VMId_t vm_id);
        void NewTask(Time_t now, TaskId_t task_id);
        void PeriodicCheck(Time_t now);
        void Shutdown(Time_t now);
        void TaskComplete(Time_t now, TaskId_t task_id);
        private:
            vector<VMId_t> vms;
            vector<MachineId_t> machines;
};

class RoundRobin{
     public: 
        RoundRobin() {};
        void Init();
        void MigrationComplete(Time_t time, VMId_t vm_id);
        void NewTask(Time_t now, TaskId_t task_id);
        void PeriodicCheck(Time_t now);
        void Shutdown(Time_t now);
        void TaskComplete(Time_t now, TaskId_t task_id);
        private:
            vector<VMId_t> vms;
            vector<MachineId_t> machines;
};


