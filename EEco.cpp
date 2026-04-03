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

static unsigned total_machines = 16;

const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
void EEco::Init()
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
        MachineInfo_t curr_machine = Machine_GetInfo(i);
        Machine_SetState(i, S5);
        off.push_back(i);
    }

    // SimOutput("EEco Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);
}

void EEco::MigrationComplete(Time_t time, VMId_t vm_id)
{
    // Update your data structure. The VM now can receive new tasks
}

void EEco::NewTask(Time_t now, TaskId_t task_id)
{
    // EEco algorithms, fills the current machine up before going on to the next one
    bool GPUCapable = IsTaskGPUCapable(task_id);

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;

    float best_util = std::numeric_limits<float>::max();

    MachineId_t best_machine = INVALID_MACHINE;
    float best_cpu_util = std::numeric_limits<float>::max();
    MachineId_t best_cpu_machine = INVALID_MACHINE;

    vector<MachineId_t>::iterator running_it;

    for (running_it = running.begin(); running_it != running.end(); running_it++)
    {
        // Assign tasks based on the info at hand
        // Choose machines with least number of tasks and most memory available

        MachineInfo_t curr_machine_info = Machine_GetInfo(*running_it);
        float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
        // Approximate that the number of tasks correspond to number of active cpus active
        float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;

        float curr_util = max(cpu_util, memory_util);

        if (curr_machine_info.cpu != req_cpu)
        {
            continue;
        }
        unsigned remaining_memory = curr_machine_info.memory_size - curr_machine_info.memory_used;
        if (remaining_memory < required_memory)
        {
            continue;
        }
        if (best_machine == INVALID_MACHINE)
        {
            // At this point, we know that the current machine is compatible, so let's set it as the
            // base
            best_machine = *running_it;
        }
        if (best_util > curr_util)
        {
            best_util = curr_util;
            best_machine = *running_it;
        }
    }

    if (best_machine == INVALID_MACHINE)
    {
        for (int i = 0; i < (int)off.size(); i++)
        {
            MachineInfo_t machineInfo = Machine_GetInfo(off[i]);
            if (machineInfo.cpu == req_cpu)
            {
                intermediate.push_back(off[i]);
                Machine_SetState(off[i], S0);
                off.erase(off.begin() + i);
                
            }
        }
    }

    if (best_machine == INVALID_MACHINE)
    {
        SimOutput("NewTask(): No compatible machine for task " + to_string(task_id), 1);
        return;
    }

    machines_and_tasks[best_machine].push_back(task_id);

    for (auto vm : machine_and_vms[best_machine])
    {

        VMInfo_t info = VM_GetInfo(vm);
        if (info.cpu == req_cpu)
        {
            VM_AddTask(vm, task_id, new_task_info.priority);
            tasks_and_vms[task_id] = vm;
            return;
        }
    }

    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[best_machine].push_back(new_vm);
    VM_Attach(new_vm, best_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
}

void EEco::PeriodicCheck(Time_t now)
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
    for (int i = 0; i < intermediate.size(); i++)
    {

        MachineInfo_t info = Machine_GetInfo(intermediate[i]);

        if (info.s_state == S0)
        {
            running.push_back(intermediate[i]);
            intermediate.erase(intermediate.begin() + i);
            i--;
        }
    }
}

void EEco::Shutdown(Time_t time)
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
void EEco::TaskComplete(Time_t now, TaskId_t task_id)
{

    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
    MachineId_t curr_machine = VM_GetInfo(tasks_and_vms[task_id]).machine_id;

    // Remove the current task from the data structure
    RemoveTask(task_id, curr_machine);

    for (int i = 0; i < running.size(); i++)
    {
        MachineInfo_t info = Machine_GetInfo(running[i]);

        if (info.active_tasks == 0 && info.s_state == S0)
        {
            off.push_back(running[i]);
            Machine_SetState(running[i], S5);
            running.erase(running.begin() + i);
            i--;
        }
    }
}