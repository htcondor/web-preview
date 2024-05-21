#!/usr/bin/python

import sys

if len(sys.argv) is not 3:
    print("ERROR: Input and/or output filenames not provided. Aborting.")
    print("Syntax: add-dagman-join-nodes.py <input-filename> <output-filename>")
    exit(1)

dag_input_filename = str(sys.argv[1])
dag_output_filename = str(sys.argv[2])

try:
    with open(dag_input_filename) as file:
        dag_input = file.readlines()
except IOError:
    print("ERROR: Could not open input file " + dag_input_filename + ", aborting.")
    exit(1)

total_edges = 0
optimal_edges = 0
join_nodes = 0
dag_output = ""

for line in dag_input:
    if "PARENT" in line:
        parent_nodes = line[0:line.index("CHILD")-1]
        child_nodes = line[line.index("CHILD"):len(line)]
        num_parents = parent_nodes.count(" ")
        num_children = child_nodes.count(" ")

        total_edges += num_parents * num_children

        if num_parents > 1 and num_children > 1:
            join_nodes = join_nodes + 1
            join_node_name = "_condor_join_node_" + str(join_nodes)
            dag_output += "JOB " + join_node_name + " noop.sub NOOP\n"
            dag_output += parent_nodes + " CHILD " + join_node_name + "\n"
            dag_output += "PARENT " + join_node_name + " " + child_nodes
            optimal_edges += num_parents + num_children
        else:
            dag_output += line
            optimal_edges += num_parents * num_children
    else:
        dag_output += line

output_file = open(dag_output_filename, "w")
output_file.write(dag_output)
output_file.close()


print("The original DAG (" + dag_input_filename + ") has " + format(total_edges, ',') + " edges.")
print("The optimized output DAG (" + dag_output_filename + ") has " + format(optimal_edges, ',') + " edges.")


