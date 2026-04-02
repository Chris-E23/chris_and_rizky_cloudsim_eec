#include "Scheduler.hpp"
#include <map>
#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h>
#include <set>

static map<MachineId_t, vector<VMId_t>> machine_and_vms;
static map<TaskId_t, VMId_t> tasks_and_vms;
static std::set<VMId_t> migrating_vms;
static map<MachineId_t, vector<TaskId_t>> machines_and_tasks;
static vector<TaskId_t> task_queue;
long prev_size = -1;
static unsigned total_machines = 16;

const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
const MachineId_t INVALID_TASK = std::numeric_limits<MachineId_t>::max();

void MinMin::Init()
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
    }

    SimOutput("MinMin Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);
}

void MinMin::MigrationComplete(Time_t time, VMId_t vm_id)
{
    // Update your data structure. The VM now can receive new tasks
    migrating_vms.erase(vm_id);
}
int slaPriority(SLAType_t sla) {
     switch(sla) {
        case SLA0: return 0;
        case SLA1: return 1;
        case SLA2: return 2;
        case SLA3: return 3;
        default:   return 4;
    }
}
   

void MinMin::NewTask(Time_t now, TaskId_t task_id)
{

    //Calculate the least amount of work with different configurations
    task_queue.push_back(task_id);

    if (task_queue.size() > 15)
    {
        set<MachineId_t> used_machines;
        map<TaskId_t, map<MachineId_t, float>> task_machine_time;

        while (task_queue.size())
        {
            TaskId_t curr = task_queue.back();
            TaskInfo_t curr_task_info = GetTaskInfo(curr);
            task_queue.pop_back();
            
            for (MachineId_t machine_id = 0; machine_id < total_machines; machine_id++)
            {
                MachineInfo_t curr_machine_info = Machine_GetInfo(machine_id);
                if (curr_machine_info.s_state != S0)
                {
                    continue;
                }

                if (curr_machine_info.cpu != curr_task_info.required_cpu)
                {
                    continue;
                }

                unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;
                if (free_mem < curr_task_info.required_memory)
                {
                    continue;
                }
                task_machine_time[curr][machine_id] = (float)curr_task_info.remaining_instructions /
                                                      (float)curr_machine_info.performance[0];
            }
        }
        map<TaskId_t, map<MachineId_t, float>>::iterator map_it;
        // Go through the map while it's still alive. Each time, find the lowest
        // time and choose that to minimize the total time
        while (task_machine_time.size())
        {
            float best_time = std::numeric_limits<float>::max();
            MachineId_t best_machine = INVALID_MACHINE;
            TaskId_t best_task = INVALID_TASK;
            int best_sla_priority = std::numeric_limits<int>::max();
            
            for (map_it = task_machine_time.begin(); map_it != task_machine_time.end(); map_it++)
            {
                map<MachineId_t, float>::iterator machine_it;
                TaskInfo_t curr_task = GetTaskInfo(map_it->first);
                int curr_sla_priority = slaPriority(curr_task.required_sla);
                for (machine_it = map_it->second.begin(); machine_it != map_it->second.end(); machine_it++)
                {
                    if (curr_sla_priority < best_sla_priority ||
                    (curr_sla_priority == best_sla_priority && machine_it->second < best_time))
                    {
                        best_time = machine_it->second;
                        best_machine = machine_it->first;
                        best_task = map_it->first;
                        best_sla_priority = curr_sla_priority;
                    }
                    
                }
            }
            if (best_machine == INVALID_MACHINE)
                break;

            TaskInfo_t best_task_info = GetTaskInfo(best_task);
            machines_and_tasks[best_machine].push_back(best_task);
            bool found = false;
            for (auto vm : machine_and_vms[best_machine])
            {
                if (migrating_vms.count(vm))
                    continue;

                VMInfo_t info = VM_GetInfo(vm);
                if (info.cpu == best_task_info.required_cpu)
                {
                    VM_AddTask(vm, best_task, best_task_info.priority);
                    tasks_and_vms[best_task] = vm;
                    // Return as soon as we find a suitable vm
                    found = true;
                }
            }
            // If we could not find a suitable vm, then make a vm
            if (!found)
            {
                VMId_t new_vm = VM_Create(best_task_info.required_vm, best_task_info.required_cpu);
                machine_and_vms[best_machine].push_back(new_vm);
                VM_Attach(new_vm, best_machine);
                VM_AddTask(new_vm, best_task, best_task_info.priority);
                tasks_and_vms[best_task] = new_vm;
            }

            // Remove the task from consideration since we just dealt with it
            task_machine_time.erase(best_task);
            for (map_it = task_machine_time.begin(); map_it != task_machine_time.end(); map_it++)
            {
                map<MachineId_t, float>::iterator machine_it;

                for (machine_it = map_it->second.begin(); machine_it != map_it->second.end(); machine_it++)
                { // Add all the times to the machines
                    if(machine_it->first == best_machine){
                        machine_it->second += best_time;
                    }
                    
                }
            }
        }
    }
}

