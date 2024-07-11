import numpy as np
import threading
from queue import PriorityQueue
from typing import Tuple


class BBTask:
    """
    A class used to represent a Branch-and-Bound Task.

    Attributes:
        root_level (int): The root level, the task focuses on levels starting from root_level.
        values (np.array): The values of each level.
        max_values (np.array): The maximum values of each level.
        length (int): The length of the values array.
        level (int): The current level being processed.

    Methods:
        backtracking() -> int:
            Executes the backtracking algorithm and returns the current level.

        split_task(min_deepth:int=0) -> "BBTask":
            Splits the task into two sub-tasks.

        next_node(is_feasible: bool) -> bool:
            Moves to the next node if feasible, otherwise backtracks.
    """

    def __init__(self, root_level: int, values: np.array, max_values: np.array) -> None:
        """
        Constructs all the necessary attributes for the BBTask object.

        Args:
            root_level (int): The root level, the task focuses on levels starting from root_level.
            values (np.array): The values of each level.
            max_values (np.array): The maximum values of each level.
        """
        self.root_level = root_level
        self.values = values
        self.max_values = max_values
        self.length = len(values)
        self.level = root_level

    def __lt__(self, other: "BBTask") -> bool:
        """
        Compares two BBTask objects based on their workload.

        Args:
            other (BBTask): The other BBTask object to be compared.

        Returns:
            bool: True if the current task has a smaller workload than the other task, False otherwise.
        """
        return self.workload() < other.workload()

    def backtracking(self) -> int:
        """
        Executes the backtracking algorithm.

        The algorithm:
        1. Finds the first level that has not reached the maximum value.
        2. Increments the value of the current level by 1 during backtracking.
        3. Resets the level after leaving it.

        Returns:
            int: The current level after backtracking or -1 if the root level is reached.
        """
        while self.level >= self.root_level:
            if self.values[self.level] < self.max_values[self.level]:
                self.values[self.level] += 1
                return self.level
            self.values[self.level] = 0
            self.level -= 1
        return -1

    def split_task(self, min_deepth: int = 0) -> "BBTask":
        """
        Splits the task into two sub-tasks.

        The algorithm:
        1. Finds the first level that has not reached the maximum value and splits the task.
        2. The root of the sub-task is the split level.
        3. Changes the maximum value of the split level to the current value of the split level.
        4. The value of the split level is the rounded half of the available values.

        Args:
            min_deepth (int, optional): Used to avoid splitting too small tasks. Defaults to 0.

        Returns:
            BBTask: A new BBTask object representing the sub-task or None if no split is possible.
        """
        level = self.root_level
        while level < (self.length - min_deepth):
            if self.values[level] < self.max_values[level]:
                values = np.copy(self.values)
                max_values = np.copy(self.max_values)
                value = np.max(
                    [int((values[level] + max_values[level]) / 2), values[level] + 1]
                )
                values[level] = int(value)
                self.max_values[level] = values[level] - 1
                return BBTask(level, values, max_values)
            level += 1
        return None

    def next_node(self, is_feasible: bool) -> bool:
        """
        Moves to the next node if feasible, otherwise backtracks.

        Args:
            is_feasible (bool): Indicates whether the current node is feasible.

        Returns:
            bool: True if moved to the next node, False if backtracking.
        """
        if is_feasible and self.level < self.length:
            self.level += 1
            self.values[self.level] = 0
            return True

        self.level = self.backtracking()
        return self.level > 0

    def workload(self):
        """
        Returns the workload of the task.

        Returns:
            int: The workload of the task.
        """

        level = self.root_level
        while level < self.length:
            if self.values[level] < self.max_values[level]:
                break
        return level - self.root_level


def constraints(level:int, values: np.array) -> bool:
    """
    Checks if the current values are feasible.

    Returns:
        bool: True if the current values are feasible, False otherwise.
    """
    return np.sum(values[:(level + 1)]) > 3


def cost_function(level:int, values: np.array) -> int:
    """
    Calculates the cost function of the values.

    Args:
        values (np.array): The values to be evaluated.

    Returns:
        int: The cost function of the values.
    """
    return np.sum(values[:(level + 1)])


