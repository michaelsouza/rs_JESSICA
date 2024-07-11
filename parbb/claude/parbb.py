import multiprocessing as mp
from queue import PriorityQueue
from dataclasses import dataclass, field
from typing import Any, List, Callable
import random
import time


@dataclass(order=True)
class Node:
    # Class attribute annotations
    priority: float  # Indicates the priority of the node, used for sorting/comparison
    item: Any = field(
        compare=False
    )  # Holds the data of the node. `field(compare=False)` means this field is excluded from comparison operations

    def __init__(self, priority: float, data: Any):
        # Constructor for the Node class
        self.priority = priority  # Sets the priority of the node
        self.item = {
            "data": data,
            "fitness": priority,
        }  # Initializes the item with data and priority. Here, priority is referred to as 'fitness'.


def worker(
    task_queue: mp.Queue,
    result_queue: mp.Queue,
    branch_func: Callable,
    prune_func: Callable,
    best_cost: mp.Value,
):
    """
    Worker function for parallel branch and bound algorithm.

    This function continuously processes nodes from a task queue, applies branching and pruning,
    and sends the results to a result queue. It updates a shared best_cost value when a better
    solution is found.

    Parameters:
    - task_queue (mp.Queue): Queue from which the worker fetches nodes to process.
    - result_queue (mp.Queue): Queue where the worker puts processed nodes.
    - branch_func (Callable): Function to generate new nodes from a given node.
    - prune_func (Callable): Function to decide whether to prune a node based on its cost.
    - best_cost (mp.Value): Shared object to store the cost of the best solution found so far.

    The worker stops processing when it receives a None object from the task queue.
    """

    while True:
        node = task_queue.get()  # Fetch a node from the task queue
        if node is None:  # Check if it's the signal to stop processing
            break

        # Process the node using the branching function to generate new nodes
        new_nodes = branch_func(node.item["data"])

        # Iterate over the generated nodes
        for new_node_data in new_nodes:
            # Use the pruning function to decide whether to keep this new node
            if not prune_func(new_node_data, best_cost):
                # Calculate the fitness (or cost) of the new node
                fitness = calculate_fitness(new_node_data)
                # Put the new node into the result queue with its fitness as priority
                result_queue.put(Node(priority=fitness, data=new_node_data))

                # If the new node represents a complete solution, possibly update the best cost
                if (
                    new_node_data.depth == 10
                ):  # Assuming depth 10 indicates a complete solution
                    with best_cost.get_lock():  # Ensure thread-safe update of best_cost
                        # Update best_cost if the new node's cost is lower
                        best_cost.value = min(best_cost.value, new_node_data.cost)

    result_queue.put(None)  # Signal that this worker has finished processing


def parallel_branch_and_prune(
    initial_nodes: List[Any],
    branch_func: Callable,
    prune_func: Callable,
    num_workers: int = 4,
    max_iterations: int = 1000,
):
    """
    Implements a parallel branch and prune algorithm to solve optimization problems.

    This function initializes worker processes that concurrently process nodes from a task queue,
    applying a branching function to generate new nodes and a pruning function to discard
    non-promising nodes. The best solution found during the process is returned.

    Parameters:
    - initial_nodes (List[Any]): List of initial nodes to start the algorithm.
    - branch_func (Callable): Function that takes a node and returns a list of new nodes.
    - prune_func (Callable): Function that decides whether a node should be pruned.
    - num_workers (int): Number of parallel worker processes to use.
    - max_iterations (int): Maximum number of iterations to perform.

    Returns:
    - The best node found during the process, or None if no solution is found.
    """

    # Initialize multiprocessing queues for tasks and results
    task_queue = mp.Queue() # Queue for storing nodes to be processed
    result_queue = mp.Queue() # Queue for storing results (processed nodes) from workers

    # Initialize shared (d: double) memory value for tracking the best cost found across workers
    best_cost = mp.Value("d", float("inf"))

    # Initialize a priority queue with the initial nodes, prioritized by their fitness
    pq = PriorityQueue()
    for node_data in initial_nodes:
        fitness = calculate_fitness(
            node_data
        )  # Calculate fitness for each initial node
        pq.put(Node(priority=fitness, data=node_data))  # Add node to the priority queue

    # Start worker processes
    processes = []
    for _ in range(num_workers):
        # Each worker process will execute the 'worker' function
        p = mp.Process(
            target=worker,
            args=(task_queue, result_queue, branch_func, prune_func, best_cost),
        )
        p.start()
        processes.append(p)

    iteration = 0  # Track the number of iterations
    workers_done = 0  # Track the number of workers that have completed processing
    while iteration < max_iterations and workers_done < num_workers:
        # Distribute work from the priority queue to the task queue for workers to process
        while not pq.empty():
            task_queue.put(pq.get())

        # Collect results from workers and add them back to the priority queue
        result = result_queue.get()
        if result is None:  # A None result indicates a worker has finished processing
            workers_done += 1
        else:
            pq.put(
                result
            )  # Add the result back to the priority queue for further processing

        iteration += 1  # Increment the iteration count

    # Signal workers to terminate by sending a None value for each worker
    for _ in range(num_workers):
        task_queue.put(None)

    # Wait for all worker processes to complete
    for p in processes:
        p.join()

    # Return the best node found, if any
    return pq.get().item if not pq.empty() else None


