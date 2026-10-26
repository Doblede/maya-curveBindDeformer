import maya.cmds as cmds

# Handle Maya version differences for PySide
try:
    from PySide6 import QtWidgets, QtCore
except ImportError:
    from PySide2 import QtWidgets, QtCore


class CurveBindManager(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(CurveBindManager, self).__init__(parent)
        self.setWindowTitle("Curve Bind Manager")
        self.resize(350, 400)
        self.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint)
        
        self.master_curve = None
        self.build_ui()

    def build_ui(self):
        main_layout = QtWidgets.QVBoxLayout(self)
        
        # --- Master Curve Section ---
        master_group = QtWidgets.QGroupBox("Master Driver Curve")
        master_layout = QtWidgets.QHBoxLayout(master_group)
        
        self.master_le = QtWidgets.QLineEdit()
        self.master_le.setReadOnly(True)
        self.master_le.setPlaceholderText("Select a curve and load...")
        
        load_master_btn = QtWidgets.QPushButton("<< Load Selected")
        load_master_btn.clicked.connect(self.load_master_curve)
        
        master_layout.addWidget(self.master_le)
        master_layout.addWidget(load_master_btn)
        
        # --- Connected Geometries Section ---
        driven_group = QtWidgets.QGroupBox("Bound Geometries")
        driven_layout = QtWidgets.QVBoxLayout(driven_group)
        
        self.driven_list = QtWidgets.QListWidget()
        self.driven_list.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        
        driven_layout.addWidget(self.driven_list)
        
        # --- Action Buttons ---
        action_layout = QtWidgets.QHBoxLayout()
        
        bind_btn = QtWidgets.QPushButton("Bind Selected")
        bind_btn.setToolTip("Select geometries to bind to the Master Curve.")
        bind_btn.clicked.connect(self.bind_selected_geometries)
        
        unbind_btn = QtWidgets.QPushButton("Unbind Selected")
        unbind_btn.setToolTip("Remove deformer from selected items in the list.")
        unbind_btn.clicked.connect(self.unbind_selected_geometries)
        
        action_layout.addWidget(bind_btn)
        action_layout.addWidget(unbind_btn)
        
        # Assemble Main Layout
        main_layout.addWidget(master_group)
        main_layout.addWidget(driven_group)
        main_layout.addLayout(action_layout)

    # ---------------------------------------------------------
    # Core Logic
    # ---------------------------------------------------------
    
    def load_master_curve(self):
        """Loads the selected curve as the Master and finds all connected geometry."""
        sel = cmds.ls(selection=True, dag=True, type="nurbsCurve")
        if not sel:
            cmds.warning("Please select a NURBS curve to set as the master.")
            return
            
        # Get the transform of the shape
        transform = cmds.listRelatives(sel[0], parent=True)[0]
        self.master_curve = transform
        self.master_le.setText(transform)
        
        self.refresh_driven_list()

    def refresh_driven_list(self):
        """Queries the node graph to find everything driven by the master curve."""
        self.driven_list.clear()
        if not self.master_curve:
            return
            
        # Look at what the master curve's worldSpace is connected to
        shape = cmds.listRelatives(self.master_curve, shapes=True)[0]
        connections = cmds.listConnections(f"{shape}.worldSpace", type="curveDeformerNode", source=False, destination=True)
        
        if not connections:
            return
            
        # Remove duplicates (a node might be listed multiple times depending on query)
        deformers = list(set(connections))
        
        # Find the geometries driven by these deformers
        for def_node in deformers:
            driven_geos = cmds.deformer(def_node, query=True, geometry=True)
            if driven_geos:
                for geo in driven_geos:
                    item = QtWidgets.QListWidgetItem(geo)
                    # Store the deformer node name in the item for easy unbinding later
                    item.setData(QtCore.Qt.UserRole, def_node)
                    self.driven_list.addItem(item)

    def bind_selected_geometries(self):
        """Binds the currently selected viewport geometry to the Master Curve."""
        if not self.master_curve:
            cmds.warning("Please set a Master Curve first.")
            return
            
        sel = cmds.ls(selection=True)
        if not sel:
            cmds.warning("Select the geometry you want to bind.")
            return
            
        # --- SAFETY CHECK ---
        # Filter out the master curve if it was accidentally selected
        valid_geos = []
        for geo in sel:
            if geo == self.master_curve:
                cmds.warning(f"Skipping '{geo}': Cannot bind the master curve to itself to avoid DG cycles.")
                continue
            valid_geos.append(geo)
            
        # If the list is empty after filtering, exit gracefully
        if not valid_geos:
            return
            
        for geo in valid_geos:
            # Generate a Rest Curve automatically by duplicating the Master
            rest_curve = cmds.duplicate(self.master_curve, name=f"{self.master_curve}_rest")[0]
            cmds.hide(rest_curve)  # Hide it so the viewport stays clean
            
            # Select the geo so our custom command operates on it
            cmds.select(geo, replace=True)
            
            # Execute the custom C++ command!
            try:
                cmds.bindCurve(driver=self.master_curve, rest=rest_curve)
                print(f"Successfully bound {geo} to {self.master_curve}.")
            except Exception as e:
                cmds.warning(f"Failed to bind {geo}: {e}")
                
        # Restore original selection (minus the filtered master curve) and refresh UI
        cmds.select(valid_geos, replace=True)
        self.refresh_driven_list()

    def unbind_selected_geometries(self):
        """Removes the deformer from geometries selected in the UI list."""
        selected_items = self.driven_list.selectedItems()
        if not selected_items:
            cmds.warning("Select items in the list below to unbind.")
            return
            
        for item in selected_items:
            def_node = item.data(QtCore.Qt.UserRole)
            
            # Delete the deformer node to clean up the graph
            if cmds.objExists(def_node):
                cmds.delete(def_node)
                print(f"Removed deformer: {def_node}")
                
        self.refresh_driven_list()


# Launch the UI
if __name__ == "__main__":
    # Ensure no multiple instances are open
    if cmds.window("CurveBindManagerUI_Window", exists=True):
        cmds.deleteUI("CurveBindManagerUI_Window")
        
    global curve_bind_manager_ui
    curve_bind_manager_ui = CurveBindManager()
    curve_bind_manager_ui.setObjectName("CurveBindManagerUI_Window")
    curve_bind_manager_ui.show()