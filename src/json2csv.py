#!/usr/bin/python3
from sys import stdin
import json

commands = stdin.read()
begin, end = commands.rfind("[["), commands.rfind("]]") + 2;
data = json.loads(commands[begin:end])
data.sort()
for row in data:
    print(",".join(str(val) for val in row))
    
