#include <maya/MFnPlugin.h>
#include "CurveDeformerNode.h"
#include "BindCurveCmd.h"

MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, "David De Juan", "1.0", "Any");

    // Register the Deformer Node
    status = plugin.registerNode("curveDeformerNode", 
                                 CurveDeformerNode::id, 
                                 CurveDeformerNode::creator, 
                                 CurveDeformerNode::initialize, 
                                 MPxNode::kDeformerNode);
    if (!status) {
        status.perror("Failed to register curveDeformerNode");
        return status;
    }

    // Register the custom Maya Command
    status = plugin.registerCommand("bindCurve", 
                                    BindCurveCmd::creator, 
                                    BindCurveCmd::newSyntax);
    if (!status) {
        status.perror("Failed to register bindCurve command");
        return status;
    }

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    // Deregister the Command
    status = plugin.deregisterCommand("bindCurve");
    if (!status) {
        status.perror("Failed to deregister bindCurve command");
        return status;
    }

    // Deregister the Node
    status = plugin.deregisterNode(CurveDeformerNode::id);
    if (!status) {
        status.perror("Failed to deregister curveDeformerNode");
        return status;
    }

    return MS::kSuccess;
}