# maya-curveDeformer
An Autodesk Maya deformer that efficiently drives multiple curves using a single master curve. It implementa the parallel - transport algorithm to keep the orientation.

**Curve Deformer** calculates spatial offsets between a static "Rest" curve and an animated "Driver" curve, applying the resulting transformations to the mesh. It comes complete with a parallelized C++ evaluation engine, a custom command for robust Undo/Redo support, and a companion PySide UI for easy rig management.

## ✨ Key Features

- **Multi-Threaded Evaluation (Intel TBB):** The core point-iteration loop uses `tbb::parallel_for`, allowing the deformer to scale across multiple CPU cores for massive performance gains on dense geometry.
- **Stable Twist & Framing:** Incorporates a `startTwistMatrix` attribute that uses quaternions to calculate a stable up-vector relative to the curve's bend, preventing common flipping artifacts.
- **Custom Setup Command (`bindCurve`):** Features a dedicated `MPxCommand` that utilizes Maya's `MDGModifier`. This ensures node creation and DG graph wiring are handled safely, with native support for Maya's Undo/Redo queue.
- **Companion Python UI:** Includes a PySide2/PySide6 compatible tool (`curveBindManager.py`) that automates the generation of hidden rest curves and provides a simple interface to bind/unbind geometries to a master curve.

## 🚀 Installation

1. **The Plugin:** Compile the C++ source using CMake and the Maya Developer Kit. Place the resulting `curveDeformer.mll` (Windows) / `.so` (Linux) / `.bundle` (macOS) in a directory mapped to your `MAYA_PLUG_IN_PATH`.
2. **The Script:** Place `curveBindManager.py` in your Maya scripts folder (e.g., `Documents/maya/202X/prefs/scripts`).
3. **Load:** Open Maya's Plug-in Manager and load the plugin.

## 💻 How to Use It

### Method 1: Using the Manager UI (Recommended)

1. Open the Maya Script Editor (Python tab) and run:
```python
import curveBindManager
import importlib
importlib.reload(curveBindManager)
```
2. Select your main animated curve in the viewport and click **"<< Load Selected"** under *Master Driver Curve*.
3. Select any geometries you want to deform and click **"Bind Selected"**. The tool will automatically generate a hidden Rest Curve and wire the C++ deformer.
4. You can select bound items from the list and click **"Unbind Selected"** to safely remove the deformer.

### Method 2: Using the Custom Maya Command

You can manually trigger the C++ command via MEL or Python. You must have the target geometry selected.

```python
import maya.cmds as cmds

# cmds.bindCurve( driver="curveName", rest="restCurveName", twist="optionalTwistNode" )
cmds.select("myHighResGeo")
cmds.bindCurve(d="curve1", r="curve1_rest")
```

## 🎛️ Node Attributes (`curveDeformerNode`)

- **`driverCurve` (`dc`):** The animated NURBS curve driving the deformation.
- **`restCurve` (`rc`):** The static, default-pose NURBS curve used to calculate initial spatial offsets.
- **`startTwistMatrix` (`stm`):** (Optional) A matrix input (e.g., from a locator) used to define the base Up-Vector for the curve's internal framing.
- **`envelope`:** Maya's standard deformer weight attribute (0.0 to 1.0) to blend the deformation on or off.

## 🏗️ Architecture / For Developers

- **`curveDeformerNode` (`MPxDeformerNode`):** The core calculation engine (ID: `0x00080011`). Handles the Intel TBB threaded point deformation.
- **`BindCurveCmd` (`MPxCommand`):** A custom command that wraps MEL execution inside an `MDGModifier` to safely build the deformer network and handle the DG undo/redo stack.
- **`pluginMain_2.cpp`:** Registers both the Node and the Command under the author David De Juan.
- **`curveBindManager.py`:** A `QtWidgets.QDialog` interface that queries `worldSpace` DG connections to dynamically list bound geometries and automate the `bindCurve` command execution.
