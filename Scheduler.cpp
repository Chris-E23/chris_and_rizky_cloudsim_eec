//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"
#include <map>
#include <limits>
static bool migrating = false;
static unsigned total_machines = 16;
map<MachineId_t, vector<VMId_t>> machine_and_vms;
map<TaskId_t, VMId_t> tasks_and_vms;

void Scheduler::Init() {
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
    

   
  

    SimOutput("Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);



}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks


}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Get the task parameters
    //  IsGPUCapable(task_id);
    //  GetMemory(task_id);
    //  RequiredVMType(task_id);
    //  RequiredSLA(task_id);
    //  RequiredCPUType(task_id);
    // Decide to attach the task to an existing VM, 
    //      vm.AddTask(taskid, Priority_T priority); or
    // Create a new VM, attach the VM to a machine
    //      VM vm(type of the VM)
    //      vm.Attach(machine_id);
    //      vm.AddTask(taskid, Priority_t priority) or
    // Turn on a machine, create a new VM, attach it to the VM, then add the task
    //
    // Turn on a machine, migrate an existing VM from a loaded machine....
    //
    // Other possibilities as desired
    // Skeleton code, you need to change it according to your algorithm

    //Greedy Algorithm 
    
    //This task will have to be assigned to a machine, but we make it negative one 
    //in the case that there is no compatible machine. If there is none, then we 
    //can handle it in the end
    GreedyScheduler(vms, task_id);
   
    
    
}

void Scheduler::GreedyScheduler(vector<VMId_t>vms, TaskId_t task_id){
    bool GPUCapable = IsTaskGPUCapable(task_id);

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;
  
    float best_util = std::numeric_limits<float>::max();

    MachineId_t best_machine = -1;
    
    for(MachineId_t machine_id = 0; machine_id < total_machines; machine_id++){
        //Assign tasks based on the info at hand 
        //Choose machines with least number of tasks and most memory available
  
        MachineInfo_t curr_machine_info = Machine_GetInfo(machine_id);
        float cpu_util = (float)curr_machine_info.active_tasks / (float)curr_machine_info.num_cpus;
        // Approximate that the number of tasks correspond to number of active cpus active 
        float memory_util = (float)curr_machine_info.memory_used / (float)curr_machine_info.memory_size;

        float curr_util = max(cpu_util, memory_util);
        if(GPUCapable && !curr_machine_info.gpus){
            continue;
        }
        if(curr_machine_info.cpu != req_cpu){
            continue;
        }   
        unsigned remaining_memory = curr_machine_info.memory_size - curr_machine_info.memory_used;
        if(remaining_memory < required_memory){
            continue;
        }
        if(best_machine == -1){
            //At this point, we know that the current machine is compatible, so let's set it as the
            // base 
            best_machine = machine_id;
        }

        if(best_util > curr_util){
            best_util = curr_util;
            best_machine = machine_id;

        }
      
    }
    if(best_machine == -1){
          printf("Best machine not found! All machines being used at capacity!\n");
    }

    MachineInfo_t best = Machine_GetInfo(best_machine);
    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[best_machine].push_back(new_vm);
    VM_Attach(new_vm, best_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary


}

void Scheduler::Shutdown(Time_t time) {
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

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
    
    VMId_t current_vm = tasks_and_vms[task_id];
    VM_RemoveTask(current_vm, task_id);
    /*Might be strenous, consider doing this in schedulercheck. 
    */
    for(int i = 0; i < vms.size(); i++){
        VMInfo_t current_vm = VM_GetInfo(vms[i]);
        if(current_vm.active_tasks.size() == 0){
            VM_Shutdown(vms[i]);
        }
    }
   



}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();

}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);

}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
    static unsigned counts = 0;
    counts++;

}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
    
    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    // Activate more machines?
    TaskInfo_t task_info = GetTaskInfo(task_id);
    if(task_info.completion > task_info.target_completion){


    }


}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
   

}


