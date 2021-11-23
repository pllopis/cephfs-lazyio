# README

## Project description

The goal of this project is to provide a simple way to try out Lazy I/O on CephFS.

There are two main ways to enable Lazy I/O at the time of this writing:

* Using the `client_force_lazyio` ceph config option, which enables Lazy I/O globally. This only works for libcephfs and ceph-fuse, but not for kernel mounts.
* Using the libcephfs calls directly. These work on file handles so they can be made to work with a kernel mount as well.

This library is intended to be preloaded via LD\_PRELOAD. It will intercept any `open()` syscalls and enable Lazy I/O, when required, via a special cephfs ioctl call.

## Inputs

There are two environment variables that can be used as inputs to control the behaviour of the lazyio.so library:

* `LAZYIO_CEPHFS_PREFIX`: Required. Only the file paths that are prefixed by the value of this env variable will have Lazy I/O activated when they are open()ed.
* `LAZYIO_LOG`: Optional. The path to the log file. The caller's PID will be appended to the filename. Any open() calls (even those outside the prefix) and ioctl() calls (for those paths matching the prefix) will be logged to this file.

## Usage

Example usage with IOR using the POSIX interface:

```
$ cat filetestlazy.sh
#!/bin/bash
export LAZYIO_LOG="lazy.log"
export LAZYIO_CEPHFS_PREFIX="/hpcscratch/user/pllopiss/test/lazyio"
export LD_PRELOAD="/hpcscratch/user/pllopiss/src/lazyio/lazyio.so"
/hpcscratch/user/pllopiss/src/ior/src/ior -a POSIX -w -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 1000 -o file

$ srun -N 6 --ntasks-per-node 1 filetestlazy.sh

IOR-3.4.0+dev: MPI Coordinated Test of Parallel I/O                                                                                                                                                                                                           
Began               : Tue Nov 23 18:43:21 2021                                                                                                                                                                                                                
Command line        : /hpcscratch/user/pllopiss/src/ior/src/ior -a POSIX -w -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 1000 -o file                                                                                                                                   
Machine             : Linux hpc-be007.cern.ch                                                                                                                                                                                                                 
TestID              : 0                                                                                                                                                                                                                                       
StartTime           : Tue Nov 23 18:43:21 2021                                                                                                                                                                                                                
Path                : file                                                                                                                                                                                                                                    
FS                  : 68.4 TiB   Used FS: 50.2%   Inodes: 50.8 Mi   Used Inodes: 100.0%                                                                                                                                                                       
                                                                                                                                                                                                                                                              
Options:                                                                                                                                                                                                                                                      
api                 : POSIX                                       
apiVersion          :              
test filename       : file         
access              : single-shared-file                          
type                : independent              
segments            : 1000         
ordering in a file  : sequential
ordering inter file : constant task offset
task offset         : 1            
nodes               : 6            
tasks               : 6            
clients per node    : 1            
repetitions         : 1            
xfersize            : 2 MiB        
blocksize           : 2 MiB        
aggregate filesize  : 11.72 GiB                                           
                                                               
Results:                           
                         
access    bw(MiB/s)  IOPS       Latency(s)  block(KiB) xfer(KiB)  open(s)    wr/rd(s)   close(s)   total(s)   iter
------    ---------  ----       ----------  ---------- ---------  --------   --------   --------   --------   ----
write     1750.53    876.69     6.84        2048.00    2048.00    0.010898   6.84       0.000284   6.86       0
                                                               
Summary of all tests:                                                     
Operation   Max(MiB)   Min(MiB)  Mean(MiB)     StdDev   Max(OPs)   Min(OPs)  Mean(OPs)     StdDev    Mean(s) Stonewall(s) Stonewall(MiB) Test# #Tasks tPN reps fPP reord reordoff reordrand seed segcnt   blksiz    xsize aggs(MiB)   API RefNum
write        1750.53    1750.53    1750.53       0.00     875.27     875.27     875.27       0.00    6.85505         NA            NA     0      6   1    1   0     1        1         0    0   1000  2097152  2097152   12000.0 POSIX      0
Finished            : Tue Nov 23 18:43:28 2021                            
```

Compare the achieved throughput to the vanilla performance without enabling Lazy I/O:

