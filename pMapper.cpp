#include "Scheduler.hpp"
#include <map>
#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <bits/stdc++.h>
static map<MachineId_t, vector<VMId_t>> machine_and_vms;
static map<TaskId_t, VMId_t> tasks_and_vms;
static std::set<VMId_t> migrating_vms;
static map<MachineId_t, vector<TaskId_t>> machines_and_tasks;
static std::set<MachineId_t> sleeping_machines;
static unsigned total_machines = 16;
vector<MachineId_t> sorted_list;
const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
bool MachineEnergyComparison(MachineId_t a, MachineId_t b)
{

    return Machine_GetEnergy(a) < Machine_GetEnergy(b);
}
void pMapper::Init()
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
    std::copy(machines.begin(), machines.end(), std::back_inserter(sorted_list));
    sort(sorted_list.begin(), sorted_list.end(), MachineEnergyComparison);
    SimOutput("pMapper Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);
}

void pMapper::MigrationComplete(Time_t time, VMId_t vm_id)
{
    // Update your data structure. The VM now can receive new tasks
    migrating_vms.erase(vm_id);
}

MachineId_t findLowestUtilization(TaskId_t task_id, vector<MachineId_t> list)
{

    MachineId_t curr_machine = INVALID_MACHINE;
    TaskInfo_t task_info = GetTaskInfo(task_id);
    CPUType_t req_cpu = task_info.required_cpu;
    vector<MachineId_t>::iterator it;
    unsigned required_memory = task_info.required_memory;
    float best_util = std::numeric_limits<float>::max();
    for (it = list.begin(); it != list.end(); it++)
    {
        MachineInfo_t curr_machine_info = Machine_GetInfo(*it);
        float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
        // Approximate that the number of tasks correspond to number of active cpus active
        float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;
        unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;
        float curr_util = max(memory_util, cpu_util);
        if (curr_machine_info.s_state != S0)
            continue;

        if (curr_machine_info.cpu != req_cpu)
        {
            continue;
        }
        if (free_mem < required_memory)
        {
            continue;
        }
        if (best_util > curr_util)
        {
            curr_machine = *it;
            best_util = cpu_util;
        }
    }
    return curr_machine;
}
void pMapper::NewTask(Time_t now, TaskId_t task_id)
{
    // pMapper algorithms, fills the current machine up before going on to the next one

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;
    SLAType_t sla = new_task_info.required_sla;

    // Sorted

    vector<MachineId_t>::iterator it;
    MachineId_t curr_machine = INVALID_MACHINE;
    float best_cpu_util = std::numeric_limits<float>::max();
    MachineId_t best_cpu_machine = INVALID_MACHINE;
    float factor = 1.0;
    if (sleeping_machines.size() == total_machines)
    {
        // Mass wake up to avoid no machines working
        std::set<MachineId_t>::iterator sl_it;

        for (sl_it = sleeping_machines.begin(); sl_it != sleeping_machines.end(); sl_it++)
        {
            Machine_SetState(*sl_it, S0);
        }
        sleeping_machines.clear();
    }
    if (sla == SLA0)
        curr_machine = findLowestUtilization(task_id, sorted_list);
      
    if (!curr_machine)
    {
        for (it = sorted_list.begin(); it != sorted_list.end(); it++)
        {
            MachineInfo_t curr_machine_info = Machine_GetInfo(*it);
            float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
            // Approximate that the number of tasks correspond to number of active cpus active
            // float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;
            // unsigned free_mem = machine_and_memory[*it];
            unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;

            if (curr_machine_info.s_state != S0)
                continue;

            if (curr_machine_info.cpu != req_cpu)
            {
                continue;
            }
            if (free_mem < required_memory)
            {
                continue;
            }
            if (new_task_info.required_sla == SLA0 && cpu_util > 0.8)
            {
                continue;
            }
            if (new_task_info.required_sla == SLA1 && cpu_util > 0.9)
            {
                continue;
            }
            if (new_task_info.required_sla == SLA2 && cpu_util > 1)
            {
                continue;
            }

            curr_machine = *it;
            break;
        }
    }

    if (curr_machine == INVALID_MACHINE)
    {
        float best_util = std::numeric_limits<float>::max();

        for (MachineId_t machine_id = 0; machine_id < total_machines; machine_id++)
        {

            MachineInfo_t curr_machine_info = Machine_GetInfo(machine_id);
            float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
            // Approximate that the number of tasks correspond to number of active cpus active
            float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;
            float curr_util = max(cpu_util, memory_util);
            unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;

            if (curr_machine_info.s_state != S0)
                continue;

            if (sleeping_machines.count(machine_id))
                continue;
            if (curr_machine_info.cpu != req_cpu)
            {
                continue;
            }

            if (best_util > curr_util)
            {
                best_util = curr_util;
                curr_machine = machine_id;
            }
        }
    }

    // Update the data structure
    machines_and_tasks[curr_machine].push_back(task_id);

    for (auto vm : machine_and_vms[curr_machine])
    {
        if (migrating_vms.count(vm))
            continue;
        VMInfo_t info = VM_GetInfo(vm);
        if (info.cpu == req_cpu)
        {
            VM_AddTask(vm, task_id, new_task_info.priority);
            tasks_and_vms[task_id] = vm;
            return;
        }
    }

    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[curr_machine].push_back(new_vm);
    VM_Attach(new_vm, curr_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
}

void pMapper::PeriodicCheck(Time_t now)
{
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    // printf("UPDATE:\n\n");
    // printf("Time:  %uld\n", now);
    // for (MachineId_t i = 0; i < total_machines; i++)
    // {
    //     // SimOutput("Current Energy Usage of Machine " + to_string(i) + ": " + to_string(Machine_GetEnergy(i)), 0);
    //     printf("Current task count of machine %d: %lu\n", i, machines_and_tasks[i].size());
    // }
}

void pMapper::Shutdown(Time_t time)
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
    vector<TaskId_t> curr_tasks = machines_and_tasks[curr_machine];
    int ind = -1;
    for (int i = 0; i < curr_tasks.size(); i++)
    {
        if (curr_tasks[i] == task_id)
        {
            ind = i;
        }
    }
    if (ind != -1)
    {
        machines_and_tasks[curr_machine].erase(machines_and_tasks[curr_machine].begin() + ind);
    }
    // tasks_and_vms[task_id] = NULL;
    tasks_and_vms.erase(task_id);
}