void MinMin::PeriodicCheck(Time_t now)
{
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary

    // for (MachineId_t i = 0; i < total_machines; i++)
    // {
    //     // SimOutput("Current Energy Usage of Machine " + to_string(i) + ": " + to_string(Machine_GetEnergy(i)), 0);
    //     printf("Current task count of machine : %lu\n", i, machines_and_tasks[i].size());
    // }
    
    if (task_queue.size() > 0 && prev_size == task_queue.size())
    {
       set<MachineId_t> used_machines;
        map<TaskId_t, map<MachineId_t, float>> task_machine_time;

        while (task_queue.size())
        {
            TaskId_t curr = task_queue.back();
            TaskInfo_t curr_task_info = GetTaskInfo(curr);
            task_queue.pop_back();
            
            for (MachineId_t machine_id = 0; machine_id < total_machines; machine_id++)
            {
                MachineInfo_t curr_machine_info = Machine_GetInfo(machine_id);
                if (curr_machine_info.s_state != S0)
                {
                    continue;
                }

                if (curr_machine_info.cpu != curr_task_info.required_cpu)
                {
                    continue;
                }

                unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;
                if (free_mem < curr_task_info.required_memory)
                {
                    continue;
                }
                task_machine_time[curr][machine_id] = (float)curr_task_info.remaining_instructions /
                                                      (float)curr_machine_info.performance[0];
            }
        }
        map<TaskId_t, map<MachineId_t, float>>::iterator map_it;
        // Go through the map while it's still alive. Each time, find the lowest
        // time and choose that to minimize the total time
        while (task_machine_time.size())
        {
            float best_time = std::numeric_limits<float>::max();
            MachineId_t best_machine = INVALID_MACHINE;
            TaskId_t best_task = INVALID_TASK;
            int best_sla_priority = std::numeric_limits<int>::max();
            
            for (map_it = task_machine_time.begin(); map_it != task_machine_time.end(); map_it++)
            {
                map<MachineId_t, float>::iterator machine_it;
                TaskInfo_t curr_task = GetTaskInfo(map_it->first);
                int curr_sla_priority = slaPriority(curr_task.required_sla);
                for (machine_it = map_it->second.begin(); machine_it != map_it->second.end(); machine_it++)
                {
                    if (curr_sla_priority < best_sla_priority ||
               (curr_sla_priority == best_sla_priority && machine_it->second < best_time))
                {
                    best_time = machine_it->second;
                    best_machine = machine_it->first;
                    best_task = map_it->first;
                    best_sla_priority = curr_sla_priority;
                }
                }
            }
            if (best_machine == INVALID_MACHINE)
                break;

            TaskInfo_t best_task_info = GetTaskInfo(best_task);
            machines_and_tasks[best_machine].push_back(best_task);
            // printf("Best machine: %lu\n", best_machine);
            // printf("Best task: %lu\n", best_task);
            bool found = false;
            for (auto vm : machine_and_vms[best_machine])
            {
                if (migrating_vms.count(vm))
                    continue;

                VMInfo_t info = VM_GetInfo(vm);
                if (info.cpu == best_task_info.required_cpu)
                {
                    VM_AddTask(vm, best_task, best_task_info.priority);
                    tasks_and_vms[best_task] = vm;
                    // Return as soon as we find a suitable vm
                    found = true;
                }
            }
            // If we could not find a suitable vm, then make a vm
            if (!found)
            {
                VMId_t new_vm = VM_Create(best_task_info.required_vm, best_task_info.required_cpu);
                machine_and_vms[best_machine].push_back(new_vm);
                VM_Attach(new_vm, best_machine);
                VM_AddTask(new_vm, best_task, best_task_info.priority);
                tasks_and_vms[best_task] = new_vm;
            }

            // Remove the task from consideration since we just dealt with it
            task_machine_time.erase(best_task);
            for (map_it = task_machine_time.begin(); map_it != task_machine_time.end(); map_it++)
            {
                map<MachineId_t, float>::iterator machine_it;

                for (machine_it = map_it->second.begin(); machine_it != map_it->second.end(); machine_it++)
                { // Add all the times to the machines
                    if(machine_it->first == best_machine){
                        machine_it->second += best_time;
                    }
                    
                }
            }
        } 
    }
    prev_size = task_queue.size();
}

void MinMin::Shutdown(Time_t time)
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

void MinMin::TaskComplete(Time_t now, TaskId_t task_id)
{

    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
}