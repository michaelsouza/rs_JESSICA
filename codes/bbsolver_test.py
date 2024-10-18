import unittest
import wntr
import pandas as pd
from codes.bbsolver import *

class TestBBSolver(unittest.TestCase):
    def test_sim(self):
        sim = wntr.sim.EpanetSimulator(WN)
        out = sim.run_sim()
        pressures = out.node["pressure"].to_dict()
        tank_levels = out.node["head"].to_dict()
        
        df = {'Type':[], 'Time':[], 'Node':[], 'Value':[]}
        df['Type'] = out.node["pressure"].index.to_list()
        df['Time'] = out.node["pressure"].index.to_list()
        df['Node'] = out.node["pressure"].columns.to_list()
        df['Value'] = out.node["pressure"].to_list()
        print(df)
        pass

