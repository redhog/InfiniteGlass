import InfiniteGlass
import json
import yaml
import uuid
import os
import sqlite3
import sys
import pkg_resources
import array
from . import manager

def main():
    with InfiniteGlass.Display() as display:
        island_manager = manager.IslandManager(display)
            
