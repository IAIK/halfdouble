import numpy as np 
import time
import random

from sklearn.metrics import precision_recall_fscore_support as pr

from solver import solve

# evaluation function for the solver
def eval(functions, solver_length, physical_addresses):

  n_ignored_bits = 12       # we control the lower 12 bits
  n_mask_size    = 20       # we need around 19 bits to get the scrambling

  # throw away functions which have only bits below ignored bits as these never appear in the side channel
  n_functions_solver = len([f for f in functions if f >= (1<<n_ignored_bits)]) 

  # calculate actual hardware bank id
  def get_bank_id(padr):
    result = 0
    for i, f in enumerate(functions):
      result |= (bin(padr & f).count('1') % 2) << i
    return result

  # permute the bank id to an observable bank id as in the SC
  # ie we don't now the real index so we permute the actual bank id to the order we observe it
  def permute_bank_id(pattern):
    seen = {}
    count = 0
    perm =[]
    for p in pattern:
      if p not in seen:
        seen[p] = count
        count+=1
      perm.append(seen[p])
    return perm


  pattern = permute_bank_id([get_bank_id(padr) for padr in physical_addresses])

  # check if the physical addresses are contignious
  def get_ground_truth(pp):
    return np.all(np.diff(pp) == 0x1000)

  correct = 0
  overall_count = 0

  ground_truth = []
  prediction = []


  for i in range(0,pattern_length-solver_length,solver_length):

    truth = get_ground_truth(physical_addresses[i:i+solver_length])
    pred  = solve(pattern[i:i+solver_length], n_mask_size, n_ignored_bits, n_functions_solver)

    correct       += (truth == pred)
    overall_count += 1

    ground_truth.append(truth)
    prediction.append(pred)

    print(f"{i:4d} correct = {correct*100/overall_count:5.2f}% \r", end='', flush=True)

  _, _, bFscore, _ = pr(ground_truth, prediction, average='binary')
  return bFscore, overall_count



# the functions of different devices
functions = [
  [0x0000000000000400, 0x0000000000002000, 0x0000000000004000, 0x0000000000008000], #op5
  [0x0000000000000100, 0x0000000000011000, 0x0000000000022000, 0x0000000000044000, 0xc0000000], #chromebook
  [0x0000000000000200, 0x0000000000000400, 0x000000004e9d2000, 0x00000000d3a74000, 0x00000000a74e8000 ], #px3
  [0x0000000000892000, 0x0000000001124000, 0x0000000000449000, 0x000000000061c100, 0x0000000000c26600] #s9
]

names = ["op5", "chromebook", "px3", "s9"]


random.seed(0xdead)

max_physical   = 1 << 34
pattern_length = 5000

# generation function
def get_phys(max_cont_pages):
  physical_addresses = []

  while len(physical_addresses) < pattern_length:
    start = random.randrange(0,max_physical,0x1000)
    size  = random.randint(0, max_cont_pages * 2)
    end   = start + size * 0x1000

    physical_addresses.extend( list(range(start, end, 0x1000)) )
  return physical_addresses

# eval loop

print(f"    device;    slength;     fscore;       MBps")
      
for n, f in zip(names,functions):

  for solver_length in [8,16,24,48,64,96,128]:

    p = get_phys(solver_length)

    start_time = time.time()
    bFscore, iterations = eval(f, solver_length, p)
    end_time = time.time()

    dur = (end_time - start_time) / iterations

    MBps = (solver_length * 0x1000 / 1024**2) / dur 

    print(f"{n:>10}; {solver_length:>10}; {bFscore:>10.2}; {MBps:>10.2}")

