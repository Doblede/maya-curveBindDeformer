/**
 * @file bindCurveCmd.h
 * @brief Custom Maya command for setting up the Curve Deformer network.
 */

#pragma once

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MDGModifier.h>

/**
 * @class BindCurveCmd
 * @brief Command to initialize and connect a curveDeformerNode.
 */
class BindCurveCmd : public MPxCommand {
public:
    BindCurveCmd();
    virtual ~BindCurveCmd();

    /**
     * @brief Instantiates the command.
     * @return A pointer to a new BindCurveCmd object.
     */
    static void* creator();

    /**
     * @brief Defines the syntax and flags for the MEL/Python command.
     * @return The MSyntax object specifying expected flags (-d, -r, -t).
     */
    static MSyntax newSyntax();

    /**
     * @brief Specifies whether this command supports undo.
     * @return True, as the command relies on MDGModifier for safe undos.
     */
    virtual bool isUndoable() const override { return true; }

    /**
     * @brief Executes the command's primary logic.
     * @param args The arguments passed from MEL/Python.
     * @return MS::kSuccess if the network is successfully built.
     */
    virtual MStatus doIt(const MArgList& args) override;

    /**
     * @brief Redoes the command after an undo.
     * @return Status of the MDGModifier execution.
     */
    virtual MStatus redoIt() override;

    /**
     * @brief Undoes the command, removing the created nodes and connections.
     * @return Status of the MDGModifier undo operation.
     */
    virtual MStatus undoIt() override;

private:
    MDGModifier dgModifier; ///< Handles node creation and connections safely.
    bool isSetup;           ///< Tracks if the modifier has been initialized.
};