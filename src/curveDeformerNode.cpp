/**
 * @file curveDeformerNode.cpp
 * @brief Implementation of the parallelized CurveDeformerNode.
 */

#include "CurveDeformerNode.h"

#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPointArray.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MGlobal.h>

// Intel TBB for multi-threading optimizations
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

// Request a unique block from Autodesk for production
MTypeId CurveDeformerNode::id(0x00080011);
MObject CurveDeformerNode::aDriverCurve;
MObject CurveDeformerNode::aRestCurve;
MObject CurveDeformerNode::aStartTwistMatrix;

CurveDeformerNode::CurveDeformerNode() {}
CurveDeformerNode::~CurveDeformerNode() {}

void* CurveDeformerNode::creator() {
    return new CurveDeformerNode();
}

MStatus CurveDeformerNode::initialize() {
    MFnTypedAttribute tAttr;
    MFnMatrixAttribute mAttr;

    // Initialize Nurbs Curve inputs
    aDriverCurve = tAttr.create("driverCurve", "dc", MFnData::kNurbsCurve);
    aRestCurve = tAttr.create("restCurve", "rc", MFnData::kNurbsCurve);
    
    // Default matrix attribute for twist/up-vector derivation
    aStartTwistMatrix = mAttr.create("startTwistMatrix", "stm");
    mAttr.setStorable(true);
    mAttr.setKeyable(true);

    addAttribute(aDriverCurve);
    addAttribute(aRestCurve);
    addAttribute(aStartTwistMatrix);

    // Ensure Maya knows that changing these inputs requires re-evaluating the output geometry
    attributeAffects(aDriverCurve, outputGeom);
    attributeAffects(aRestCurve, outputGeom);
    attributeAffects(aStartTwistMatrix, outputGeom);

    return MS::kSuccess;
}

MStatus CurveDeformerNode::deform(MDataBlock& data, MItGeometry& iter, const MMatrix& localToWorldMatrix, unsigned int mIndex) {
    // Early exit if envelope is 0
    double env = data.inputValue(envelope).asFloat();
    if (env <= 0.0) return MS::kSuccess;

    MObject driverObj = data.inputValue(aDriverCurve).asNurbsCurve();
    MObject restObj = data.inputValue(aRestCurve).asNurbsCurve();

    if (driverObj.isNull() || restObj.isNull()) return MS::kSuccess;

    // Extract the Up-Vector from the twist matrix to stabilize the curve framing
    MMatrix startMat = data.inputValue(aStartTwistMatrix).asMatrix();
    MVector baseUp(startMat[1][0], startMat[1][1], startMat[1][2]); 
    if (baseUp.length() < 0.001) {
        baseUp = MVector(0, 1, 0); 
    } else {
        baseUp.normalize();
    }

    MPointArray points;
    iter.allPositions(points, MSpace::kObject);
    MMatrix worldToLocalMatrix = localToWorldMatrix.inverse();

    // Multi-threaded loop utilizing Intel TBB for performance scaling across cores
    tbb::parallel_for(tbb::blocked_range<size_t>(0, points.length()),
        [&](const tbb::blocked_range<size_t>& range) {
            
            // Function sets must be initialized locally inside the thread to avoid race conditions
            MFnNurbsCurve driverFn(driverObj);
            MFnNurbsCurve restFn(restObj);

            for (size_t i = range.begin(); i != range.end(); ++i) {
                MPoint ptLocal = points[(unsigned int)i];
                MPoint ptWorld = ptLocal * localToWorldMatrix;

                // 1. Calculate Rest Frame
                double param;
                MPoint closestRestPt = restFn.closestPoint(ptWorld, &param, 0.0, MSpace::kWorld);
                
                MVector restTangent = restFn.tangent(param, MSpace::kWorld).normal();
                MVector restUp(0, 1, 0);
                MVector restBinormal = (restTangent ^ restUp).normal();
                MVector restNormal = (restBinormal ^ restTangent).normal();

                double restMatArray[4][4] = {
                    {restTangent.x, restTangent.y, restTangent.z, 0.0},
                    {restNormal.x,  restNormal.y,  restNormal.z,  0.0},
                    {restBinormal.x,restBinormal.y,restBinormal.z,0.0},
                    {closestRestPt.x, closestRestPt.y, closestRestPt.z, 1.0}
                };
                MMatrix restMatrix(restMatArray);
                
                // Store the point's spatial offset relative to the rest curve
                MPoint localOffset = ptWorld * restMatrix.inverse();

                // 2. Calculate Animated Frame
                MPoint animPt;
                driverFn.getPointAtParam(param, animPt, MSpace::kWorld);
                MVector animTangent = driverFn.tangent(param, MSpace::kWorld).normal();

                // Rotate the base up-vector based on how the curve tangent has bent
                MQuaternion curveBendRot(restTangent, animTangent);
                MVector finalUp = baseUp.rotateBy(curveBendRot).normal();

                MVector animBinormal = (animTangent ^ finalUp).normal();
                MVector animNormal = (animBinormal ^ animTangent).normal();

                double animMatArray[4][4] = {
                    {animTangent.x, animTangent.y, animTangent.z, 0.0},
                    {animNormal.x,  animNormal.y,  animNormal.z,  0.0},
                    {animBinormal.x,animBinormal.y,animBinormal.z,0.0},
                    {animPt.x,      animPt.y,      animPt.z,      1.0}
                };
                MMatrix animMatrix(animMatArray);

                // 3. Apply Animated Transformation
                MPoint newPtWorld = localOffset * animMatrix;
                
                // Apply global envelope weighting and return to local space
                MPoint finalPtWorld = ptWorld + ((newPtWorld - ptWorld) * env);
                points[(unsigned int)i] = finalPtWorld * worldToLocalMatrix;
            }
        }
    );

    iter.setAllPositions(points);
    return MS::kSuccess;
}