#include "Scheduler.hpp"
#include <map>
#include <limits>
static bool migrating = false;
static unsigned total_machines = 16;
static map<MachineId_t, vector<VMId_t>> machine_and_vms;
static map<TaskId_t, VMId_t> tasks_and_vms;
const MachineId_t INVALID_MACHINE = std::numeric_limits<MachineId_t>::max();
static map<MachineId_t, vector<TaskId_t>> machines_and_tasks;

void GreedyScheduler::Init() {
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
    

   
  

    SimOutput("Scheduler::Init(): VM ids are " + to_string(vms[0]) + " and " + to_string(vms[1]), 3);



}

void GreedyScheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks


}

void GreedyScheduler::NewTask(Time_t now, TaskId_t task_id) {
    //Greedy Algorithm 
    
    //This task will have to be assigned to a machine, but we make it negative one 
    //in the case that there is no compatible machine. If there is none, then we 
    //can handle it in the end

    bool GPUCapable = IsTaskGPUCapable(task_id);

    TaskInfo_t new_task_info = GetTaskInfo(task_id);
    SLAType_t curr_sla = new_task_info.required_sla;
    CPUType_t req_cpu = new_task_info.required_cpu;
    unsigned required_memory = new_task_info.required_memory;
  
    float best_util = std::numeric_limits<float>::max();

    MachineId_t best_machine = INVALID_MACHINE;
    float best_cpu_util = std::numeric_limits<float>::max(); 
    MachineId_t best_cpu_machine = INVALID_MACHINE; 

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
        if(best_machine == INVALID_MACHINE){
            //At this point, we know that the current machine is compatible, so let's set it as the
            // base 
            best_machine = machine_id;
        }

        if(best_util > curr_util){
            best_util = curr_util;
            best_machine = machine_id;

        }
      
    }
    if(best_machine == INVALID_MACHINE){
          printf("Best machine not found! All machines being used at capacity!\n");
          
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
    MachineInfo_t best = Machine_GetInfo(best_machine);
    VMId_t new_vm = VM_Create(new_task_info.required_vm, new_task_info.required_cpu);
    machine_and_vms[best_machine].push_back(new_vm);
    VM_Attach(new_vm, best_machine);
    VM_AddTask(new_vm, task_id, new_task_info.priority);
    tasks_and_vms[task_id] = new_vm;
    
   
    
    
}


void GreedyScheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    
    // for(MachineId_t i = 0; i < total_machines; i++){
    //     SimOutput("Current Energy Usage of Machine " + to_string(i) + ": " + to_string(Machine_GetEnergy(i)), 0);
    // }

}

void GreedyScheduler::Shutdown(Time_t time) {
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

void GreedyScheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
    
    VMId_t current_vm = tasks_and_vms[task_id];
   
   



}