void pMapper::TaskComplete(Time_t now, TaskId_t task_id)
{

    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
    MachineId_t curr_machine = VM_GetInfo(tasks_and_vms[task_id]).machine_id;
    // // Remove the current task from the data structure

    RemoveTask(task_id, curr_machine);

    if (true)
    {
        // Now we attempt to migrate some tasks
        float best_util = std::numeric_limits<float>::max();

        MachineId_t best_machine = INVALID_MACHINE;
        float best_cpu_util = std::numeric_limits<float>::max();
        MachineId_t best_cpu_machine = INVALID_MACHINE;

        // Go through all machines to find the least utilized machine
        for (MachineId_t machine_id = 0; machine_id < total_machines; machine_id++)
        {
            MachineInfo_t curr_machine_info = Machine_GetInfo(machine_id);
            float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
            // Approximate that the number of tasks correspond to number of active cpus active
            float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;

            float curr_util = max(cpu_util, memory_util);
            if (curr_machine_info.active_tasks == 0)
            {
                // We are ensuring that the machine we choose has at least one task
                continue;
            }

            if (best_util > curr_util && cpu_util < 0.1)
            {
                best_util = curr_util;
                best_machine = machine_id;
            }
        }

        /*We only want to migrate if very necessarily, this means that not enough memory, or cpu_utiliz-
        aztion is super high.
        */

        if (best_machine != INVALID_MACHINE && machine_and_vms[best_machine].size() > 0)
        { // This means we found a machine who's workload we can transfer

            vector<VMId_t> curr_vms = machine_and_vms[best_machine];

            int best_vm_ind = -1;
            long best_instr_count = std::numeric_limits<long>::max();
            // Find the lowest utilized vm on that machine
            for (int i = 0; i < curr_vms.size(); i++)
            {
                VMInfo_t curr_vm = VM_GetInfo(curr_vms[i]);
                vector<TaskId_t> curr_tasks = curr_vm.active_tasks;
                long curr_instr_count = 0;
                if (migrating_vms.count(curr_vms[i]))
                {
                    continue;
                }
                bool skip_vm = false;
                for (int j = 0; j < curr_tasks.size(); j++)
                {
                    // Sum up amount of instructions in the current_vm
                    TaskInfo_t curr_task = GetTaskInfo(curr_tasks[j]);
                    // This means that there is a task that has an SLA of 0
                    if (curr_task.required_sla == SLA0)
                    {
                        skip_vm = true;
                        break;
                    }
                    curr_instr_count += curr_task.remaining_instructions;
                }

                if (!skip_vm && best_instr_count > curr_instr_count)
                {
                    best_instr_count = curr_instr_count;
                    best_vm_ind = i;
                }
            }
            // Remove the VM from this machine
            if (best_vm_ind == -1)
            {
                // If we cannot find a VM to migrate, then stop the function
                return;
            }
            VMId_t best_vm_id = curr_vms[best_vm_ind];

            // We only want to migrate the workload to a more highly utilized server
            // if it's compatible, otherwise, just keep it on the current machine
            VMInfo_t new_vm_info = VM_GetInfo(best_vm_id);
            CPUType_t req_cpu = new_vm_info.cpu;
            MachineId_t target_machine = INVALID_MACHINE;
            float best_util = 0;
            // Go through the highest utilized machines and select one
            long required_memory = 0;
            for (int i = 0; i < new_vm_info.active_tasks.size(); i++)
            {
                TaskInfo_t temp = GetTaskInfo(new_vm_info.active_tasks[i]);
                required_memory += temp.required_memory;
            }
            int factor = 1;
            while (target_machine == INVALID_MACHINE && factor < 10)
            {
                vector<MachineId_t>::iterator it;
                for (it = sorted_list.begin(); it != sorted_list.end(); it++)
                {

                    MachineInfo_t curr_machine_info = Machine_GetInfo(*it);
                    float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
                    // Approximate that the number of tasks correspond to number of active cpus active
                    float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;
                    float curr_util = max(cpu_util, memory_util);
                    unsigned free_mem = curr_machine_info.memory_size - curr_machine_info.memory_used;

                    if (curr_machine_info.s_state != S0)
                        continue;
                    if (sleeping_machines.count(*it))
                        continue;

                    if (*it == best_machine)
                    {
                        continue;
                    }
                    if (curr_machine_info.cpu != req_cpu)
                    {
                        continue;
                    }
                    if (free_mem < required_memory)
                    {
                        continue;
                    }
                    if (cpu_util > factor)
                    {
                        continue;
                    }
                    target_machine = *it;
                }
                factor += 1;
            }

            vector<TaskId_t>::iterator taskIt;

            if (target_machine != INVALID_MACHINE && !migrating_vms.count(best_vm_id))
            {
                MachineInfo_t temp = Machine_GetInfo(target_machine);
                if (temp.s_state == S0)
                {
                    for (taskIt = new_vm_info.active_tasks.begin(); taskIt != new_vm_info.active_tasks.end(); taskIt++)
                    {
                        RemoveTask(*taskIt, best_machine);
                        machines_and_tasks[target_machine].push_back(*taskIt);
                        tasks_and_vms[*taskIt] = best_vm_id;
                    }
                    migrating_vms.insert(best_vm_id);

                    // printf("Target Machine ID: %d\n", target_machine);
                    // printf("Best Machine ID: %d\n", best_machine);
                    // printf("Best VM ID: %d\n", best_vm_id);
                    // printf("Target machine s state %d\n", temp.s_state);

                    machine_and_vms[best_machine].erase(
                        std::remove(machine_and_vms[best_machine].begin(),
                                    machine_and_vms[best_machine].end(), best_vm_id),
                        machine_and_vms[best_machine].end());
                    machine_and_vms[target_machine].push_back(best_vm_id);

                    VM_Migrate(best_vm_id, target_machine);
                }
            }

            for (MachineId_t i = 0; i < total_machines; i++)
            {
                MachineInfo_t info = Machine_GetInfo(i);
                if (i == target_machine)
                {
                    continue;
                }
                if (info.active_tasks == 0 && info.s_state == S0 && machine_and_vms[i].empty())
                {
                    Machine_SetState(i, S5);
                    sleeping_machines.insert(i);
                }
            }
        }
    }
}