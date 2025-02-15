# His script isn't working for me for some reason

import os
import re
import sys
import fnmatch
import subprocess as sp
from pathlib import Path

testcase_dir = Path("testcases/")
if not testcase_dir.is_dir():
    print(f"Error: Can't find testcase dir at {testcase_dir}", file=sys.stderr)
    exit(1)

test_outs_all = [case for case in os.listdir(testcase_dir)]
test_outs = fnmatch.filter(test_outs_all, "*.out")

run_names_all = [case for case in os.listdir()]
run_names = fnmatch.filter(run_names_all, "test[0-9]*")

failed = set()
for i in range(len(test_outs)):
    result1 = sp.run(f"./{run_names[i]}".split(), stdout=sp.PIPE)
    result2 = sp.run(f"cat {testcase_dir.joinpath(test_outs[i])}".split(), stdout=sp.PIPE)

    for line1, line2 in zip(str(result1.stdout).split("\\n"), str(result2.stdout).split("\\r\\n")):
        if (line1 != line2):
            print(f"mine:\t\t{line1}", f"example:\t{line2}", sep="\n")
            failed.add(f"testcase {i}")
    
    # print(re.sub("\\\\n", "\n", str(diff.stdout)))

for case in sorted(failed):
    print(f"Failed: {case}")