import argparse
import numpy as np
import glob

def write_line(f, label, pmp, i):
    f.write(f" {label}")
    for j in range(i, i+6):
        f.write(f"           \t{pmp[j]}")
    f.write("\n")

def write_pmp(f, label, pmp):
    write_line(f, label, pmp, 0)
    write_line(f, label, pmp, 6)
    write_line(f, label, pmp, 12)
    write_line(f, label, pmp, 18)


def parse_x(x, suffix):
    x = x.split("=")[1].replace("{", "[").replace("}", "]")
    x = np.array(eval(x))[1:, :]

    fn = "/home/michael/github/rs_JESSICA/networks/any-town.inp"
    with open(fn, "r") as f:
        lines = f.readlines()
        pmp_lines = {"PMP111": [], "PMP222": [], "PMP333": []}
        for i, line in enumerate(lines):
            if line.strip().startswith("PMP111"):
                pmp_lines["PMP111"].append(i)
            elif line.strip().startswith("PMP222"):
                pmp_lines["PMP222"].append(i)
            elif line.strip().startswith("PMP333"):
                pmp_lines["PMP333"].append(i)
        print(pmp_lines)

    fn = fn.replace(".inp", f"_parsed_{suffix}.inp")
    done = {key: False for key in pmp_lines.keys()}
    with open(fn, "w") as f:
        for i, line in enumerate(lines):
            if i in pmp_lines["PMP111"]:
                pmp = x[:, 0]
                label = "PMP111"
                if not done["PMP111"]:
                    write_pmp(f, label, pmp)
                    done["PMP111"] = True
            elif i in pmp_lines["PMP222"]:
                pmp = x[:, 1]
                label = "PMP222"
                if not done["PMP222"]:
                    write_pmp(f, label, pmp)
                    done["PMP222"] = True
            elif i in pmp_lines["PMP333"]:
                pmp = x[:, 2]
                label = "PMP333"
                if not done["PMP333"]:
                    write_pmp(f, label, pmp)
                    done["PMP333"] = True
            else:
                f.write(f"{line}")

if __name__ == "__main__":
    set_y = set()
    for fn in glob.glob("/home/michael/github/rs_JESSICA/epanet-dev/build/release/bin/logger_RANK_*.log"):
        rank = fn.split("RANK_")[-1].replace(".log", "")
        with open(fn, "r") as f:
            content = f.read() 
        
        count = 0
        for row in content.split('\n'):
            # create input file
            if f'Rank[{rank}]: x = {{' in row:
                count += 1
                parse_x(row, f"rank_{rank}_{count:03d}")
            
            # check if y is new
            if f'Rank[{rank}]: y = ' in row:
                y = row.split("= ")[1]
                if y not in set_y:
                    set_y.add(y)
                else:
                    raise ValueError(f"Duplicate y: {y}")
        
