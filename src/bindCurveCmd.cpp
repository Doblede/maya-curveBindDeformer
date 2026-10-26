/**
 * @file bindCurveCmd.cpp
 * @brief Implementation of the BindCurveCmd class.
 */

#include "BindCurveCmd.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MDagPath.h>

BindCurveCmd::BindCurveCmd() : isSetup(false) {}
BindCurveCmd::~BindCurveCmd() {}

void* BindCurveCmd::creator() {
    return new BindCurveCmd();
}

// Define the arguments: cmds.bindCurve(driver="x", rest="y", twist="z")
MSyntax BindCurveCmd::newSyntax() {
    MSyntax syntax;
    syntax.addFlag("-d", "-driver", MSyntax::kString);
    syntax.addFlag("-r", "-rest", MSyntax::kString);
    syntax.addFlag("-t", "-twist", MSyntax::kString);
    
    // Expects exactly 1 selected item (the high-res geometry to bind)
    syntax.setObjectType(MSyntax::kSelectionList, 1, 1);
    syntax.useSelectionAsDefault(true);
    return syntax;
}

MStatus BindCurveCmd::doIt(const MArgList& args) {
    MArgDatabase argData(syntax(), args);
    MSelectionList list;
    argData.getObjects(list);

    if (list.length() == 0) {
        MGlobal::displayError("bindCurve: Please select the geometry to deform.");
        return MS::kFailure;
    }

    MString driverName, restName, twistName;
    if (argData.isFlagSet("-d")) argData.getFlagArgument("-d", 0, driverName);
    if (argData.isFlagSet("-r")) argData.getFlagArgument("-r", 0, restName);
    if (argData.isFlagSet("-t")) argData.getFlagArgument("-t", 0, twistName);

    if (driverName.length() == 0 || restName.length() == 0) {
        MGlobal::displayError("bindCurve: -driver (-d) and -rest (-r) curves are required.");
        return MS::kFailure;
    }

    MDagPath drivenGeo;
    list.getDagPath(0, drivenGeo);

    // 1. Create Deformer using MDGModifier via commandToExecute
    // Wrapping the MEL deformer command inside the modifier allows Maya's native 
    // undo queue to safely handle set memberships and shape orig node generation.
    MString defName = drivenGeo.partialPathName() + "_curveDef";
    MString defCmd = "deformer -type curveDeformerNode -name " + defName + " " + drivenGeo.partialPathName();
    
    dgModifier.commandToExecute(defCmd);

    // 2. Wire the connections
    // Because the node hasn't been created in Maya's DG graph yet (dgModifier.doIt() 
    // hasn't executed), we push MEL connection commands to the modifier to wire it up cleanly.
    MString connectDriver = "connectAttr -f " + driverName + ".worldSpace[0] " + defName + ".driverCurve";
    MString connectRest = "connectAttr -f " + restName + ".worldSpace[0] " + defName + ".restCurve";
    
    dgModifier.commandToExecute(connectDriver);
    dgModifier.commandToExecute(connectRest);

    if (twistName.length() > 0) {
        MString connectTwist = "connectAttr -f " + twistName + ".worldMatrix[0] " + defName + ".startTwistMatrix";
        dgModifier.commandToExecute(connectTwist);
    }

    isSetup = true;
    return redoIt();
}

MStatus BindCurveCmd::redoIt() {
    if (isSetup) {
        return dgModifier.doIt(); // Executes all buffered MEL commands
    }
    return MS::kFailure;
}

MStatus BindCurveCmd::undoIt() {
    if (isSetup) {
        return dgModifier.undoIt(); // Reverts all buffered MEL commands
    }
    return MS::kFailure;
}