```
$ srun -N 6 --ntasks-per-node 1 /hpcscratch/user/pllopiss/src/ior/src/ior -a POSIX -w -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 1000 -o file

IOR-3.4.0+dev: MPI Coordinated Test of Parallel I/O
Began               : Tue Nov 23 19:05:43 2021
Command line        : /hpcscratch/user/pllopiss/src/ior/src/ior -a POSIX -w -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 1000 -o file
Machine             : Linux hpc-be007.cern.ch
TestID              : 0
StartTime           : Tue Nov 23 19:05:43 2021
Path                : file
FS                  : 68.4 TiB   Used FS: 50.2%   Inodes: 50.8 Mi   Used Inodes: 100.0%

Options:
api                 : POSIX
apiVersion          :
test filename       : file
access              : single-shared-file
type                : independent
segments            : 1000
ordering in a file  : sequential
ordering inter file : constant task offset
task offset         : 1
nodes               : 6
tasks               : 6
clients per node    : 1
repetitions         : 1
xfersize            : 2 MiB
blocksize           : 2 MiB
aggregate filesize  : 11.72 GiB

Results:

access    bw(MiB/s)  IOPS       Latency(s)  block(KiB) xfer(KiB)  open(s)    wr/rd(s)   close(s)   total(s)   iter
------    ---------  ----       ----------  ---------- ---------  --------   --------   --------   --------   ----
write     536.31     268.20     22.34       2048.00    2048.00    0.003488   22.37      0.000182   22.38      0   

Summary of all tests:
Operation   Max(MiB)   Min(MiB)  Mean(MiB)     StdDev   Max(OPs)   Min(OPs)  Mean(OPs)     StdDev    Mean(s) Stonewall(s) Stonewall(MiB) Test# #Tasks tPN reps fPP reord reordoff reordrand seed segcnt   blksiz    xsize aggs(MiB)   API RefNum
write         536.31     536.31     536.31       0.00     268.15     268.15     268.15       0.00   22.37521         NA            NA     0      6   1    1   0     1        1         0    0   1000  2097152  2097152   12000.0 POSIX      0
Finished            : Tue Nov 23 19:06:05 2021
```

For even better performance though, use libcephfs directly. This bypasses the need for this project altogether, but will obviously not work with applications that are POSIX.
The following demonstrates using IOR's CEPHFS interface instead of POSIX:
```
→ mpirun --allow-run-as-root -np 6 -hostfile hostfile -map-by node /hpcscratch/user/pllopiss/src/ior/src/ior -a CEPHFS -k -w -c -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 10000 -o /hpcscratch/user/pllopiss/test/lazyio/file --cephfs.user=hpcscidbe --cephfs.conf jim.conf  --cephfs.prefix /volumes/_nogroup/355f485c-6319-4ffe-acd6-94a07f2a14b4 --cephfs.olazy
IOR-3.4.0+dev: MPI Coordinated Test of Parallel I/O                       
Began               : Tue Nov 23 13:10:11 2021                            
Command line        : /hpcscratch/user/pllopiss/src/ior/src/ior -a CEPHFS -k -w -c -C -Q 1 -g -G 27 -e -t 2m -b 2m -s 10000 -o user/pllopiss/test/lazyio/file --cephfs.user=hpcscidbe --cephfs.conf jim.conf --cephfs.prefix /volumes/_nogroup/355f485c-6319-4
ffe-acd6-94a07f2a14b4 --cephfs.olazy                                      
Machine             : Linux hpc-photon007.cern.ch                                                                 
TestID              : 0                                                                                                                                                                                                                                       
StartTime           : Tue Nov 23 13:10:11 2021                                                                 
Path                : user/pllopiss/test/lazyio/file                      
FS                  : 68.4 TiB   Used FS: 50.3%   Inodes: 18.6 Mi   Used Inodes: 100.0%
                                                                                                                                                                                                                                                             
Options:                                                                                                                                                                                                                                                     
api                 : CEPHFS                                                           
apiVersion          :                                                     
test filename       : user/pllopiss/test/lazyio/file                      
access              : single-shared-file                                  
type                : collective                                          
segments            : 10000                                                                                       
ordering in a file  : sequential                                                                                  
ordering inter file : constant task offset                                                                     
task offset         : 1                                                   
nodes               : 6                                                   
tasks               : 6                                                                                                                                                                                                                         
clients per node    : 1                                                                                                                                                                                                                                       
repetitions         : 1                                                         
xfersize            : 2 MiB                                                                                       
blocksize           : 2 MiB                                                                                       
aggregate filesize  : 117.19 GiB                                                                                                                                                                                                                              
                                                                                          
Results:                                                                               
WARNING: The file "user/pllopiss/test/lazyio/file" exists already and will be overwritten                                                                                                                                                       
                                                                                                                                                                                                                                             
access    bw(MiB/s)  IOPS       Latency(s)  block(KiB) xfer(KiB)  open(s)    wr/rd(s)   close(s)   total(s)   iter
------    ---------  ----       ----------  ---------- ---------  --------   --------   --------   --------   ----
write     3646       1824.56    32.05       2048.00    2048.00    0.032266   32.88      0.000641   32.92      0   
                                                                                                                  
Summary of all tests:                                                                                                                                                                                                                                         
Operation   Max(MiB)   Min(MiB)  Mean(MiB)     StdDev   Max(OPs)   Min(OPs)  Mean(OPs)     StdDev    Mean(s) Stonewall(s) Stonewall(MiB) Test# #Tasks tPN reps fPP reord reordoff reordrand seed segcnt   blksiz    xsize aggs(MiB)   API RefNum
write        3645.52    3645.52    3645.52       0.00    1822.76    1822.76    1822.76       0.00   32.91714         NA            NA     0      6   1    1   0     1        1         0    0  10000  2097152  2097152  120000.0 CEPHFS      0
Finished            : Tue Nov 23 13:10:44 2021
```
