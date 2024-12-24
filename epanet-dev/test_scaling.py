#!/usr/bin/env python3

import os
import sys
import time
import subprocess
import statistics
from pathlib import Path
import csv

def run_test(executable: str, num_processes: int, num_runs: int = 3, h_max: int = 5, num_activations: int = 3) -> tuple[float, float]:
    """
    Run the MPI program multiple times and return the average execution time and standard deviation.
    
    Args:
        executable: Path to the executable
        num_processes: Number of MPI processes to use
        num_runs: Number of times to run the test
    
    Returns:
        tuple of (average_time, standard_deviation)
    """
    times = []
    
    for _ in range(num_runs):
        start_time = time.time()
        try:
            # Run MPI program and suppress output
            subprocess.run(
                ['mpirun', '-n', str(num_processes), str(executable), "--h_max", str(h_max), "--num_activations", str(num_activations)],
                check=True
            )
        except subprocess.CalledProcessError as e:
            print(f"Error running MPI program: {e}")
            sys.exit(1)
            
        execution_time = time.time() - start_time
        times.append(execution_time)
    
    return statistics.mean(times), statistics.stdev(times)

def main():
    # Configuration
    max_processes = 16
    num_runs = 3
    h_max = 12
    num_activations = 3
    
    # Setup paths
    build_dir = '/home/michael/gitrepos/rs_JESSICA/epanet-dev/release'
    executable = os.path.join(build_dir, 'run-epanet3')
    results_file = os.path.join(build_dir, 'mpi_scaling_results.csv')
    
    # Check if build directory exists
    if not os.path.exists(build_dir):
        print(f"Error: {build_dir} directory not found. Please build the project first.")
        sys.exit(1)
    
    # Change to build directory
    os.chdir(build_dir)
    
    # Check if executable exists
    if not os.path.exists(executable):
        print(f"Error: {executable} not found. Please build the project first.")
        sys.exit(1)
    
    # Prepare results file
    with open(results_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['Processes', 'Average Time (s)', 'Std Dev (s)', 'Speedup', 'Efficiency'])
    
    # Store the single process time for speedup calculations
    single_process_time = None
    
    # Run tests
    for n in range(1, max_processes + 1):
        print(f"\nRunning with {n} processes...")
        
        avg_time, std_dev = run_test(executable, n, num_runs, h_max, num_activations)
        
        # Store single process time for speedup calculations
        if n == 1:
            single_process_time = avg_time
        
        # Calculate speedup and efficiency
        speedup = single_process_time / avg_time if single_process_time else 0
        efficiency = speedup / n
        
        # Save results
        with open(results_file, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([n, f"{avg_time:.3f}", f"{std_dev:.3f}", f"{speedup:.3f}", f"{efficiency:.3f}"])
        
        print(f"Average time: {avg_time:.3f} seconds")
        print(f"Std Dev: {std_dev:.3f} seconds")
        print(f"Speedup: {speedup:.3f}x")
        print(f"Efficiency: {efficiency:.3f}")
        print("-" * 40)
    
    print(f"\nTesting complete! Results saved to {results_file}")

if __name__ == "__main__":
    main() 