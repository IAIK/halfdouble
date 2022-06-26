from z3 import *


def solve(pattern, n_mask_size, n_ignored_bits, n_functions):
  s = SolverFor("QF_BV")

  # start offset of the cont range
  offset = BitVec("offset", n_mask_size - n_ignored_bits)
  # mask functions
  masks  = [ BitVec(f"mask_{i}", n_mask_size - n_ignored_bits)  for i in range(n_functions) ]

  # xor all bits ... ie hw(x) & 1
  def xor_bits(x):
    sum = BitVecVal(0, 1)
    for i in range(x.size()):
      sum = sum ^ Extract(i, i, x)
    return sum

  # get bank id in z3 expr
  def get_bank_z3(page):
    # the padr is the offset + index ()
    padr = offset + BitVecVal(page, n_mask_size - n_ignored_bits)
    result = BitVecVal(0, n_functions)
    for i, m in enumerate(masks):
      masked = m & padr
      result = result | (ZeroExt(n_functions-1, xor_bits(masked) ) << i)
    return result

  # each sets first entry
  first_banks = []

  # go over the bank sets
  for bank_id in range(max(pattern)):
    banks = []

    # find ids from the set do this this way as z3 does not like numpy :(
    for i, bid in enumerate(pattern):
      if bid != bank_id:
        continue
      banks.append(get_bank_z3(i))

    if len(banks) == 0:
      continue

    first_banks.append(banks[0])

    # enforce that all the banks with the same ids are in the same bank
    for b in banks[1:]:
      s.add(b == banks[0])

  # enforce that non set members become different ids
  for i in range(len(first_banks)):
    for j in range(i+1,len(first_banks)):
      s.add(first_banks[i] != first_banks[j])


  #with open("out.smt", "w") as f:
  #  f.write("(set-logic QF_BV)\n")
  #  f.write(s.sexpr())
  #  f.write("(check-sat)")


  # verify if solvable
  return s.check() == sat

