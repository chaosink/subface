# subface

A collection of triangle mesh subdivision/tessellation/decimation implementations.

# Compiling

```
git clone https://github.com/chaosink/subface.git --recursive
cd subface
mkdir build
cd build
cmake ..
cmake --build . --config Debug # Or Release
```

# Usage

* Command line

```
Usage: subface [-h] [--cmd] [--export_obj] [--save_png] [--smooth] [--fix_camera] [--cull] [--transparent] [--render VAR] [--method VAR] [--level VAR] OBJ_file_path

Process geometries with one of the following methods:
    1.LoopSubdivideSmooth
    2.LoopSubdivideSmoothNoLimit
    3.LoopSubdivideFlat
    4.Tessellate4
    5.Tessellate4_1
    6.Tessellate3
    7.Decimate_ShortestEdge_V0
    8.Decimate_ShortestEdge_Midpoint
    9.MeshoptDecimate
    10.MeshoptDecimateSloppy


Positional arguments:
  OBJ_file_path         OBJ file path

Optional arguments:
  -h, --help            shows help message and exits
  -v, --version         prints version information and exits
  -c, --cmd             run in command line mode
  -e, --export_obj      export OBJ in command line mode
  -s, --save_png        save PNG in command line mode
  -n, --smooth          use smooth normal
  -f, --fix_camera      fix camera
  -u, --cull            enable face culling
  -t, --transparent     enable transparent window
  -r, --render          render mode ID [default: 0]
  -m, --method          processing method ID [default: 1]
  -l, --level           processing level [default: 0]
```

* Rendering

key | function
-|-
`Tab` | switch rendering mode: FacesWireframe (default), FacesOnly, WireframeOnly
`N` | enable / disable (default) smooth rendering
`C` | enable / disable (default) face culling
`T` | enable / disable (default) transparent window
`F` | use camera's fixed parameters
`R` | refresh camera's fixed parameters
`P` | print camera's current parameters
`F2` | save the screenshot as a PNG file
`ESC` | exit

* Camera controlling

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

* Triangle mesh processing

key | function
-|-
`Ctrl` + `1`,`2`,...,`9`,`0` | choose processing methods, `0` for 10, default: `1`
`0`-`9` | processing level, `0` for the original mesh (default)
`,`/`.` | decimate one less/more triangle for the decimation methods
`O` | export the processed mesh as an OBJ file

# Results

* Rendering modes

<table>
	<!-- FacesWireframe, FacesOnly -->
	<tr align="center">
		<th width="50%">
			FacesWireframe
		</th>
		<th width="50%">
			FacesOnly
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=0).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/rendering_modes/suzanne.LoopSubdivideSmooth(level=0).Normal_flat.FacesOnly.Cull_true.png"></img>
		</td>
	</tr>
	<!-- WireframeOnly with face culling, WireframeOnly without face culling -->
	<tr align="center">
		<th>
			WireframeOnly, Cull_false
		</th>
		<th>
			WireframeOnly, Cull_true
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/rendering_modes/suzanne.LoopSubdivideSmooth(level=0).Normal_flat.WireframeOnly.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/rendering_modes/suzanne.LoopSubdivideSmooth(level=0).Normal_flat.WireframeOnly.Cull_true.png"></img>
		</td>
	</tr>
</table>

* LoopSubdivideSmooth, Normal_flat vs Normal_smooth

<table>
	<!-- original mesh -->
	<tr align="center">
		<th width="50%">
			original mesh, Normal_flat
		</th>
		<th width="50%">
			original mesh, Normal_smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=0).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=0).Normal_smooth.FacesWireframe.Cull_true.png"></img>
		</td>
	</tr>
	<!-- level=1 -->
	<tr align="center">
		<th>
			level=1, Normal_flat
		</th>
		<th>
			level=1, Normal_smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=1).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=1).Normal_smooth.FacesWireframe.Cull_true.png"></img>
		</td>
	</tr>
	<!-- level=2 -->
	<tr align="center">
		<th>
			level=2, Normal_flat
		</th>
		<th>
			level=2, Normal_smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=2).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=2).Normal_smooth.FacesWireframe.Cull_true.png"></img>
		</td>
	</tr>
	<!-- level=3 -->
	<tr align="center">
		<th>
			level=3, Normal_flat
		</th>
		<th>
			level=3, Normal_smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=3).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=3).Normal_smooth.FacesWireframe.Cull_true.png"></img>
		</td>
	</tr>
	<!-- level=4 -->
	<tr align="center">
		<th>
			level=4, Normal_flat
		</th>
		<th>
			level=4, Normal_smooth
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=4).Normal_flat.FacesWireframe.Cull_true.png"></img>
		</td>
		<td>
			<img src="result/LoopSubdivideSmooth(level=n)/suzanne.LoopSubdivideSmooth(level=4).Normal_smooth.FacesWireframe.Cull_true.png"></img>
		</td>
	</tr>
</table>

* Processing methods: subdivision and tessellation methods, level=2

<table>
	<tr align="center">
		<th width="50%">
			LoopSubdivideSmooth
		</th>
		<th width="50%">
			LoopSubdivideSmoothNoLimit
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/processing_methods/suzanne.LoopSubdivideSmooth(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/processing_methods/suzanne.LoopSubdivideSmoothNoLimit(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
	</tr>
	<tr align="center">
		<th>
			LoopSubdivideFlat
		</th>
		<th>
			Tessellate4
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/processing_methods/suzanne.LoopSubdivideFlat(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/processing_methods/suzanne.Tessellate4(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
	</tr>
	<tr align="center">
		<th>
			Tessellate4_1
		</th>
		<th>
			Tessellate3
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/processing_methods/suzanne.Tessellate4_1(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/processing_methods/suzanne.Tessellate3(level=2).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
	</tr>
</table>

* Processing methods: decimation methods, level=5 (decimation ratio=50%)

<table>
	<tr align="center">
		<th width="50%">
			Decimate_ShortestEdge_V0
		</th>
		<th width="50%">
			Decimate_ShortestEdge_Midpoint
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/processing_methods/suzanne.Decimate_ShortestEdge_V0(level=5).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/processing_methods/suzanne.Decimate_ShortestEdge_Midpoint(level=5).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
	</tr>
	<tr align="center">
		<th>
			MeshoptDecimate
		</th>
		<th>
			MeshoptDecimateSloppy
		</th>
	</tr>
	<tr align="center">
		<td>
			<img src="result/processing_methods/suzanne.MeshoptDecimate(level=5).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
		<td>
			<img src="result/processing_methods/suzanne.MeshoptDecimateSloppy(level=5).Normal_flat.FacesWireframe.Cull_false.png"></img>
		</td>
	</tr>
</table>
