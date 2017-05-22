#!/usr/bin/python3
from sys import argv, stdout
from wand.image import Image
from subprocess import run, PIPE
import json

def centercrop(image, x, y, r):
	m = min(3*r, x, y, image.width - x, image.height - y)
	result = image.clone()
	left, top = int(x-m), int(y-m)
	args = {"left": int(x-m), "top": int(y-m), "right": int(x+m), "bottom": int(y+m)}
	result.crop(**args)
	return result, args["left"], args["top"]

sandbox = "eye_corr eye_gradcorr eye_hough eye_hough2 eye_hough3 eye_iris eye_limbus eye_limbus2 eye_grad_newton".split()
data = list()
for line in open("../data/ground_truth.txt"):
	filename, *values = line.split()
	image = Image(filename="../data/"+filename)
	for i in range(0, len(values), 3):
		true_x, true_y, true_r = (float(v) for v in values[i:i+3])
		sub, offset_x, offset_y = centercrop(image, true_x, true_y, true_r)
		tmp_file = "tmp_cropped.png"
		sub.save(filename=tmp_file)
		block = dict()
		block["truth"] = (true_x, true_y)
		for program in sandbox:
			res = run(["./"+program, tmp_file, "-q", str(true_r)], stdout=PIPE)
			if res.returncode == 0:
				x, y = (float(v) for v in res.stdout.decode().split())
				block[program] = (x + offset_x, y + offset_y)
		data.append({"key": (filename, "LR"[i//3]), "values": block})

with open("evaluation.js", "w") as f:
	f.write("var measurements = ")
	json.dump(data, f)

