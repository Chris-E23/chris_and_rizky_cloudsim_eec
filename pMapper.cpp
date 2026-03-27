#include "Scheduler.hpp"
#include <map>
#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
map<MachineId_t, vector<VMId_t>> machine_and_vms;
map<TaskId_t, VMId_t> tasks_and_vms;
map<VMId_t, vector<TaskId_t>>
static bool migrating = false;
static unsigned total_machines = 16;

void pMapper::Init() {
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
    for(unsigned i = 0; i < total_machines; i++) {
        machines.push_back(MachineId_t(i));
    }    
    for(unsigned i = 0; i < total_machines; i++){
        MachineInfo_t curr_machine = Machine_GetInfo(i);
        vms.push_back(VM_Create(LINUX, curr_machine.cpu));
        VM_Attach(vms[i], machines[i]);
        machine_and_vms[i].push_back(vms[i]);
    }
    



    bool dynamic = false;
    if(dynamic)
        for(unsigned i = 0; i<4 ; i++)
            for(unsigned j = 0; j < 8; j++)
                Machine_SetCorePerformance(MachineId_t(0), j, P3);
    

   
  

    SimOutput("pMapper Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);



}

void pMapper::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks


}

bool MachineEnergyComparison(MachineId_t a, MachineId_t b){

    return Machine_GetEnergy(a) < Machine_GetEnergy(a);
}
void pMapper::NewTask(Time_t now, TaskId_t task_id) {
    //Greedy Algorithm 
    
    //This task will have to be assigned to a machine, but we make it negative one 
    //in the case that there is no compatible machine. If there is none, then we 
    //can handle it in the end

    bool GPUCapable = IsTaskGPUCapable(task_id);

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;
    vector<MachineId_t> sorted_list;
    //Sorte
    std::copy(machines.begin(), machines.end(), std::back_inserter(sorted_list));
    sort(sorted_list.begin(), sorted_list.end(), MachineEnergyComparison);

    vector<MachineId_t>::iterator it; 
    MachineId_t curr_machine = -1;
    float best_cpu_util = std::numeric_limits<float>::max();
    MachineId_t best_cpu_machine = -1;
    for(it = sorted_list.begin(); it != sorted_list.end(); it++){
        MachineInfo_t curr_machine_info = Machine_GetInfo(*it);
        float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
        // Approximate that the number of tasks correspond to number of active cpus active 
        float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;
        if(best_cpu_util > cpu_util){
            best_cpu_util = cpu_util;
            best_cpu_machine = *it;
        }
        if(memory_util < 100){
            curr_machine = *it;
            //The first machine that we find that is not fully utilized should be used
            break;
        }

    }
    if(curr_machine == -1){
        //If we can't find a machine with no memory utilized, then use the machine with 
        //the least cpu util
        curr_machine = best_cpu_machine;
    }
   
    MachineInfo_t best = Machine_GetInfo(curr_machine);
    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[curr_machine].push_back(new_vm);
    VM_Attach(new_vm, curr_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
    
    
}


void pMapper::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    
    for(MachineId_t i = 0; i < total_machines; i++){
        SimOutput("Current Energy Usage of Machine " + to_string(i) + ": " + to_string(Machine_GetEnergy(i)), 0);
    }

}

void pMapper::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void pMapper::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
   
    vector<VMId_t>::iterator vm_it; 
    //When migrating, we should look for the best available VM to migrate to
    for(vm_it = vms.begin(); vm_it != vms.end(); vm_it++){
        

    }
    
   



}