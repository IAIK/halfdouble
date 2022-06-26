import numpy as np

functions = [0x0000000000000100, 0x0000000000011000, 0x0000000000022000, 0x0000000000044000] #chromebook

# calculate actual hardware bank id
def get_bank_id(padr):
  result = 0
  for i, f in enumerate(functions):
    result |= (bin(padr & f).count('1') % 2) << i
  return result

patterns = {}

# walk over physical contiguous memory
for i in range(0x1000):
  padr = 0 + i * 0x1000

  bid = get_bank_id(padr)

  if bid not in patterns:
    patterns[bid] = []

  if len(patterns[bid]) <= 16:
    patterns[bid].append(i)

# print and calc diff
print("-"*70)
print("Bank | " +" ".join([f"d{x:<2d}" for x in range(16)]))
print("-"*70)

for bank, idxs in patterns.items():

  diff = np.diff(np.array(idxs))
  s = " ".join([f"{x:3d}" for x in diff])
  print(f"  {bank:2d} | {s}")

print("-"*70)