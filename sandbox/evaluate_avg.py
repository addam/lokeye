#!/usr/bin/python3
import pandas as pd
import numpy as np
from itertools import groupby, count
from wand.image import Image
from itertools import count

def centercrop(image, x, y, r):
	m = min(3*r, x, y, image.width - x, image.height - y)
	result = image.clone()
	result.crop(left=int(x-m), top=int(y-m), right=int(x+m), bottom=int(y+m))
	return result

truth = dict()
for line in open("../data/ground_truth.txt"):
	filename, *values = line.split()
	for i in range(0, len(values), 3):
		x, y, r = (float(v) for v in values[i:i+3])
		truth[filename, "LR"[i//3]] = x, y, r

df = pd.read_csv("evaluation.csv")
#df["r"] = (df["x"]**2 + df["y"]**2)**0.5
df.sort_values(["file", "side", "distance"], inplace=True)
print("""<script>
function recalculate(algo, limit)
{
	var good = 0, bad = 0;
	for (table of document.getElementsByTagName("table")) {
		table.className = "";
		for (row of table.rows) {
			if (row.cells[0].textContent === algo) {
				if (Number(row.cells[1].textContent) < limit) {
					table.className =  "good";
					good += 1;
				} else {
					table.className =  "bad";
					bad += 1;
				}
			}
		}
	}
	document.getElementById("counts").textContent = good + " / " + (good + bad);
}
</script>""")
print("""<style>
img {float: left;}
table tr td { border: 1px solid grey; padding: 1px 3px;}
table {float:left; clear: left; border-collapse: collapse}
.good {background-color: lightgreen;}
.bad {background-color: pink;}
</style>""")
d = pd.pivot_table(df, index=("algorithm"), values=df.columns[-1:])
print(d.sort_values("distance").to_html())
print("<hr style='clear:left;'>")
print("""<input id="limit" value=1>
<select name="algo" onchange="recalculate(this.value, Number(document.getElementById('limit').value))">""")
for algo in d.index.values:
	print("<option name='{0}'>{0}</option>".format(algo))
print("""
</select>
<br>
<span id="counts">0/0</span>
<br>""")

index = count()
for (filename, side), values in groupby(df.values, lambda x: tuple(x[1:3])):
	image = Image(filename="../data/"+filename)
	tmp_filename = "cropped/{}.jpg".format(next(index))
	centercrop(image, *truth[filename, side]).save(filename="../data/"+tmp_filename)
	print(pd.DataFrame(np.vstack(values), columns="algorithm file side distance".split()).sort_values("distance").to_html(columns="algorithm distance".split(), index=False))
	print("<img src='{}' style='float:left'>".format(tmp_filename))

index = count()