class TreeNode:
    def __init__(self, value, depth, parent_cost=0):
        """
        Initializes a new instance of the TreeNode class.

        Parameters:
        - value: The value associated with this node. This can be any data type depending on the use case.
        - depth: The depth of the node in the tree. Root node starts at depth 0, and depth increases by 1 down the tree.
        - parent_cost (optional): The cost associated with the parent node. This is used to calculate the node's total cost.
          Default is 0, which is typically used for the root node that has no parent.

        Attributes:
        - value: Stores the value passed to the node.
        - depth: Stores the depth of the node in the tree.
        - cost: The total cost of reaching this node from the root. It is calculated as the parent's cost plus a random value between 0 and 10.
          This cost can represent, for example, the cumulative path cost in a pathfinding algorithm.
        - left: Pointer to the left child of the node. Initialized to None.
        - right: Pointer to the right child of the node. Initialized to None.
        """
        self.value = value  # Assign the value passed to the node
        self.depth = depth  # Assign the depth of the node in the tree
        self.cost = parent_cost + random.uniform(
            0, 10
        )  # Calculate and assign the node's cost
        self.left = None  # Initialize the left child to None
        self.right = None  # Initialize the right child to None


def calculate_fitness(node: TreeNode) -> float:
    """
    Calculates the fitness of a given TreeNode.

    The fitness is calculated as the negative of the node's cost. This is because, in many optimization problems,
    a lower cost is preferred, so by negating the cost, a higher fitness value indicates a more desirable node.

    Parameters:
    - node (TreeNode): The TreeNode instance for which to calculate fitness.

    Returns:
    - float: The calculated fitness value for the node.
    """
    return -node.cost  # Return the negated cost as the fitness value


def example_branch_func(node: TreeNode) -> List[TreeNode]:
    """
    Example branching function for generating new TreeNodes from a given node.

    This function simulates a branching process where each node can generate two new nodes until a maximum depth of 10 is reached.
    The value of the new nodes is determined by doubling the parent node's value and optionally adding one, to create a binary tree-like structure.
    The function also simulates processing time by sleeping for a random duration between 0.1 and 0.5 seconds.

    Parameters:
    - node (TreeNode): The parent node from which new nodes are to be generated.

    Returns:
    - List[TreeNode]: A list of generated TreeNode instances. The list contains two new nodes if the parent node's depth is less than 10,
      otherwise, it returns an empty list.
    """
    # Simulate random processing time
    time.sleep(random.uniform(0.1, 0.5))

    new_nodes = []  # Initialize an empty list to hold the generated nodes
    if (
        node.depth < 10
    ):  # Check if the current node's depth is less than the maximum allowed depth
        # Generate and append the first new node with doubled value
        new_nodes.append(TreeNode(node.value * 2, node.depth + 1, node.cost))
        # Generate and append the second new node with doubled value plus one
        new_nodes.append(TreeNode(node.value * 2 + 1, node.depth + 1, node.cost))
    return new_nodes  # Return the list of generated nodes


def example_prune_func(node: TreeNode, best_cost: mp.Value) -> bool:
    """
    Example pruning function to decide whether a given TreeNode should be pruned.

    This function checks if the cost associated with the TreeNode is greater than or equal to the current best cost
    found for a complete solution. If so, the node is considered not promising for leading to a better solution and is pruned.

    Parameters:
    - node (TreeNode): The TreeNode instance to evaluate for pruning.
    - best_cost (mp.Value): A multiprocessing Value object that stores the cost of the best solution found so far.
      It is shared among multiple processes and provides a thread-safe way to update and access the best cost.

    Returns:
    - bool: Returns True if the node should be pruned (i.e., its cost is not better than the best cost), otherwise False.

    Note:
    - The function uses a lock on the best_cost value to ensure thread-safe read and update operations,
      preventing race conditions in a multiprocessing environment.
    """
    with best_cost.get_lock():  # Acquire a lock on the best_cost value to ensure thread-safe access
        # Prune if the node's cost is already worse than the best complete solution
        if node.cost >= best_cost.value:
            return True  # Indicate that the node should be pruned
    # also prune if the depth + cost is even
    if (node.depth + node.cost) % 2 == 0: # simulating feasibility
        return True
    # If the function hasn't returned True, it means the node's cost is less than the best cost, so return False
    return False  # Indicate that the node should not be pruned


if __name__ == "__main__":
    # Initialize the root of the tree with a value of 1 and at depth 0
    initial_node = TreeNode(1, 0)

    # Execute the parallel branch-and-prune algorithm starting from the initial node
    # The algorithm uses the example_branch_func for branching and example_prune_func for pruning
    result = parallel_branch_and_prune(
        [initial_node], example_branch_func, example_prune_func
    )

    # Check if a solution (best node) was found by the algorithm
    if result:
        # If a solution is found, print the value, depth, and cost of the best node
        print(
            f"Best node: Value = {result['data'].value}, Depth = {result['data'].depth}, Cost = {result['data'].cost}"
        )
    else:
        # If no solution is found, print a message indicating that
        print("No solution found.")

    # Print the cost and depth of the final (best) node for additional insights
    print(f"\nFinal cost: {result['data'].cost}")
    print(f"Final depth: {result['data'].depth}")

    # Reconstruct and print the path from the root node to the best node
    # This is done by tracing back from the best node to the root by calculating the parent's value
    path = []  # Initialize an empty list to store the path
    node = result["data"]  # Start from the best node
    while node.value > 1:  # Loop until the root node is reached
        path.append(node.value)  # Add the current node's value to the path
        # Calculate the parent node's value and create a new TreeNode for it
        # This assumes a specific branching strategy used in example_branch_func
        node = TreeNode((node.value - 1) // 2, node.depth - 1)
    path.append(1)  # Add the root node's value to the path
    path.reverse()  # Reverse the path to start from the root node

    # Print the reconstructed path to the best node
    print("\nPath to best node:")
    print(" -> ".join(map(str, path)))
