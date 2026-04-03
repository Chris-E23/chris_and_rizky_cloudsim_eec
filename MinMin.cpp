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
long prev_count = 0;
const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
const MachineId_t INVALID_TASK = std::numeric_limits<MachineId_t>::max();
std::set<MachineId_t> sleeping_machines;
static float max_mips = 0; 

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
    // for (unsigned i = 0; i < total_machines; i++)
    // {
    //     MachineInfo_t curr_machine = Machine_GetInfo(i);
    //     vms.push_back(VM_Create(LINUX, curr_machine.cpu));
    //     VM_Attach(vms[i], machines[i]);
    //     machine_and_vms[i].push_back(vms[i]);
    // }

    // SimOutput("MinMin Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);
    for(MachineId_t machine_id = 0; machine_id < total_machines; machine_id++){
        max_mips = max(max_mips, (float)Machine_GetInfo(machine_id).performance[0]);
    }
}

void MinMin::MigrationComplete(Time_t time, VMId_t vm_id)
{
    // Update your data structure. The VM now can receive new tasks
    migrating_vms.erase(vm_id);
}
int slaPriority(SLAType_t sla)
{
    switch (sla)
    {
    case SLA0:
        return 0;
    case SLA1:
        return 1;
    case SLA2:
        return 2;
    default:
        return 3;
    }
}

void MinMin::NewTask(Time_t now, TaskId_t task_id)
{

    // Calculate the least amount of work with different configurations
    task_queue.push_back(task_id);
    TaskInfo_t task_info = GetTaskInfo(task_id);
    if (task_info.required_sla == SLA0)
    {
        ScheduleBatch(); // schedule immediately
        return;
    }
    if (task_queue.size() < 20)
        return; // wait for a full batch

    ScheduleBatch();
}
void MinMin::ScheduleBatch()
{

    // Track memory reserved within this batch to avoid over-committing
    map<MachineId_t, unsigned> reserved_mem;
    // Track estimated load added within this batch
    map<MachineId_t, float> reserved_time;

    map<TaskId_t, map<MachineId_t, float>> ect;

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
    for (TaskId_t taskid : task_queue)
    {
        float factor = 1.0;
        while (ect[taskid].size() == 0)
        {
            TaskInfo_t ti = GetTaskInfo(taskid);
            for (MachineId_t machineid = 0; machineid < total_machines; machineid++)
            {
                MachineInfo_t mi = Machine_GetInfo(machineid);
                float cpu_util = (float)mi.active_tasks / float(mi.num_cpus);

                if (mi.s_state != S0 || sleeping_machines.count(machineid))
                    continue;
                if (mi.cpu != ti.required_cpu)
                    continue;
                if (mi.memory_size - mi.memory_used < ti.required_memory)
                    continue;
                if (ti.required_sla == SLA0 && mi.performance[0] < max_mips * 0.8f)
                    continue; // SLA0 never goes to slow machines

                if (ti.required_sla == SLA0 && cpu_util > 2 * factor)
                    continue;
                if (ti.required_sla == SLA1 && cpu_util > 4 * factor)
                    continue;

                float base = (float)ti.remaining_instructions / (float)mi.performance[0];

                // Penalize loaded machines: a machine at 2x utilization takes ~2x longer
                // Clamp to avoid exploding scores on transiently busy machines
                float load_penalty = max(1.0f, cpu_util);

                ect[taskid][machineid] = base * load_penalty + reserved_time[machineid];
            }

            factor += 1;
        }
    }

    task_queue.clear();

    while (!ect.empty())
    {
        float best_time = std::numeric_limits<float>::max();
        MachineId_t best_machine = INVALID_MACHINE;
        TaskId_t best_task = INVALID_TASK;
        uint64_t best_sla = std::numeric_limits<int>::max();
        // Find the best SLA and the best time
        for (auto &[tid, ms] : ect)
        {
            int sla = slaPriority(GetTaskInfo(tid).required_sla);
            TaskInfo_t curr_task = GetTaskInfo(tid);
            float best_util = std::numeric_limits<float>::max();
            for (auto &[mid, t] : ms)
            {
                MachineInfo_t curr_machine = Machine_GetInfo(mid);

                // If there isn't any memory remaining, then don't use the machine
                if (curr_machine.memory_size - curr_machine.memory_used < curr_task.required_memory)
                {
                    continue;
                }
                if (sla < best_sla || (sla == best_sla && t < best_time))
                {
                    best_time = t;
                    best_machine = mid;
                    best_task = tid;
                    best_sla = sla;
                }
            }
        }

        if (best_machine == INVALID_MACHINE)
        {
            // This means that there were no more machines with enough memory
            // Select the machine with simply the best processing time
            for (auto &[tid, ms] : ect)
            {
                int sla = slaPriority(GetTaskInfo(tid).required_sla);
                uint64_t best_energy = std::numeric_limits<uint64_t>::max();
                for (auto &[mid, t] : ms)
                {
                    if (sla < best_sla || (sla == best_sla && t < best_time))
                    {
                        best_time = t;
                        best_machine = mid;
                        best_task = tid;
                        best_sla = sla;
                    }
                }
            }
        }
        TaskInfo_t info = GetTaskInfo(best_task);
        reserved_mem[best_machine] += info.required_memory;
        reserved_time[best_machine] += best_time;
        // Found best machine for task
        machines_and_tasks[best_machine].push_back(best_task);

        bool found = false;
        for (auto vm : machine_and_vms[best_machine])
        {
            if (migrating_vms.count(vm))
                continue;
            if (VM_GetInfo(vm).cpu == info.required_cpu)
            {
                VM_AddTask(vm, best_task, info.priority);
                tasks_and_vms[best_task] = vm;
                found = true;
                break;
            }
        }
        if (!found)
        {
            VMId_t nv = VM_Create(info.required_vm, info.required_cpu);
            machine_and_vms[best_machine].push_back(nv);
            VM_Attach(nv, best_machine);
            VM_AddTask(nv, best_task, info.priority);
            tasks_and_vms[best_task] = nv;
        }

        ect.erase(best_task);
        // Update the time across machines
        for (auto &[tid, ms] : ect)
        {
            for (auto &[machine, time] : ms)
            {
                MachineInfo_t mi = Machine_GetInfo(machine);
                float base = (float)GetTaskInfo(tid).remaining_instructions / mi.performance[0];
                float cpu_util = (float)mi.active_tasks / (float)mi.num_cpus;
                float load_penalty = max(1.0f, cpu_util);

                ms[machine] = base * load_penalty + reserved_time[machine];
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

    if (!task_queue.empty() && prev_count == task_queue.size())
        ScheduleBatch();
    prev_count = task_queue.size();
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
    if (tasks_and_vms.find(task_id) == tasks_and_vms.end())
        return; // ← guard

    VMId_t vm = tasks_and_vms[task_id];
    tasks_and_vms.erase(task_id);

    // Remove from machine task list
    MachineId_t mid = VM_GetInfo(vm).machine_id;
    auto &tasklist = machines_and_tasks[mid];
    tasklist.erase(std::remove(tasklist.begin(), tasklist.end(), task_id),
                   tasklist.end());

    // for (MachineId_t i = 0; i < total_machines; i++)
    // {
    //     MachineInfo_t info = Machine_GetInfo(i);
    //     if (info.active_tasks == 0 && info.s_state == S0 && machine_and_vms[i].empty())
    //     {
    //         Machine_SetState(i, S5);
    //         sleeping_machines.insert(i);
    //     }
    // }
}