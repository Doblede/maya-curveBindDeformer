#pragma once

#include <maya/MPxDeformerNode.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MDataBlock.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>

class CurveDeformerNode : public MPxDeformerNode {
public:
    CurveDeformerNode();
    virtual ~CurveDeformerNode();

    static void* creator();
    static MStatus initialize();

    // The main deformation calculation
    virtual MStatus deform(MDataBlock& data, MItGeometry& iter, const MMatrix& mat, unsigned int mIndex) override;

    // Node ID and Attributes
    static MTypeId id;
    static MObject aDriverCurve;
    static MObject aRestCurve;
    static MObject aStartTwistMatrix;
};