class BBWorker(threading.Thread):
    """
    A class used to represent a Branch-and-Bound Worker.

    Attributes:
        id (int): The identifier for the worker.
        manager (BBManager): The manager overseeing this worker.
        task (BBTask): The current task being processed by the worker.

    Methods:
        process_task(task: BBTask) -> None:
            Processes the given task using the branch-and-bound algorithm.

        is_feasible() -> bool:
            Determines if the current node is feasible based on problem-specific constraints.
    """

    def __init__(self, manager: "BBManager", id: int, task:BBTask) -> None:
        """
        Constructs all the necessary attributes for the BBWorker object

        Args:
            manager (BBManager): The manager overseeing this worker.
            id (int): The identifier for the worker.
            task (BBTask): The current task being processed by the worker.
        """
        threading.Thread.__init__(self)
        self.manager = manager
        self.task = task
        self.id = id        
        self.condition = manager.condition # Condition variable

    
    def start(self) -> None:
        """
        Runs the worker thread.

        The algorithm:
        1. Waits for a signal from the manager.
        2. Processes the task using the branch-and-bound algorithm.
        3. Notifies the manager when the task is completed.        
        """
        is_feasible = constraints(self.task.level, self.task.values)
        while self.task.next_node(is_feasible):            

            is_feasible = constraints(self.task.level, self.task.values)
            if not is_feasible:  # prune the branch
                continue

            cost = cost_function(self.task.level, self.task.values)
            if cost > self.manager.upper_bound:  # prune the branch
                continue

            if self.task.level == (self.task.length - 1):  # leaf node
                self.manager.append_solution(cost, self.task.values)

        self.manager.task_done(self.id)
        self.task = None


class BBManager:
    """
    A class used to manage Branch-and-Bound Workers and aggregate results.

    Attributes:
        num_workers (int): The number of workers to manage.
        upper_bound (int): The upper bound for the cost function.
        solutions (List[Tuple[int, np.array]]): The list of solutions found by the workers.
        task_queue (PriorityQueue): The queue of tasks to be processed.
        lock (threading.Lock): The lock used to synchronize access to the solutions list.
        workers (List[BBWorker]): The list of workers managed by the manager.

    Methods:
        task_done(worker_id: int, task: BBTask) -> None:
            Notifies the manager that a task has been completed by a worker.

        append_solution(cost: int, values: np.array) -> None:
            Appends a solution to the list of solutions and updates the upper bound.

        add_task(task: BBTask) -> None:
            Adds a task to the task queue.
    """

    def __init__(self, num_workers: int, task:BBTask, upper_bound: int=float("inf")) -> None:
        """
        Constructs all the necessary attributes for the BBManager object.

        Args:
            num_workers (int): The number of workers to manage.
            upper_bound (int): The upper bound for the cost function.
        """
        self.num_workers = num_workers
        self.upper_bound = upper_bound
        self.solutions = []
        self.workers = {}
        self.lock = threading.Lock()
        self.condition = threading.Condition(self.lock) # Condition variable
        tasks = self.create_tasks(task)

        for i in range(num_workers):
            worker = BBWorker(self, i, tasks[i])
            self.workers[i] = worker
            worker.start()

    def create_tasks(self, task: BBTask) -> None:
        """
        Splits the task into smaller tasks and adds them to the task queue.

        Args:
            task (BBTask): The task to be split and added to the queue.
        """
        tasks = [task]
        while len(tasks) < self.num_workers:
            largest_task = max(tasks, key=lambda x: x.workload())
            sub_task = largest_task.split_task()
            if sub_task is not None:
                tasks.append(sub_task)
        return tasks

    def append_solution(self, cost: int, values: np.array) -> None:
        """
        Appends a solution to the list of solutions.

        Args:
            cost (int): The cost of the solution.
            values (np.array): The values of the solution.
        """
        with self.lock:
            if cost < self.upper_bound:
                self.upper_bound = cost
                self.solutions = []
            self.solutions.append(np.copy(values))            

    def task_done(self, worker_id: int, task: BBTask) -> None:
        """
        Notifies the manager that a task has been completed by a worker.

        Args:
            worker_id (int): The identifier of the worker.
            task (BBTask): The task that has been completed.
        """
        # drop worker
        self.workers.pop(worker_id)

        with self.lock:
            largest_task, largest_workload = None, 0
            for worker in self.workers:
                if worker.task is None:
                    continue

                if worker.task.workload() > largest_workload:
                    largest_workload = worker.task.workload()
                    largest_task = worker.task

            if largest_task is None:
                return
            
            sub_task = largest_task.split_task()
            if sub_task is None:
                return
                        
            self.workers[worker_id] = BBWorker(self, worker_id, sub_task)
            self.workers[worker_id].start()
