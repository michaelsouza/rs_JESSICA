import json

# File paths for the JSON snapshots
file_path1 = '/home/michael/gitrepos/rs_JESSICA/epanet-dev/debug/cost1/snapshot_24_0.json'
file_path2 = '/home/michael/gitrepos/rs_JESSICA/epanet-dev/debug/cost2/snapshot_24_3.json'

# Recursive function to compare JSON objects
def compare_json(obj1, obj2, element=''):
    for k in obj1:
        element_k = element + f".{k}"
        # print(element_k)
        v1 = obj1[k]
        v2 = obj2[k]
        if isinstance(v1, dict):
            compare_json(v1, v2, element_k)
        elif isinstance(v1, list):
            if len(v1) != len(v2):
                print(element_k)
                continue
            is_equal = True
            for i in range(len(v1)):
                if isinstance(v1[i], dict):
                    compare_json(v1[i], v2[i], element_k + f"[{i}]")
                    continue
                if v1[i] != v2[i]:
                    is_equal = False
                    break
            if not is_equal:
                print(element)
                continue
        else:
            if v1 != v2:
                print(element)

# Load JSON data
with open(file_path1, "r") as file1:
    print(f"file1 {file_path1}")
    json1 = json.load(file1)
    
with open(file_path2, "r") as file2:
    print(f"file2 {file_path2}")
    json2 = json.load(file2)

# Compare the JSON objects
compare_json(json1, json2)
