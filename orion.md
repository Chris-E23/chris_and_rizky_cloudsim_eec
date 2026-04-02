machine class:
{
	Number of machines: 4
	CPU type: X86
	Number of cores: 112
	Memory: 2097152
	S-States: [8000, 6400, 5600, 4400, 2500, 720, 0]
	P-States: [22, 15, 10, 6]
	C-States: [22, 6, 2, 0]
	MIPS: [9000, 6750, 4500, 2250]
	GPUs: yes
}
machine class:
{
	Number of machines: 16
	CPU type: X86
	Number of cores: 64
	Memory: 4194304
	S-States: [1800, 1450, 1300, 1000, 600, 150, 0]
	P-States: [16, 11, 7, 4]
	C-States: [16, 4, 1, 0]
	MIPS: [6000, 4500, 3000, 1500]
	GPUs: no
}
task class:
{
	Start time: 0
	End time: 120000000
	Inter arrival: 600
	Expected runtime: 300000
	Memory: 128
	VM type: LINUX
	GPU enabled: no
	SLA type: SLA0
	CPU type: X86
	Task type: WEB
	Seed: 18382
}
task class:
{
	Start time: 1000000
	End time: 120000000
	Inter arrival: 8000
	Expected runtime: 6000000
	Memory: 384
	VM type: LINUX
	GPU enabled: no
	SLA type: SLA1
	CPU type: X86
	Task type: STREAM
	Seed: 29482
}
task class:
{
	Start time: 2000000
	End time: 120000000
	Inter arrival: 5000
	Expected runtime: 4500000
	Memory: 4096
	VM type: LINUX
	GPU enabled: yes
	SLA type: SLA1
	CPU type: X86
	Task type: AI
	Seed: 38572
}
task class:
{
	Start time: 6000000
	End time: 120000000
	Inter arrival: 7750
	Expected runtime: 18000000
	Memory: 28672
	VM type: LINUX
	GPU enabled: yes
	SLA type: SLA2
	CPU type: X86
	Task type: HPC
	Seed: 41029
}
task class:
{
	Start time: 3000000
	End time: 120000000
	Inter arrival: 10000
	Expected runtime: 18000000
	Memory: 131072
	VM type: LINUX
	GPU enabled: no
	SLA type: SLA2
	CPU type: X86
	Task type: STREAM
	Seed: 51831
}