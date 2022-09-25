# subface

A collection of triangle subdivision/tessellation/decimation implementations.

# Compiling

```
git clone https://github.com/chaosink/subface.git --recursive
cd subface
mkdir build
cd build
cmake ..
make
```

# Usage

```
./subface model_file.obj
```

* camera controlling

key | function
-|-
(`Shift` +) `W`/`S`/`A`/`D`/`Q`/`E` | (slowly) move forward / backward / left / right / down / up
(`Shift` +) `J`/`L`/`I`/`K` | (slowly) turn left / right / up / down
`-`/`=` | slow down / speed up movement
`[`/`]` | slow down / speed up turning
`Space` | reset camera and model
`mouse left button` | rotate model
`mouse right button` | change camera direction
`mouse scroll wheel` | adjust field of view

* rendering

key | function
-|-
`Ctrl` + `1`-`8` | choose processing methods from<br/>1.SubdivideSmooth<br/>2.SubdivideSmoothNoLimit<br/>3.SubdivideFlat (same as 4.Tessellate4)<br/>4.Tessellate4<br/>5.Tessellate4_1 (another 1-to-4 triangle tessellation pattern than 4.Tessellate4)<br/>6.Tessellate3<br/>7.Decimate<br/>8.DecimateSloppy 
`0`-`9` | processing level, `0` for the original mesh (default)
`Tab` | switch rendering mode: faces + wireframe (default), faces, wireframe
`N` | enable / disable (default) smooth rendering
`C` | enable (default) / disable face culling
`T` | enable (default) / disable transparent window
`O` | export the processed mesh as OBJ file

# Results

* rendering modes

<table>
	<!-- faces + wireframe, faces -->
	<tr align="center">
		<th>
			faces + wireframe
		</th>
		<th>
			faces
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-0_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-0_face.png"></img>
		</td>
	</tr>
	<!-- wireframe with face culling, wireframe without face culling -->
	<tr align="center">
		<th>
			wireframe with face culling
		</th>
		<th>
			wireframe without face culling
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-0_wireframe_cull-face.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-0_wireframe_no-cull-face.png"></img>
		</td>
	</tr>
</table>

* 1.SubdivideSmooth, flat vs smooth

<table>
	<!-- original mesh -->
	<tr align="center">
		<th>
			original mesh, flat
		</th>
		<th>
			original mesh, smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-0_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-0_smooth.png"></img>
		</td>
	</tr>
	<!-- level 1 -->
	<tr align="center">
		<th>
			level 1, flat
		</th>
		<th>
			level 1, smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-1_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-1_smooth.png"></img>
		</td>
	</tr>
	<!-- level 2 -->
	<tr align="center">
		<th>
			level 2, flat
		</th>
		<th>
			level 2, smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-2_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-2_smooth.png"></img>
		</td>
	</tr>
	<!-- level 3 -->
	<tr align="center">
		<th>
			level 3, flat
		</th>
		<th>
			level 3, smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-3_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-3_smooth.png"></img>
		</td>
	</tr>
	<!-- level 4 -->
	<tr align="center">
		<th>
			level 4, flat
		</th>
		<th>
			level 4, smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="./result/suzanne_level-4_flat.png"></img>
		</td>
		<td>
			<img src="./result/suzanne_level-4_smooth.png"></img>
		</td>
	</tr>
</table>
