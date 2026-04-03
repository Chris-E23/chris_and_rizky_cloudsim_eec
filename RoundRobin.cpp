#include "Scheduler.hpp"
#include <map>
#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h>
static map<MachineId_t, vector<VMId_t>> machine_and_vms;
static map<TaskId_t, VMId_t> tasks_and_vms;
static map<MachineId_t, vector<TaskId_t>> machines_and_tasks;
vector<MachineId_t> running;
vector<MachineId_t> intermediate;
vector<MachineId_t> off;
map<CPUType_t, vector<MachineId_t>> cputypes_and_machines;
map<CPUType_t, int> rr_index;
long task_count = 0;
static unsigned total_machines = 16;

const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
void RoundRobin::Init()
{
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    //

    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);
    total_machines = Machine_GetTotal();
    for (unsigned i = 0; i < total_machines; i++)
    {
        machines.push_back(MachineId_t(i));
    }
    for (unsigned i = 0; i < total_machines; i++)
    {
        MachineInfo_t curr_machine = Machine_GetInfo(i);
        vms.push_back(VM_Create(LINUX, curr_machine.cpu));
        VM_Attach(vms[i], machines[i]);
        machine_and_vms[i].push_back(vms[i]);
        cputypes_and_machines[curr_machine.cpu].push_back(i);
    }

    SimOutput("Round Robin Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);
}

void RoundRobin::MigrationComplete(Time_t time, VMId_t vm_id)
{
    // Update your data structure. The VM now can receive new tasks
}

void RoundRobin::NewTask(Time_t now, TaskId_t task_id)
{
    // RoundRobin algorithms, fills the current machine up before going on to the next one
    bool GPUCapable = IsTaskGPUCapable(task_id);

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;

    MachineId_t best_machine = INVALID_MACHINE;
    MachineId_t best_cpu_machine = INVALID_MACHINE;
  

    MachineId_t curr_machine = INVALID_MACHINE;
    vector<MachineId_t> available_machines = cputypes_and_machines[req_cpu];

    int &idx = rr_index[req_cpu];
    idx++;
    for (int i = 0; i < available_machines.size(); i++)
    {
        MachineId_t mid = available_machines[(idx + i) % available_machines.size()];
        MachineInfo_t info = Machine_GetInfo(mid);

        if (info.s_state != S0)
            continue;

        if (info.memory_size - info.memory_used < required_memory)
            continue;
        curr_machine = mid; 
    }
    for (auto vm : machine_and_vms[curr_machine])
    {
        VMInfo_t info = VM_GetInfo(vm);
        if (info.cpu == req_cpu)
        {
            VM_AddTask(vm, task_id, new_task_info.priority);
            tasks_and_vms[task_id] = vm;
            return;
        }
    }

    MachineInfo_t best = Machine_GetInfo(curr_machine);
    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[curr_machine].push_back(new_vm);
    VM_Attach(new_vm, curr_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
}

void RoundRobin::PeriodicCheck(Time_t now)
{
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    // printf("UPDATE:\n\n");
    // printf("Time:  %lu\n", now);
    // for (MachineId_t i = 0; i < total_machines; i++)
    // {
    //     // SimOutput("Current Energy Usage of Machine " + to_string(i) + ": " + to_string(Machine_GetEnergy(i)), 0);
    //     printf("Current task count of machine: %lu\n", i, machines_and_tasks[i].size());
    // }
}

void RoundRobin::Shutdown(Time_t time)
{
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for (auto &vm : vms)
    {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}
void static RemoveTask(TaskId_t task_id, MachineId_t curr_machine)
{
    auto &tasks = machines_and_tasks[curr_machine];
    tasks.erase(remove(tasks.begin(), tasks.end(), task_id), tasks.end());
    tasks_and_vms.erase(task_id); // fully remove instead of setting to NULL/0
}
void RoundRobin::TaskComplete(Time_t now, TaskId_t task_id)
{

    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
    MachineId_t curr_machine = VM_GetInfo(tasks_and_vms[task_id]).machine_id;
    // Remove the current task from the data structure
    RemoveTask(task_id, curr_machine);

}