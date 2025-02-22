#! /usr/bin/python3

# His script isn't working for me for some reason

import os
import sys
import fnmatch
import argparse
import subprocess as sp
from pathlib import Path

class ArgsType(argparse.Namespace):
    just_out: bool | None
    testcase: str | None
    make_range: list[int] | None
    ignore_blocked: bool | None

def is_range(val: str):
    if val.isnumeric():
        return list(range(int(val), int(val)+1))
    try:
        start, finish = val.split("-")
        return list(range(int(start), int(finish)+1))
    except Exception:
        argparse.ArgumentTypeError(f"{val} is not a of the form [start: int]-[end: int]")

parser = argparse.ArgumentParser()
parser.add_argument("--just_out", action="store_true", help="Prints the output of the testcases run.")
parser.add_argument("-t", "--testcase", type=str, help="Only runs given testcase.")
parser.add_argument("-n", "--make_range", type=is_range, help="Makes only the testcases in the range given by [start]-[finish] inclusive.")
parser.add_argument("--ignore_blocked", action="store_true", help="Doesn't count failed test when Blocked(...) doesn't match.")
flags: ArgsType = parser.parse_args()

sp.run(["make", "clean"])

if flags.make_range:
    make_list = ""
    for n in flags.make_range:
        make_list += (f"test{n:02} ")
    make_list += "\n"

    with open("Makefile", "r") as f:
        make_data = f.readlines()
    for i in range(len(make_data)):
        line = make_data[i].split()
        if line != [] and line[0] == "TESTS":
            make_data[i] = f"TESTS = {make_list}"
    with open("Makefile", "w") as f:
        for line in make_data:
            f.write(line)

sp.run(["make"])

run_names_all = [case for case in os.listdir()]
if flags.testcase:
    filt = flags.testcase
else:
    filt = "test[0-9][0-9]"
run_names = fnmatch.filter(run_names_all, filt)

if flags.just_out:
    for i in range(len(run_names)):
        result1 = sp.run(f"./{run_names[i]}".split())

else:
    testcase_dir = Path("testcases/")
    if not testcase_dir.is_dir():
        print(f"Error: Can't find testcase dir at {testcase_dir}", file=sys.stderr)
        exit(1)

    failed = set()
    for run_name in run_names:
        out_name = f"{run_name}.out"
        result1 = sp.run(f"./{run_name}".split(), stdout=sp.PIPE)
        result2 = sp.run(f"cat {testcase_dir.joinpath(out_name)}".split(), stdout=sp.PIPE)

        for line1, line2 in zip(str(result1.stdout).split("\\n"), str(result2.stdout).split("\\n")):
            if flags.ignore_blocked:
                if "Blocked(" in line2:
                    line2 = line2.split("(")[0]

            if (line1 != line2):
                print(f"{run_name}:")
                print(f"mine:\t\t{line1}", f"example:\t{line2}", sep="\n", end="\n\n")
                failed.add(f"{run_name}")
        
    all_passed = True
    for case in sorted(failed):
        all_passed = False
        print(f"Failed: {case}")

    if all_passed:
        print("All testcases passed!")