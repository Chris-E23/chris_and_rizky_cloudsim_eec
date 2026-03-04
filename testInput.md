machine class:
{
        Number of machines: 24
        Number of cores: 16
        CPU type: ARM
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1000, 800, 600, 400]
        GPUs: no
}
task class:
{
# Standard Web Task
        Start time: 1000
        End time : 10800000000
        Inter arrival: 10000
        Expected runtime: 20000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 5318008
}
task class:
{
# Standard Web Task
        Start time: 1000
        End time : 10800000000
        Inter arrival: 10000
        Expected runtime: 20000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 5318008
}
task class:
{
# Malicious Crypto Task
        Start time: 10800000000
        End time : 18000000000
        Inter arrival: 10000
        Expected runtime: 200000000
        Memory: 4096
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: ARM
        Task type: CRYPTO
        Seed: 520230
}
task class:
{
# Malicious Crypto Task
        Start time: 10800000000
        End time : 18000000000
        Inter arrival: 10000
        Expected runtime: 200000000
        Memory: 4096
        VM type: WIN
        GPU enabled: yes
        SLA type: SLA0
        CPU type: ARM
        Task type: CRYPTO
        Seed: 520230
}


